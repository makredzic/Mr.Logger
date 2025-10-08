#pragma once

#include <MR/Memory/Buffer.hpp>
#include <memory>
#include <vector>
#include <mutex>

namespace MR::Memory {
struct Pool {
    std::vector<std::unique_ptr<Buffer>> buffers;
    size_t next_index;
    size_t pool_size;
    size_t buffer_size;
    mutable std::mutex mutex_;

    Pool(size_t pool_sz, size_t buf_sz);

    std::unique_ptr<Buffer> tryAcquire();
    bool tryRelease(std::unique_ptr<Buffer> buffer);
};
}