/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "src/protos/master/master.grpc.pb.h"
/* #include "src/protos/chunk_report/chunk_report.grpc.pb.h" */

#include "absl/flags/flag.h"
#include "absl/flags/usage.h"
#include "absl/flags/parse.h"

#include "src/chunkserver/chunkserver_impl.h"

ABSL_FLAG(std::string, master_ip, "0.0.0.0", "my ip");
ABSL_FLAG(std::string, master_port, "50051", "master port");
ABSL_FLAG(std::string, self_ip, "0.0.0.0", "my ip");
ABSL_FLAG(std::string, self_port, "50052", "my port");
ABSL_FLAG(int, chunk_size, 256, "chunk size in bytes");
ABSL_FLAG(std::string, chunk_path, "/chunk_storage", "chunk storage path");

using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using master::Master;
using master::HelloReply;
using master::HelloRequest;

using namespace std;

class MasterClient {
public:
  MasterClient(std::shared_ptr<Channel> channel)
      : stub_(Master::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string SayHello(const std::string &user) {
    // Data we are sending to the server.
    HelloRequest request;
    request.set_name(user);

    // Container for the data we expect from the server.
    HelloReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    grpc::Status status = stub_->SayHello(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.message();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

private:
  std::unique_ptr<Master::Stub> stub_;
};

/* class GreeterServiceImpl final : public Greeter::Service { */
/*   Status SayHello(ServerContext *context, const HelloRequest *request, */
/*                   HelloReply *reply) override { */
/*     std::string prefix("Hello "); */
/*     reply->set_message(prefix + request->name()); */
/*     return Status::OK; */
/*   } */

/*   Status SayHelloAgain(ServerContext *context, const HelloRequest *request, */
/*                        HelloReply *reply) override { */
/*     std::string prefix("Hello again "); */
/*     reply->set_message(prefix + request->name()); */
/*     return Status::OK; */
/*   } */
/* }; */

// Print Current Time
void print_time_point(std::chrono::system_clock::time_point timePoint) {
  std::time_t timeStamp = std::chrono::system_clock::to_time_t(timePoint);
  std::cout << std::ctime(&timeStamp) << std::endl;
}

/* void RunServer() { */

/*   std::string client_ip = absl::GetFlag(FLAGS_self_ip); */
/*   std::string client_port = absl::GetFlag(FLAGS_self_port); */
/*   std::string client_address = client_ip + ":" + client_port; */
/*   std::cout << "Client's own address: " << client_address << std::endl; */

/*   GreeterServiceImpl service; */

/*   ServerBuilder builder; */
/*   // Listen on the given address without any authentication mechanism. */
/*   builder.AddListeningPort(client_address, grpc::InsecureServerCredentials()); */
/*   // Register "service" as the instance through which we'll communicate with */
/*   // clients. In this case it corresponds to an *synchronous* service. */
/*   builder.RegisterService(&service); */
/*   // Finally assemble the server. */
/*   std::unique_ptr<Server> server(builder.BuildAndStart()); */
/*   std::cout << "Server listening on " << client_address << std::endl; */

/*   // Wait for the server to shutdown. Note that some other thread must be */
/*   // responsible for shutting down the server for this call to ever return. */
/*   server->Wait(); */
/* } */

void RunClient(string master_address, string self_address) {


  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint (in this case,
  // localhost at port 50051). We indicate that the channel isn't authenticated
  // (use of InsecureChannelCredentials()).
  MasterClient masterclient(
      grpc::CreateChannel(master_address, grpc::InsecureChannelCredentials()));

  // Run the "client"
  while(true) {
    std::string reply = masterclient.SayHello(self_address);
    std::cout << "MasterClient received: " << reply << std::endl;

    // create a time point pointing to 1 second in future
    std::chrono::system_clock::time_point timePoint =
        std::chrono::system_clock::now() + std::chrono::seconds(1);
    std::cout << "Going to Sleep Until :: ";
    print_time_point(timePoint);
    // Sleep Till specified time point
    // Accepts std::chrono::system_clock::time_point as argument
    std::this_thread::sleep_until(timePoint);
    std::cout << "Current Time :: ";
    // Print Current Time
    print_time_point(std::chrono::system_clock::now());
  }
}

void RunChunkServerClient(string master_address, string self_address) {
  cout << "hi there" << endl;
  // Instantiate the channel
  auto channel = grpc::CreateChannel(master_address, grpc::InsecureChannelCredentials());
  auto path = absl::GetFlag(FLAGS_chunk_path);
  auto chunk_size = absl::GetFlag(FLAGS_chunk_size);

  ChunkserverImpl chunkserver(channel, path, chunk_size);
  HALT_IF_ERROR(chunkserver.start());

  /* // [Testing] Allocate a chunk file */
  /* HALT_IF_ERROR(chunkserver.allocateChunk("test_chunk", 5)); */

  /* // [Testing] Write a chunk file */
  /* string hello_str = "Hello World"; */
  /* chunkserver.setData("test_chunk", 5, "client_a", {0, 11}, hello_str); */
  /* chunkserver.requestWrite("test_chunk", 5, "client_a"); */

  /* // [Testing] Read a chunk file */
  /* auto ret = chunkserver.getChunkData("test_chunk", 5, {0, 11}); */
  /* auto v = ret.ValueOrDie(); */
  /* string s(v.begin(), v.end()); */
  /* cout << "Read: [" << s << "]" << endl; */

  // Wait for the chunkserver thread to end (it doesn't)
  cout << "Join?" << endl;
  chunkserver.join();
}

int main(int argc, char **argv) {
  absl::SetProgramUsageMessage("A chunkserver that updates the master periodically");
  // Parse command line arguments
  absl::ParseCommandLine(argc, argv);

  cout << "Hello world!!!!!!" << endl;

  // Get the master address
  std::string master_ip = absl::GetFlag(FLAGS_master_ip);
  std::string master_port = absl::GetFlag(FLAGS_master_port);
  std::string master_address = master_ip + ":" + master_port;
  
  std::string self_ip = absl::GetFlag(FLAGS_self_ip);
  std::string self_port = absl::GetFlag(FLAGS_self_port);
  std::string self_address = self_ip + ":" + self_port;

  // Begin the server in a separate thread
  /* std::thread th(&RunServer); */

  // Begin the client
  /* RunClient(master_address, self_address); */

  RunChunkServerClient(master_address, self_address);

  return 0;
}
