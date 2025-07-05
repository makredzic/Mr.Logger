#pragma once

#include <cstddef>
#include <iostream>
#include <liburing.h>
#include <stdexcept>
#include <coroutine>

#include <MR/IO/WriteOnlyFile.hpp>


namespace MR::IO {
class IOUring {
public:
   struct WriteAwaiter {
    IOUring* ring;
    const WriteOnlyFile& file;
    void* buffer;
    size_t len;
    int result = -1;  // Store io_uring result
    
    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> h) {
        
      auto* user_data = new UserData{h, this};
      
      // SUBMIT TO IO_URING
      ring->prepareWrite(file, buffer, len, user_data);
    }
    int await_resume() {
      // Return the I/O result (bytes written or error)
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

    // TODO

  }

  IOUring(const IOUring &) = delete;
  IOUring &operator=(const IOUring &) = delete;
  IOUring(IOUring &&) = delete;
  IOUring &operator=(IOUring &&) = delete;

  inline size_t capacity() const { return queue_depth_; }

  inline WriteAwaiter write(const WriteOnlyFile& file, void* buffer, size_t len) {
    return WriteAwaiter{this, file, buffer, len, -1};
  }

  inline void processCompletions() {

    io_uring_cqe* cqe;
    unsigned head;
    unsigned completed = 0;
    
    io_uring_for_each_cqe(&ring_, head, cqe) {

        auto* user_data = static_cast<UserData*>(io_uring_cqe_get_data(cqe));
        
        if (user_data) {
            // Store the I/O result in the awaiter
            user_data->awaiter->result = cqe->res;
            user_data->handle.resume();
            delete user_data;
        }
        completed++;
    }
    
    if (completed > 0) {
        io_uring_cq_advance(&ring_, completed);
    }
  }

  inline void submitPendingSQEs() {

    size_t status = io_uring_submit(&ring_);
    if (status < 0) {
      std::string err{"io_uring_submit status is less than the pending_sqe_count. Status: " + std::to_string(status)};
      throw std::runtime_error{err};
    }

  }

private:
  size_t queue_depth_;
  io_uring ring_;

  inline void prepareWrite(const WriteOnlyFile& file, void* buffer, size_t size, void* user_data) {

    // GET SUBMISSION QUEUE ENTRY
    io_uring_sqe *sqe = io_uring_get_sqe(&ring_);
    if (!sqe) {
      std::cout << "Could not get SQE. Queue could be possibly full.\n";
      throw std::runtime_error{"get sqe"};
    }

    // PREPARE WRITE
    io_uring_sqe_set_data(sqe, user_data);
    io_uring_prep_write(sqe, file.fd(), buffer, size, (uint64_t)-1);
  }


};

} // namespace MR::IO