#include <grpcpp/grpcpp.h>

#include "src/protos/master/master.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using master::Master;
using master::HelloReply;
using master::HelloRequest;

class MasterServiceImpl final : public Master::Service {
  Status SayHello(ServerContext *context, const HelloRequest *request,
                  HelloReply *reply) override;

  Status SayHelloAgain(ServerContext *context, const HelloRequest *request,
                       HelloReply *reply) override;
};
