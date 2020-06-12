#ifndef MASTER_TRACK_CHUNK_SERVERS_H
#define MASTER_TRACK_CHUNK_SERVERS_H

#include <chrono>
#include <map>
#include <mutex>
#include <set>
#include <vector>
#include <shared_mutex>
#include <chrono>
#include <random>

// gRPC
#include <grpcpp/grpcpp.h>
#include "src/protos/master/master.grpc.pb.h"
#include "src/protos/chunkserver/chunkserver.grpc.pb.h"

// Protobuf
#include "google/protobuf/stubs/status.h"
#include "google/protobuf/stubs/statusor.h"

using std::string;
using std::vector;

using google::protobuf::util::Status;
using google::protobuf::util::StatusOr;
using google::protobuf::util::error::Code;

class MasterTrackChunkservers {
public:
  void hi();
  MasterTrackChunkservers(size_t n_replicas);

  // Record that we heard a heartbeat from a chunkserver
  // Returns true if it's the first time we've heard from this chunkserver
  bool register_heartbeat(string chunkserver);

  // Print out the chunkserver last-heard times
  void show_last_heard();

  // Print the view of the system as seen by the master
  void show_master_state_view();

  // On receipt of a chunk list from chunkserver, update the chunk handles
  // Like in GFS, the chunkserver is the SOT on its owned chunks
  void update_mappings(string chunkserver, std::vector<string> chunk_handles);

  // Map (file name, chunk index) to chunk handle
  StatusOr<string> get_chunk_handle(string fname, int chunk_index);

  // Register a chunkserver with the master
  void store_reverse_channel(string chunkserver);

  // Which chunk servers store this chunk handle?
  vector<string> get_chunkservers(string chunk_handle);

  // Allocate a file of a given number of chunks..
  Status allocate(string fname, int n_chunks);

  // Ask a particular chunkserver to allocate a particular chunk handle
  grpc::Status request_allocate_chunk(string chunkserver, string chunk_handle);

private:
  size_t n_replicas;
  string random_string(int n);

  std::minstd_rand0 *rgen;

  // Keep track of the last time we heard from each chunkserver
  mutable std::shared_mutex last_heard_mutex;
  std::map<std::string, std::chrono::system_clock::time_point> last_heard;

  // Keep track of the chunk handles that each chunkserver has
  // and the chunkservers that each chunk resides on
  mutable std::shared_mutex chunk_maps_mutex;
  std::map<std::string, std::set<std::string>> chunkserver_to_chunks;
  std::map<std::string, std::set<std::string>> chunk_to_chunkservers;

  // Map from file name to vector of chunk indices
  mutable std::shared_mutex chunk_handles_mutex;
  std::map<std::string, std::vector<std::string>> file_chunk_handles;

  // Way to contact chunkservers
  mutable std::shared_mutex chunkserver_stubs_mutex;
  std::map<std::string, std::unique_ptr<chunkserver::Chunkserver::Stub>> chunkserver_stubs;

  // Just keep track of all chunkservers
  mutable std::shared_mutex active_chunk_servers_mutex;
  std::vector<string> active_chunk_servers;

  void remove_chunkserver(string chunkserver);
};

#endif
