#pragma once

#include <MR/Memory/Buffer.hpp>
#include <memory>
#include <atomic>
#include <vector>

namespace MR::Memory {
struct Pool {
    std::vector<std::unique_ptr<Buffer>> buffers;
    std::atomic<size_t> next_index{0};
    size_t pool_size;
    size_t buffer_size;
    
    Pool(size_t pool_sz, size_t buf_sz);
    
    std::unique_ptr<Buffer> tryAcquire();
    bool tryRelease(std::unique_ptr<Buffer> buffer);
};
}