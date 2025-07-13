
#include <MR/Logger/WriteRequest.hpp>
#include <MR/IO/IOUring.hpp>
#include <MR/IO/WriteOnlyFile.hpp>
#include <MR/Logger/Logger.hpp>
#include <string>

using namespace MR::IO;
using namespace std::chrono_literals;

// Example custom struct
struct Point { 
  int a; 
  int b; 
};

std::string toStr(const Point& pt) { return std::string{"a = " + std::to_string(pt.a) + "b = " + std::to_string(pt.b)}; }
REGISTER_MR_LOGGER_TO_STRING(Point, toStr)


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