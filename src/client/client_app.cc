/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "absl/flags/flag.h"
#include "absl/flags/usage.h"
#include "absl/flags/parse.h"

#include "src/client/client_lib.h"

ABSL_FLAG(std::string, master_ip, "0.0.0.0", "master ip");
ABSL_FLAG(std::string, master_port, "50051", "master port");

using namespace std;

int main(int argc, char **argv) {
  absl::SetProgramUsageMessage("A client");
  // Parse command line arguments
  absl::ParseCommandLine(argc, argv);

  cout << "I'm a client!" << endl;

  std::string master_ip = absl::GetFlag(FLAGS_master_ip);
  std::string master_port = absl::GetFlag(FLAGS_master_port);
  std::string master_address = master_ip + ":" + master_port;
  
  ClientLib Chunky(master_address);

  Chunky.start();

  // Open a file
  ChunkyFile f = Chunky.open("test.txt");

  // Allocate..
  f.reserve(256);
  // Read from the file
  /* Data s; */
  /* size_t bytes_read = f.read({0,5}, s); */

  /* cout << s << endl; */

  return 0;
}
