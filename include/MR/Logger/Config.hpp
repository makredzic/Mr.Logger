#pragma once

#include <MR/Interface/ThreadSafeQueue.hpp>
#include <MR/Logger/WriteRequest.hpp>
#include <cstdint>
#include <memory>
namespace MR::Logger {
  struct Config {

    // All severities always end up here
    std::string log_file_name;

    // Log files are rotated automatically. New log file will be used
    // when this size is reached. 
    size_t max_log_size_bytes;

    // number of log messages that are submitted
    // in batches to io_uring
    // (must be <= queue_depth)
    uint16_t batch_size;

    // The central queue which holds log requests from
    // the front-end that are sent to the back-end. The worker thread
    // will constantly pop messages in a single loop
    // and submit them in batches (defined with batch_size) to io_uring
    // until the queue is emptied or it reaches max_logs_per_iteration 
    // afterwhich the same thread will stop dequeuing log requests
    // and will start processing CQEs. Internally, this controls the io_uring queue depth
    // Note, after this number of messages is reached, any processed messages
    // from the queue will also be submited (flushed) before the thread moves on
    // to process the CQEs.
    uint16_t max_logs_per_iteration;

    // io_uring queue depth (by default, it is the max_logs_per_iteration amount)
    // The queue depth can be manually increased for whatever reason. Note that
    // upon initializing the Logger, if the overriden queue_depth is less than
    // max_logs_per_iteration, a runtime error will be thrown.
    uint16_t queue_depth;

    // Buffer pool configuration
    uint16_t small_buffer_pool_size;
    uint16_t medium_buffer_pool_size;
    uint16_t large_buffer_pool_size;
    uint16_t small_buffer_size;
    uint16_t medium_buffer_size;
    uint16_t large_buffer_size;

    // For experimentation, different implementations of a ThreadSafeQueue can be used, e.g.
    // 1) StdQueue - my mutex/lock based std::queue wrapper
    // 2) moodycamel::ConcurrentQueue - lock free multi producer - multi consumer queue 
    // 3) ???
    std::shared_ptr<Interface::ThreadSafeQueue<WriteRequest>> _queue;


  };
}