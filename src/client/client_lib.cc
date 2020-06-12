#include "client_lib.h"
#include <ctime>
#include <random>
#include <algorithm>
#include <cstdlib>
#include <chrono>

/* ChunkserverImpl::ChunkserverImpl(std::shared_ptr<Channel> channel, string
 * self_address, fs::path path, size_t chunk_size_bytes) */
/*     : stub_(master::Master::NewStub(channel)), self_address(self_address),
 * path(path), chunk_size_bytes(chunk_size_bytes) {} */

ChunkyFile::ChunkyFile(ClientLib *client_lib, size_t chunk_size_bytes,
                       string fname)
    : client_lib(client_lib), chunk_size_bytes(chunk_size_bytes), fname(fname) {}

Status ChunkyFile::reserve(size_t bytes) {
  size_t n_chunks = (bytes + chunk_size_bytes - 1) / chunk_size_bytes;
  cout << "ChunkyFile::reserve -- With a chunk size of " << chunk_size_bytes << " we reserve " << n_chunks << " for " << bytes << " bytes" << endl;
  return client_lib->allocate(fname, n_chunks); 
}

string ChunkyFile::name() {
  return fname;
}

size_t ChunkyFile::read(ByteRange range, Data &data) {
  string buf;

  // For each chunk in the range, get the chunkservers for it...
  size_t start_byte = range.offset;
  size_t len_bytes = range.nbytes;

  size_t starting_index = start_byte / chunk_size_bytes; // the chunk containing the first byte needed
  size_t last_index = (start_byte + len_bytes - 1) / chunk_size_bytes; // the chunk containing the last byte needed

  // In this loop i is the chunk index
  for(size_t i = starting_index; i <= last_index; i++) {
    size_t chunk_start_byte = std::max(start_byte, i * chunk_size_bytes);
    size_t chunk_len_bytes = std::min((i+1)*chunk_size_bytes, start_byte+len_bytes) - chunk_start_byte;

    auto chunk_data = client_lib->get_data(fname, i, {chunk_start_byte, chunk_len_bytes});
    if (chunk_data.status().ok()) {
      buf.append(chunk_data.ValueOrDie());
    }
    else {
      std::cout << "One chunk failed" << std::endl;
      break;
    }

  }

  data.append(buf);
  return buf.length();
}

Status ChunkyFile::write(ByteRange range, Data data) { 
  cout << "ChunkyFile::write " << range.offset << " " << range.nbytes << endl;

  // For each chunk in the range, get the chunkservers for it...
  size_t start_byte = range.offset;
  size_t len_bytes = range.nbytes;

  size_t starting_index = start_byte / chunk_size_bytes; // the chunk containing the first byte needed
  size_t last_index = (start_byte + len_bytes - 1) / chunk_size_bytes; // the chunk containing the last byte needed

  // In this loop i is the chunk index
  int data_pos = 0;
  for(size_t i = starting_index; i <= last_index; i++) {
    size_t chunk_start_byte = std::max(start_byte, i * chunk_size_bytes);
    size_t chunk_len_bytes = std::min((i+1)*chunk_size_bytes, start_byte+len_bytes) - chunk_start_byte;

    // Grab a slice of the string
    const string slice = data.substr(data_pos, chunk_len_bytes);
    data_pos += chunk_len_bytes;

    // TODO: re-try on failure
    auto status = client_lib->send_data(fname, i, {chunk_start_byte, chunk_len_bytes}, slice); 
    if (!status.ok()) {
      return Status::UNKNOWN;
    }
  }

  return Status::OK;
}

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
    for (int i=0;i<reply.chunkserver_names_size();i++) {
      chunkserver_names.push_back(reply.chunkserver_names(i));
    }

    cout << "Success! Chunkserver list:";
    for (string cs : chunkserver_names) {
      cout << " " << cs;
    }
    cout << endl;

    chunk_handle = reply.chunk_handle();
  }
  else {
    cout << "Failure! Did not get any chunkservers" << endl;
  }
  // TODO: failure cases???
  return chunkserver_names;
}

Status ClientLib::allocate(string fname, size_t n_chunks) {
  cout << "Inside ClientLib::allocate" << endl;
  master::ClientAllocateChunk request;
  request.set_client_name(client_id);
  request.set_file_name(fname);
  request.set_n_chunks(n_chunks);

  master::ClientAllocateChunkReply reply;
  grpc::ClientContext context;

  cout << "Client is sending an allocation request to master" << endl;
  auto status = master_stub->AllocateChunk(&context, request, &reply);

  if (status.ok()) {
    cout << "Success - master allocated chunks for us" << endl;
    return Status::OK;
  }
  else {
    cout << "Failure! " << status.error_message() << endl;
    return Status::UNKNOWN;
  }
}

Status ClientLib::connect_to_chunkservers(vector<string> chunkservers) {
  for (string chunkserver: chunkservers) {
    // Skip if we already have a connection to this guy
    // Unfortunately I had to disable this because it... doesn't work?
    if (chunkserver_stubs.count(chunkserver) != 0 || true) {
      /* cout << "Initializing a channel to " << chunkserver << endl; */
      // Begin communication with the chunkserver
      auto channel =
          grpc::CreateChannel(chunkserver, grpc::InsecureChannelCredentials());

      chunkserver_stubs[chunkserver] = chunkserver::Chunkserver::NewStub(channel);
    }
    else {
      cout << "We already have a channel to " << chunkserver << endl;
    }
  }

  return Status::OK;
}

StatusOr<string> ClientLib::get_data_from_chunkserver(string chunkserver, string chunk_handle, ByteRange range) {
  string str;
  
  chunkserver::ReadChunkDataRequest request;
  request.set_chunk_handle(chunk_handle);

  common::ByteRange *br = request.mutable_range();
  br->set_start(range.offset);
  br->set_length(range.nbytes);

  // Unused..
  request.set_chunk_version(0);

  chunkserver::ReadChunkDataReply reply;
  grpc::ClientContext context;

  cout << "Client is querying " << chunkserver << " for data" << endl;
  auto status = chunkserver_stubs[chunkserver]->ReadChunkData(&context, request, &reply);

  if (status.ok()) {
    cout << "Success! Got data from a chunkserver" << std::endl;
    return reply.data();
  }
  else {
    cout << "Not a success - client failed to get data from a chunkserver" << endl;
    return Status::UNKNOWN;
  }
}

StatusOr<string> ClientLib::get_data(string fname, size_t chunk_index, ByteRange range) {
  // TODO: do this repeatedly until success.
  string chunk_handle;
  // Get the list of chunkservers from the master
  vector<string> chunkservers = get_chunkservers(fname, chunk_index, chunk_handle);

  // Establish a connection to any chunkservers that have not yet been connected to
  connect_to_chunkservers(chunkservers);

  // Randomly shuffle the chunkserver list so we don't always query the same one
  std::random_shuffle(chunkservers.begin(), chunkservers.end());

  // Query at least one of the chunkservers for data
  for (string chunkserver : chunkservers) {
    auto data = get_data_from_chunkserver(chunkserver, chunk_handle, range);
    if (data.status().ok()) {
      // Print which chunkserver we got it from, and the current time - for graphing purposes
      auto current_time = std::chrono::steady_clock::now() - clientlib_start_time;
      cout << "[[ " << chunkserver << " " << std::chrono::duration_cast<std::chrono::microseconds>(current_time).count() << " microseconds" << endl;;
      return data.ValueOrDie();
    }
  }

  return Status::UNKNOWN;
}

Status ClientLib::send_data(string fname, size_t chunk_index, ByteRange range, string data) {
  cout << "ClientLib::send_data " << fname << " " << range.offset << " " << range.nbytes << endl;
  string chunk_handle;
  // Get the list of chunkservers from the master
  vector<string> chunkservers = get_chunkservers(fname, chunk_index, chunk_handle);

  cout << "For chunk handle " << chunk_handle<< " we got chunkservers: ";
  for (string cs : chunkservers) cout << cs << " ";
  cout << endl;

  // Establish a connection to any chunkservers that have not yet been connected to
  connect_to_chunkservers(chunkservers);
  
  // Send data to the chunkservers
  chunkserver::SendChunkDataRequest request;
  request.set_client_id(client_id);
  request.set_chunk_handle(chunk_handle);
  request.set_chunk_version(0);
  common::ByteRange *br = request.mutable_range();
  br->set_start(range.offset);
  br->set_length(range.nbytes);
  request.set_data(data);

  chunkserver::CommitChunkDataRequest commit_request;
  commit_request.set_client_id(client_id);
  commit_request.set_chunk_handle(chunk_handle);
  commit_request.set_chunk_version(0);

  bool at_least_one_worked = false;
  for (string chunkserver : chunkservers) {
    chunkserver::SendChunkDataReply reply;
    grpc::ClientContext context;
    cout << "Client is sending data to " << chunkserver << endl;

    auto send_status = chunkserver_stubs[chunkserver]->SendChunkData(&context, request, &reply);

    cout << "Client sent data" << endl;

    if (!send_status.ok()) {
      cout << "Status was not OK: " << send_status.error_message() << endl;
      continue;
    }
    chunkserver::CommitChunkDataReply commit_reply;
    grpc::ClientContext commit_context;

    cout << "Client is now committing data" << endl;
    auto commit_status = chunkserver_stubs[chunkserver]->CommitChunkData(&commit_context, commit_request, &commit_reply);
    cout << "Sent commit" << endl;
    
    if (commit_status.ok()) {
      cout << "Commit worked" << endl;
      at_least_one_worked = true;
    }
    else {
      cout << "Commit failed" << endl;
    }
  }

  return at_least_one_worked ? Status::OK : Status::UNKNOWN;
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

  // Record the starting time
  clientlib_start_time = std::chrono::steady_clock::now();

  // Begin communication with the master
  auto channel =
      grpc::CreateChannel(master_address, grpc::InsecureChannelCredentials());

  master_stub = master::Master::NewStub(channel);
  return Status::OK;
}

ChunkyFile ClientLib::open(string fname) {
  return ChunkyFile(this, 128, fname);
}
