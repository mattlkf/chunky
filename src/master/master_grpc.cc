#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "src/master/master_grpc.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using master::Master;
using master::HelloReply;
using master::HelloRequest;

using namespace std;

/* class MasterServiceImpl final : public Master::Service { */
Status MasterServiceImpl::SayHello(ServerContext *context, const HelloRequest *request,
                HelloReply *reply) {
  std::string prefix("Master says ");
  trackchunkservers->hi();
  reply->set_message(prefix + request->name());
  return Status::OK;
}

Status MasterServiceImpl::SayHelloAgain(ServerContext *context, const HelloRequest *request,
                     HelloReply *reply) {
  std::string prefix("Master says again ");
  reply->set_message(prefix + request->name());
  return Status::OK;
}
