#pragma once

#include <MR/Memory/Buffer.hpp>
#include <MR/Memory/Pool.hpp>

#include <memory>
#include <cstddef>

namespace MR::Memory {

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
    Pool small_pool_;
    Pool medium_pool_;
    Pool large_pool_;
    
    std::unique_ptr<Buffer> createBuffer(size_t size);
};

} // namespace MR::Memory