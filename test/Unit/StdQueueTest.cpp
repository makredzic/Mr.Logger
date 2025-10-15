#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <MR/Queue/StdQueue.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

namespace MR::Queue::Test {

class StdQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        queue_ = std::make_unique<StdQueue<int>>();
    }

    void TearDown() override {
        queue_->shutdown();
        queue_.reset();
    }

    std::unique_ptr<StdQueue<int>> queue_;
};

TEST_F(StdQueueTest, ConstructorInitialization) {
    EXPECT_TRUE(queue_->empty());
    EXPECT_EQ(queue_->size(), 0);
}

TEST_F(StdQueueTest, PushAndTryPopSingleElement) {
    queue_->push(42);

    EXPECT_FALSE(queue_->empty());
    EXPECT_EQ(queue_->size(), 1);

    auto result = queue_->tryPop();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 42);

    EXPECT_TRUE(queue_->empty());
    EXPECT_EQ(queue_->size(), 0);
}

TEST_F(StdQueueTest, PushRvalueAndTryPop) {
    int value = 100;
    queue_->push(std::move(value));

    auto result = queue_->tryPop();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 100);
}

TEST_F(StdQueueTest, PushMultipleElements) {
    for (int i = 0; i < 10; ++i) {
        queue_->push(i);
    }

    EXPECT_EQ(queue_->size(), 10);

    for (int i = 0; i < 10; ++i) {
        auto result = queue_->tryPop();
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), i);
    }

    EXPECT_TRUE(queue_->empty());
}

TEST_F(StdQueueTest, TryPopOnEmptyQueue) {
    auto result = queue_->tryPop();
    EXPECT_FALSE(result.has_value());
}

TEST_F(StdQueueTest, PopBlocksUntilElementAvailable) {
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

TEST_F(StdQueueTest, ShutdownUnblocksWaitingPop) {
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

TEST_F(StdQueueTest, PushAfterShutdownDoesNothing) {
    queue_->shutdown();

    queue_->push(42);

    EXPECT_TRUE(queue_->empty());
    EXPECT_EQ(queue_->size(), 0);
}

TEST_F(StdQueueTest, FIFOOrdering) {
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

TEST_F(StdQueueTest, SizeTracking) {
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

TEST_F(StdQueueTest, EmptyCheck) {
    EXPECT_TRUE(queue_->empty());

    queue_->push(1);
    EXPECT_FALSE(queue_->empty());

    queue_->tryPop();
    EXPECT_TRUE(queue_->empty());
}

class StdQueueConcurrencyTest : public ::testing::Test {
protected:
    void SetUp() override {
        queue_ = std::make_unique<StdQueue<int>>();
    }

    void TearDown() override {
        queue_->shutdown();
        queue_.reset();
    }

    std::unique_ptr<StdQueue<int>> queue_;
};

TEST_F(StdQueueConcurrencyTest, ConcurrentPushAndTryPop) {
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

TEST_F(StdQueueConcurrencyTest, ConcurrentPushAndBlockingPop) {
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

TEST_F(StdQueueConcurrencyTest, HighContentionStressTest) {
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
                } else {
                    auto result = queue_->tryPop();
                    if (result.has_value()) {
                        pops++;
                    }
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

TEST_F(StdQueueConcurrencyTest, MultipleConsumersWithShutdown) {
    const int num_consumers = 5;
    std::vector<std::thread> threads;
    std::atomic<int> returns_with_empty{0};

    for (int i = 0; i < num_consumers; ++i) {
        threads.emplace_back([&]() {
            auto result = queue_->pop();
            if (!result.has_value()) {
                returns_with_empty++;
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    queue_->shutdown();

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(returns_with_empty, num_consumers);
}

TEST_F(StdQueueConcurrencyTest, MixedOperations) {
    const int num_threads = 6;
    const int operations_per_thread = 50;
    std::vector<std::thread> threads;
    std::atomic<int> total_pushed{0};
    std::atomic<int> total_popped{0};

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                queue_->push(t * operations_per_thread + i);
                total_pushed++;

                std::this_thread::sleep_for(std::chrono::microseconds(10));

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

}
