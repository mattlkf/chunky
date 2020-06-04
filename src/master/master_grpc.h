#include <grpcpp/grpcpp.h>

#include "src/protos/master/master.grpc.pb.h"
#include "src/master/master_track_chunkservers.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using master::Master;
using master::HelloReply;
using master::HelloRequest;
using std::string;

class MasterServiceImpl final : public Master::Service {
public:
  explicit MasterServiceImpl(MasterTrackChunkservers *trackchunkservers_) : trackchunkservers(trackchunkservers_) {
  }
  Status SayHello(ServerContext *context, const HelloRequest *request,
                  HelloReply *reply) override;

  Status SayHelloAgain(ServerContext *context, const HelloRequest *request,
                       HelloReply *reply) override;
private:
  MasterTrackChunkservers* trackchunkservers;
};
