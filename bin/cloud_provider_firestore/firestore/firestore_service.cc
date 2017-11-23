// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "peridot/bin/cloud_provider_firestore/firestore/firestore_service.h"

namespace cloud_provider_firestore {

struct FirestoreService::DocumentResponseCall {
  void set_on_empty(fxl::Closure on_empty) { this->on_empty = on_empty; }

  // Context used to make the remote call.
  grpc::ClientContext context;

  // Reader used to retrieve the result of the remote call.
  std::unique_ptr<
      grpc::ClientAsyncResponseReader<google::firestore::v1beta1::Document>>
      response_reader;

  // Response of the remote call.
  google::firestore::v1beta1::Document response;

  // Response status of the remote call.
  grpc::Status status;

  // Callback to be called upon completing the remote call.
  std::function<void(bool)> on_complete;

  // Callback to be called when the call object can be deleted.
  fxl::Closure on_empty;
};

FirestoreService::FirestoreService(fxl::RefPtr<fxl::TaskRunner> main_runner,
                                   std::shared_ptr<grpc::Channel> channel)
    : main_runner_(std::move(main_runner)),
      firestore_(google::firestore::v1beta1::Firestore::NewStub(channel)) {
  polling_thread_ = std::thread(&FirestoreService::Poll, this);
}

FirestoreService::~FirestoreService() {
  cq_.Shutdown();
  polling_thread_.join();
}

void FirestoreService::CreateDocument(
    google::firestore::v1beta1::CreateDocumentRequest request,
    std::function<void(grpc::Status status,
                       google::firestore::v1beta1::Document document)>
        callback) {
  FXL_DCHECK(main_runner_->RunsTasksOnCurrentThread());

  DocumentResponseCall& call = document_response_calls_.emplace();

  call.response_reader =
      firestore_->AsyncCreateDocument(&call.context, request, &cq_);

  call.on_complete = [&call, callback = std::move(callback)](bool ok) {
    callback(std::move(call.status), std::move(call.response));
    if (call.on_empty) {
      call.on_empty();
    }
  };
  call.response_reader->Finish(&call.response, &call.status, &call.on_complete);
}

std::unique_ptr<ListenCallHandler> FirestoreService::Listen(
    ListenCallClient* client) {
  FXL_DCHECK(main_runner_->RunsTasksOnCurrentThread());

  auto stream_factory = [cq = &cq_, firestore = firestore_.get()](
                            grpc::ClientContext* context, void* tag) {
    return firestore->AsyncListen(context, cq, tag);
  };
  auto& call = listen_calls_.emplace(client, std::move(stream_factory));
  return std::make_unique<ListenCallHandler>(&call);
}

void FirestoreService::Poll() {
  void* tag;
  bool ok = false;
  while (cq_.Next(&tag, &ok)) {
    FXL_DCHECK(tag);
    auto callable = reinterpret_cast<std::function<void(bool)>*>(tag);
    main_runner_->PostTask([callable, ok] { (*callable)(ok); });
  }
}

}  // namespace cloud_provider_firestore
