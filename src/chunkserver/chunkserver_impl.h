#ifndef GFS_SERVER_CHUNK_SERVER_H_
#define GFS_SERVER_CHUNK_SERVER_H_

#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// ABSL cmd-line option parsing
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/time/time.h"

// Protobuf
#include "google/protobuf/stubs/status.h"
#include "google/protobuf/stubs/statusor.h"

// C++17 filesystem
#include <experimental/filesystem> // for GCC 7, use <filesystem> if GCC 8

// LevelDB
#include "leveldb/db.h"

#include "src/common/types.h"

// gRPC
#include <grpcpp/grpcpp.h>
#include "src/protos/master/master.grpc.pb.h"

// Threading
#include <thread>

// For propagating errors when calling a function.
#define RETURN_IF_ERROR(...)                                                   \
  do {                                                                         \
    google::protobuf::util::Status _status = (__VA_ARGS__);                    \
    if (!_status.ok())                                                         \
      return _status;                                                          \
  } while (0)

#define HALT_IF_ERROR(...)                                                     \
  do {                                                                         \
    google::protobuf::util::Status _status = (__VA_ARGS__);                    \
    if (!_status.ok()) {                                                       \
      cout << _status.ToString() << endl;                                      \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

// For propagating errors when calling a LevelDB function.
#define RETURN_IF_DB_ERROR(...)                                                \
  do {                                                                         \
    leveldb::Status _db_status = (__VA_ARGS__);                                \
    if (!_db_status.ok())                                                      \
      return Status(Code::INTERNAL,                                            \
                    "Database error: " + _db_status.ToString());               \
  } while (0)

namespace fs = std::experimental::filesystem;

using std::cout;
using std::endl;
using std::map;
using std::pair;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

using grpc::Channel;

using google::protobuf::util::Status;
using google::protobuf::util::StatusOr;
using google::protobuf::util::error::Code;

using master::Master;
using master::HelloReply;
using master::HelloRequest;

/* typedef string UUID; */
/* typedef uint32_t VersionNumber; */
/* typedef string ClientId; */
/* typedef string Data; */
/* typedef struct { */
/*   size_t offset; */
/*   size_t nbytes; */
/* } ByteRange; */

class ChunkserverImpl {
public:
  ChunkserverImpl(std::shared_ptr<Channel> channel, fs::path path, size_t chunk_size_bytes);
  Status start();
  StatusOr<vector<UUID>> getChunkHandles();
  StatusOr<vector<VersionNumber>> getVersionNumbers();

  // Master: tells chunkserver to allocate a new chunk
  // Chunkserver: allocates a chunk in persistent storage
  Status allocateChunk(UUID, VersionNumber);

  // Master: updates version number of chunk on all up-to-date chunkservers
  // before granting lease Chunkserver: simply updates chunk version number
  Status setChunkVersionNumber(UUID, VersionNumber);

  // Client: requests UUID, VersionNumber, ByteRange
  // Chunkserver: verify UUID in set and VersionNumber matches, returns data
  StatusOr<Data> getChunkData(UUID, VersionNumber, ByteRange);

  // Returns the current version number of a chunk
  StatusOr<VersionNumber> getChunkVersionNumber(UUID);

  // Client: sends data to the chunkserver to be written later (on RequestWrite)
  // Chunkserver: stores a mapping from (UUID, ClientId) to (ByteRange, Data) in
  // memory
  // TODO: Reject if lease for this chunk not held
  // TODO: If multiple replicas exist, forward data from client to other
  // replicas
  Status setData(UUID, VersionNumber, ClientId, ByteRange, Data);

  // Client: sends write request with chunk handle (UUID) to primary chunkserver
  // Chunkserver (if primary):
  //  - (TODO) increments mutation number for chunk, thus serializing this write
  //  - (TODO) forwards (write request (UUID + ClientId) to replicas + the
  //  mutation number (for order))
  //  - (TODO) all replicas perform the write (at present, only primary does it)
  //  - (TODO) primary chunkserver returns SUCCESS to gfs client when all
  //  replicas succeed in writing
  // TODO: Reject if lease for this chunk not held
  Status requestWrite(UUID, VersionNumber, ClientId);

  // TODO: Add Lease-related functions, e.g. GrantLease, RevokeLease
  // Status grantLease(...)
  // Status revokeLease(...)

  // TODO: Add Recovery-related functions, e.g. getChunk, recoverChunk
  // These are needed when a chunkserver comes back online and the master notes
  // that the chunkserver has an old version of a held chunk Status
  // recoverChunk(...) - master calls this to tell replica to recover a chunk
  // StatusOr< .. chunk ..> getChunk(...) - allows one replica to request chunk
  // from another

  // TODO: add data-integrity checks (guard against corrupted data on disk)
  // (Probably out of scope for this entire project?)
private:
  // This is what we can use to send messages to the Master
  std::unique_ptr<master::Master::Stub> stub_;

  fs::path path;
  size_t chunk_size_bytes;

  // Used to store chunk_handle <=> chunk_metadata mapping
  leveldb::DB *db;

  unordered_set<UUID> chunk_handles;

  // Used to buffer writes from client before they are committed to storage
  map<pair<UUID, VersionNumber>, map<ClientId, vector<pair<ByteRange, Data>>>>
      buffered_data;

  Status openDatabase();
  Status loadChunkHandlesFromFileSystem();
  Status loadChunkHandlesFromDatabase();
  StatusOr<VersionNumber> loadChunkMeta(UUID);
  Status storeChunkMeta(UUID, VersionNumber);
  Status validateRange(ByteRange);
  Status validateChunk(UUID, VersionNumber);
  Status commitBufferedWrites(UUID, VersionNumber, ClientId);
};

#endif  // GFS_SERVER_CHUNK_SERVER_H_
