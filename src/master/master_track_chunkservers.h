#ifndef MASTER_TRACK_CHUNK_SERVERS_H
#define MASTER_TRACK_CHUNK_SERVERS_H

#include <chrono>
#include <map>
#include <mutex>

using std::string;

class MasterTrackChunkservers {
public:
  void hi();

  // Record that we heard a heartbeat from a chunkserver
  void register_heartbeat(string ip);

  // Print out the chunkserver last-heard times
  void show_last_heard();

  // Print the view of the system as seen by the master
  void show_master_state_view();
private:
  // Lock the chunkserver state
  std::mutex mtx;
  // Keep track of the last time we heard from each chunkserver
  std::map<std::string, std::chrono::system_clock::time_point> last_heard;
};

#endif
