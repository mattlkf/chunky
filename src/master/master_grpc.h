#include <grpcpp/grpcpp.h>

#include "src/master/master_track_chunkservers.h"
#include "src/protos/master/master.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;

using master::HelloReply;
using master::HelloRequest;
using master::Master;
using std::string;

using master::ChunkserverHeartbeat;
using master::ChunkserverChunkList;
using master::ClientAllocateChunk;
using master::ClientReadChunk;
using master::ClientWriteChunk;

using master::ChunkserverHeartbeatReply;
using master::ChunkserverChunkListReply;
using master::ClientAllocateChunkReply;
using master::ClientReadChunkReply;
using master::ClientWriteChunkReply;


class MasterServiceImpl final : public Master::Service {
public:
  explicit MasterServiceImpl(MasterTrackChunkservers *trackchunkservers_)
      : trackchunkservers(trackchunkservers_) {}
  grpc::Status SayHello(ServerContext *context, const HelloRequest *request,
                  HelloReply *reply) override;

  grpc::Status SayHelloAgain(ServerContext *context, const HelloRequest *request,
                       HelloReply *reply) override;

  grpc::Status SendHeartbeat(ServerContext *context,
                       const ChunkserverHeartbeat *request,
                       ChunkserverHeartbeatReply *reply) override;

  grpc::Status SendChunkList(ServerContext *context,
                       const ChunkserverChunkList *request,
                       ChunkserverChunkListReply *reply) override;

  grpc::Status AllocateChunk(ServerContext *context,
                             const ClientAllocateChunk *request,
                             ClientAllocateChunkReply *reply) override;

  grpc::Status ReadChunk(ServerContext *context, const ClientReadChunk *request,
                         ClientReadChunkReply *reply) override;

  grpc::Status WriteChunk(ServerContext *context,
                          const ClientWriteChunk *request,
                          ClientWriteChunkReply *reply) override;

private:
  MasterTrackChunkservers *trackchunkservers;
};
