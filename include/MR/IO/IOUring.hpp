#pragma once

#include <cstddef>
#include <liburing.h>
#include <stdexcept>
#include <coroutine>
#include <atomic>
#include <chrono>

#include <MR/IO/WriteOnlyFile.hpp>


namespace MR::IO {
class IOUring {
private:
  std::atomic<bool> is_operational_{true};

public:
   struct WriteAwaiter {
    IOUring* ring;
    const WriteOnlyFile& file;
    void* buffer;
    size_t len;
    int result = -1; 
    
    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> h) {
      try {
        auto* user_data = new UserData{h, this};

        // SUBMIT TO IO_URING
        bool success = ring->prepareWrite(file, buffer, len, user_data);
        if (!success) {
          delete user_data;
          result = -EAGAIN;
          h.resume();
        }
      } catch (...) {
        result = -ENOMEM;
        h.resume();
      }
    }
    int await_resume() {
      return result;
    }
  };

  struct UserData {
    std::coroutine_handle<> handle;
    WriteAwaiter* awaiter;  // Back-pointer to get the result
  };


  inline explicit IOUring(size_t queue_depth) : queue_depth_{queue_depth} {
    auto status = io_uring_queue_init(queue_depth, &ring_, 0);
    if (status < 0) {
      throw std::runtime_error("error initializing io_uring: " + std::to_string(status));
    }
  }

  inline ~IOUring() { 
    io_uring_queue_exit(&ring_);
  }

  IOUring(const IOUring &) = delete;
  IOUring &operator=(const IOUring &) = delete;
  IOUring(IOUring &&) = delete;
  IOUring &operator=(IOUring &&) = delete;

  inline size_t capacity() const { return queue_depth_; }

  inline bool isOperational() const { return is_operational_.load(std::memory_order_acquire); }
  inline void markFailed() noexcept { is_operational_.store(false, std::memory_order_release); }

  inline WriteAwaiter write(const WriteOnlyFile& file, void* buffer, size_t len) {
    return WriteAwaiter{this, file, buffer, len, -1};
  }

  inline void processCompletions() noexcept {
    try {
      io_uring_cqe* cqe;
      unsigned head;
      unsigned completed = 0;

      io_uring_for_each_cqe(&ring_, head, cqe) {

          auto* user_data = static_cast<UserData*>(io_uring_cqe_get_data(cqe));

          if (user_data) {
              // Store the I/O result in the awaiter
              user_data->awaiter->result = cqe->res;
              user_data->handle.resume();
              delete user_data;  // Clean up UserData
          }
          completed++;
      }

      if (completed > 0) {
          io_uring_cq_advance(&ring_, completed);
      }
    } catch (...) {
      // Critical: if CQE processing fails, mark as failed
      markFailed();
    }
  }

  // Wait for at least one CQE with timeout
  inline bool waitForCompletion(std::chrono::microseconds timeout) noexcept {
    if (!is_operational_.load(std::memory_order_acquire)) {
      return false;
    }

    __kernel_timespec ts;
    ts.tv_sec = timeout.count() / 1000000;
    ts.tv_nsec = (timeout.count() % 1000000) * 1000;

    io_uring_cqe* cqe;
    int ret = io_uring_wait_cqe_timeout(&ring_, &cqe, &ts);

    if (ret == 0) {
      // Don't advance here - let processCompletions() handle it
      return true;
    }
    return false; // timeout or error
  }

  inline bool submitPendingSQEs() noexcept {

    if (!is_operational_.load(std::memory_order_acquire)) {
      return false;
    }

    int status = io_uring_submit(&ring_);
    if (status < 0) {
      markFailed();
      return false;
    }

    return true;
  }

private:
  size_t queue_depth_;
  io_uring ring_;

  inline bool prepareWrite(const WriteOnlyFile& file, void* buffer, size_t size, void* user_data) noexcept {

    // Check if ring is still operational
    if (!is_operational_.load(std::memory_order_acquire)) {
      return false;
    }

    // GET SUBMISSION QUEUE ENTRY
    io_uring_sqe *sqe = io_uring_get_sqe(&ring_);
    if (!sqe) {
      // Queue is full - this is normal backpressure, not a failure
      // Return false to signal that the SQE could not be prepared
      return false;
    }

    // PREPARE WRITE
    io_uring_sqe_set_data(sqe, user_data);
    io_uring_prep_write(sqe, file.fd(), buffer, size, (uint64_t)-1);
    return true;
  }


};

} // namespace MR::IO