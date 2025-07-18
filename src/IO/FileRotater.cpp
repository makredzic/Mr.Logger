#include <MR/IO/FileRotater.hpp>
#include <filesystem>
#include <string>
#include <string_view>

namespace MR::IO {

FileRotater::FileRotater(const std::string& filename, size_t max_size_bytes)
    : max_size_bytes_(max_size_bytes), current_size_(0) {
    extractBaseAndExtension(filename);
}

void FileRotater::extractBaseAndExtension(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos != std::string::npos && dot_pos > 0) {
        base_name_ = filename.substr(0, dot_pos);
        extension_ = filename.substr(dot_pos);
    } else {
        base_name_ = filename;
        extension_ = "";
    }
}

std::string FileRotater::getNextRotatedName() {
    int counter = 1;
    std::string rotated_name;
    
    do {
        rotated_name = base_name_ + std::to_string(counter) + extension_;
        counter++;
    } while (std::filesystem::exists(rotated_name));
    
    return rotated_name;
}

bool FileRotater::shouldRotate() const {
    return current_size_ >= max_size_bytes_;
}

void FileRotater::rotate() {
    std::string current_filename = getCurrentFilename();
    
    if (std::filesystem::exists(current_filename)) {
        std::string rotated_name = getNextRotatedName();
        std::filesystem::rename(current_filename, rotated_name);
    }
    
    current_size_ = 0;
}

void FileRotater::updateCurrentSize(size_t bytes_written) {
    current_size_ += bytes_written;
}

const std::string& FileRotater::getCurrentFilename() const {
    static std::string current_filename = base_name_ + extension_;
    return current_filename;
}

void FileRotater::reset() {
    current_size_ = 0;
}

}