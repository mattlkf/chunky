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

// TODO: wrap this in a StatusOr
vector<string> ClientLib::get_chunkservers(string fname, size_t chunk_index, string &chunk_handle) {
  master::ClientReadChunk request;
  request.set_client_name(client_id);
  request.set_file_name(fname);
  request.set_chunk_index(chunk_index);

  master::ClientReadChunkReply reply;
  grpc::ClientContext context;

  cout << "Client is querying master for a list of chunkservers" << endl;
  auto status = master_stub->ReadChunk(&context, request, &reply);

  vector<string> chunkserver_names;
  if (status.ok()) {
    cout << "Success! Got some chunkservers to query" << endl;
    for (int i=0;i<reply.chunkserver_names_size();i++) {
      chunkserver_names.push_back(reply.chunkserver_names(i));
    }
  }
  else {
    cout << "Failure! Did not get any chunkservers" << endl;
  }
  // TODO: failure cases???
  return chunkserver_names;
}

Status ClientLib::connect_to_chunkservers(vector<string> chunkservers) {
  for (string chunkserver: chunkservers) {
    // Skip if we already have a connection to this guy
    if (chunkserver_stubs.count(chunkserver) != 0) {
      // Begin communication with the chunkserver
      auto channel =
          grpc::CreateChannel(chunkserver, grpc::InsecureChannelCredentials());

      chunkserver_stubs[chunkserver] = chunkserver::Chunkserver::NewStub(channel);
    }
  }

  return Status::OK;
}

StatusOr<string> ClientLib::get_data_from_chunkserver(string chunkserver, string chunk_handle, ByteRange range) {
  string str;
  return str;
}

string ClientLib::get_data(string fname, size_t chunk_index, ByteRange range) {
  // TODO: do this repeatedly until success.
  string chunk_handle;
  // Get the list of chunkservers from the master
  vector<string> chunkservers = get_chunkservers(fname, chunk_index, chunk_handle);

  // Establish a connection to any chunkservers that have not yet been connected to
  connect_to_chunkservers(chunkservers);

  // Query at least one of the chunkservers for data
  for (string chunkserver : chunkservers) {
    auto status = get_data_from_chunkserver(chunkserver, chunk_handle, range);
  }


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
