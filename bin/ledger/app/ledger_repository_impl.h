// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERIDOT_BIN_LEDGER_APP_LEDGER_REPOSITORY_IMPL_H_
#define PERIDOT_BIN_LEDGER_APP_LEDGER_REPOSITORY_IMPL_H_

#include "lib/fidl/cpp/bindings/binding_set.h"
#include "lib/fidl/cpp/bindings/interface_ptr_set.h"
#include "lib/fxl/macros.h"
#include "lib/ledger/fidl/ledger.fidl.h"
#include "peridot/bin/ledger/app/ledger_manager.h"
#include "peridot/bin/ledger/app/sync_watcher_set.h"
#include "peridot/bin/ledger/cloud_sync/public/user_config.h"
#include "peridot/bin/ledger/cloud_sync/public/user_sync.h"
#include "peridot/bin/ledger/encryption/impl/encryption_service_factory_impl.h"
#include "peridot/bin/ledger/environment/environment.h"
#include "peridot/bin/ledger/fidl/debug.fidl.h"
#include "peridot/bin/ledger/fidl/internal.fidl.h"
#include "peridot/lib/callback/auto_cleanable.h"
#include "peridot/lib/convert/convert.h"

namespace ledger {

class LedgerRepositoryImpl : public LedgerRepository,
                             public LedgerRepositoryDebug {
 public:
  LedgerRepositoryImpl(std::string base_storage_dir,
                       Environment* environment,
                       std::unique_ptr<SyncWatcherSet> watchers,
                       std::unique_ptr<cloud_sync::UserSync> user_sync);
  ~LedgerRepositoryImpl() override;

  void set_on_empty(const fxl::Closure& on_empty_callback) {
    on_empty_callback_ = on_empty_callback;
  }

  void BindRepository(
      fidl::InterfaceRequest<LedgerRepository> repository_request);

  // Releases all handles bound to this repository impl.
  std::vector<fidl::InterfaceRequest<LedgerRepository>> Unbind();

 private:
  // LedgerRepository:
  void GetLedger(fidl::Array<uint8_t> ledger_name,
                 fidl::InterfaceRequest<Ledger> ledger_request,
                 const GetLedgerCallback& callback) override;
  void Duplicate(fidl::InterfaceRequest<LedgerRepository> request,
                 const DuplicateCallback& callback) override;
  void SetSyncStateWatcher(
      fidl::InterfaceHandle<SyncWatcher> watcher,
      const SetSyncStateWatcherCallback& callback) override;

  void GetLedgerRepositoryDebug(
      fidl::InterfaceRequest<LedgerRepositoryDebug> request,
      const GetLedgerRepositoryDebugCallback& callback) override;

  void CheckEmpty();

  // LedgerRepositoryDebug:
  void GetInstancesList(const GetInstancesListCallback& callback) override;

  void GetLedgerDebug(fidl::Array<uint8_t> ledger_name,
                      fidl::InterfaceRequest<LedgerDebug> request,
                      const GetLedgerDebugCallback& callback) override;

  const std::string base_storage_dir_;
  Environment* const environment_;
  encryption::EncryptionServiceFactoryImpl encryption_service_factory_;
  std::unique_ptr<SyncWatcherSet> watchers_;
  std::unique_ptr<cloud_sync::UserSync> user_sync_;
  callback::AutoCleanableMap<std::string,
                             LedgerManager,
                             convert::StringViewComparator>
      ledger_managers_;
  fidl::BindingSet<LedgerRepository> bindings_;
  fxl::Closure on_empty_callback_;

  fidl::BindingSet<LedgerRepositoryDebug> ledger_repository_debug_bindings_;

  FXL_DISALLOW_COPY_AND_ASSIGN(LedgerRepositoryImpl);
};

}  // namespace ledger

#endif  // PERIDOT_BIN_LEDGER_APP_LEDGER_REPOSITORY_IMPL_H_
