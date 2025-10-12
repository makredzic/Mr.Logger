#pragma once

#include <MR/Logger/WriteRequest.hpp>
#include <MR/Memory/BufferPool.hpp>
#include <MR/Logger/SeverityLevel.hpp>

#include <memory>
#include <functional>
#include <cstring>
#include <optional>

#include <fmt/format.h>
#include <fmt/std.h>
#include <fmt/chrono.h>

namespace MR::IO {

/**
 * WritePreparer is responsible for preparing write requests for submission.
 *
 * This includes:
 * - Formatting log messages into buffers
 * - Optionally coalescing multiple messages into a single buffer
 * - Managing the staging buffer for coalescing
 *
 * This class does NOT interact with io_uring - it only prepares data.
 * The caller (event loop) is responsible for submitting prepared buffers to io_uring.
 */
class WritePreparer {
public:
    using ErrorReporter = std::function<void(const char*, const std::string&)>;

    struct Config {
        uint16_t coalesce_size;  // Number of messages to coalesce (0 = disabled)
        size_t staging_buffer_size = 16384;  // 16KB staging buffer
    };

    /**
     * Result of preparing a write request.
     * Contains an optional buffer ready for writing.
     */
    struct PreparedWrite {
        std::unique_ptr<Memory::Buffer> buffer;  // Buffer ready for io_uring write (null if staged)
        bool should_flush_batch;                
    };

    WritePreparer(
        Config config,
        Memory::BufferPool& buffer_pool,
        ErrorReporter error_reporter
    )
        : config_(config)
        , buffer_pool_(buffer_pool)
        , error_reporter_(std::move(error_reporter))
        , staging_buffer_(std::make_unique<char[]>(config.staging_buffer_size))
    {}

    /**
     * Prepare a write request for submission.
     *
     * Returns a PreparedWrite which may contain:
     * - A buffer ready for immediate writing, OR
     * - Nothing (message was staged for coalescing)
     *
     * The should_flush_batch flag indicates when the caller should
     * submit pending writes to io_uring.
     */
    PreparedWrite prepareWrite(Logger::WriteRequest&& request) {
        if (config_.coalesce_size > 1) {
            return prepareCoalescedWrite(std::move(request));
        } else {
            // Coalescing disabled - prepare individual write
            return prepareIndividualWrite(std::move(request));
        }
    }

    /**
     * Flush any staged messages and return a buffer ready for writing.
     * Returns nullopt if there's nothing staged.
     */
    std::optional<std::unique_ptr<Memory::Buffer>> flushStaged() {
        if (staging_offset_ == 0) {
            return std::nullopt;
        }

        try {
            // Copy staging buffer to persistent buffer from pool
            auto persistent_buffer = buffer_pool_.acquire(staging_offset_);
            std::memcpy(persistent_buffer->data, staging_buffer_.get(), staging_offset_);
            persistent_buffer->size = staging_offset_;

            // Reset staging state
            staging_offset_ = 0;
            messages_in_staging_ = 0;

            return persistent_buffer;
        } catch (const std::exception& e) {
            error_reporter_("WritePreparer::flushStaged", e.what());
            return std::nullopt;
        }
    }

    bool hasStaged() const {
        return staging_offset_ > 0;
    }

private:
    /**
     * Prepare a write with coalescing enabled.
     */
    PreparedWrite prepareCoalescedWrite(Logger::WriteRequest&& request) {
        // Format message directly into staging buffer
        size_t formatted_size = formatTo(
            std::move(request),
            staging_buffer_.get() + staging_offset_,
            config_.staging_buffer_size - staging_offset_
        );

        // Check if formatting succeeded (didn't overflow)
        if (formatted_size > 0 && staging_offset_ + formatted_size <= config_.staging_buffer_size) {
            staging_offset_ += formatted_size;
            messages_in_staging_++;

            // Flush staging buffer when:
            // 1. Reached coalesce threshold, OR
            // 2. Buffer is nearly full (>90%)
            bool should_flush = (messages_in_staging_ >= config_.coalesce_size) ||
                               (staging_offset_ > config_.staging_buffer_size * 9 / 10);

            if (should_flush && staging_offset_ > 0) {
                auto buffer = flushStaged();
                if (buffer.has_value()) {
                    return PreparedWrite{std::move(buffer.value()), true};
                }
            }

            // Message staged, nothing to write yet
            return PreparedWrite{nullptr, false};
        } else {
            // Buffer overflow or format error
            // Flush current staging buffer first
            auto flushed = flushStaged();

            // Then prepare this message individually as fallback
            auto individual = prepareIndividualWrite(std::move(request));

            // If we flushed something, return that first
            // The current message will be lost in this case, which matches original behavior
            if (flushed.has_value()) {
                return PreparedWrite{std::move(flushed.value()), true};
            }

            return individual;
        }
    }

    /**
     * Prepare an individual write (no coalescing).
     */
    PreparedWrite prepareIndividualWrite(Logger::WriteRequest&& request) {
        try {
            // Estimate required buffer size (with some padding for safety)
            size_t estimated_size = request.data.size() + 256;

            // Acquire buffer from pool
            auto buffer = buffer_pool_.acquire(estimated_size);

            // Format directly into buffer
            size_t actual_size = formatTo(std::move(request), buffer->as_char(), buffer->capacity);
            buffer->size = actual_size;

            return PreparedWrite{std::move(buffer), false};
        } catch (const std::exception& e) {
            error_reporter_("WritePreparer::prepareIndividualWrite", e.what());
            return PreparedWrite{nullptr, false};
        }
    }

    /**
     * Format a write request into a buffer.
     */
    size_t formatTo(Logger::WriteRequest&& request, char* buffer, size_t capacity) {
#ifdef LOGGER_TEST_SEQUENCE_TRACKING
        auto result = fmt::format_to_n(
            buffer, capacity - 1,
            "[{}] [{}] [Thread: {}] [Seq: {}]: {}\n",
            request.timestamp,
            Logger::sevLvlToStr(request.level),
            request.threadId,
            request.sequence_number,
            std::move(request.data)
        );
#else
        auto result = fmt::format_to_n(
            buffer, capacity - 1,
            "[{}] [{}] [Thread: {}]: {}\n",
            request.timestamp,
            Logger::sevLvlToStr(request.level),
            request.threadId,
            std::move(request.data)
        );
#endif

        // Null terminate
        if (result.size < capacity) {
            buffer[result.size] = '\0';
        }

        return result.size;
    }

    Config config_;
    Memory::BufferPool& buffer_pool_;
    ErrorReporter error_reporter_;

    // Staging buffer for coalescing
    std::unique_ptr<char[]> staging_buffer_;
    size_t staging_offset_ = 0;
    size_t messages_in_staging_ = 0;
};

} // namespace MR::IO
