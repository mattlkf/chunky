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

#include "src/master/master_grpc.h"
#include "src/master/master_track_chunkservers.h"

ABSL_FLAG(std::string, self_ip, "0.0.0.0", "my ip");
ABSL_FLAG(std::string, self_port, "50052", "my port");

// Print Current Time
void print_time_point(std::chrono::system_clock::time_point timePoint) {
  std::time_t timeStamp = std::chrono::system_clock::to_time_t(timePoint);
  std::cout << std::ctime(&timeStamp) << std::endl;
}

void RunServer(MasterTrackChunkservers* trackchunkservers) {
  std::string my_ip = absl::GetFlag(FLAGS_self_ip);
  std::string my_port = absl::GetFlag(FLAGS_self_port);
  std::string my_address = my_ip + ":" + my_port;
  std::cout << "Master's own address: " << my_address << std::endl;

  MasterServiceImpl service(trackchunkservers);

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

void PrintState(MasterTrackChunkservers *trackchunkservers) {
  // Query the last-heard times of all chunkservers
  while(true) {
    std::cout << "Printing state" << std::endl;
    /* trackchunkservers->show_last_heard(); */
    trackchunkservers->show_master_state_view();
    std::this_thread::sleep_for (std::chrono::milliseconds(3000));
  }
}

int main(int argc, char **argv) {
  absl::SetProgramUsageMessage("A master that receives messages from chunkservers");
  // Parse command line arguments
  absl::ParseCommandLine(argc, argv);

  // Create logic classes
  MasterTrackChunkservers trackchunkservers;

  // Begin the server in a separate thread
  std::thread th(&RunServer, &trackchunkservers);

  // Begin the state printer in a separate thread
  std::thread thread_state_print(&PrintState, &trackchunkservers);

  // Wait for the server to exit
  std::cout << "Waiting for the server to exit" << std::endl;
  th.join();

  return 0;
}

