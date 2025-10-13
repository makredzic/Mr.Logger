#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <MR/Logger/Logger.hpp>
#include <MR/Logger/Config.hpp>
#include <thread>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <vector>
#include <barrier>

#ifdef LOGGER_TEST_SEQUENCE_TRACKING
#include <MR/Queue/StdQueue.hpp>
#endif

namespace MR::Logger::Test {

class LoggerIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_log_file_ = std::filesystem::temp_directory_path() / "logger_integration_test.log";

        if (std::filesystem::exists(test_log_file_)) {
            std::filesystem::remove(test_log_file_);
        }

        config_ = {
            .log_file_name = test_log_file_.string(),
            .max_log_size_bytes = 100 * 1024 * 1024,
            .batch_size = 64,
            .queue_depth = 1024,
            ._queue = std::make_shared<Queue::StdQueue<WriteRequest>>()
        };

#ifdef LOGGER_TEST_SEQUENCE_TRACKING
        // Reset the global sequence counter for each test
        Queue::global_sequence_counter.store(0, std::memory_order_seq_cst);
#endif

        Logger::_reset();
        Logger::init(config_);
    }

    void TearDown() override {
        Logger::_reset();
        if (std::filesystem::exists(test_log_file_)) {
            std::filesystem::remove(test_log_file_);
        }
    }

    std::vector<std::string> readLogFile() {
        std::ifstream file(test_log_file_);
        std::vector<std::string> lines;
        std::string line;
        
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        
        return lines;
    }

    void waitForLogCompletion(int expected_messages) {
        auto start = std::chrono::steady_clock::now();
        const std::chrono::seconds timeout{5};
        
        while (std::chrono::steady_clock::now() - start < timeout) {
            if (std::filesystem::exists(test_log_file_)) {
                auto lines = readLogFile();
                if (lines.size() >= static_cast<size_t>(expected_messages)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    return;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    std::filesystem::path test_log_file_;
    Config config_;
};

TEST_F(LoggerIntegrationTest, SingleThreadLogging) {
    auto logger = Logger::get();
    
    logger->info("Message 1");
    logger->info("Message 2");
    logger->info("Message 3");
    
    waitForLogCompletion(3);
    
    auto lines = readLogFile();
    ASSERT_EQ(lines.size(), 3);
    
    EXPECT_THAT(lines[0], testing::HasSubstr("Message 1"));
    EXPECT_THAT(lines[1], testing::HasSubstr("Message 2"));
    EXPECT_THAT(lines[2], testing::HasSubstr("Message 3"));
    
    for (const auto& line : lines) {
        EXPECT_THAT(line, testing::HasSubstr("[INFO]"));
    }
}

TEST_F(LoggerIntegrationTest, TwoThreadLogging) {
    auto logger = Logger::get();
    
    std::vector<std::string> all_messages;
    std::mutex messages_mutex;
    
    std::thread t1([&logger, &all_messages, &messages_mutex]() {
        for (int i = 1; i <= 5; ++i) {
            std::string msg = "Thread1-Message" + std::to_string(i);
            {
                std::lock_guard<std::mutex> lock(messages_mutex);
                all_messages.push_back(msg);
                logger->info(msg);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    std::thread t2([&logger, &all_messages, &messages_mutex]() {
        for (int i = 1; i <= 5; ++i) {
            std::string msg = "Thread2-Message" + std::to_string(i);
            {
                std::lock_guard<std::mutex> lock(messages_mutex);
                all_messages.push_back(msg);
                logger->info(msg);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    t1.join();
    t2.join();
    
    waitForLogCompletion(10);
    
    auto lines = readLogFile();
    ASSERT_EQ(lines.size(), 10);
    
    // First verify all messages exist
    for (const auto& expected_msg : all_messages) {
        EXPECT_THAT(lines, testing::Contains(testing::HasSubstr(expected_msg)));
    }
    
    // Then verify ordering by matching each line to expected messages in sequence
    size_t expected_index = 0;
    for (const auto& line : lines) {
        if (expected_index < all_messages.size()) {
            EXPECT_THAT(line, testing::HasSubstr(all_messages[expected_index]));
            expected_index++;
        }
    }
}

TEST_F(LoggerIntegrationTest, ThreeThreadLogging) {
    auto logger = Logger::get();
    
    std::vector<std::string> all_messages;
    std::mutex messages_mutex;
    
    auto thread_func = [&logger, &all_messages, &messages_mutex](int thread_id) {
        for (int i = 1; i <= 4; ++i) {
            std::string msg = "Thread" + std::to_string(thread_id) + "-Message" + std::to_string(i);
            {
                std::lock_guard<std::mutex> lock(messages_mutex);
                all_messages.push_back(msg);
                logger->info(msg);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    };
    
    std::thread t1(thread_func, 1);
    std::thread t2(thread_func, 2);
    std::thread t3(thread_func, 3);
    
    t1.join();
    t2.join();
    t3.join();
    
    waitForLogCompletion(12);
    
    auto lines = readLogFile();
    ASSERT_EQ(lines.size(), 12);
    
    // First verify all messages exist
    for (const auto& expected_msg : all_messages) {
        EXPECT_THAT(lines, testing::Contains(testing::HasSubstr(expected_msg)));
    }
    
    // Then verify ordering by matching each line to expected messages in sequence
    size_t expected_index = 0;
    for (const auto& line : lines) {
        if (expected_index < all_messages.size()) {
            EXPECT_THAT(line, testing::HasSubstr(all_messages[expected_index]));
            expected_index++;
        }
    }
    
    for (const auto& line : lines) {
        EXPECT_THAT(line, testing::HasSubstr("[INFO]"));
        EXPECT_THAT(line, testing::HasSubstr("[Thread:"));
    }
}

TEST_F(LoggerIntegrationTest, EarlyLoggerClosureMultiThreaded) {
    const int num_threads = 4;
    const int messages_per_thread = 100000;
    
    std::vector<std::string> queued_messages;
    std::mutex queued_messages_mutex;
    
    std::vector<std::thread> threads;
    std::barrier sync_point(num_threads + 1);  // Synchronize thread start
    
    // Create a scope to control logger lifetime
    {
        auto logger = Logger::get();

        // Launch threads that write messages
        for (int thread_id = 0; thread_id < num_threads; ++thread_id) {
            threads.emplace_back([&, thread_id]() {
                // Wait for all threads to start together
                sync_point.arrive_and_wait();
                
                int msg_count = 0;
                while (msg_count < messages_per_thread) {
                    std::string msg = "Thread" + std::to_string(thread_id) + "_Msg" + std::to_string(msg_count);
                    
                    try {
                        // Only track messages that were successfully queued
                        {
                            std::lock_guard<std::mutex> lock(queued_messages_mutex);
                            logger->info(msg);
                            queued_messages.push_back(msg);
                        }
                        msg_count++;
                        
                    } catch (const std::exception& e) {
                        // Logger queue might be shut down - this is expected during early closure
                        std::cout << "EXCEPTION: " << e.what() << std::endl;
                        break;
                    }
                }
            });
        }
        
        // Start all threads simultaneously
        sync_point.arrive_and_wait();

        // Logger is now destroyed - join all threads to clean up properly
        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

    // FORCE SHUTDOWN OF LOGGER
    Logger::_reset();

    // Verify num messages in the file
    auto lines = readLogFile();
    ASSERT_EQ(lines.size(), queued_messages.size()) 
        << "Expected " << queued_messages.size() << " messages in file, but got " << lines.size()
        << " - Logger destructor did not properly flush all queued messages!";
    
    // Verify ordering by matching each line to expected messages in sequence
    size_t expected_index = 0;
    for (const auto& line : lines) {
        if (expected_index < queued_messages.size()) {
            EXPECT_THAT(line, testing::HasSubstr(queued_messages[expected_index]));
            expected_index++;
        }
    }
}

TEST_F(LoggerIntegrationTest, FlushBasic) {
    auto logger = Logger::get();

    for (int i = 0; i < 100; ++i) {
        logger->info("Message {}", i);
    }

    logger->flush();

    auto lines = readLogFile();
    ASSERT_EQ(lines.size(), 100);

    for (int i = 0; i < 100; ++i) {
        EXPECT_THAT(lines[i], testing::HasSubstr("Message " + std::to_string(i)));
    }
}

TEST_F(LoggerIntegrationTest, FlushEmpty) {
    auto logger = Logger::get();

    // Flush on empty queue should return immediately
    auto start = std::chrono::high_resolution_clock::now();
    logger->flush();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Should complete very quickly (< 10ms)
    EXPECT_LT(duration.count(), 10);
}

TEST_F(LoggerIntegrationTest, FlushWithNewMessages) {
    auto logger = Logger::get();

    // Write initial batch
    for (int i = 0; i < 50; ++i) {
        logger->info("Batch1-Message{}", i);
    }

    // Flush first batch
    logger->flush();

    auto lines = readLogFile();
    ASSERT_EQ(lines.size(), 50);

    // Write second batch
    for (int i = 0; i < 50; ++i) {
        logger->info("Batch2-Message{}", i);
    }

    // Flush second batch
    logger->flush();

    lines = readLogFile();
    ASSERT_EQ(lines.size(), 100);
}

TEST_F(LoggerIntegrationTest, FlushMultiThreaded) {
    auto logger = Logger::get();

    const int num_threads = 4;
    const int messages_per_thread = 100;

    std::vector<std::thread> threads;

    for (int tid = 0; tid < num_threads; ++tid) {
        threads.emplace_back([&, tid]() {
            for (int i = 0; i < messages_per_thread; ++i) {
                logger->info("Thread{}-Message{}", tid, i);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    logger->flush();

    auto lines = readLogFile();
    ASSERT_EQ(lines.size(), num_threads * messages_per_thread);
}

#ifdef LOGGER_TEST_SEQUENCE_TRACKING
TEST_F(LoggerIntegrationTest, SequenceNumberOrderingWithoutSync) {
    auto logger = Logger::get();

    auto thread_func = [&](int thread_id) {
        for (int i = 1; i <= 10; ++i) {
            std::string msg = "Thread" + std::to_string(thread_id) + "-Message" + std::to_string(i);
            logger->info(msg);  // NO external sync here!

            // Small delay to avoid overwhelming
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    };

    std::thread t1(thread_func, 1);
    std::thread t2(thread_func, 2);
    std::thread t3(thread_func, 3);

    t1.join();
    t2.join();
    t3.join();

    waitForLogCompletion(30);

    auto lines = readLogFile();
    ASSERT_EQ(lines.size(), 30);

    // Sequence numbers should now be present in the log lines

    // Extract sequence numbers from log lines and verify they are in ascending order
    std::vector<uint64_t> sequence_numbers;
    for (const auto& line : lines) {
        // Look for pattern [Seq: 123]
        size_t seq_start = line.find("[Seq: ");
        if (seq_start != std::string::npos) {
            seq_start += 6;  // Skip "[Seq: "
            size_t seq_end = line.find("]", seq_start);
            if (seq_end != std::string::npos) {
                std::string seq_str = line.substr(seq_start, seq_end - seq_start);
                sequence_numbers.push_back(std::stoull(seq_str));
            }
        }
    }

    // Verify we extracted all sequence numbers
    ASSERT_EQ(sequence_numbers.size(), 30);

    // Debug: Print first few sequence numbers to understand the pattern
    std::cout << "First 10 sequence numbers: ";
    for (size_t i = 0; i < std::min(sequence_numbers.size(), size_t(10)); ++i) {
        std::cout << sequence_numbers[i] << " ";
    }
    std::cout << std::endl;

    // Verify sequence numbers are in ascending order (0, 1, 2, ..., 29)
    for (size_t i = 0; i < sequence_numbers.size(); ++i) {
        EXPECT_EQ(sequence_numbers[i], i)
            << "Sequence number at position " << i << " should be " << i
            << " but was " << sequence_numbers[i];
    }
}
#endif

TEST_F(LoggerIntegrationTest, SingleMessageFlush) {
    // Test that a single message gets written despite batch_size=64
    auto logger = Logger::get();

    logger->info("Single message test");

    // Wait for message to be processed and written
    waitForLogCompletion(1);

    auto lines = readLogFile();
    ASSERT_EQ(lines.size(), 1);
    EXPECT_THAT(lines[0], testing::HasSubstr("Single message test"));
    EXPECT_THAT(lines[0], testing::HasSubstr("[INFO]"));
}

TEST_F(LoggerIntegrationTest, FewerThanBatchSize) {
    // Test that 5 messages get written despite batch_size=64
    auto logger = Logger::get();

    for (int i = 1; i <= 5; ++i) {
        logger->info("Message {}", i);
    }

    // Wait for all messages to be processed
    waitForLogCompletion(5);

    auto lines = readLogFile();
    ASSERT_EQ(lines.size(), 5);

    for (int i = 0; i < 5; ++i) {
        EXPECT_THAT(lines[i], testing::HasSubstr("Message " + std::to_string(i + 1)));
    }
}

TEST_F(LoggerIntegrationTest, ExactlyBatchSizeMinusOne) {
    // Test that batch_size-1 messages get written (edge case)
    auto logger = Logger::get();

    const int message_count = config_.batch_size - 1;  // 63 messages (batch=64)

    for (int i = 1; i <= message_count; ++i) {
        logger->info("Message {}", i);
    }

    // Wait for all messages to be processed
    waitForLogCompletion(message_count);

    auto lines = readLogFile();
    ASSERT_EQ(lines.size(), message_count);
}

TEST_F(LoggerIntegrationTest, SmallBatchWithCoalescing) {
    // Verify small batches work with coalescing enabled
    // The default logger config from SetUp has coalesce_size=0 (disabled)
    // So we need to create a logger with coalescing enabled
    Logger::_reset();

    // Use a different log file to avoid conflicts
    auto coalesce_log_file = std::filesystem::temp_directory_path() / "logger_coalesce_test.log";
    if (std::filesystem::exists(coalesce_log_file)) {
        std::filesystem::remove(coalesce_log_file);
    }

    Config custom_config = {
        .log_file_name = coalesce_log_file.string(),
        .max_log_size_bytes = 100 * 1024 * 1024,
        .batch_size = 32,
        .queue_depth = 512,
        .small_buffer_pool_size = 0,
        .medium_buffer_pool_size = 0,
        .large_buffer_pool_size = 0,
        .small_buffer_size = 0,
        .medium_buffer_size = 0,
        .large_buffer_size = 0,
        .shutdown_timeout_seconds = 3,
        ._queue = std::make_shared<Queue::StdQueue<WriteRequest>>(),
        .coalesce_size = 32  // Enable coalescing
    };

    Logger::init(custom_config);

    auto logger = Logger::get();

    // Log only 3 messages (much less than coalesce_size=32)
    logger->info("Coalesced message 1");
    logger->info("Coalesced message 2");
    logger->info("Coalesced message 3");

    // Explicitly flush to ensure all messages are written
    logger->flush();

    // Read from the coalesce-specific log file
    std::ifstream file(coalesce_log_file);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    file.close();

    ASSERT_EQ(lines.size(), 3);

    EXPECT_THAT(lines[0], testing::HasSubstr("Coalesced message 1"));
    EXPECT_THAT(lines[1], testing::HasSubstr("Coalesced message 2"));
    EXPECT_THAT(lines[2], testing::HasSubstr("Coalesced message 3"));

    // Clean up
    if (std::filesystem::exists(coalesce_log_file)) {
        std::filesystem::remove(coalesce_log_file);
    }
}

TEST_F(LoggerIntegrationTest, SmallBatchMultiThreaded) {
    // Test that small batches from multiple threads are all written
    auto logger = Logger::get();

    std::thread t1([&logger]() {
        logger->info("Thread1-Message1");
        logger->info("Thread1-Message2");
    });

    std::thread t2([&logger]() {
        logger->info("Thread2-Message1");
        logger->info("Thread2-Message2");
    });

    t1.join();
    t2.join();

    // Total: 4 messages (much less than batch_size=64)
    waitForLogCompletion(4);

    auto lines = readLogFile();
    ASSERT_EQ(lines.size(), 4);

    // Verify all messages are present (order may vary due to threading)
    EXPECT_THAT(lines, testing::Contains(testing::HasSubstr("Thread1-Message1")));
    EXPECT_THAT(lines, testing::Contains(testing::HasSubstr("Thread1-Message2")));
    EXPECT_THAT(lines, testing::Contains(testing::HasSubstr("Thread2-Message1")));
    EXPECT_THAT(lines, testing::Contains(testing::HasSubstr("Thread2-Message2")));
}

TEST_F(LoggerIntegrationTest, ZeroMessagesNoHang) {
    // Test that logger can be destroyed without logging any messages
    // This should complete quickly without hanging
    auto logger = Logger::get();

    auto start = std::chrono::high_resolution_clock::now();

    // Immediately reset logger without logging anything
    Logger::_reset();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Should complete very quickly (< 100ms)
    EXPECT_LT(duration.count(), 100);
}

TEST_F(LoggerIntegrationTest, IncrementalSmallBatches) {
    // Test multiple small batches with delays between them
    auto logger = Logger::get();

    // First small batch
    logger->info("Batch1-Message1");
    logger->info("Batch1-Message2");

    waitForLogCompletion(2);
    auto lines = readLogFile();
    ASSERT_EQ(lines.size(), 2);

    // Second small batch after some time
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    logger->info("Batch2-Message1");
    logger->info("Batch2-Message2");
    logger->info("Batch2-Message3");

    waitForLogCompletion(5);
    lines = readLogFile();
    ASSERT_EQ(lines.size(), 5);

    EXPECT_THAT(lines[0], testing::HasSubstr("Batch1-Message1"));
    EXPECT_THAT(lines[1], testing::HasSubstr("Batch1-Message2"));
    EXPECT_THAT(lines[2], testing::HasSubstr("Batch2-Message1"));
    EXPECT_THAT(lines[3], testing::HasSubstr("Batch2-Message2"));
    EXPECT_THAT(lines[4], testing::HasSubstr("Batch2-Message3"));
}

}