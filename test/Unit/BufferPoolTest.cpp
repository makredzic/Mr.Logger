#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <MR/Memory/BufferPool.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

namespace MR::Memory::Test {

class BufferPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        pool_ = std::make_unique<BufferPool>();
    }

    void TearDown() override {
        pool_.reset();
    }

    std::unique_ptr<BufferPool> pool_;
};

TEST_F(BufferPoolTest, ConstructorInitialization) {
    EXPECT_EQ(pool_->getTotalBuffers(), BufferPool::SMALL_POOL_SIZE + BufferPool::MEDIUM_POOL_SIZE + BufferPool::LARGE_POOL_SIZE);
    EXPECT_EQ(pool_->getAvailableBuffers(), BufferPool::SMALL_POOL_SIZE + BufferPool::MEDIUM_POOL_SIZE + BufferPool::LARGE_POOL_SIZE);
}

TEST_F(BufferPoolTest, AcquireSmallBuffer) {
    auto buffer = pool_->acquire(512);

    ASSERT_NE(buffer, nullptr);
    EXPECT_EQ(buffer->capacity, BufferPool::SMALL_BUFFER_SIZE);
    EXPECT_EQ(buffer->size, 0);
    EXPECT_NE(buffer->data, nullptr);
}

TEST_F(BufferPoolTest, AcquireMediumBuffer) {
    auto buffer = pool_->acquire(2048);

    ASSERT_NE(buffer, nullptr);
    EXPECT_EQ(buffer->capacity, BufferPool::MEDIUM_BUFFER_SIZE);
    EXPECT_EQ(buffer->size, 0);
    EXPECT_NE(buffer->data, nullptr);
}

TEST_F(BufferPoolTest, AcquireLargeBuffer) {
    auto buffer = pool_->acquire(8192);

    ASSERT_NE(buffer, nullptr);
    EXPECT_EQ(buffer->capacity, BufferPool::LARGE_BUFFER_SIZE);
    EXPECT_EQ(buffer->size, 0);
    EXPECT_NE(buffer->data, nullptr);
}

TEST_F(BufferPoolTest, AcquireVeryLargeBuffer) {
    size_t very_large_size = 32768;
    auto buffer = pool_->acquire(very_large_size);

    ASSERT_NE(buffer, nullptr);
    EXPECT_EQ(buffer->capacity, very_large_size);
    EXPECT_EQ(buffer->size, 0);
    EXPECT_NE(buffer->data, nullptr);
}

TEST_F(BufferPoolTest, AcquireZeroSizeBuffer) {
    auto buffer = pool_->acquire(0);

    ASSERT_NE(buffer, nullptr);
    EXPECT_EQ(buffer->capacity, BufferPool::SMALL_BUFFER_SIZE);
}

TEST_F(BufferPoolTest, AcquireExactBoundarySize) {
    auto small_buffer = pool_->acquire(BufferPool::SMALL_BUFFER_SIZE);
    auto medium_buffer = pool_->acquire(BufferPool::MEDIUM_BUFFER_SIZE);
    auto large_buffer = pool_->acquire(BufferPool::LARGE_BUFFER_SIZE);

    EXPECT_EQ(small_buffer->capacity, BufferPool::SMALL_BUFFER_SIZE);
    EXPECT_EQ(medium_buffer->capacity, BufferPool::MEDIUM_BUFFER_SIZE);
    EXPECT_EQ(large_buffer->capacity, BufferPool::LARGE_BUFFER_SIZE);
}

TEST_F(BufferPoolTest, ReleaseSmallBuffer) {
    size_t initial_available = pool_->getAvailableBuffers();

    auto buffer = pool_->acquire(512);
    EXPECT_EQ(pool_->getAvailableBuffers(), initial_available - 1);

    pool_->release(std::move(buffer));
    EXPECT_EQ(pool_->getAvailableBuffers(), initial_available);
}

TEST_F(BufferPoolTest, ReleaseMediumBuffer) {
    size_t initial_available = pool_->getAvailableBuffers();

    auto buffer = pool_->acquire(2048);
    EXPECT_EQ(pool_->getAvailableBuffers(), initial_available - 1);

    pool_->release(std::move(buffer));
    EXPECT_EQ(pool_->getAvailableBuffers(), initial_available);
}

TEST_F(BufferPoolTest, ReleaseLargeBuffer) {
    size_t initial_available = pool_->getAvailableBuffers();

    auto buffer = pool_->acquire(8192);
    EXPECT_EQ(pool_->getAvailableBuffers(), initial_available - 1);

    pool_->release(std::move(buffer));
    EXPECT_EQ(pool_->getAvailableBuffers(), initial_available);
}

TEST_F(BufferPoolTest, ReleaseNullBuffer) {
    size_t initial_available = pool_->getAvailableBuffers();

    pool_->release(nullptr);

    EXPECT_EQ(pool_->getAvailableBuffers(), initial_available);
}

TEST_F(BufferPoolTest, ReleaseVeryLargeBuffer) {
    size_t initial_available = pool_->getAvailableBuffers();

    auto buffer = pool_->acquire(32768);
    pool_->release(std::move(buffer));

    EXPECT_EQ(pool_->getAvailableBuffers(), initial_available);
}

TEST_F(BufferPoolTest, ExhaustSmallPool) {
    std::vector<std::unique_ptr<Buffer>> buffers;

    for (size_t i = 0; i < BufferPool::SMALL_POOL_SIZE + 5; ++i) {
        auto buffer = pool_->acquire(512);
        ASSERT_NE(buffer, nullptr);
        EXPECT_EQ(buffer->capacity, BufferPool::SMALL_BUFFER_SIZE);
        buffers.push_back(std::move(buffer));
    }

    EXPECT_EQ(pool_->getAvailableBuffers(), BufferPool::MEDIUM_POOL_SIZE + BufferPool::LARGE_POOL_SIZE);
}

TEST_F(BufferPoolTest, ExhaustMediumPool) {
    std::vector<std::unique_ptr<Buffer>> buffers;

    for (size_t i = 0; i < BufferPool::MEDIUM_POOL_SIZE + 5; ++i) {
        auto buffer = pool_->acquire(2048);
        ASSERT_NE(buffer, nullptr);
        EXPECT_EQ(buffer->capacity, BufferPool::MEDIUM_BUFFER_SIZE);
        buffers.push_back(std::move(buffer));
    }

    EXPECT_EQ(pool_->getAvailableBuffers(), BufferPool::SMALL_POOL_SIZE + BufferPool::LARGE_POOL_SIZE);
}

TEST_F(BufferPoolTest, ExhaustLargePool) {
    std::vector<std::unique_ptr<Buffer>> buffers;

    for (size_t i = 0; i < BufferPool::LARGE_POOL_SIZE + 5; ++i) {
        auto buffer = pool_->acquire(8192);
        ASSERT_NE(buffer, nullptr);
        EXPECT_EQ(buffer->capacity, BufferPool::LARGE_BUFFER_SIZE);
        buffers.push_back(std::move(buffer));
    }

    EXPECT_EQ(pool_->getAvailableBuffers(), BufferPool::SMALL_POOL_SIZE + BufferPool::MEDIUM_POOL_SIZE);
}

TEST_F(BufferPoolTest, AcquireReleasePattern) {
    for (int iteration = 0; iteration < 10; ++iteration) {
        std::vector<std::unique_ptr<Buffer>> buffers;

        for (size_t i = 0; i < 10; ++i) {
            buffers.push_back(pool_->acquire(512));
            buffers.push_back(pool_->acquire(2048));
            buffers.push_back(pool_->acquire(8192));
        }

        for (auto& buffer : buffers) {
            pool_->release(std::move(buffer));
        }

        size_t expected_total = BufferPool::SMALL_POOL_SIZE + BufferPool::MEDIUM_POOL_SIZE + BufferPool::LARGE_POOL_SIZE;
        EXPECT_EQ(pool_->getAvailableBuffers(), expected_total);
    }
}

class BufferPoolRaceConditionTest : public ::testing::Test {
protected:
    void SetUp() override {
        pool_ = std::make_unique<BufferPool>();
    }

    void TearDown() override {
        pool_.reset();
    }

    std::unique_ptr<BufferPool> pool_;
};

TEST_F(BufferPoolRaceConditionTest, ConcurrentAcquireSmallBuffers) {
    const int num_threads = 10;
    const int buffers_per_thread = 20;
    std::vector<std::thread> threads;
    std::atomic<int> successful_acquisitions{0};
    std::atomic<int> failed_acquisitions{0};

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < buffers_per_thread; ++i) {
                auto buffer = pool_->acquire(512);
                if (buffer != nullptr) {
                    successful_acquisitions++;
                    EXPECT_EQ(buffer->capacity, BufferPool::SMALL_BUFFER_SIZE);
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                } else {
                    failed_acquisitions++;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(successful_acquisitions + failed_acquisitions, num_threads * buffers_per_thread);
    EXPECT_GT(successful_acquisitions, 0);
}

TEST_F(BufferPoolRaceConditionTest, ConcurrentAcquireReleaseCycle) {
    const int num_threads = 8;
    const int cycles_per_thread = 50;
    std::vector<std::thread> threads;
    std::atomic<int> successful_cycles{0};

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < cycles_per_thread; ++i) {
                auto buffer = pool_->acquire(1024);
                if (buffer != nullptr) {
                    EXPECT_EQ(buffer->capacity, BufferPool::SMALL_BUFFER_SIZE);
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                    pool_->release(std::move(buffer));
                    successful_cycles++;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_GT(successful_cycles, 0);

    size_t expected_total = BufferPool::SMALL_POOL_SIZE + BufferPool::MEDIUM_POOL_SIZE + BufferPool::LARGE_POOL_SIZE;
    EXPECT_EQ(pool_->getAvailableBuffers(), expected_total);
}

TEST_F(BufferPoolRaceConditionTest, ConcurrentMixedSizeAcquisitions) {
    const int num_threads = 6;
    const int operations_per_thread = 30;
    std::vector<std::thread> threads;
    std::atomic<int> total_operations{0};

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            std::vector<std::unique_ptr<Buffer>> buffers;

            for (int i = 0; i < operations_per_thread; ++i) {
                size_t size;
                switch ((i + t) % 3) {
                    case 0: size = 512; break;
                    case 1: size = 2048; break;
                    case 2: size = 8192; break;
                }

                auto buffer = pool_->acquire(size);
                if (buffer != nullptr) {
                    buffers.push_back(std::move(buffer));
                    total_operations++;
                }

                if (i % 5 == 0 && !buffers.empty()) {
                    pool_->release(std::move(buffers.back()));
                    buffers.pop_back();
                }
            }

            for (auto& buffer : buffers) {
                pool_->release(std::move(buffer));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_GT(total_operations, 0);

    size_t expected_total = BufferPool::SMALL_POOL_SIZE + BufferPool::MEDIUM_POOL_SIZE + BufferPool::LARGE_POOL_SIZE;
    EXPECT_EQ(pool_->getAvailableBuffers(), expected_total);
}

TEST_F(BufferPoolRaceConditionTest, StressTestHighContention) {
    const int num_threads = 16;
    const int duration_ms = 100;
    std::vector<std::thread> threads;
    std::atomic<bool> stop_flag{false};
    std::atomic<int> acquisitions{0};
    std::atomic<int> releases{0};

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            std::vector<std::unique_ptr<Buffer>> local_buffers;

            while (!stop_flag) {
                if (local_buffers.size() < 5) {
                    auto buffer = pool_->acquire(512 + (t % 3) * 1024);
                    if (buffer != nullptr) {
                        local_buffers.push_back(std::move(buffer));
                        acquisitions++;
                    }
                }

                if (!local_buffers.empty() && local_buffers.size() > 2) {
                    pool_->release(std::move(local_buffers.back()));
                    local_buffers.pop_back();
                    releases++;
                }
            }

            for (auto& buffer : local_buffers) {
                pool_->release(std::move(buffer));
                releases++;
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
    stop_flag = true;

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_GT(acquisitions, 0);
    EXPECT_GT(releases, 0);

    size_t expected_total = BufferPool::SMALL_POOL_SIZE + BufferPool::MEDIUM_POOL_SIZE + BufferPool::LARGE_POOL_SIZE;
    EXPECT_EQ(pool_->getAvailableBuffers(), expected_total);
}

TEST_F(BufferPoolRaceConditionTest, ConcurrentGetAvailableBuffers) {
    const int num_threads = 4;
    const int num_operations = 100;
    std::vector<std::thread> threads;
    std::atomic<bool> start_flag{false};

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            while (!start_flag) {
                std::this_thread::yield();
            }

            for (int i = 0; i < num_operations; ++i) {
                size_t available = pool_->getAvailableBuffers();
                EXPECT_LE(available, pool_->getTotalBuffers());

                auto buffer = pool_->acquire(512);
                if (buffer != nullptr) {
                    pool_->release(std::move(buffer));
                }
            }
        });
    }

    start_flag = true;

    for (auto& thread : threads) {
        thread.join();
    }

    size_t expected_total = BufferPool::SMALL_POOL_SIZE + BufferPool::MEDIUM_POOL_SIZE + BufferPool::LARGE_POOL_SIZE;
    EXPECT_EQ(pool_->getAvailableBuffers(), expected_total);
}

}