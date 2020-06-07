#include "client_lib.h"
#include <ctime>
#include <random>
/* ChunkserverImpl::ChunkserverImpl(std::shared_ptr<Channel> channel, string self_address, fs::path path, size_t chunk_size_bytes) */
/*     : stub_(master::Master::NewStub(channel)), self_address(self_address), path(path), chunk_size_bytes(chunk_size_bytes) {} */

ChunkyFile::ChunkyFile(size_t chunk_size_bytes) : chunk_size_bytes(chunk_size_bytes) {
}

size_t ChunkyFile::read(ByteRange range, Data& data) {
  data.append("Dummy data");
  return data.size();
}

Status ChunkyFile::write(ByteRange range, Data data) {
  return Status::OK;
}

Status ChunkyFile::close() {
  return Status::OK;
}

ClientLib::ClientLib(string master_address) : master_address(master_address) {
  // Assign ourselves a random ID string of length n
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::minstd_rand0 generator (seed);  // minstd_rand0 is a standard linear_congruential_engine

  int n = 8;
  client_id = "";
  for(int i=0;i<n;i++) client_id.push_back((char)('a' + (generator() % ('z'-'a'))));
}

Status ClientLib::start() {
  cout << "Starting the client library with ID " << client_id << endl;
  return Status::OK;
}

ChunkyFile ClientLib::open(string fname) {
  return ChunkyFile(128);
}
