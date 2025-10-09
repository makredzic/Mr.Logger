#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <MR/Interface/ThreadSafeQueue.hpp>
#ifdef LOGGER_TEST_SEQUENCE_TRACKING
#include <atomic>
#include <type_traits>
#endif

#ifdef LOGGER_TEST_SEQUENCE_TRACKING
namespace MR {
  namespace Logger {
    struct WriteRequest;
  }
}
#endif

namespace MR::Queue {

#ifdef LOGGER_TEST_SEQUENCE_TRACKING
  // Global sequence counter for all WriteRequest instances
  extern std::atomic<uint64_t> global_sequence_counter;
#endif

  template <typename T>
  class StdQueue : public Interface::ThreadSafeQueue<T> {

    private:
      #define LOCK() std::unique_lock<std::mutex> lock{mutex_};

      std::queue<T> queue_;
      mutable std::mutex mutex_;
      std::condition_variable cv_;
      
      bool stop_{false};

    public:

      inline void shutdown() override {
        {
          LOCK();
          stop_ = true;
        }
        cv_.notify_all();
      }

      inline StdQueue() = default;
      inline ~StdQueue() {
        shutdown();
      }
      
      inline void push(const T& element) override {
        {
          LOCK();
          if (stop_) return;

#ifdef LOGGER_TEST_SEQUENCE_TRACKING
          // Assign sequence number if this is a WriteRequest
          if constexpr (std::is_same_v<T, MR::Logger::WriteRequest>) {
            T element_copy = element;
            element_copy.sequence_number = global_sequence_counter.fetch_add(1, std::memory_order_seq_cst);
            queue_.push(std::move(element_copy));
          } else {
            queue_.push(element);
          }
#else
          queue_.push(element);
#endif
        }
        cv_.notify_one();
      }

      inline void push(T&& element) override {
        {
          LOCK();
          if (stop_) return;

#ifdef LOGGER_TEST_SEQUENCE_TRACKING
          // Assign sequence number if this is a WriteRequest
          if constexpr (std::is_same_v<T, MR::Logger::WriteRequest>) {
            element.sequence_number = global_sequence_counter.fetch_add(1, std::memory_order_seq_cst);
          }
#endif
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