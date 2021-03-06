// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERIDOT_BIN_LEDGER_CLOUD_SYNC_IMPL_PAGE_DOWNLOAD_H_
#define PERIDOT_BIN_LEDGER_CLOUD_SYNC_IMPL_PAGE_DOWNLOAD_H_

#include "lib/cloud_provider/fidl/cloud_provider.fidl.h"
#include "lib/fidl/cpp/bindings/binding.h"
#include "lib/fxl/functional/closure.h"
#include "lib/fxl/macros.h"
#include "lib/fxl/memory/ref_ptr.h"
#include "lib/fxl/tasks/task_runner.h"
#include "peridot/bin/ledger/cloud_sync/impl/batch_download.h"
#include "peridot/bin/ledger/cloud_sync/public/sync_state_watcher.h"
#include "peridot/bin/ledger/encryption/public/encryption_service.h"
#include "peridot/bin/ledger/storage/public/page_sync_delegate.h"
#include "peridot/lib/backoff/backoff.h"
#include "peridot/lib/callback/managed_container.h"
#include "peridot/lib/callback/scoped_task_runner.h"

namespace cloud_sync {
// PageDownload handles all the download operations (commits and objects) for a
// page.
class PageDownload : public cloud_provider::PageCloudWatcher,
                     public storage::PageSyncDelegate {
 public:
  // Delegate ensuring coordination between PageDownload and the class that owns
  // it.
  class Delegate {
   public:
    // Report that the download state changed.
    virtual void SetDownloadState(DownloadSyncState sync_state) = 0;
  };

  PageDownload(callback::ScopedTaskRunner* task_runner,
               storage::PageStorage* storage,
               encryption::EncryptionService* encryption_service,
               cloud_provider::PageCloudPtr* page_cloud,
               Delegate* delegate,
               std::unique_ptr<backoff::Backoff> backoff);

  ~PageDownload() override;

  // Downloads the initial backlog of remote commits, and sets up the remote
  // watcher upon success.
  void StartDownload();

  // Returns if PageDownload is idle.
  bool IsIdle();

 private:
  // cloud_provider::PageCloudWatcher:
  void OnNewCommits(fidl::Array<cloud_provider::CommitPtr> commits,
                    fidl::Array<uint8_t> position_token,
                    const OnNewCommitsCallback& callback) override;

  void OnNewObject(fidl::Array<uint8_t> id,
                   fsl::SizedVmoTransportPtr data,
                   const OnNewObjectCallback& callback) override;

  void OnError(cloud_provider::Status status) override;

  // Called when the initial commit backlog is downloaded.
  void BacklogDownloaded();

  // Starts watching for Cloud commit notifications.
  void SetRemoteWatcher(bool is_retry);

  // Downloads the given batch of commits.
  void DownloadBatch(fidl::Array<cloud_provider::CommitPtr> commits,
                     fidl::Array<uint8_t> position_token,
                     fxl::Closure on_done);

  // storage::PageSyncDelegate:
  void GetObject(
      storage::ObjectIdentifier object_identifier,
      std::function<void(storage::Status,
                         std::unique_ptr<storage::DataSource::DataChunk>)>
          callback) override;

  void DecryptObject(
      storage::ObjectIdentifier object_identifier,
      std::unique_ptr<storage::DataSource> content,
      std::function<void(storage::Status,
                         std::unique_ptr<storage::DataSource::DataChunk>)>
          callback);

  void HandleGetObjectError(
      storage::ObjectIdentifier object_identifier,
      bool is_permanent,
      const char error_name[],
      std::function<void(storage::Status,
                         std::unique_ptr<storage::DataSource::DataChunk>)>
          callback);

  void HandleDownloadCommitError(const char error_description[]);

  // Sets the state for commit download.
  void SetCommitState(DownloadSyncState new_state);

  void RetryWithBackoff(fxl::Closure callable);

  // Owned by whoever owns this class.
  callback::ScopedTaskRunner* const task_runner_;
  storage::PageStorage* const storage_;
  encryption::EncryptionService* const encryption_service_;
  cloud_provider::PageCloudPtr* const page_cloud_;
  Delegate* const delegate_;

  std::unique_ptr<backoff::Backoff> backoff_;

  const std::string log_prefix_;

  // Work queue:
  // The current batch of remote commits being downloaded.
  std::unique_ptr<BatchDownload> batch_download_;
  // Pending remote commits to download.
  fidl::Array<cloud_provider::CommitPtr> commits_to_download_;
  fidl::Array<uint8_t> position_token_;
  // Container for in-progress datasource.
  callback::ManagedContainer managed_container_;

  // State:
  // Commit download state.
  DownloadSyncState commit_state_ = DOWNLOAD_STOPPED;
  int current_get_object_calls_ = 0;

  fidl::Binding<cloud_provider::PageCloudWatcher> watcher_binding_;

  FXL_DISALLOW_COPY_AND_ASSIGN(PageDownload);
};

}  // namespace cloud_sync

#endif  // PERIDOT_BIN_LEDGER_CLOUD_SYNC_IMPL_PAGE_DOWNLOAD_H_
