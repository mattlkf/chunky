#include "src/master/master_track_chunkservers.h"
#include <iostream>

void MasterTrackChunkservers::hi() {
  std::cout << "Hi from MasterTrackChunkservers" << std::endl;
}

void MasterTrackChunkservers::store_reverse_channel(string chunkserver) {
  auto channel = grpc::CreateChannel(chunkserver, grpc::InsecureChannelCredentials());
  chunkserver_stubs[chunkserver] = chunkserver::Chunkserver::NewStub(channel);
  return;
}

bool MasterTrackChunkservers::register_heartbeat(string chunkserver) {
  // Registering heartbeat: need exclusive lock
  std::unique_lock lock(last_heard_mutex);
  // found: true if chunkserver has been heard from before
  std::cout << "Count: " << last_heard.count(chunkserver) << std::endl;
  bool never_heard = (last_heard.count(chunkserver) == 0);
  last_heard[chunkserver] = std::chrono::system_clock::now();
  return never_heard;
}

void MasterTrackChunkservers::show_last_heard() {
  std::shared_lock lock(last_heard_mutex);
  for (auto const &[ip, t] : last_heard) {
    std::time_t timeStamp = std::chrono::system_clock::to_time_t(t);
    std::cout << ip << " " << std::ctime(&timeStamp);
  }
}

string MasterTrackChunkservers::get_chunk_handle(string fname, int chunk_index) {
  // Just looking up state: non-exclusive lock
  std::shared_lock lock(chunk_handles_mutex);
  // Look up the map. If nonexistent, return error.
  return file_chunk_handles[fname][chunk_index];
}

void MasterTrackChunkservers::show_master_state_view() {
  std::chrono::system_clock::time_point timePointNow =
      std::chrono::system_clock::now();
  std::cout << std::endl;

  // Just reading the state: be non-exclusive
  std::shared_lock lock(last_heard_mutex);

  for (auto const &[ip, t] : last_heard) {
    // How long ago was this ip heard from?
    auto delta_ms = std::chrono::duration_cast<std::chrono::milliseconds>(timePointNow - t);

    std::time_t timeStamp = std::chrono::system_clock::to_time_t(t);
    string last_heard_time_str = std::ctime(&timeStamp);
    std::cout << ip << " " << (delta_ms.count() <= 1000 ? "OK" : "NOT OK") << " " << delta_ms.count() << " " << std::endl;
    /* std::cout << ip << " " << std::ctime(&timeStamp); */
  }
}

void MasterTrackChunkservers::update_mappings(string chunkserver, std::vector<string> chunk_handles) {
  // Updating the mappings: need an exclusive lock
  std::unique_lock lock(chunk_maps_mutex);

  // The new mapping is taken as authoritative so we first invalidate the old mappings
  for (string handle : chunkserver_to_chunks[chunkserver]) {
    chunk_to_chunkservers[handle].erase(chunkserver);
  }
  chunkserver_to_chunks[chunkserver].clear();

  // Use the new mapping
  for (string handle : chunk_handles) {
    chunkserver_to_chunks[chunkserver].insert(handle);
    chunk_to_chunkservers[handle].insert(chunkserver);
  }
  return;
}
