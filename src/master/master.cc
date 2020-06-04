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

#include "absl/flags/flag.h"
#include "absl/flags/usage.h"
#include "absl/flags/parse.h"

ABSL_FLAG(std::string, self_ip, "0.0.0.0", "my ip");
ABSL_FLAG(std::string, self_port, "50052", "my port");

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

class MasterServiceImpl final : public Master::Service {
  Status SayHello(ServerContext *context, const HelloRequest *request,
                  HelloReply *reply) override {
    std::string prefix("Master says ");
    reply->set_message(prefix + request->name());
    return Status::OK;
  }

  Status SayHelloAgain(ServerContext *context, const HelloRequest *request,
                       HelloReply *reply) override {
    std::string prefix("Master says again ");
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

  std::string my_ip = absl::GetFlag(FLAGS_self_ip);
  std::string my_port = absl::GetFlag(FLAGS_self_port);
  std::string my_address = my_ip + ":" + my_port;
  std::cout << "Master's own address: " << my_address << std::endl;

  MasterServiceImpl service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(my_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Master Server listening on " << my_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char **argv) {
  absl::SetProgramUsageMessage("A master that receives messages from chunkservers");
  // Parse command line arguments
  absl::ParseCommandLine(argc, argv);

  // Begin the server in a separate thread
  std::thread th(&RunServer);

  // Wait for the server to exit
  std::cout << "Waiting for the server to exit" << std::endl;
  th.join();

  return 0;
}

