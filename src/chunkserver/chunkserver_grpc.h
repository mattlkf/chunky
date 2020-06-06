#include <grpcpp/grpcpp.h>

#include "src/protos/master/master.grpc.pb.h"
#include "src/protos/chunkserver/chunkserver.grpc.pb.h"
#include "src/chunkserver/chunkserver_impl.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class ChunkserverServiceImpl final : public Chunkserver::Service {
public:
  explicit ChunkserverServiceImpl(ChunkserverImpl *chunkserver_impl) : chunkserver_impl(chunkserver_impl) {
  }
  Status SayHello(ServerContext *context, const HelloRequest *request,
                  HelloReply *reply) override;
private:
  ChunkserverImpl *chunkserver_impl;
};

