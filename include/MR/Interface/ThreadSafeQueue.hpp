#pragma once
#include <optional>


namespace MR::Interface {

  template<typename T>
  class ThreadSafeQueue {
    public:
      virtual ~ThreadSafeQueue() = default;
      virtual void push(const T&) = 0;
      virtual void push(T&&) = 0;
      virtual std::optional<T> tryPop() = 0;
      virtual std::optional<T> pop() = 0;
      virtual bool empty() const = 0;
      virtual size_t size() const = 0;
      virtual void shutdown() = 0;

      ThreadSafeQueue() = default;

      ThreadSafeQueue(const ThreadSafeQueue&) = delete;
      ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

      ThreadSafeQueue(ThreadSafeQueue&&) = default;
      ThreadSafeQueue& operator=(ThreadSafeQueue&&) = default;
  };
}