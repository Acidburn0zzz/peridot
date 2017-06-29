// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_MODULAR_SRC_USER_RUNNER_REMOTE_INVOKER_IMPL_H_
#define APPS_MODULAR_SRC_USER_RUNNER_REMOTE_INVOKER_IMPL_H_

#include "apps/ledger/services/public/ledger.fidl.h"
#include "apps/modular/lib/fidl/operation.h"
#include "apps/modular/services/remote/remote_invoker.fidl.h"
#include "lib/fidl/cpp/bindings/binding_set.h"
#include "lib/fidl/cpp/bindings/interface_request.h"

namespace modular {

// See services/user/remote_invoker.fidl for details.
//
// Provides interface for calls to remote devices
class RemoteInvokerImpl : RemoteInvoker {
 public:
  RemoteInvokerImpl(ledger::Ledger* const ledger);
  ~RemoteInvokerImpl() override;

  void Connect(fidl::InterfaceRequest<RemoteInvoker> request);

 private:
  // |RemoteInvoker|
  void StartOnDevice(const fidl::String& device_id,
                     const fidl::String& story_id,
                     const StartOnDeviceCallback& callback) override;

  fidl::BindingSet<RemoteInvoker> bindings_;
  OperationQueue operation_queue_;
  ledger::Ledger* const ledger_;

  // Operations implemented here.
  class StartOnDeviceCall;

  FTL_DISALLOW_COPY_AND_ASSIGN(RemoteInvokerImpl);
};

}  // namespace modular

#endif  // APPS_MODULAR_SRC_USER_RUNNER_REMOTE_INVOKER_IMPL_H_
