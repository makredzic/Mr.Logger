#pragma once

#include <condition_variable>
#include <mutex>
#include <vector>
#include <MR/Interface/ThreadSafeQueue.hpp>

namespace MR::Queue {

  template <typename T>
  class FixedSizeBlockingQueue: public Interface::ThreadSafeQueue<T> {

    private:
      #define LOCK() std::unique_lock<std::mutex> lock{mutex_};

      std::vector<T> buffer_;
      size_t head_{0};
      size_t tail_{0};
      size_t count_{0};
      size_t capacity_;

      mutable std::mutex mutex_;
      std::condition_variable cv_full_;
      std::condition_variable cv_empty_;

      bool stop_{false};

    public:

      inline void shutdown() override {
        {
          LOCK();
          stop_ = true;
        }
        cv_full_.notify_all();
        cv_empty_.notify_all();
      }

      inline explicit FixedSizeBlockingQueue(size_t capacity)
        : capacity_(capacity) {
        buffer_.resize(capacity);
      }

      inline ~FixedSizeBlockingQueue() {
        shutdown();
      }
      
      inline void push(const T& element) override {
        {
          LOCK();

          // Wait while buffer is full and not stopped
          cv_full_.wait(lock, [this](){
            return count_ < capacity_ || stop_;
          });

          if (stop_) return;

          buffer_[tail_] = element;
          tail_ = (tail_ + 1) % capacity_;
          ++count_;
        }
        cv_empty_.notify_one();
      }

      inline void push(T&& element) override {
        {
          LOCK();

          // Wait while buffer is full and not stopped
          cv_full_.wait(lock, [this](){
            return count_ < capacity_ || stop_;
          });

          if (stop_) return;

          buffer_[tail_] = std::move(element);
          tail_ = (tail_ + 1) % capacity_;
          ++count_;
        }
        cv_empty_.notify_one();
      }

      inline std::optional<T> tryPop() override {
        LOCK();
        if (count_ == 0) {
          return {};
        }

        T element = std::move(buffer_[head_]);
        head_ = (head_ + 1) % capacity_;
        --count_;

        cv_full_.notify_one();
        return element;
      }

      inline std::optional<T> pop() override {

        LOCK();

        cv_empty_.wait(lock, [this](){
          return count_ > 0 || stop_;
        });

        if (count_ == 0) {
          return {};
        }

        T element = std::move(buffer_[head_]);
        head_ = (head_ + 1) % capacity_;
        --count_;

        cv_full_.notify_one();
        return element;
      }

      inline bool empty() const override {
        LOCK();
        return count_ == 0;
      }

      inline size_t size() const override {
        LOCK();
        return count_;
      }

  };
}