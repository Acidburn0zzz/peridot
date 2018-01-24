// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERIDOT_BIN_LEDGER_TOOL_INSPECT_COMMAND_H_
#define PERIDOT_BIN_LEDGER_TOOL_INSPECT_COMMAND_H_

#include <memory>

#include "lib/fxl/strings/string_view.h"
#include "peridot/bin/ledger/cloud_sync/public/user_config.h"
#include "peridot/bin/ledger/coroutine/coroutine_impl.h"
#include "peridot/bin/ledger/encryption/public/encryption_service.h"
#include "peridot/bin/ledger/storage/impl/ledger_storage_impl.h"
#include "peridot/bin/ledger/storage/public/page_storage.h"
#include "peridot/bin/ledger/tool/command.h"

namespace tool {

// Command that cleans the local and remote storage of Ledger.
class InspectCommand : public Command {
 public:
  explicit InspectCommand(std::vector<std::string> args);
  ~InspectCommand() override {}

  // Command:
  void Start(fxl::Closure on_done) override;

 private:
  void ListPages(fxl::Closure on_done);

  void DisplayCommit(fxl::Closure on_done);
  void PrintCommit(std::unique_ptr<const storage::Commit> commit,
                   fxl::Closure on_done);

  void DisplayCommitGraph(fxl::Closure on_done);
  void DisplayGraphCoroutine(coroutine::CoroutineHandler* handler,
                             storage::PageId page_id,
                             fxl::Closure on_done);

  void PrintHelp(fxl::Closure on_done);

  std::unique_ptr<storage::LedgerStorageImpl> GetLedgerStorage();

  std::unique_ptr<encryption::EncryptionService> encryption_service_;
  std::unique_ptr<storage::PageStorage> storage_;
  const std::vector<std::string> args_;
  std::string app_id_;
  std::string user_repository_path_;
  coroutine::CoroutineServiceImpl coroutine_service_;

  FXL_DISALLOW_COPY_AND_ASSIGN(InspectCommand);
};

}  // namespace tool

#endif  // PERIDOT_BIN_LEDGER_TOOL_INSPECT_COMMAND_H_
