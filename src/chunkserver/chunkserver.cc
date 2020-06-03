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

#include "src/protos/chunk_report/chunk_report.grpc.pb.h"

#include "absl/flags/flag.h"
#include "absl/flags/usage.h"
#include "absl/flags/parse.h"

ABSL_FLAG(std::string, master_ip, "0.0.0.0", "my ip");
ABSL_FLAG(std::string, master_port, "50051", "master port");
ABSL_FLAG(std::string, self_ip, "0.0.0.0", "my ip");
ABSL_FLAG(std::string, self_port, "50052", "my port");

using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using chunk_report::Greeter;
using chunk_report::HelloReply;
using chunk_report::HelloRequest;

using namespace std;

class GreeterClient {
public:
  GreeterClient(std::shared_ptr<Channel> channel)
      : stub_(Greeter::NewStub(channel)) {}

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
    Status status = stub_->SayHello(&context, request, &reply);

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
  std::unique_ptr<Greeter::Stub> stub_;
};

class GreeterServiceImpl final : public Greeter::Service {
  Status SayHello(ServerContext *context, const HelloRequest *request,
                  HelloReply *reply) override {
    std::string prefix("Hello ");
    reply->set_message(prefix + request->name());
    return Status::OK;
  }

  Status SayHelloAgain(ServerContext *context, const HelloRequest *request,
                       HelloReply *reply) override {
    std::string prefix("Hello again ");
    reply->set_message(prefix + request->name());
    return Status::OK;
  }
};

// Print Current Time
void print_time_point(std::chrono::system_clock::time_point timePoint) {
  std::time_t timeStamp = std::chrono::system_clock::to_time_t(timePoint);
  std::cout << std::ctime(&timeStamp) << std::endl;
}

void RunServer() {

  std::string client_ip = absl::GetFlag(FLAGS_self_ip);
  std::string client_port = absl::GetFlag(FLAGS_self_port);
  std::string client_address = client_ip + ":" + client_port;
  std::cout << "Client's own address: " << client_address << std::endl;

  GreeterServiceImpl service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(client_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << client_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

void RunClient() {
  std::cout << "Running the client" << std::endl;
  std::string server_ip = absl::GetFlag(FLAGS_master_ip);
  std::string server_port = absl::GetFlag(FLAGS_master_port);
  std::string server_address = server_ip + ":" + server_port;
  std::cout << "Client querying server address: " << server_address
            << std::endl;

  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint (in this case,
  // localhost at port 50051). We indicate that the channel isn't authenticated
  // (use of InsecureChannelCredentials()).
  GreeterClient greeter(
      grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));
  std::string user("world");

  // Run the "client"
  while(true) {
    std::string reply = greeter.SayHello(user);
    std::cout << "Greeter received: " << reply << std::endl;

    // create a time point pointing to 1 second in future
    std::chrono::system_clock::time_point timePoint =
        std::chrono::system_clock::now() + std::chrono::seconds(3);
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

int main(int argc, char **argv) {
  absl::SetProgramUsageMessage("A chunkserver that updates the master periodically");
  // Parse command line arguments
  absl::ParseCommandLine(argc, argv);

  /* print_time_point(std::chrono::system_clock::now()); */

  // Begin the server in a separate thread
  std::thread th(&RunServer);

  // Begin the client
  RunClient();

  // Wait for the server to exit
  std::cout << "Waiting for the server to exit" << std::endl;
  th.join();

  return 0;
}
