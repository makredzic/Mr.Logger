
#include <MR/Logger/WriteRequest.hpp>
#include <MR/Queue/StdQueue.hpp>
#include <MR/IO/IOUring.hpp>
#include <MR/IO/WriteOnlyFile.hpp>
#include <thread>
#include <vector>
#include <iostream>
#include <MR/Logger/Logger.hpp>

using namespace MR::IO;
using namespace std::chrono_literals;

int main() {

  MR::Logger::Logger log{std::make_shared<MR::Queue::StdQueue<MR::Logger::WriteRequest>>()};

  int i = 1;
  while (i <= 10) {
    std::cout << i << ". Writing to log\n";
    log.info(std::to_string(i) + ". Logging message.");
    i++;
  }

  // WAITS FOR LOGGING TO FINISH
  std::this_thread::sleep_for(1000ms);

  return 0;
}