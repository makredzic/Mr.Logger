#include <MR/Memory/BufferPool.hpp>

namespace MR::Memory {

BufferPool::BufferPool() 
    : small_pool_(SMALL_POOL_SIZE, SMALL_BUFFER_SIZE),
      medium_pool_(MEDIUM_POOL_SIZE, MEDIUM_BUFFER_SIZE),
      large_pool_(LARGE_POOL_SIZE, LARGE_BUFFER_SIZE) {
}

BufferPool::~BufferPool() = default;

std::unique_ptr<Buffer> BufferPool::acquire(size_t required_size) {
    std::unique_ptr<Buffer> buffer = nullptr;
    
    if (required_size <= SMALL_BUFFER_SIZE) {
        buffer = small_pool_.tryAcquire();
    } else if (required_size <= MEDIUM_BUFFER_SIZE) {
        buffer = medium_pool_.tryAcquire();
    } else if (required_size <= LARGE_BUFFER_SIZE) {
        buffer = large_pool_.tryAcquire();
    }
    
    if (!buffer) {
        buffer = createBuffer(required_size);
    }
    
    return buffer;
}

void BufferPool::release(std::unique_ptr<Buffer> buffer) {
    if (!buffer) return;
    
    bool released = false;
    
    if (buffer->capacity == SMALL_BUFFER_SIZE) {
        released = small_pool_.tryRelease(std::move(buffer));
    } else if (buffer->capacity == MEDIUM_BUFFER_SIZE) {
        released = medium_pool_.tryRelease(std::move(buffer));
    } else if (buffer->capacity == LARGE_BUFFER_SIZE) {
        released = large_pool_.tryRelease(std::move(buffer));
    }
    
    if (!released) {
        // Buffer will be destroyed when it goes out of scope
    }
}

size_t BufferPool::getTotalBuffers() const {
    return SMALL_POOL_SIZE + MEDIUM_POOL_SIZE + LARGE_POOL_SIZE;
}

size_t BufferPool::getAvailableBuffers() const {
    size_t available = 0;
    
    for (const auto& buffer : small_pool_.buffers) {
        if (buffer) ++available;
    }
    for (const auto& buffer : medium_pool_.buffers) {
        if (buffer) ++available;
    }
    for (const auto& buffer : large_pool_.buffers) {
        if (buffer) ++available;
    }
    
    return available;
}

std::unique_ptr<Buffer> BufferPool::createBuffer(size_t size) {
    return std::make_unique<Buffer>(size);
}

} // namespace MR::Memory