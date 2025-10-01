
#include <MR/Memory/Pool.hpp>
#include <MR/Memory/Buffer.hpp>

namespace MR::Memory {
    
    Pool::Pool(size_t pool_sz, size_t buf_sz) : pool_size(pool_sz), buffer_size(buf_sz) {
        buffers.reserve(pool_sz);
        for (size_t i = 0; i < pool_sz; ++i) {
            buffers.emplace_back(std::make_unique<Buffer>(buf_sz));
        }
    }
    
    std::unique_ptr<Buffer> Pool::tryAcquire() {
        std::lock_guard<std::mutex> lock(mutex_);
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
    
    bool Pool::tryRelease(std::unique_ptr<Buffer> buffer) {
        if (buffer->capacity != buffer_size) {
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        for (size_t i = 0; i < pool_size; ++i) {
            if (!buffers[i]) {
                buffers[i] = std::move(buffer);
                return true;
            }
        }
        return false;
    }
}