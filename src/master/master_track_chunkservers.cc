#include "src/master/master_track_chunkservers.h"
#include <iostream>

using std::cout;
using std::endl;

void MasterTrackChunkservers::hi() {
  std::cout << "Hi from MasterTrackChunkservers" << std::endl;
}

MasterTrackChunkservers::MasterTrackChunkservers(size_t n_replicas)
    : n_replicas(n_replicas) {
  // Assign ourselves a random ID string of length n
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  rgen = new std::minstd_rand0(seed);

  // Debug
  std::cout << "Inited MasterTrackChunkserver " << random_string(5)
            << std::endl;
}

string MasterTrackChunkservers::random_string(int n) {
  string s;
  for (int i = 0; i < n; i++)
    s.push_back((char)('a' + ((*rgen)() % ('z' - 'a'))));
  return s;
}

void MasterTrackChunkservers::store_reverse_channel(string chunkserver) {
  std::unique_lock lock(chunkserver_stubs_mutex);
  auto channel =
      grpc::CreateChannel(chunkserver, grpc::InsecureChannelCredentials());
  chunkserver_stubs[chunkserver] = chunkserver::Chunkserver::NewStub(channel);
  return;
}

bool MasterTrackChunkservers::register_heartbeat(string chunkserver) {
  bool never_heard;
  {
    // Registering heartbeat: need exclusive lock
    std::unique_lock lock(last_heard_mutex);
    // found: true if chunkserver has been heard from before
    /* std::cout << "Count: " << last_heard.count(chunkserver) << std::endl; */
    never_heard = (last_heard.count(chunkserver) == 0);
    last_heard[chunkserver] = std::chrono::system_clock::now();
  }

  // If this chunkserver has never been heard from before, add it to the active
  // list
  if (never_heard) {
    std::unique_lock lock(active_chunk_servers_mutex);
    active_chunk_servers.push_back(chunkserver);
  }
  return never_heard;
}

void MasterTrackChunkservers::show_last_heard() {
  std::shared_lock lock(last_heard_mutex);
  for (auto const &[ip, t] : last_heard) {
    std::time_t timeStamp = std::chrono::system_clock::to_time_t(t);
    std::cout << ip << " " << std::ctime(&timeStamp);
  }
}

StatusOr<string> MasterTrackChunkservers::get_chunk_handle(string fname,
                                                           int chunk_index) {
  // Just looking up state: non-exclusive lock
  std::shared_lock lock(chunk_handles_mutex);
  // Look up the map. If nonexistent, return error.
  if (chunk_index < file_chunk_handles[fname].size()) {
    return file_chunk_handles[fname][chunk_index];
  }

  return Status(Code::NOT_FOUND, "Chunk index does not exist");
}

grpc::Status
MasterTrackChunkservers::request_allocate_chunk(string chunkserver,
                                                string chunk_handle) {
  std::shared_lock lock(chunkserver_stubs_mutex);

  chunkserver::AllocateChunkRequest request;
  request.set_chunk_handle(chunk_handle);

  chunkserver::AllocateChunkReply reply;
  grpc::ClientContext context;

  auto status =
      chunkserver_stubs[chunkserver]->AllocateChunk(&context, request, &reply);

  if (!status.ok()) {
    std::cout << "Error allocating chunk on " << chunkserver << ": "
              << status.error_message() << std::endl;
  }

  return status;
}

Status MasterTrackChunkservers::allocate(string fname, int n_chunks) {
  std::cout << "Inside MasterTrackChunkservers::allocate -- fname " << fname
            << ", n_chunks " << n_chunks << std::endl;

  // Don't allow double-allocation
  {
    std::shared_lock lock(chunk_handles_mutex);
    if (file_chunk_handles[fname].size() > 0) {
      return Status(Code::UNKNOWN,
                    "File " + fname + " was previously allocated");
    }
  }

  for (int i = 0; i < n_chunks; i++) {
    //  1) generate a random chunk ID
    string chunk_handle = "chunk_" + random_string(8);

    //  2) select K = min(n_replicas, #active chunkservers) chunkservers to
    //  allocate the chunk on
    std::set<string> chosen_chunkservers;
    {
      std::shared_lock lock(active_chunk_servers_mutex);
      size_t k = std::min(n_replicas, active_chunk_servers.size());
      cout << "Will allocate on " << k << " replicas" << std::endl;
      while (chosen_chunkservers.size() < k) {
        // Choose a random chunkserver
        string candidate_chunkserver =
            active_chunk_servers[(*rgen)() % active_chunk_servers.size()];

        // Don't re-allocate on the same chunkserver
        if (chosen_chunkservers.count(candidate_chunkserver) != 0)
          continue;

        // 3) Request that they agree to host the chunk
        auto status =
            request_allocate_chunk(candidate_chunkserver, chunk_handle);

        // If they agree, then add them to the set of chunkservers for this
        // chunk
        if (status.ok()) {
          chosen_chunkservers.insert(candidate_chunkserver);
        }
      }
    }
    //  4) update the mappings from this chunk to the chunkservers that
    //  allocated it
    {
      std::unique_lock lock(chunk_maps_mutex);
      chunk_to_chunkservers[chunk_handle] = chosen_chunkservers;
      for (string chunkserver : chosen_chunkservers) {
        chunkserver_to_chunks[chunkserver].insert(chunk_handle);
      }
    }

    //  5) record this in our mapping for future lookup
    {
      std::unique_lock lock(chunk_maps_mutex);
      file_chunk_handles[fname].push_back(chunk_handle);
    }
  }

  return Status::OK;
}

vector<string> MasterTrackChunkservers::get_chunkservers(string chunk_handle) {
  std::shared_lock lock(chunk_maps_mutex);

  vector<string> chunkservers; // to return

  // TODO: handle the case where the chunk handle is not in the map
  if (chunk_to_chunkservers.count(chunk_handle) == 0) {
    return chunkservers;
  }

  for (string chunkserver : chunk_to_chunkservers[chunk_handle]) {
    chunkservers.push_back(chunkserver);
  }

  return chunkservers;
}

void MasterTrackChunkservers::remove_chunkserver(string chunkserver) {
  // Remove from last-heard list
  {
    std::unique_lock lock(last_heard_mutex);
    last_heard.erase(chunkserver);
  }
  // Remove from list of active chunk servers
  {
    std::unique_lock lock(active_chunk_servers_mutex);
    // erase-remove idiom
    active_chunk_servers.erase(std::remove(active_chunk_servers.begin(),
                                           active_chunk_servers.end(),
                                           chunkserver),
                               active_chunk_servers.end());
  }

  // Remove from mappings
  {
    std::unique_lock lock(chunk_maps_mutex);
    for (string chunk_handle : chunkserver_to_chunks[chunkserver]) {
      chunk_to_chunkservers[chunk_handle].erase(chunkserver);
    }
    chunkserver_to_chunks.erase(chunkserver);
  }
}

void MasterTrackChunkservers::show_master_state_view() {
  std::chrono::system_clock::time_point timePointNow =
      std::chrono::system_clock::now();
  std::cout << std::endl;

  vector<string> to_erase;
  // Show the time each chunkserver was last heard from
  {
    std::shared_lock lock(last_heard_mutex);

    for (auto const &[ip, t] : last_heard) {
      // How long ago was this ip heard from?
      auto delta_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
          timePointNow - t);

      std::time_t timeStamp = std::chrono::system_clock::to_time_t(t);
      string last_heard_time_str = std::ctime(&timeStamp);
      std::cout << ip << " " << (delta_ms.count() <= 400 ? "OK" : "NOT OK")
                << " " << delta_ms.count() << " " << std::endl;
      /* std::cout << ip << " " << std::ctime(&timeStamp); */
      if (delta_ms.count() > 400) {
        std::cout << "--- REMOVING CHUNKSERVER " << ip << "----" << endl;
        to_erase.push_back(ip);
      }
    }
  }

  for (string chunkserver : to_erase) {
      remove_chunkserver(chunkserver);
  }

  // Display the mappings from chunkservers to chunks
  {
    std::shared_lock lock(chunk_maps_mutex);
    for (auto const &[chunkserver, chunk_handles] : chunkserver_to_chunks) {
      cout << "[" << chunkserver << ": " << chunk_handles.size() << "]" << endl;
      /* cout << endl; */
      /* cout << "Chunks of [" << chunkserver << "]" << endl; */
      /* for (const string &chunk_handle : chunk_handles) { */
      /*   cout << "   " << chunk_handle << endl; */
      /* } */
    }
  }
}

void MasterTrackChunkservers::update_mappings(
    string chunkserver, std::vector<string> chunk_handles) {
  // Updating the mappings: need an exclusive lock
  std::unique_lock lock(chunk_maps_mutex);

  // The new mapping is taken as authoritative so we first invalidate the old
  // mappings
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
