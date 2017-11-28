// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERIDOT_BIN_CLOUD_PROVIDER_FIREBASE_NETWORK_NETWORK_SERVICE_IMPL_H_
#define PERIDOT_BIN_CLOUD_PROVIDER_FIREBASE_NETWORK_NETWORK_SERVICE_IMPL_H_

#include "lib/fxl/tasks/task_runner.h"
#include "lib/network/fidl/network_service.fidl.h"
#include "peridot/bin/cloud_provider_firebase/network/network_service.h"
#include "peridot/lib/backoff/exponential_backoff.h"
#include "peridot/lib/callback/auto_cleanable.h"
#include "peridot/lib/callback/scoped_task_runner.h"

namespace ledger {

class NetworkServiceImpl : public NetworkService {
 public:
  NetworkServiceImpl(
      fxl::RefPtr<fxl::TaskRunner> task_runner,
      std::function<network::NetworkServicePtr()> network_service_factory);
  ~NetworkServiceImpl() override;

  fxl::RefPtr<callback::Cancellable> Request(
      std::function<network::URLRequestPtr()> request_factory,
      std::function<void(network::URLResponsePtr)> callback) override;

 private:
  class RunningRequest;

  network::NetworkService* GetNetworkService();

  void RetryGetNetworkService();

  backoff::ExponentialBackoff backoff_;
  bool in_backoff_ = false;
  std::function<network::NetworkServicePtr()> network_service_factory_;
  network::NetworkServicePtr network_service_;
  callback::AutoCleanableSet<RunningRequest> running_requests_;

  // Must be the last member field.
  callback::ScopedTaskRunner task_runner_;
};

}  // namespace ledger

#endif  // PERIDOT_BIN_CLOUD_PROVIDER_FIREBASE_NETWORK_NETWORK_SERVICE_IMPL_H_
