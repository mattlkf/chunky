#ifndef GFS_SERVER_CHUNK_SERVER_SERVICE_H_
#define GFS_SERVER_CHUNK_SERVER_SERVICE_H_

#include <grpcpp/grpcpp.h>

#include "src/chunkserver/chunkserver_impl.h"
#include "src/protos/chunkserver/chunkserver.grpc.pb.h"
#include "src/protos/master/master.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;

using chunkserver::Chunkserver;

using chunkserver::AllocateChunkReply;
using chunkserver::AllocateChunkRequest;

using chunkserver::SendChunkDataReply;
using chunkserver::SendChunkDataRequest;

using chunkserver::ReadChunkDataReply;
using chunkserver::ReadChunkDataRequest;

using chunkserver::CommitChunkDataReply;
using chunkserver::CommitChunkDataRequest;

class ChunkserverServiceImpl final : public Chunkserver::Service {
public:
  explicit ChunkserverServiceImpl(ChunkserverImpl *chunkserver_impl)
      : chunkserver_impl(chunkserver_impl) {}

  grpc::Status AllocateChunk(ServerContext *context,
                       const AllocateChunkRequest *request,
                       AllocateChunkReply *reply) override;

  grpc::Status SendChunkData(ServerContext *context,
                       const SendChunkDataRequest *request,
                       SendChunkDataReply *reply) override;

  grpc::Status CommitChunkData(ServerContext *context,
                         const CommitChunkDataRequest *request,
                         CommitChunkDataReply *reply) override;

  grpc::Status ReadChunkData(ServerContext *context,
                       const ReadChunkDataRequest *request,
                       ReadChunkDataReply *reply) override;

private:
  ChunkserverImpl *chunkserver_impl;
};

#endif
