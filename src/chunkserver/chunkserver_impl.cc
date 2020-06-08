#include "chunkserver_impl.h"

namespace fs = std::experimental::filesystem;

ChunkserverImpl::ChunkserverImpl(std::shared_ptr<Channel> channel, string self_address, fs::path path, size_t chunk_size_bytes)
    : stub_(master::Master::NewStub(channel)), self_address(self_address), path(path), chunk_size_bytes(chunk_size_bytes) {}

Status ChunkserverImpl::loadChunkHandlesFromDatabase() {
  cout << "Loading chunk handles from database" << endl;
  auto it = this->db->NewIterator(leveldb::ReadOptions());
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    this->chunk_handles.insert(it->key().ToString());
  }

  // Check if all chunk_handles have backing file
  cout << "Checking if all storage files present" << endl;
  for (const UUID &chunk_handle : this->chunk_handles) {
    if (fs::is_regular_file(path / chunk_handle) == false) {
      return Status(Code::NOT_FOUND,
                    "Did not find backing file for UUID " + chunk_handle);
    }
  }

  // [Testing] Print the loaded chunk handles
  cout << "Database Contents" << endl;
  for (const UUID &chunk_handle : this->chunk_handles) {
    auto vn = this->loadChunkMeta(chunk_handle).ValueOrDie();
    cout << "{" << chunk_handle << " : " << vn << "}" << endl;
  }
  return Status::OK;
}

Status ChunkserverImpl::openDatabase() {
  // The path to the database
  string db_path = (path / "db").string();

  leveldb::Options options;
  options.create_if_missing = true; // Create the database if it does not exist

  // Open the database
  RETURN_IF_DB_ERROR(leveldb::DB::Open(options, db_path, &this->db));

  // Retrieve all the chunk version numbers
  return Status::OK;
}

// Call this before anything else
Status ChunkserverImpl::start() {
  // Sleep for some number of seconds (DEBUGGING)
  std::this_thread::sleep_for (std::chrono::seconds(6));
  cout << "Starting" << endl;

  // Return an error if the storage directory doesn't exist
  if (fs::is_directory(this->path) == false) {
    return Status(Code::NOT_FOUND, "Invalid storage directory");
  }

  // Load metadata database from storage
  RETURN_IF_ERROR(this->openDatabase());
  // Grab all the filenames in the storage directory.
  // Currently: filename is the chunk-handle UUID.
  RETURN_IF_ERROR(this->loadChunkHandlesFromDatabase());

  // Report to master the list of chunk handles and their version numbers
  // GFS paper, section 4.5
  th = new std::thread(&ChunkserverImpl::sendHeartbeats, this);

  return Status::OK;
}

Status ChunkserverImpl::join() {
  cout << "Joining" << endl;
  th->join();
  return Status::OK;
}

Status ChunkserverImpl::sendHeartbeats() {
  bool first_contact_with_master = true;
  while(true) {
    cout << "Hi from chunkserver" << endl;

    master::ChunkserverHeartbeat request;

    request.set_chunkserver_name(self_address);

    master::ChunkserverHeartbeatReply reply;
    grpc::ClientContext context;

    cout << "Sending heartbeat..." << endl;
    grpc::Status status = stub_->SendHeartbeat(&context, request, &reply);
    cout << "Sent heartbeat" << endl;

    if (status.ok()) {
      cout << "OK: " << (reply.update_needed() ? "Update needed" : "No update needed") << endl;
    }
    else {
      cout << status.error_code() << ": " << status.error_message() << endl;
      // Sleep for some number of seconds
      std::this_thread::sleep_for (std::chrono::seconds(1));
      continue;

    }

    // If the master requests it, or if this is our first time contacting master, send chunk list
    bool send_chunk_list = (status.ok() && reply.update_needed()) || first_contact_with_master;

    if (send_chunk_list) {
      sendChunkList();
    }

    first_contact_with_master = false; // subsequent iters are not the first

    // Sleep for some number of seconds
    std::this_thread::sleep_for (std::chrono::seconds(1));
  }
  return Status::OK;
}

Status ChunkserverImpl::sendChunkList() {
  // Take a lock on our chunk list
  mut_chunk_set.lock();
  
  master::ChunkserverChunkList request;
  request.set_chunkserver_name(self_address);

  for (UUID handle : chunk_handles) {
    cout << "I have handle: " << handle << endl;
    common::Chunk *chunk = request.add_chunks();
    chunk->set_chunkid(handle);
  }

  master::ChunkserverChunkListReply reply;
  grpc::ClientContext context;

  cout << "Sending the chunk list..." << endl;
  grpc::Status status = stub_->SendChunkList(&context, request, &reply);

  mut_chunk_set.unlock();
  return Status::OK;
}

StatusOr<vector<UUID>> ChunkserverImpl::getChunkHandles() {
  vector<UUID> v(chunk_handles.begin(), chunk_handles.end());
  return v;
}

Status ChunkserverImpl::allocateChunk(UUID chunk_handle, VersionNumber vn) {
  // A UUID:VersionNumber pair for the DB
  leveldb::Slice key(chunk_handle);
  leveldb::Slice value((const char *)&vn, sizeof vn);

  // A file to store the data
  fs::path chunk_file_path = this->path / chunk_handle;

  // Check if the chunk_handle already has a database entry
  if (this->loadChunkMeta(chunk_handle).status().code() != Code::NOT_FOUND) {
    return Status(Code::ALREADY_EXISTS, "Chunk already allocated in DB");
  }

  // Check if there's already a backing file for this chunk
  if (fs::exists(chunk_file_path)) {
    return Status(Code::ALREADY_EXISTS, "Chunk backing file exists in FS");
  }

  // Actually write to the DB
  RETURN_IF_ERROR(this->storeChunkMeta(chunk_handle, vn));
  // And create a backing file
  // (this creates a sparse file which occupies very little actual space)
  std::ofstream(chunk_file_path).put(' ');
  auto free_space_prior = fs::space(chunk_file_path).free;
  fs::resize_file(chunk_file_path,
                  chunk_size_bytes); // Sets the file size to chunk size
  auto free_space_after = fs::space(chunk_file_path).free;
  cout << "Used space on disk: " << free_space_after - free_space_prior << endl;

  this->chunk_handles.insert(chunk_handle);

  return Status::OK;
}

StatusOr<VersionNumber> ChunkserverImpl::loadChunkMeta(UUID chunk_handle) {
  leveldb::Slice key(chunk_handle);
  string value;
  if (this->db->Get(leveldb::ReadOptions(), key, &value).IsNotFound()) {
    return Status(Code::NOT_FOUND, "chunk handle not found in DB");
  }
  VersionNumber vn = *((VersionNumber *)value.c_str());
  return vn;
}

Status ChunkserverImpl::storeChunkMeta(UUID chunk_handle, VersionNumber vn) {
  leveldb::Slice key(chunk_handle);
  leveldb::Slice value((const char *)&vn, sizeof vn);
  RETURN_IF_DB_ERROR(this->db->Put(leveldb::WriteOptions(), key, value));
  return Status::OK;
}

Status ChunkserverImpl::setChunkVersionNumber(UUID chunk_handle, VersionNumber vn) {
  // TODO: in future when metadata is more than just version number,
  // read-then-write to edit just the version number

  // Remove all the buffered writes to the previous version.
  // (If a client subsequently requests a write of previously sent data it will
  // fail)
  return this->storeChunkMeta(chunk_handle, vn);
}

StatusOr<VersionNumber> ChunkserverImpl::getChunkVersionNumber(UUID chunk_handle) {
  // TODO: in future when metadata is more than just version number, read and
  // return field
  return this->loadChunkMeta(chunk_handle);
}

// maybe this should be a member of the ByteRange class instead??
Status ChunkserverImpl::validateRange(ByteRange range) {
  if (range.offset < 0 || range.nbytes < 0 ||
      range.offset + range.nbytes >= chunk_size_bytes) {
    // TODO: also check for overflow to be safe??
    return Status(Code::INVALID_ARGUMENT, "Invalid byte range provided");
  }
  return Status::OK;
}

Status ChunkserverImpl::validateChunk(UUID chunk_handle, VersionNumber vn) {
  // Check that chunk handle exists
  if (this->chunk_handles.count(chunk_handle) == 0) {
    return Status(Code::NOT_FOUND,
                  "Tried to get data from a nonexistent chunk");
  }

  // Check that backing file exists (unnecessary since checked at start)
  fs::path chunk_file_path = path / chunk_handle;
  if (fs::is_regular_file(chunk_file_path) == false) {
    return Status(Code::INTERNAL, "Backing file nonexistent");
  }

  /* // Check that backing file is the right size (maybe do this at init
   * instead) */
  if (fs::file_size(chunk_file_path) != chunk_size_bytes) {
    cout << fs::file_size(chunk_file_path) << endl;
    return Status(Code::INTERNAL, "Backing file invalid size");
  }

  // Check that version number matches (necessary for the protocol)
  auto stat = getChunkVersionNumber(chunk_handle);
  if (!stat.status().ok()) {
    return Status(Code::INTERNAL, "Couldn't get the chunk version number");
  }
  if (vn != stat.ValueOrDie()) {
    return Status(Code::INVALID_ARGUMENT, "Version number does not match");
  }

  return Status::OK;
}

Status ChunkserverImpl::setData(UUID chunk_handle, VersionNumber vn,
                            ClientId client_id, ByteRange range, Data data) {
  // Check that the data range is OK
  RETURN_IF_ERROR(this->validateRange(range));

  // Check if the chunk handle is valid
  RETURN_IF_ERROR(this->validateChunk(chunk_handle, vn));

  // Check if the data is of the right length
  if (data.size() != range.nbytes) {
    return Status(Code::INVALID_ARGUMENT,
                  "Data length does not match range length");
  }

  // Don't actually write to a file; just buffer it in memory
  this->buffered_data[{chunk_handle, vn}][client_id].push_back({range, data});
  return Status::OK;
}

Status ChunkserverImpl::commitBufferedWrites(UUID chunk_handle, VersionNumber vn,
                                         ClientId client_id) {
  fs::path chunk_file_path = this->path / chunk_handle;
  std::fstream f(chunk_file_path,
                 std::fstream::binary | std::fstream::in | std::fstream::out);

  // Write to file all the buffered writes for this particular client
  for (auto range_and_data : buffered_data[{chunk_handle, vn}][client_id]) {
    ByteRange range = range_and_data.first;
    Data data = range_and_data.second;
    // No need to validate data or range here - validated when added to buffer.
    f.seekp(range.offset);
    f.write(data.data(), range.nbytes);
  }

  f.close();
  return Status::OK;
}

Status ChunkserverImpl::requestWrite(UUID chunk_handle, VersionNumber vn,
                                 ClientId client_id) {
  // Check that chunk and version number are current
  RETURN_IF_ERROR(this->validateChunk(chunk_handle, vn));

  // Phase 1: just replay all the writes from this client to this chunk version
  return commitBufferedWrites(chunk_handle, vn, client_id);
}

StatusOr<Data> ChunkserverImpl::getChunkData(UUID chunk_handle, VersionNumber vn,
                                         ByteRange range) {
  // Check that the chunk handle and version number is OK
  RETURN_IF_ERROR(this->validateChunk(chunk_handle, vn));
  // Check that the reading range is valid
  RETURN_IF_ERROR(this->validateRange(range));

  // Construct the chunk file path by appending chunk_handle to our directory
  fs::path chunk_file_path = path / chunk_handle;
  std::ifstream is(chunk_file_path, std::ifstream::binary);

  // Allocate vector to read data from file.
  // Not doing is.read(s.data(),..) because s.data() is not mutable
  vector<char> v;
  v.resize(range.nbytes);
  is.seekg(range.offset); // Begin reading at range.offset
  is.read(v.data(),
          range.nbytes); // Copy over range.nbytes from file into vector

  // Handle the case where less than the requested number of bytes were read
  if (!is) {
    // Trim the vector to exactly the number of bytes that were read from file
    v.resize(is.gcount());
  }

  // Init a string from the vector - we'll return this on success
  string s(v.begin(), v.end());

  // Sanity check that the string length equals the bytes read from file
  if (s.size() != (size_t)is.gcount()) {
    return Status(Code::INTERNAL,
                  "Inconsistent file read count and returned data length");
  }

  is.close();
  return s;
}
