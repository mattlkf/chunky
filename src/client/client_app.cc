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
#include <random>
#include <chrono>
#include <algorithm>

#include "absl/flags/flag.h"
#include "absl/flags/usage.h"
#include "absl/flags/parse.h"

#include "src/client/client_lib.h"

ABSL_FLAG(std::string, master_ip, "0.0.0.0", "master ip");
ABSL_FLAG(std::string, master_port, "50051", "master port");

using namespace std;

string random_string(size_t n) {
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::minstd_rand0 generator(seed);

  string ret = "";
  for (size_t i = 0; i < n; i++)
    ret.push_back((char)('a' + (generator() % ('z' - 'a'))));
  return ret;
}

void demo_simplest(ClientLib &Chunky) {

  // Open a file
  ChunkyFile f = Chunky.open("test_" + random_string(5) + ".txt");

  // Allocate..
  f.reserve(256);

  // Write a little to the file...
  f.write({0,7}, "cs244b!");
  // Read from the file
  string s;
  auto start = chrono::steady_clock::now();
  f.read({0,7}, s);
  auto end = chrono::steady_clock::now();

	cout << "Elapsed time in nanoseconds : "
		<< chrono::duration_cast<chrono::nanoseconds>(end - start).count()
		<< " ns" << endl;

	cout << "Elapsed time in microseconds : "
		<< chrono::duration_cast<chrono::microseconds>(end - start).count()
		<< " µs" << endl;

	cout << "Elapsed time in milliseconds : "
		<< chrono::duration_cast<chrono::milliseconds>(end - start).count()
		<< " ms" << endl;

	cout << "Elapsed time in seconds : "
		<< chrono::duration_cast<chrono::seconds>(end - start).count()
		<< " sec";

  cout << s << endl;
}

void demo_repeated_reads(ClientLib &Chunky, size_t len) {
  // Open a file
  ChunkyFile f = Chunky.open("test_" + random_string(5) + ".txt");

  // Allocate..
  f.reserve(len);

  string data = "cs244b!";

  f.write({0,7}, data);

  auto loop_start = chrono::steady_clock::now();

  // Continuously read the data from the chunkservers
  for(int i=0;true;i++) {
    string data_returned;
    auto start = chrono::steady_clock::now();
    size_t bytes_read = f.read({0,7}, data_returned);
    auto end = chrono::steady_clock::now();

    if (data.compare(data_returned) == 0) {
      cout << "[" << i << "] Success: data retrieved same as data written (" << bytes_read << " bytes)" << endl;
      cout << endl;
    }
    cout << "Elapsed time in microseconds : "
      << chrono::duration_cast<chrono::microseconds>(end - start).count()
      << " µs" << endl;

    cout << "Elapsed time in milliseconds : "
      << chrono::duration_cast<chrono::milliseconds>(end - start).count()
      << " ms" << endl;

    cout << "Microseconds elapsed since start" << chrono::duration_cast<chrono::microseconds>(end-loop_start).count() << "µs" << endl;

    std::this_thread::sleep_for (std::chrono::milliseconds(3000));
  }
}

void for_graph(ClientLib &Chunky, size_t len, int n_files) {
  // Open a file
  string data = "cs244b!";

  vector<ChunkyFile> files;
  for (int i=0;i<n_files;i++) {
    ChunkyFile f = Chunky.open("test_" + random_string(5) + ".txt");
    f.reserve(len);
    f.write({0,7}, data);
    files.push_back(f);
  }

  auto loop_start = chrono::steady_clock::now();

  // Continuously read the data from the chunkservers
  for(int i=0;true;i++) {
    // Choose a random file
    vector<ChunkyFile> out;
    std::sample(files.begin(), files.end(), std::back_inserter(out), 1, std::mt19937{std::random_device{}()});
    ChunkyFile f = out[0];
    cout << "Reading from " << f.name() << endl;
    string data_returned;
    auto start = chrono::steady_clock::now();
    size_t bytes_read = f.read({0,7}, data_returned);
    auto end = chrono::steady_clock::now();

    if (data.compare(data_returned) == 0) {
      cout << "[" << i << "] Success: data retrieved same as data written (" << bytes_read << " bytes)" << endl;
      cout << endl;
    }

    cout << "Latency in milliseconds : "
      << chrono::duration_cast<chrono::milliseconds>(end - start).count()
      << " ms" << endl;

    /* cout << "Microseconds elapsed since start" << chrono::duration_cast<chrono::microseconds>(end-loop_start).count() << "µs" << endl; */

    std::this_thread::sleep_for (std::chrono::milliseconds(10));
  }
}


int main(int argc, char **argv) {
  absl::SetProgramUsageMessage("A client");
  absl::ParseCommandLine(argc, argv);

  std::string master_ip = absl::GetFlag(FLAGS_master_ip);
  std::string master_port = absl::GetFlag(FLAGS_master_port);
  std::string master_address = master_ip + ":" + master_port;
  
  ClientLib Chunky(master_address);
  Chunky.start();

  /* demo_simplest(Chunky); */

  /* demo_repeated_reads(Chunky,256); */
  for_graph(Chunky,256,100);

  return 0;
}
