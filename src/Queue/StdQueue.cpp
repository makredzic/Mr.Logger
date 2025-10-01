#ifdef LOGGER_TEST_SEQUENCE_TRACKING
#include <MR/Queue/StdQueue.hpp>
#include <atomic>

namespace MR::Queue {
    // Global sequence counter definition
    std::atomic<uint64_t> global_sequence_counter{0};
}
#endif