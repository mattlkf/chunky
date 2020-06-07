#include "client_lib.h"
#include <ctime>
#include <random>
/* ChunkserverImpl::ChunkserverImpl(std::shared_ptr<Channel> channel, string
 * self_address, fs::path path, size_t chunk_size_bytes) */
/*     : stub_(master::Master::NewStub(channel)), self_address(self_address),
 * path(path), chunk_size_bytes(chunk_size_bytes) {} */

ChunkyFile::ChunkyFile(ClientLib *client_lib, size_t chunk_size_bytes,
                       string fname)
    : client_lib(client_lib), chunk_size_bytes(chunk_size_bytes), fname(fname) {}

size_t ChunkyFile::read(ByteRange range, Data &data) {
  data.append("Dummy data");
  size_t initial_length = data.size();

  // For each chunk in the range, get the chunkservers for it...
  size_t start_byte = range.offset;
  size_t len_bytes = range.nbytes;

  size_t starting_index = start_byte / chunk_size_bytes; // the chunk containing the first byte needed
  size_t last_index = (start_byte + len_bytes - 1) / chunk_size_bytes; // the chunk containing the last byte needed

  // In this loop i is the chunk index
  for(size_t i = starting_index; i <= last_index; i++) {
    size_t chunk_start_byte = std::max(start_byte, i * chunk_size_bytes);
    size_t chunk_len_bytes = std::min((i+1)*chunk_size_bytes, start_byte+len_bytes) - chunk_start_byte;

    data.append(client_lib->get_data(fname, i, {chunk_start_byte, chunk_len_bytes}));
  }

  return data.size() - initial_length;
}

Status ChunkyFile::write(ByteRange range, Data data) { return Status::OK; }

Status ChunkyFile::close() { return Status::OK; }

Status ClientLib::get_chunkservers(string fname, size_t chunk_index) {
  return Status::OK;
}

string ClientLib::get_data(string fname, size_t chunk_index, ByteRange range) {
  return "";
}

ClientLib::ClientLib(string master_address) : master_address(master_address) {
  // Assign ourselves a random ID string of length n
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::minstd_rand0 generator(
      seed); // minstd_rand0 is a standard linear_congruential_engine

  int n = 8;
  client_id = "";
  for (int i = 0; i < n; i++)
    client_id.push_back((char)('a' + (generator() % ('z' - 'a'))));
}

Status ClientLib::start() {
  cout << "Starting the client library with ID " << client_id << endl;

  // Begin communication with the master
  auto channel =
      grpc::CreateChannel(master_address, grpc::InsecureChannelCredentials());

  master_stub = master::Master::NewStub(channel);
  return Status::OK;
}

ChunkyFile ClientLib::open(string fname) {
  return ChunkyFile(this, 128, fname);
}
