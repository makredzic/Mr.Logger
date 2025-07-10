#pragma once

#include <cstddef>
#include <cstdlib>

namespace MR::Memory {
struct Buffer {
    void* data;
    size_t size;
    size_t capacity;
    
    inline Buffer(size_t cap) : size(0), capacity(cap) {
        data = malloc(cap);
    }
    
    inline ~Buffer() {
        if (data) {
            free(data);
        }
    }
    
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    
    inline Buffer(Buffer&& other) noexcept : data(other.data), size(other.size), capacity(other.capacity) {
        other.data = nullptr;
        other.size = 0;
        other.capacity = 0;
    }
    
    inline Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            if (data) free(data);
            data = other.data;
            size = other.size;
            capacity = other.capacity;
            other.data = nullptr;
            other.size = 0;
            other.capacity = 0;
        }
        return *this;
    }
    
    inline void clear() {
        size = 0;
    }
    
    inline char* as_char() const {
        return static_cast<char*>(data);
    }
};
}