// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_LEDGER_SRC_AUTH_PROVIDER_TEST_TEST_AUTH_PROVIDER_H_
#define APPS_LEDGER_SRC_AUTH_PROVIDER_TEST_TEST_AUTH_PROVIDER_H_

#include "apps/ledger/src/auth_provider/auth_provider.h"

#include "lib/ftl/tasks/task_runner.h"

namespace auth_provider {
namespace test {

class TestAuthProvider : public AuthProvider {
 public:
  explicit TestAuthProvider(ftl::RefPtr<ftl::TaskRunner> task_runner);

  // AuthProvider:
  ftl::RefPtr<callback::Cancellable> GetFirebaseToken(
      std::function<void(AuthStatus, std::string)> callback) override;

  ftl::RefPtr<callback::Cancellable> GetFirebaseUserId(
      std::function<void(AuthStatus, std::string)> callback) override;

  std::string token_to_return;

  AuthStatus status_to_return = AuthStatus::OK;

  std::string user_id_to_return;

 private:
  ftl::RefPtr<ftl::TaskRunner> task_runner_;
};

}  // namespace test
}  // namespace auth_provider

#endif  // APPS_LEDGER_SRC_AUTH_PROVIDER_TEST_TEST_AUTH_PROVIDER_H_
