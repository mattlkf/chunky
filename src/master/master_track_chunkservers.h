#ifndef MASTER_TRACK_CHUNK_SERVERS_H
#define MASTER_TRACK_CHUNK_SERVERS_H

#include <chrono>
#include <map>
#include <mutex>
#include <set>
#include <vector>

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

private:
  // Lock the chunkserver state
  std::mutex mtx;
  // Keep track of the last time we heard from each chunkserver
  std::map<std::string, std::chrono::system_clock::time_point> last_heard;

  // Keep track of the chunk handles that each chunkserver has
  // and the chunkservers that each chunk resides on
  std::mutex mut_chunk_maps;
  std::map<std::string, std::set<std::string>> chunkserver_to_chunks;
  std::map<std::string, std::set<std::string>> chunk_to_chunkservers;
};

#endif
