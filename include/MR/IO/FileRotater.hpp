#pragma once

#include <string>

namespace MR::IO {

  class FileRotater {
  private:
    std::string base_name_;
    std::string extension_;
    size_t max_size_bytes_;
    size_t current_size_;
    
    std::string getNextRotatedName();
    void extractBaseAndExtension(const std::string& filename);
    
  public:
    FileRotater(const std::string& filename, size_t max_size_bytes);
    
    bool shouldRotate() const;
    void rotate();
    void updateCurrentSize(size_t bytes_written);
    const std::string& getCurrentFilename() const;
    void reset();
  };

}