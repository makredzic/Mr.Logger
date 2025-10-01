#include <MR/Logger/WriteRequest.hpp>
#include <MR/Logger/Logger.hpp>
#include <MR/Interface/ThreadSafeQueue.hpp>
#include <memory>
#include <string>
#include <thread>

using namespace MR::IO;
using namespace std::chrono_literals;

// Example custom struct
struct Point { 
  int a; 
  int b; 
  std::string to_string() const {
    return std::string{"a = " + std::to_string(a) + ", b = " + std::to_string(b)};
  }
};

// std::string toStr(const Point& pt) { return std::string{"a = " + std::to_string(pt.a) + ", b = " + std::to_string(pt.b)}; }
MRLOGGER_TO_STRING(Point)


int main() {

  MR::Logger::init();
  auto log = MR::Logger::get();

  log->info("Test 1");
  log->info("Test {}", 2);
  log->info("Test {} + {} = {}", 1, 2, 3);


  Point pt{6,9};
  log->info("My point = {}", pt); 


  return 0;
}