#pragma once
#include <condition_variable>
#include <mutex>
#include <vector>
#include <optional>
#include <stdexcept>
#include <bit>
#include <MR/Interface/ThreadSafeQueue.hpp>

namespace MR::Queue {

template <typename T>
class FixedSizeBlockingQueue : public Interface::ThreadSafeQueue<T> {
private:
    std::vector<T> buffer_;

    size_t head_{0};
    size_t tail_{0};
    size_t count_{0};
    const size_t capacity_;

    mutable std::mutex mutex_;
    std::condition_variable cv_not_full_;
    std::condition_variable cv_not_empty_;

    bool stopped_{false};

public:
    explicit FixedSizeBlockingQueue(size_t requested_capacity)
        : capacity_(requested_capacity) {
        if (requested_capacity == 0) {
            throw std::invalid_argument("Queue capacity must be > 0");
        }
        buffer_.resize(capacity_);
    }

    ~FixedSizeBlockingQueue() override {
        shutdown();
    }

    FixedSizeBlockingQueue(const FixedSizeBlockingQueue&) = delete;
    FixedSizeBlockingQueue& operator=(const FixedSizeBlockingQueue&) = delete;
    FixedSizeBlockingQueue(FixedSizeBlockingQueue&&) = delete;
    FixedSizeBlockingQueue& operator=(FixedSizeBlockingQueue&&) = delete;

    void push(const T& item) override {
        std::unique_lock<std::mutex> lock(mutex_);

        cv_not_full_.wait(lock, [this]() {
            return count_ < capacity_ || stopped_;
        });

        if (stopped_) {
            return;
        }

        buffer_[tail_] = item;
        tail_ = (tail_ + 1) % capacity_;
        ++count_;

        lock.unlock();
        cv_not_empty_.notify_one();
    }

    void push(T&& item) override {
        std::unique_lock<std::mutex> lock(mutex_);

        cv_not_full_.wait(lock, [this]() {
            return count_ < capacity_ || stopped_;
        });

        if (stopped_) {
            return;
        }

        buffer_[tail_] = std::move(item);
        tail_ = (tail_ + 1) % capacity_;
        ++count_;

        lock.unlock();
        cv_not_empty_.notify_one();
    }

    std::optional<T> tryPop() override {
        std::lock_guard<std::mutex> lock(mutex_);

        if (count_ == 0) {
            return std::nullopt;
        }

        T item = std::move(buffer_[head_]);
        head_ = (head_ + 1) % capacity_;
        --count_;

        cv_not_full_.notify_one();
        return item;
    }

    std::optional<T> pop() override {
        std::unique_lock<std::mutex> lock(mutex_);

        cv_not_empty_.wait(lock, [this]() {
            return count_ > 0 || stopped_;
        });

        if (stopped_ && count_ == 0) {
            return std::nullopt;
        }

        T item = std::move(buffer_[head_]);
        head_ = (head_ + 1) % capacity_;
        --count_;

        lock.unlock();
        cv_not_full_.notify_one();

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
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopped_ = true;
        }
        cv_not_full_.notify_all();
        cv_not_empty_.notify_all();
    }

    size_t capacity() const {
        return capacity_;
    }

    bool full() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_ >= capacity_;
    }

    bool is_stopped() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stopped_;
    }
};

} // namespace MR::Queue