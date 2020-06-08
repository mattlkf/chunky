#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <grpcpp/grpcpp.h>

#include "src/master/master_grpc.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using master::HelloReply;
using master::HelloRequest;
using master::Master;

using master::ChunkserverChunkList;
using master::ChunkserverHeartbeat;
using master::ClientAllocateChunk;
using master::ClientReadChunk;
using master::ClientWriteChunk;

using master::ChunkserverChunkListReply;
using master::ChunkserverHeartbeatReply;
using master::ClientAllocateChunkReply;
using master::ClientReadChunkReply;
using master::ClientWriteChunkReply;

using namespace std;

/* class MasterServiceImpl final : public Master::Service { */
grpc::Status MasterServiceImpl::SayHello(ServerContext *context,
                                   const HelloRequest *request,
                                   HelloReply *reply) {
  std::string prefix("Master says ");
  trackchunkservers->register_heartbeat(request->name());
  reply->set_message(prefix + request->name());
  return grpc::Status::OK;
}

grpc::Status MasterServiceImpl::SayHelloAgain(ServerContext *context,
                                        const HelloRequest *request,
                                        HelloReply *reply) {
  std::string prefix("Master says again ");
  reply->set_message(prefix + request->name());
  return grpc::Status::OK;
}

grpc::Status MasterServiceImpl::SendHeartbeat(ServerContext *context,
                                        const ChunkserverHeartbeat *request,
                                        ChunkserverHeartbeatReply *reply) {
  cout << "Master received a SendHeartbeat request" << endl;
  string cs_name = request->chunkserver_name();

  // Record the heartbeat (also checks if master heard this chunkserver before)
  bool never_heard = trackchunkservers->register_heartbeat(cs_name);

  // If this is the first time, initialize a reverse-channel to this chunkserver
  if (never_heard) {
    trackchunkservers->store_reverse_channel(cs_name);
  }

  // If the master has not heard this chunkserver before, it needs chunk list
  reply->set_update_needed(never_heard);
  return grpc::Status::OK;
}

grpc::Status MasterServiceImpl::SendChunkList(ServerContext *context,
                                        const ChunkserverChunkList *request,
                                        ChunkserverChunkListReply *reply) {
  cout << "Master received a SendChunkList request" << endl;
  
  vector<string> chunk_handles;
  for (int i=0;i < request->chunks_size(); i++) {
    cout << "Chunk handle " << ": " << request->chunks(i).chunkid() << endl;
    chunk_handles.push_back(request->chunks(i).chunkid());
  }

  // Update the master's mappings from chunk handles -> chunk servers
  // and vice versa

  trackchunkservers->update_mappings(request->chunkserver_name(), chunk_handles);
  return grpc::Status::OK;
}

grpc::Status MasterServiceImpl::AllocateChunk(ServerContext *context,
                                        const ClientAllocateChunk *request,
                                        ClientAllocateChunkReply *reply) {
  cout << "MasterServiceImpl::Allocate -- Master received a ClientAllocateChunk request" << endl;
  trackchunkservers->allocate(request->file_name(), request->n_chunks());
  return grpc::Status::OK;
}

grpc::Status MasterServiceImpl::ReadChunk(ServerContext *context,
                                    const ClientReadChunk *request,
                                    ClientReadChunkReply *reply) {
  cout << "Master received a ClientReadChunk request" << endl;

  // Grab the associated chunk handle for the (filename,chunk index) tuple if it exists
  auto data = trackchunkservers->get_chunk_handle(request->file_name(), request->chunk_index());
  if (!data.status().ok()) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND, data.status().ToString());
  }
  string chunk_handle = data.ValueOrDie();

  // Return the list of chunk servers that have this handle
  vector<string> chunkservers = trackchunkservers->get_chunkservers(chunk_handle);

  // Populate the reply with the chunk handle and list of chunkservers
  reply->set_chunk_handle(chunk_handle);
  for (string chunkserver : chunkservers) {
    reply->add_chunkserver_names(chunkserver);
  }

  return grpc::Status::OK;
}

grpc::Status MasterServiceImpl::WriteChunk(ServerContext *context,
                                     const ClientWriteChunk *request,
                                     ClientWriteChunkReply *reply) {
  cout << "Master received a ClientWriteChunk request" << endl;
  // Grab the associated chunk handle for the (filename,chunk index) tuple if it exists
  auto data = trackchunkservers->get_chunk_handle(request->file_name(), request->chunk_index());
  if (!data.status().ok()) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND, data.status().ToString());
  }
  string chunk_handle = data.ValueOrDie();

  // Return the list of chunk servers that have this handle
  vector<string> chunkservers = trackchunkservers->get_chunkservers(chunk_handle);

  // Populate the reply with the chunk handle and list of chunkservers
  reply->set_chunk_handle(chunk_handle);
  for (string chunkserver : chunkservers) {
    reply->add_chunkserver_names(chunkserver);
  }
  return grpc::Status::OK;
}
