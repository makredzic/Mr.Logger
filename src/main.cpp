
#include <MR/IO/IOUring.hpp>
#include <MR/IO/WriteOnlyFile.hpp>
#include <thread>
#include <vector>
#include <iostream>

using namespace MR::IO;
using namespace std::chrono_literals;

int main() {


  WriteOnlyFile file{"log.log"};

  IOUring ring{8};

  std::vector<std::string> msgs(10);

  int i = 1;
  while (i <= 10) {
    std::cout << i << ". Writing to log\n";
    msgs.push_back({std::to_string(i) + ". ligma\n"});
    i++;
  }


  for (const auto& msg : msgs) {
    ring.submitWrite(msg, file);
  }

  // WAITS FOR LOGGING TO FINISH
  std::this_thread::sleep_for(500ms);

  return 0;
}