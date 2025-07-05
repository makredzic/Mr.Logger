#pragma once
#include <exception>
#include <coroutine>
#include <utility>


namespace MR::Coroutine {

class WriteTask {
  public:

    struct promise_type {
      int result = -1;
      std::exception_ptr exception;
      
      inline WriteTask get_return_object() {
          return WriteTask{std::coroutine_handle<promise_type>::from_promise(*this)};
      }
      inline std::suspend_never initial_suspend() { return {}; }
      inline std::suspend_always final_suspend() noexcept { return {}; }
      inline void return_void() {}
      inline void unhandled_exception() { 
          exception = std::current_exception();
      }
    };
    
    inline explicit WriteTask(std::coroutine_handle<promise_type> h) : h_(h) {}
    inline ~WriteTask() {
        if (h_)
            h_.destroy();
    }
    
    // Move only
    inline WriteTask(WriteTask&& other) noexcept : h_(std::exchange(other.h_, {})) {}
    inline WriteTask& operator=(WriteTask&& other) noexcept {
        if (this != &other) {
            if (h_)
                h_.destroy();
            h_ = std::exchange(other.h_, {});
        }
        return *this;
    }

    inline bool done() const { return h_.done(); }

  private:
    std::coroutine_handle<promise_type> h_;

};

}