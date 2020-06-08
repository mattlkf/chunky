#ifndef MASTER_TRACK_CHUNK_SERVERS_H
#define MASTER_TRACK_CHUNK_SERVERS_H

#include <chrono>
#include <map>
#include <mutex>
#include <set>
#include <vector>
#include <shared_mutex>

// gRPC
#include <grpcpp/grpcpp.h>
#include "src/protos/master/master.grpc.pb.h"
#include "src/protos/chunkserver/chunkserver.grpc.pb.h"

using std::string;

class MasterTrackChunkservers {
public:
  void hi();

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
  string get_chunk_handle(string fname, int chunk_index);

  // Register a chunkserver with the master
  void store_reverse_channel(string chunkserver);

private:
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

};

#endif
