#pragma once

#include <memory>
#include <vector>
#include <atomic>
#include <cstddef>

namespace MR::Logger {

struct Buffer {
    void* data;
    size_t size;
    size_t capacity;
    
    Buffer(size_t cap) : size(0), capacity(cap) {
        data = malloc(cap);
    }
    
    ~Buffer() {
        if (data) {
            free(data);
        }
    }
    
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    
    Buffer(Buffer&& other) noexcept : data(other.data), size(other.size), capacity(other.capacity) {
        other.data = nullptr;
        other.size = 0;
        other.capacity = 0;
    }
    
    Buffer& operator=(Buffer&& other) noexcept {
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
    
    void clear() {
        size = 0;
    }
    
    char* as_char() const {
        return static_cast<char*>(data);
    }
};

class BufferPool {
public:
    static constexpr size_t SMALL_BUFFER_SIZE = 1024;
    static constexpr size_t MEDIUM_BUFFER_SIZE = 4096;
    static constexpr size_t LARGE_BUFFER_SIZE = 16384;
    
    static constexpr size_t SMALL_POOL_SIZE = 128;
    static constexpr size_t MEDIUM_POOL_SIZE = 64;
    static constexpr size_t LARGE_POOL_SIZE = 32;
    
    BufferPool();
    ~BufferPool();
    
    std::unique_ptr<Buffer> acquire(size_t required_size);
    void release(std::unique_ptr<Buffer> buffer);
    
    size_t getTotalBuffers() const;
    size_t getAvailableBuffers() const;
    
private:
    struct Pool {
        std::vector<std::unique_ptr<Buffer>> buffers;
        std::atomic<size_t> next_index{0};
        size_t pool_size;
        size_t buffer_size;
        
        Pool(size_t pool_sz, size_t buf_sz) : pool_size(pool_sz), buffer_size(buf_sz) {
            buffers.reserve(pool_sz);
            for (size_t i = 0; i < pool_sz; ++i) {
                buffers.emplace_back(std::make_unique<Buffer>(buf_sz));
            }
        }
        
        std::unique_ptr<Buffer> tryAcquire() {
            for (size_t i = 0; i < pool_size; ++i) {
                size_t idx = next_index.fetch_add(1, std::memory_order_relaxed) % pool_size;
                if (buffers[idx]) {
                    auto buffer = std::move(buffers[idx]);
                    buffer->clear();
                    return buffer;
                }
            }
            return nullptr;
        }
        
        bool tryRelease(std::unique_ptr<Buffer> buffer) {
            if (buffer->capacity != buffer_size) {
                return false;
            }
            
            for (size_t i = 0; i < pool_size; ++i) {
                if (!buffers[i]) {
                    buffers[i] = std::move(buffer);
                    return true;
                }
            }
            return false;
        }
    };
    
    Pool small_pool_;
    Pool medium_pool_;
    Pool large_pool_;
    
    std::unique_ptr<Buffer> createBuffer(size_t size);
};

} // namespace MR::Logger