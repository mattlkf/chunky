#ifndef GFS_CLIENT_LIB_H_
#define GFS_CLIENT_LIB_H_

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

#include "src/common/types.h"

// gRPC
#include <grpcpp/grpcpp.h>
#include "src/protos/master/master.grpc.pb.h"
#include "src/protos/chunkserver/chunkserver.grpc.pb.h"

// Threading
#include <thread>
#include <mutex>

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

class ClientLib; // needed because these two classes are mutually recursive

class ChunkyFile {
  public:
    ChunkyFile(ClientLib *client_lib, size_t chunk_size_bytes, string fname);
    size_t read(ByteRange, Data&);
    Status write(ByteRange, Data);
    Status close();

  private:
    ClientLib *client_lib;
    size_t chunk_size_bytes;
    string fname;
};

class ClientLib {
public:
  ClientLib(string master_address);
  Status start();

  ChunkyFile open(string fname);
  vector<string> get_chunkservers(string fname, size_t chunk_index, string &chunk_handle);
  string get_data(string fname, size_t chunk_index, ByteRange range);

private:
  Status connect_to_chunkservers(vector<string>);
  StatusOr<string> get_data_from_chunkserver(string chunkserver, string chunk_handle, ByteRange range);
  string master_address;
  string self_address;
  
  // TODO: populate this with a random string
  string client_id;

  // This is what we can use to send messages to the Master
  std::unique_ptr<master::Master::Stub> master_stub;

  map<string, std::unique_ptr<chunkserver::Chunkserver::Stub>> chunkserver_stubs;

  // TODO: we actually need to maintain a few handles to the chunkservers...
  /* std::unique_ptr<chunkserver::chunkserver::Stub> stub_; */
};

#endif  // GFS_SERVER_CHUNK_SERVER_H_
