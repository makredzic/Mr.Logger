#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <MR/Logger/Logger.hpp>
#include <MR/Logger/Config.hpp>
#include <MR/Queue/StdQueue.hpp>
#include <filesystem>
#include <vector>
#include <string>

namespace MR::Logger::Test {

class LoggerConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_log_file_ = std::filesystem::temp_directory_path() / "logger_config_test.log";

        if (std::filesystem::exists(test_log_file_)) {
            std::filesystem::remove(test_log_file_);
        }

        logger_errors_.clear();
        Logger::_reset();
    }

    void TearDown() override {
        Logger::_reset();
        if (std::filesystem::exists(test_log_file_)) {
            std::filesystem::remove(test_log_file_);
        }
    }

    // Helper to create a config with custom error handler
    Config makeConfig(uint16_t batch_size = 0, uint16_t queue_depth = 0, uint16_t coalesce_size = 0) {
        return Config{
            .internal_error_handler = [this](const std::string& msg) {
                logger_errors_.push_back(msg);
            },
            .log_file_name = test_log_file_.string(),
            .max_log_size_bytes = 10 * 1024 * 1024,
            .batch_size = batch_size,
            .queue_depth = queue_depth,
            .small_buffer_pool_size = 0,
            .medium_buffer_pool_size = 0,
            .large_buffer_pool_size = 0,
            .small_buffer_size = 0,
            .medium_buffer_size = 0,
            .large_buffer_size = 0,
            .shutdown_timeout_seconds = 1,
            ._queue = std::make_shared<Queue::StdQueue<WriteRequest>>(),
            .coalesce_size = coalesce_size
        };
    }

    int countWarnings(const std::string& substring) {
        int count = 0;
        for (const auto& warning : logger_errors_) {
            if (warning.find(substring) != std::string::npos) {
                count++;
            }
        }
        return count;
    }

    std::filesystem::path test_log_file_;
    std::vector<std::string> logger_errors_;
};

TEST_F(LoggerConfigTest, ValidDefaultConfig) {
    auto config = makeConfig();
    EXPECT_NO_THROW(Logger::init(config));

    EXPECT_EQ(logger_errors_.size(), 0);
}

TEST_F(LoggerConfigTest, BatchSizeExceedsQueueDepth_ThrowsException) {
    auto config = makeConfig(64, 32, 0); 

    EXPECT_THROW({
        Logger::init(config);
    }, std::invalid_argument);
}

TEST_F(LoggerConfigTest, BatchSizeExceedsHalfQueueDepth_Warning) {
    // batch_size > queue_depth/2 should construct with warning
    auto config = makeConfig(40, 64, 0);

    EXPECT_NO_THROW(Logger::init(config));

    auto final_config = Logger::getConfig();

    // Verify merged config has correct values
    EXPECT_EQ(final_config.batch_size, 40);
    EXPECT_EQ(final_config.queue_depth, 64);
    EXPECT_EQ(final_config.coalesce_size, 40) << "Should auto-scale to match batch_size";

    EXPECT_EQ(countWarnings("than half of queue_depth"), 1);
}

TEST_F(LoggerConfigTest, QueueDepthLessThan8xBatchSize_Warning) {
    // queue_depth < 8×batch_size should construct with warning
    auto config = makeConfig(32, 128, 0); 

    EXPECT_NO_THROW(Logger::init(config));
    EXPECT_EQ(countWarnings("less than 8x batch_size"), 1);
}

TEST_F(LoggerConfigTest, CoalesceSizeTooSmall_Warning) {
    // coalesce_size < 0.5 × batch_size should warn
    auto config = makeConfig(32, 512, 10);

    EXPECT_NO_THROW(Logger::init(config));
    EXPECT_EQ(countWarnings("differs significantly from batch_size"), 1);
}

TEST_F(LoggerConfigTest, CoalesceSizeTooLarge_Warning) {
    // coalesce_size > 2.0 × batch_size should warn
    auto config = makeConfig(32, 512, 80); 

    EXPECT_NO_THROW(Logger::init(config));
    EXPECT_EQ(countWarnings("Warning:"), 1);
}

TEST_F(LoggerConfigTest, CoalesceSizeOptimalRange_NoWarning) {
    // coalesce_size in [0.5, 2.0] × batch_size should not warn
    auto config = makeConfig(32, 512, 32);

    EXPECT_NO_THROW(Logger::init(config));
    EXPECT_EQ(countWarnings("Warning"), 0);
}

TEST_F(LoggerConfigTest, AutoScaling_OnlyBatchSizeSpecified) {
    // When only batch_size is specified, queue_depth and coalesce_size should be auto-calculated
    auto config = makeConfig(64, 0, 0);

    EXPECT_NO_THROW(Logger::init(config));

    auto logger = Logger::get();
    auto final_config = Logger::getConfig();

    // Verify auto-scaling calculations:
    // - batch_size remains as specified
    // - queue_depth = 64 × 16 = 1024
    // - coalesce_size = 64 (matches batch_size)
    EXPECT_EQ(final_config.batch_size, 64);
    EXPECT_EQ(final_config.queue_depth, 1024) << "queue_depth should be 64 * 16";
    EXPECT_EQ(final_config.coalesce_size, 64) << "coalesce_size should match batch_size";

    // With optimal ratios, there should be no warnings
    EXPECT_EQ(logger_errors_.size(), 0) << "Should have no warnings with optimal auto-scaled config";
}

TEST_F(LoggerConfigTest, AutoScaling_VerifyQueueDepthCalculation) {
    // Verify queue_depth = batch_size × 16
    auto config = makeConfig(16, 0, 0);  // batch=16 should give queue=256

    EXPECT_NO_THROW(Logger::init(config));

    auto final_config = Logger::getConfig();

    // Verify auto-scaled queue_depth calculation
    EXPECT_EQ(final_config.batch_size, 16);
    EXPECT_EQ(final_config.queue_depth, 256) << "queue_depth should be 16 * 16";
    EXPECT_EQ(final_config.coalesce_size, 16) << "coalesce_size should match batch_size";

    // With batch=16, queue=256 is exactly 16×batch (optimal), so no warnings
    EXPECT_EQ(countWarnings("less than 8x batch_size"), 0);
}

TEST_F(LoggerConfigTest, AutoScaling_VerifyCoalesceSizeCalculation) {
    // Verify coalesce_size = batch_size
    auto config = makeConfig(48, 0, 0);  // batch=48 should give coalesce=48

    EXPECT_NO_THROW(Logger::init(config));

    auto final_config = Logger::getConfig();

    // Verify auto-scaled coalesce_size calculation
    EXPECT_EQ(final_config.batch_size, 48);
    EXPECT_EQ(final_config.queue_depth, 768) << "queue_depth should be 48 * 16";
    EXPECT_EQ(final_config.coalesce_size, 48) << "coalesce_size should match batch_size";

    // coalesce_size equals batch_size (ratio=1.0), so no warning
    EXPECT_EQ(countWarnings("differs significantly from batch_size"), 0);
}

TEST_F(LoggerConfigTest, AllZeros_UsesDefaults) {
    // All zeros should use default config values
    auto config = makeConfig(0, 0, 0);

    EXPECT_NO_THROW(Logger::init(config));

    auto final_config = Logger::getConfig();

    // Verify defaults are used (from Logger.hpp default_config_)
    // Default: batch_size=32, queue_depth=512, coalesce_size=32
    EXPECT_EQ(final_config.batch_size, 32) << "Should use default batch_size";
    EXPECT_EQ(final_config.queue_depth, 512) << "Should use default queue_depth";
    EXPECT_EQ(final_config.coalesce_size, 32) << "Should use default coalesce_size";

    // Default config is optimal, should have no warnings
    EXPECT_EQ(logger_errors_.size(), 0);
}

TEST_F(LoggerConfigTest, ZeroBatchSize_UsesDefault) {
    // batch_size=0 with explicit queue_depth should use default batch_size
    auto config = makeConfig(0, 1024, 0);  // batch=0 (default=32), queue=1024

    EXPECT_NO_THROW(Logger::init(config));

    auto final_config = Logger::getConfig();

    // Verify: batch_size uses default (32), queue_depth is user-specified (1024)
    // coalesce_size auto-scales since batch_size was 0
    EXPECT_EQ(final_config.batch_size, 32) << "Should use default batch_size";
    EXPECT_EQ(final_config.queue_depth, 1024) << "Should use user-specified queue_depth";
    EXPECT_EQ(final_config.coalesce_size, 32) << "Should use default coalesce_size";

    // Default batch_size is 32, with queue=1024, ratio is 32:1 which is optimal
    EXPECT_EQ(countWarnings("less than 8x batch_size"), 0);
}

TEST_F(LoggerConfigTest, ZeroQueueDepth_UsesDefault) {
    // queue_depth=0 with explicit batch_size triggers auto-scaling
    auto config = makeConfig(32, 0, 0);  // batch=32, queue=0 (will auto-scale to 512)

    EXPECT_NO_THROW(Logger::init(config));

    auto final_config = Logger::getConfig();

    // Verify: batch_size is user-specified, queue_depth auto-scales to 16×batch_size
    EXPECT_EQ(final_config.batch_size, 32) << "Should use user-specified batch_size";
    EXPECT_EQ(final_config.queue_depth, 512) << "Should auto-scale to 32 * 16";
    EXPECT_EQ(final_config.coalesce_size, 32) << "Should auto-scale to match batch_size";

    // Auto-scaling will set queue_depth=512, which is 16×32, optimal
    EXPECT_EQ(logger_errors_.size(), 0);
}

TEST_F(LoggerConfigTest, ZeroCoalesceSize_UsesDefault) {
    // coalesce_size=0 with explicit batch_size and queue_depth triggers auto-scaling
    auto config = makeConfig(32, 512, 0);  // coalesce=0 (will auto-scale to 32)

    EXPECT_NO_THROW(Logger::init(config));

    auto final_config = Logger::getConfig();

    // Verify: batch_size and queue_depth are user-specified, coalesce_size auto-scales
    EXPECT_EQ(final_config.batch_size, 32) << "Should use user-specified batch_size";
    EXPECT_EQ(final_config.queue_depth, 512) << "Should use user-specified queue_depth";
    EXPECT_EQ(final_config.coalesce_size, 32) << "Should auto-scale to match batch_size";

    // Auto-scaling will set coalesce_size=32, matching batch_size
    EXPECT_EQ(countWarnings("differs significantly from batch_size"), 0);
}

TEST_F(LoggerConfigTest, MultipleWarnings) {
    // Config with multiple suboptimal parameters
    auto config = makeConfig(50, 80, 10);
    // batch=50, queue=80 (50 > 40, triggers half warning)
    // queue=80 < 8×50=400 (triggers queue depth warning)
    // coalesce=10, ratio=0.2 < 0.5 (triggers coalesce warning)

    EXPECT_NO_THROW(Logger::init(config));

    EXPECT_GE(logger_errors_.size(), 2);  // At least 2 warnings
    EXPECT_EQ(countWarnings("more than half of queue_depth"), 1);
    EXPECT_EQ(countWarnings("less than 8x batch_size"), 1);
    EXPECT_EQ(countWarnings("differs significantly from batch_size"), 1);
}

TEST_F(LoggerConfigTest, MinimalBatchSize) {
    // batch_size=1 should work
    auto config = makeConfig(1, 16, 1);

    EXPECT_NO_THROW(Logger::init(config));
}

TEST_F(LoggerConfigTest, LargeBatchSize) {
    // Large batch_size should work
    auto config = makeConfig(256, 4096, 256);

    EXPECT_NO_THROW(Logger::init(config));
}

TEST_F(LoggerConfigTest, ExactlyHalfQueueDepth_NoWarning) {
    // batch_size exactly at queue_depth/2 boundary should not warn
    auto config = makeConfig(32, 64, 32);  // batch=32, queue=64 (exactly half)

    EXPECT_NO_THROW(Logger::init(config));

    // At boundary, should not trigger "more than half" warning
    EXPECT_EQ(countWarnings("more than half of queue_depth"), 0);
}

TEST_F(LoggerConfigTest, ExactlyEightTimesBatchSize_NoWarning) {
    // queue_depth exactly at 8×batch_size boundary should not warn
    auto config = makeConfig(32, 256, 32);  // batch=32, queue=256 (exactly 8x)

    EXPECT_NO_THROW(Logger::init(config));

    // At boundary, should not trigger "less than 8x" warning
    EXPECT_EQ(countWarnings("less than 8x batch_size"), 0);
}

}
