#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <grpcpp/grpcpp.h>

#include "src/chunkserver/chunkserver_grpc.h"

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

using namespace std;

grpc::Status
ChunkserverServiceImpl::AllocateChunk(ServerContext *context,
                                      const AllocateChunkRequest *request,
                                      AllocateChunkReply *reply) {
  cout << "ChunkserverServiceImpl got an AllocateChunkRequest" << endl;

  /* // Error and success cases both need this */
  /* *reply->mutable_request() = *request; */

  // Allocate the file chunk and handle possible errors
  auto status = chunkserver_impl->allocateChunk(request->chunk_handle(), 0);
  if (!status.ok()) {
    if (status.code() == Code::ALREADY_EXISTS) {
      reply->set_status(AllocateChunkReply::CONFLICT);
      return grpc::Status(grpc::StatusCode::ALREADY_EXISTS, status.ToString());
    } else {
      reply->set_status(AllocateChunkReply::UNKNOWN);
      return grpc::Status(grpc::StatusCode::UNKNOWN, status.ToString());
    }
  }

  // Success case
  reply->set_status(AllocateChunkReply::OK);
  return grpc::Status::OK;
}

grpc::Status
ChunkserverServiceImpl::SendChunkData(ServerContext *context,
                                      const SendChunkDataRequest *request,
                                      SendChunkDataReply *reply) {
  cout << "ChunkserverServiceImpl got an SendChunkDataRequest" << endl;
  /* // Error and success cases both need this */
  /* *reply->mutable_request() = *request; */

  // Buffer the data
  string client_id = request->client_id();
  auto status = chunkserver_impl->setData(
      request->chunk_handle(), request->chunk_version(), client_id,
      {request->range().start(), request->range().length()}, request->data());

  if (!status.ok()) {
    if (status.code() == Code::NOT_FOUND) {
      reply->set_status(SendChunkDataReply::NOT_FOUND);
      return grpc::Status(grpc::StatusCode::NOT_FOUND, status.ToString());
    } else if (status.code() == Code::INVALID_ARGUMENT) {
      reply->set_status(SendChunkDataReply::INVALID_VERSION);
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          status.ToString());
    } else {
      reply->set_status(SendChunkDataReply::UNKNOWN);
      return grpc::Status(grpc::StatusCode::UNKNOWN, status.ToString());
    }
  }

  // Success case
  reply->set_status(SendChunkDataReply::OK);
  return grpc::Status::OK;
}

grpc::Status
ChunkserverServiceImpl::CommitChunkData(ServerContext *context,
                                        const CommitChunkDataRequest *request,
                                        CommitChunkDataReply *reply) {
  cout << "ChunkserverServiceImpl got an CommitChunkDataRequest" << endl;
  /* // Error and success cases both need this */
  /* *reply->mutable_request() = *request; */

  // Actually perform the write
  string client_id = request->client_id();
  auto status = chunkserver_impl->requestWrite(
      request->chunk_handle(), request->chunk_version(), client_id);

  if (!status.ok()) {
    if (status.code() == Code::NOT_FOUND) {
      reply->set_status(CommitChunkDataReply::NOT_FOUND);
      return grpc::Status(grpc::StatusCode::NOT_FOUND, status.ToString());
    } else if (status.code() == Code::INVALID_ARGUMENT) {
      reply->set_status(CommitChunkDataReply::INVALID_VERSION);
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          status.ToString());
    } else {
      reply->set_status(CommitChunkDataReply::UNKNOWN);
      return grpc::Status(grpc::StatusCode::UNKNOWN, status.ToString());
    }
  }

  // Success case
  reply->set_status(CommitChunkDataReply::OK);
  return grpc::Status::OK;
}

grpc::Status
ChunkserverServiceImpl::ReadChunkData(ServerContext *context,
                                      const ReadChunkDataRequest *request,
                                      ReadChunkDataReply *reply) {
  cout << "ChunkserverServiceImpl got an ReadChunkDataRequest" << endl;
  /* // Error and success cases both need this */
  /* *reply->mutable_request() = *request; */

  // Get the data as a StatusOr
  auto data = chunkserver_impl->getChunkData(
      request->chunk_handle(), request->chunk_version(),
      {request->range().start(), request->range().length()});

  auto status = data.status();

  // Handle error cases
  if (!status.ok()) {
    if (status.code() == Code::NOT_FOUND) {
      reply->set_status(ReadChunkDataReply::NOT_FOUND);
      return grpc::Status(grpc::StatusCode::NOT_FOUND, status.ToString());
    } else if (status.code() == Code::INVALID_ARGUMENT) {
      reply->set_status(ReadChunkDataReply::INVALID_VERSION);
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          status.ToString());
    } else {
      reply->set_status(ReadChunkDataReply::UNKNOWN);
      return grpc::Status(grpc::StatusCode::UNKNOWN, status.ToString());
    }
  }

  // Success case. Should not die on ValueOrDie() since status is known OK here
  string s = data.ValueOrDie();
  reply->set_status(ReadChunkDataReply::OK);
  reply->set_data(s);
  reply->set_bytes_read(s.size());

  return grpc::Status::OK;
}
