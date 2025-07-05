#pragma once

#include <MR/Interface/ThreadSafeQueue.hpp>
#include <MR/Logger/WriteRequest.hpp>
#include <cstdint>
#include <memory>
namespace MR::Logger {
  struct Config {

    // All severities always end up here
    std::string log_file_name;

    // Optional. If not present, won't be used.
    std::string info_file_name;
    std::string warn_file_name;
    std::string error_file_name;

    // io_uring queue depth
    uint16_t queue_depth; 

    // number of log messages that are submitted
    // in batches io_uring
    // (must be <= queue_depth)
    uint16_t batch_size;

    // the central queue which moves log requests from
    // the front-end to the back-end will constantly
    // pop messages in a single loop (single-threaded)
    // and submit them in batches
    // until the queue is emptied or it reaches this number
    // afterwhich the same thread will stop dequeuing messages
    // and will start processing CQEs
    uint16_t max_logs_per_iteration;


    // For experimentation, different implementations of a ThreadSafeQueue can be used, e.g.
    // 1) StdQueue - my mutex/lock based std::queue wrapper
    // 2) moodycamel::ConcurrentQueue - lock free multi producer - multi consumer queue 
    // 3) ???
    std::shared_ptr<Interface::ThreadSafeQueue<WriteRequest>> _queue;


  };
}