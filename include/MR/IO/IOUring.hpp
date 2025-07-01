#pragma once
#include <cstddef>
#include <iostream>
#include <liburing.h>

#include <MR/IO/WriteOnlyFile.hpp>
#include <memory>
#include <stdexcept>
#include <thread>

namespace MR::IO {
class IOUring {
public:
  inline explicit IOUring(size_t queue_depth) : pending_sqe_count_{0}, queue_size_{queue_depth} {
    auto status = io_uring_queue_init(queue_depth, &ring_, 0);
    if (status < 0) {
      throw std::runtime_error("error initializing io_uring: " +
                               std::to_string(status));
    }

    worker_ = std::jthread([this](std::stop_token st) {
      this->eventLoop(std::move(st));
    });
  }

  inline ~IOUring() { 

    submitPendingSQEs();

    // force shutdown the thread to guarantee the queue is emptied
    worker_.request_stop(); 
    worker_.join();

    io_uring_queue_exit(&ring_); 
  }

  IOUring(const IOUring &) = delete;
  IOUring &operator=(const IOUring &) = delete;
  IOUring(IOUring &&) = delete;
  IOUring &operator=(IOUring &&) = delete;

  inline void submitWrite(std::unique_ptr<std::string> contents, const WriteOnlyFile& file) {

    // GET SUBMISSION QUEUE ENTRY
    io_uring_sqe *sqe = io_uring_get_sqe(&ring_);
    if (!sqe) {
      throw std::runtime_error{"Could not get SQE. Queue could be possibly full.\n"};
    }

    // PREPARE WRITE
    io_uring_prep_write(sqe, file.fd(), contents->data(), contents->size(), (uint64_t)-1);

    // TRANSFER OWNERSHIP OF THE DATA
    sqe->user_data = reinterpret_cast<uintptr_t>(contents.release());

    pending_sqe_count_++;

    // SUBMIT WRITE (if depth size reached)
    if (pending_sqe_count_ >= queue_size_) {
      submitPendingSQEs(); // non-blocking
    }
  }

private:
  size_t pending_sqe_count_;
  size_t queue_size_;
  io_uring ring_;

  std::jthread worker_;

  inline void submitPendingSQEs() {
    if (pending_sqe_count_ == 0) return;

    size_t status = io_uring_submit(&ring_);
    if (status < pending_sqe_count_) {
      std::string err{"io_uring_submit status is less than the pending_sqe_count. Status: " + std::to_string(status) + ", pending_sqe_count = " + std::to_string(pending_sqe_count_)};
      throw std::runtime_error{err};
    }

    pending_sqe_count_ = 0;
  }

  inline void eventLoop(std::stop_token stop_token) {

    while (!stop_token.stop_requested()) {

      io_uring_cqe* cqe = nullptr;
      int ret = io_uring_peek_cqe(&ring_, &cqe);
      if (ret != 0 || cqe == nullptr) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // avoid busy wait
        continue;
      }

      if (cqe->res < 0) {
        std::cerr << "cqe->res is less than 0: " << cqe->res << std::endl;
        // TODO: handle or log error
      }

      io_uring_cqe_seen(&ring_, cqe);
    }

    submitPendingSQEs();

    // ON SHUTDOWN
    io_uring_cqe* cqe;
    while (io_uring_peek_cqe(&ring_, &cqe) == 0) {
      std::cout << "Entered peak for " << *(reinterpret_cast<std::string*>(cqe->user_data)) << std::endl;
      std::unique_ptr<std::string> buffer(
        reinterpret_cast<std::string*>(cqe->user_data)
      );
      io_uring_cqe_seen(&ring_, cqe);
    }
  }


};

} // namespace MR::IO