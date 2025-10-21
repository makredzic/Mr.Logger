#pragma once
#include <condition_variable>
#include <mutex>
#include <vector>
#include <optional>
#include <stdexcept>
#include <MR/Interface/ThreadSafeQueue.hpp>

namespace MR::Queue {

template <typename T>
class CircularQueue : public Interface::ThreadSafeQueue<T> {
private:
    const size_t capacity_;
    std::vector<T> buffer_;
    size_t head_{0};
    size_t tail_{0};
    size_t count_{0};

    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
    bool stopped_{false};

public:
    explicit CircularQueue(size_t requested_capacity)
        : capacity_(requested_capacity), buffer_(requested_capacity) {
        if (requested_capacity == 0) {
            throw std::invalid_argument("Queue capacity must be > 0");
        }
    }

    ~CircularQueue() override {
        shutdown();
    }

    CircularQueue(const CircularQueue&) = delete;
    CircularQueue& operator=(const CircularQueue&) = delete;
    CircularQueue(CircularQueue&&) = delete;
    CircularQueue& operator=(CircularQueue&&) = delete;

    void push(const T& item) override {
        std::unique_lock<std::mutex> lock(mutex_);

        if (stopped_) {
            return;
        }

        buffer_[tail_] = item;
        tail_ = (tail_ + 1) % capacity_;

        if (count_ == capacity_) {
            head_ = (head_ + 1) % capacity_;
        } else {
            ++count_;
        }

        lock.unlock();
        not_empty_.notify_one();
    }

    void push(T&& item) override {
        std::unique_lock<std::mutex> lock(mutex_);

        if (stopped_) {
            throw std::runtime_error("Queue is stopped");
        }

        buffer_[tail_] = std::move(item);
        tail_ = (tail_ + 1) % capacity_;

        if (count_ == capacity_) {
            head_ = (head_ + 1) % capacity_;
        } else {
            ++count_;
        }

        lock.unlock();
        not_empty_.notify_one();
    }

    std::optional<T> tryPop() override {
        std::unique_lock<std::mutex> lock(mutex_);

        if (count_ == 0) {
            return std::nullopt;
        }

        T item = std::move(buffer_[head_]);
        head_ = (head_ + 1) % capacity_;
        --count_;

        return item;
    }

    std::optional<T> pop() override {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock, [this] { return count_ > 0 || stopped_; });

        if (stopped_ && count_ == 0) {
            return std::nullopt;
        }

        T item = std::move(buffer_[head_]);
        head_ = (head_ + 1) % capacity_;
        --count_;

        return item;
    }

    bool empty() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_ == 0;
    }

    size_t size() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_;
    }

    void shutdown() override {
        std::lock_guard<std::mutex> lock(mutex_);
        stopped_ = true;
        not_empty_.notify_all();
    }
};

} // namespace MR::Queue
