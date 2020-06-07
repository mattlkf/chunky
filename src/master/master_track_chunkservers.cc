#include "src/master/master_track_chunkservers.h"
#include <iostream>

void MasterTrackChunkservers::hi() {
  std::cout << "Hi from MasterTrackChunkservers" << std::endl;
}

bool MasterTrackChunkservers::register_heartbeat(string chunkserver) {
  mtx.lock();
  // found: true if chunkserver has been heard from before
  std::cout << "Count: " << last_heard.count(chunkserver) << std::endl;
  bool never_heard = (last_heard.count(chunkserver) == 0);
  last_heard[chunkserver] = std::chrono::system_clock::now();
  mtx.unlock();
  return never_heard;
}

void MasterTrackChunkservers::show_last_heard() {
  // TODO: make thread-safe
  for (auto const &[ip, t] : last_heard) {
    std::time_t timeStamp = std::chrono::system_clock::to_time_t(t);
    std::cout << ip << " " << std::ctime(&timeStamp);
  }
}

void MasterTrackChunkservers::show_master_state_view() {
  std::chrono::system_clock::time_point timePointNow =
      std::chrono::system_clock::now();
  std::cout << std::endl;

  mtx.lock();
  // TODO: make thread-safe
  for (auto const &[ip, t] : last_heard) {
    // How long ago was this ip heard from?
    auto delta_ms = std::chrono::duration_cast<std::chrono::milliseconds>(timePointNow - t);


    std::time_t timeStamp = std::chrono::system_clock::to_time_t(t);
    string last_heard_time_str = std::ctime(&timeStamp);
    std::cout << ip << " " << (delta_ms.count() <= 1000 ? "OK" : "NOT OK") << " " << delta_ms.count() << " " << std::endl;
    /* std::cout << ip << " " << std::ctime(&timeStamp); */
  }
  mtx.unlock();
}
