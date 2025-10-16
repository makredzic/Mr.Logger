#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <MR/Queue/FixedSizeBlockingQueue.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

namespace MR::Queue::Test {

class FixedSizeBlockingQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        queue_ = std::make_unique<FixedSizeBlockingQueue<int>>(10);
    }

    void TearDown() override {
        queue_->shutdown();
        queue_.reset();
    }

    std::unique_ptr<FixedSizeBlockingQueue<int>> queue_;
};

TEST_F(FixedSizeBlockingQueueTest, ConstructorInitialization) {
    EXPECT_TRUE(queue_->empty());
    EXPECT_EQ(queue_->size(), 0);
}

TEST_F(FixedSizeBlockingQueueTest, ConstructorWithCapacity) {
    auto large_queue = std::make_unique<FixedSizeBlockingQueue<int>>(100);
    EXPECT_TRUE(large_queue->empty());
    EXPECT_EQ(large_queue->size(), 0);
    large_queue->shutdown();
}

TEST_F(FixedSizeBlockingQueueTest, PushAndTryPopSingleElement) {
    queue_->push(42);

    EXPECT_FALSE(queue_->empty());
    EXPECT_EQ(queue_->size(), 1);

    auto result = queue_->tryPop();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 42);

    EXPECT_TRUE(queue_->empty());
    EXPECT_EQ(queue_->size(), 0);
}

TEST_F(FixedSizeBlockingQueueTest, PushRvalueAndTryPop) {
    int value = 100;
    queue_->push(std::move(value));

    auto result = queue_->tryPop();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 100);
}

TEST_F(FixedSizeBlockingQueueTest, PushMultipleElements) {
    for (int i = 0; i < 5; ++i) {
        queue_->push(i);
    }

    EXPECT_EQ(queue_->size(), 5);

    for (int i = 0; i < 5; ++i) {
        auto result = queue_->tryPop();
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), i);
    }

    EXPECT_TRUE(queue_->empty());
}

TEST_F(FixedSizeBlockingQueueTest, TryPopOnEmptyQueueDoesNotBlock) {
    auto start = std::chrono::steady_clock::now();
    auto result = queue_->tryPop();
    auto duration = std::chrono::steady_clock::now() - start;

    EXPECT_FALSE(result.has_value());
    EXPECT_LT(duration, std::chrono::milliseconds(10));
}

TEST_F(FixedSizeBlockingQueueTest, PopBlocksUntilElementAvailable) {
    std::atomic<bool> popped{false};
    int popped_value = 0;

    std::thread consumer([&]() {
        auto result = queue_->pop();
        if (result.has_value()) {
            popped_value = result.value();
            popped = true;
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(popped);

    queue_->push(99);

    consumer.join();
    EXPECT_TRUE(popped);
    EXPECT_EQ(popped_value, 99);
}

TEST_F(FixedSizeBlockingQueueTest, PushBlocksWhenQueueFull) {
    for (int i = 0; i < 10; ++i) {
        queue_->push(i);
    }

    EXPECT_EQ(queue_->size(), 10);

    std::atomic<bool> push_completed{false};

    std::thread producer([&]() {
        queue_->push(999);
        push_completed = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(push_completed);

    auto result = queue_->tryPop();
    ASSERT_TRUE(result.has_value());

    producer.join();
    EXPECT_TRUE(push_completed);
    EXPECT_EQ(queue_->size(), 10);
}

TEST_F(FixedSizeBlockingQueueTest, ShutdownUnblocksWaitingPop) {
    std::atomic<bool> returned{false};

    std::thread consumer([&]() {
        auto result = queue_->pop();
        EXPECT_FALSE(result.has_value());
        returned = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(returned);

    queue_->shutdown();

    consumer.join();
    EXPECT_TRUE(returned);
}

TEST_F(FixedSizeBlockingQueueTest, ShutdownUnblocksWaitingPush) {
    for (int i = 0; i < 10; ++i) {
        queue_->push(i);
    }

    std::atomic<bool> returned{false};

    std::thread producer([&]() {
        queue_->push(999);
        returned = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(returned);

    queue_->shutdown();

    producer.join();
    EXPECT_TRUE(returned);
}

TEST_F(FixedSizeBlockingQueueTest, PushAfterShutdownDoesNothing) {
    queue_->shutdown();

    queue_->push(42);

    EXPECT_TRUE(queue_->empty());
    EXPECT_EQ(queue_->size(), 0);
}

TEST_F(FixedSizeBlockingQueueTest, FIFOOrdering) {
    std::vector<int> values = {10, 20, 30, 40, 50};

    for (int val : values) {
        queue_->push(val);
    }

    for (int val : values) {
        auto result = queue_->tryPop();
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), val);
    }
}

TEST_F(FixedSizeBlockingQueueTest, CircularBufferWraparound) {
    // Fill and drain multiple times to test wraparound
    for (int round = 0; round < 3; ++round) {
        for (int i = 0; i < 10; ++i) {
            queue_->push(round * 10 + i);
        }

        for (int i = 0; i < 10; ++i) {
            auto result = queue_->tryPop();
            ASSERT_TRUE(result.has_value());
            EXPECT_EQ(result.value(), round * 10 + i);
        }

        EXPECT_TRUE(queue_->empty());
    }
}

TEST_F(FixedSizeBlockingQueueTest, PartialFillAndDrain) {
    // Push 5, pop 3, push 5, pop 7
    for (int i = 0; i < 5; ++i) {
        queue_->push(i);
    }

    for (int i = 0; i < 3; ++i) {
        auto result = queue_->tryPop();
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), i);
    }

    for (int i = 5; i < 10; ++i) {
        queue_->push(i);
    }

    EXPECT_EQ(queue_->size(), 7);

    for (int i = 3; i < 10; ++i) {
        auto result = queue_->tryPop();
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), i);
    }

    EXPECT_TRUE(queue_->empty());
}

TEST_F(FixedSizeBlockingQueueTest, SizeTracking) {
    EXPECT_EQ(queue_->size(), 0);

    queue_->push(1);
    EXPECT_EQ(queue_->size(), 1);

    queue_->push(2);
    EXPECT_EQ(queue_->size(), 2);

    queue_->tryPop();
    EXPECT_EQ(queue_->size(), 1);

    queue_->tryPop();
    EXPECT_EQ(queue_->size(), 0);
}

TEST_F(FixedSizeBlockingQueueTest, EmptyCheck) {
    EXPECT_TRUE(queue_->empty());

    queue_->push(1);
    EXPECT_FALSE(queue_->empty());

    queue_->tryPop();
    EXPECT_TRUE(queue_->empty());
}

class FixedSizeBlockingQueueConcurrencyTest : public ::testing::Test {
protected:
    void SetUp() override {
        queue_ = std::make_unique<FixedSizeBlockingQueue<int>>(50);
    }

    void TearDown() override {
        queue_->shutdown();
        queue_.reset();
    }

    std::unique_ptr<FixedSizeBlockingQueue<int>> queue_;
};

TEST_F(FixedSizeBlockingQueueConcurrencyTest, ConcurrentPushAndTryPop) {
    const int num_threads = 4;
    const int elements_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> successful_pops{0};

    // Producer threads
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < elements_per_thread; ++i) {
                queue_->push(t * elements_per_thread + i);
            }
        });
    }

    // Consumer threads
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < elements_per_thread; ++i) {
                while (true) {
                    auto result = queue_->tryPop();
                    if (result.has_value()) {
                        successful_pops++;
                        break;
                    }
                    std::this_thread::yield();
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(successful_pops, num_threads * elements_per_thread);
    EXPECT_TRUE(queue_->empty());
}

TEST_F(FixedSizeBlockingQueueConcurrencyTest, ConcurrentPushAndBlockingPop) {
    const int num_producers = 2;
    const int num_consumers = 2;
    const int elements_per_producer = 100;
    std::vector<std::thread> threads;
    std::atomic<int> total_consumed{0};

    // Consumer threads (start first, will block)
    for (int t = 0; t < num_consumers; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < (num_producers * elements_per_producer) / num_consumers; ++i) {
                auto result = queue_->pop();
                if (result.has_value()) {
                    total_consumed++;
                }
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Producer threads
    for (int t = 0; t < num_producers; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < elements_per_producer; ++i) {
                queue_->push(t * elements_per_producer + i);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(total_consumed, num_producers * elements_per_producer);
}

TEST_F(FixedSizeBlockingQueueConcurrencyTest, ProducerBlocksOnFullQueue) {
    auto small_queue = std::make_unique<FixedSizeBlockingQueue<int>>(5);
    std::atomic<int> total_pushed{0};
    std::atomic<int> total_popped{0};

    std::thread producer([&]() {
        for (int i = 0; i < 20; ++i) {
            small_queue->push(i);
            total_pushed++;
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    std::thread consumer([&]() {
        for (int i = 0; i < 20; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            auto result = small_queue->pop();
            if (result.has_value()) {
                total_popped++;
            }
        }
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(total_pushed, 20);
    EXPECT_EQ(total_popped, 20);
    EXPECT_TRUE(small_queue->empty());

    small_queue->shutdown();
}

TEST_F(FixedSizeBlockingQueueConcurrencyTest, HighContentionStressTest) {
    const int num_threads = 8;
    const int duration_ms = 100;
    std::vector<std::thread> threads;
    std::atomic<bool> stop_flag{false};
    std::atomic<int> pushes{0};
    std::atomic<int> pops{0};

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            while (!stop_flag) {
                if (t % 2 == 0) {
                    queue_->push(t);
                    pushes++;
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                } else {
                    auto result = queue_->tryPop();
                    if (result.has_value()) {
                        pops++;
                    }
                    std::this_thread::yield();
                }
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
    stop_flag = true;

    for (auto& thread : threads) {
        thread.join();
    }

    // Drain remaining elements
    while (true) {
        auto result = queue_->tryPop();
        if (!result.has_value()) break;
        pops++;
    }

    EXPECT_EQ(pushes, pops);
    EXPECT_TRUE(queue_->empty());
}

TEST_F(FixedSizeBlockingQueueConcurrencyTest, MultipleProducersAndConsumersWithShutdown) {
    const int num_producers = 2;
    const int num_consumers = 5;
    std::vector<std::thread> threads;
    std::atomic<int> blocked_consumers{0};

    // Start more consumers than we'll produce items for
    for (int i = 0; i < num_consumers; ++i) {
        threads.emplace_back([&]() {
            auto result = queue_->pop();
            if (!result.has_value()) {
                blocked_consumers++;
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Produce fewer items than consumers
    for (int i = 0; i < num_producers; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 2; ++j) {
                queue_->push(j);
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    queue_->shutdown();

    for (auto& thread : threads) {
        thread.join();
    }

    // Should have at least 1 blocked consumer (5 consumers, only 4 items produced)
    EXPECT_GT(blocked_consumers, 0);
}

TEST_F(FixedSizeBlockingQueueConcurrencyTest, MixedOperationsWithBoundedCapacity) {
    const int num_threads = 4;
    const int operations_per_thread = 50;
    std::vector<std::thread> threads;
    std::atomic<int> total_pushed{0};
    std::atomic<int> total_popped{0};

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                queue_->push(t * operations_per_thread + i);
                total_pushed++;

                std::this_thread::sleep_for(std::chrono::microseconds(50));

                auto result = queue_->tryPop();
                if (result.has_value()) {
                    total_popped++;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Drain remaining
    while (true) {
        auto result = queue_->tryPop();
        if (!result.has_value()) break;
        total_popped++;
    }

    EXPECT_EQ(total_pushed, num_threads * operations_per_thread);
    EXPECT_EQ(total_popped, total_pushed);
}

TEST_F(FixedSizeBlockingQueueConcurrencyTest, LargeCapacityQueue) {
    auto large_queue = std::make_unique<FixedSizeBlockingQueue<int>>(10000);

    const int num_elements = 5000;
    std::thread producer([&]() {
        for (int i = 0; i < num_elements; ++i) {
            large_queue->push(i);
        }
    });

    std::thread consumer([&]() {
        for (int i = 0; i < num_elements; ++i) {
            auto result = large_queue->pop();
            ASSERT_TRUE(result.has_value());
            EXPECT_EQ(result.value(), i);
        }
    });

    producer.join();
    consumer.join();

    EXPECT_TRUE(large_queue->empty());
    large_queue->shutdown();
}

}
