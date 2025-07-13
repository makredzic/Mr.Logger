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
            .info_file_name = "",
            .warn_file_name = "",
            .error_file_name = "",
            .queue_depth = 256,
            .batch_size = 50,
            .max_logs_per_iteration = 50,
            ._queue = std::make_shared<Queue::StdQueue<WriteRequest>>()
        };

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
            }
            logger->info(msg);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    std::thread t2([&logger, &all_messages, &messages_mutex]() {
        for (int i = 1; i <= 5; ++i) {
            std::string msg = "Thread2-Message" + std::to_string(i);
            {
                std::lock_guard<std::mutex> lock(messages_mutex);
                all_messages.push_back(msg);
            }
            logger->info(msg);
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
            }
            logger->info(msg);
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

}