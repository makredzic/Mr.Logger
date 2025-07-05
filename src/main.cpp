
#include "MR/Logger/Config.hpp"
#include <MR/Logger/WriteRequest.hpp>
#include <MR/IO/IOUring.hpp>
#include <MR/IO/WriteOnlyFile.hpp>
#include <thread>
#include <iostream>
#include <MR/Logger/Logger.hpp>

using namespace MR::IO;
using namespace std::chrono_literals;

int main() {

  MR::Logger::Logger log{};

  std::thread t1{[&log]() {
    int i = 1;
    while (i <= 12) {
      std::cout << i << ". Thread Writing to log\n";
      log.info(std::to_string(i) + ". Thread Logging message.");
      std::this_thread::sleep_for(2ms);
      i++;
    }
  }};

  int i = 1;
  while (i <= 12) {
    std::cout << i << ". Writing to log\n";
    log.info(std::to_string(i) + ". Logging message.");
    std::this_thread::sleep_for(10ms);
    i++;
  }

  // WAITS FOR LOGGING TO FINISH
  t1.join();

  return 0;
}