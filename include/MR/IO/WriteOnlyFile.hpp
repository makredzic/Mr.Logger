#pragma once

#include <stdexcept>
#include <string>
#include <fcntl.h>
#include <unistd.h>


namespace MR::IO {
class WriteOnlyFile {

private:
  std::string path_;
  int fd_;

  inline void close() noexcept {
    if (!fd_) return;
    ::close(fd_);
    fd_ = -1;
  }

public:

  inline explicit WriteOnlyFile(std::string_view file_path) : path_{file_path}, fd_{-1} {
    fd_ = open(file_path.data(), O_WRONLY | O_CREAT | O_APPEND, 0644); // O_TRUNCATE possibly too?
    if (fd_ < 0) {
      throw std::runtime_error("Fail to open log file");
    }
  }

  inline WriteOnlyFile(WriteOnlyFile &&other) noexcept : fd_{other.fd_} {
    other.fd_ = -1;
  }

  inline WriteOnlyFile& operator=(WriteOnlyFile&& other) {
    if (this == &other) return *this;

    close();
    fd_ = other.fd_;
    other.fd_ = -1;
    return *this;
  }

  inline WriteOnlyFile(const WriteOnlyFile&) = delete;
  inline WriteOnlyFile& operator=(const WriteOnlyFile&) = delete;

  inline ~WriteOnlyFile() noexcept {
    if (!fd_) return; 
    ::close(fd_);
  }

  inline int fd() const { return fd_; }
  inline const std::string& path() const { return path_; }
};
} // namespace MR::IO