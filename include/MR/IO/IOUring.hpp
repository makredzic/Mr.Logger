#pragma once
#include <cstddef>
#include <liburing.h>

#include <MR/IO/WriteOnlyFile.hpp>
#include <stdexcept>

namespace MR::IO {
class IOUring {
public:
  inline explicit IOUring(size_t queue_depth) {
    auto status = io_uring_queue_init(queue_depth, &ring_, 0);
    if (status < 0) {
      throw std::runtime_error("error initializing io_uring: " +
                               std::to_string(status));
    }
  }

  inline ~IOUring() { io_uring_queue_exit(&ring_); }

  IOUring(const IOUring &) = delete;
  IOUring &operator=(const IOUring &) = delete;
  IOUring(IOUring &&) = delete;
  IOUring &operator=(IOUring &&) = delete;

  inline void submitWrite(std::string_view data, const WriteOnlyFile& file) {

    // GET SUBMISSION QUEUE ENTRY
    io_uring_sqe *sqe = io_uring_get_sqe(&ring_);
    if (!sqe) {
      throw std::runtime_error{"Could not get SQE. Queue could be possibly full.\n"};
    }

    // PREPARE WRITE
    io_uring_prep_write(sqe, file.fd(), data.data(), data.size(), (uint64_t)-1);


    // SUBMIT WRITE
    auto status = io_uring_submit(&ring_);
    if (status < 1 ) {
      std::string err{"io_uring_submit status failed: " + std::to_string(status)};
      throw std::runtime_error{err};
    }

  }

private:
  io_uring ring_;
};

} // namespace MR::IO