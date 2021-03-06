// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERIDOT_BIN_CLOUD_PROVIDER_FIRESTORE_TESTING_CLOUD_PROVIDER_FACTORY_H_
#define PERIDOT_BIN_CLOUD_PROVIDER_FIRESTORE_TESTING_CLOUD_PROVIDER_FACTORY_H_

#include <thread>

#include "lib/app/cpp/application_context.h"
#include "lib/cloud_provider/fidl/cloud_provider.fidl.h"
#include "lib/fidl/cpp/bindings/binding_set.h"
#include "lib/fxl/memory/ref_ptr.h"
#include "lib/fxl/tasks/task_runner.h"
#include "peridot/bin/cloud_provider_firestore/fidl/factory.fidl.h"
#include "peridot/lib/firebase_auth/testing/service_account_token_provider.h"
#include "peridot/lib/network/network_service_impl.h"

namespace cloud_provider_firestore {

// Factory for real Firestore cloud provider binaries backed by fake token
// provider.
//
// This is used for end-to-end testing, including the validation test suite for
// the cloud provider.
class CloudProviderFactory {
 public:
  CloudProviderFactory(app::ApplicationContext* application_context,
                       std::string credentials_path);
  ~CloudProviderFactory();

  void Init();

  void MakeCloudProvider(
      std::string server_id,
      std::string api_key,
      fidl::InterfaceRequest<cloud_provider::CloudProvider> request);

 private:
  class TokenProviderContainer;
  app::ApplicationContext* const application_context_;
  const std::string credentials_path_;

  // Thread used to run the token manager on.
  std::thread services_thread_;
  fxl::RefPtr<fxl::TaskRunner> services_task_runner_;

  callback::AutoCleanableSet<TokenProviderContainer> token_providers_;

  app::ApplicationControllerPtr cloud_provider_controller_;
  FactoryPtr cloud_provider_factory_;

  FXL_DISALLOW_COPY_AND_ASSIGN(CloudProviderFactory);
};

}  // namespace cloud_provider_firestore

#endif  // PERIDOT_BIN_CLOUD_PROVIDER_FIRESTORE_TESTING_CLOUD_PROVIDER_FACTORY_H_
