#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <MR/Interface/ThreadSafeQueue.hpp>

namespace MR::Queue {

  template <typename T>
  class StdQueue : public Interface::ThreadSafeQueue<T> {

    private:
      #define LOCK() std::unique_lock<std::mutex> lock{mutex_};

      std::queue<T> queue_;
      mutable std::mutex mutex_;
      std::condition_variable cv_;
      
      bool stop_{false};

    public:

      inline StdQueue() = default;
      inline ~StdQueue() {
        shutdown();
      }
      
      inline void shutdown() override {
        {
          LOCK();
          stop_ = true;
        }
        cv_.notify_all();
      }


      inline void push(const T& element) override {
        {
          LOCK();
          queue_.push(element);
        }
        cv_.notify_one();
      }

      inline void push(T&& element) override {
        {
          LOCK();
          queue_.push(std::move(element));
        }
        cv_.notify_one();
      }

      inline std::optional<T> tryPop() override {
        LOCK();
        if (queue_.empty()) return {};

        T element = std::move(queue_.front());
        queue_.pop();

        return element;
      }

      inline std::optional<T> pop() override {
        
        LOCK();

        cv_.wait(lock, [this](){
          return !queue_.empty() || stop_;
        });

        if (queue_.empty()) {
          return {};
        }

        T element = std::move(queue_.front());
        queue_.pop();

        return element;
      }

      inline bool empty() const override {
        LOCK();
        return queue_.empty();
      }

      inline size_t size() const override {
        LOCK();
        return queue_.size();
      }

  };
}