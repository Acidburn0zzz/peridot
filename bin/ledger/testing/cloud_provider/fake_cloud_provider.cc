// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "peridot/bin/ledger/testing/cloud_provider/fake_cloud_provider.h"

#include "peridot/lib/convert/convert.h"

namespace ledger {

FakeCloudProvider::FakeCloudProvider(
    CloudEraseOnCheck cloud_erase_on_check,
    CloudEraseFromWatcher cloud_erase_from_watcher)
    : device_set_(cloud_erase_on_check, cloud_erase_from_watcher) {}

FakeCloudProvider::~FakeCloudProvider() {}

void FakeCloudProvider::GetDeviceSet(
    fidl::InterfaceRequest<cloud_provider::DeviceSet> device_set,
    const GetDeviceSetCallback& callback) {
  device_set_.AddBinding(std::move(device_set));
  callback(cloud_provider::Status::OK);
}

void FakeCloudProvider::GetPageCloud(
    fidl::Array<uint8_t> app_id,
    fidl::Array<uint8_t> page_id,
    fidl::InterfaceRequest<cloud_provider::PageCloud> page_cloud,
    const GetPageCloudCallback& callback) {
  const std::string key =
      convert::ToString(app_id) + "_" + convert::ToString(page_id);
  auto it = page_clouds_.find(key);
  if (it != page_clouds_.end()) {
    it->second.Bind(std::move(page_cloud));
    callback(cloud_provider::Status::OK);
    return;
  }

  auto ret =
      page_clouds_.emplace(std::piecewise_construct, std::forward_as_tuple(key),
                           std::forward_as_tuple());
  ret.first->second.Bind(std::move(page_cloud));
  callback(cloud_provider::Status::OK);
}

void FakeCloudProvider::EraseAllData(const EraseAllDataCallback& callback) {
  if (device_set_.size() == 0u && page_clouds_.empty()) {
    // If there is nothing to be erased, just report success. This allows the
    // sync tests that want to clean up the cloud before running to work.
    callback(cloud_provider::Status::OK);
    return;
  }

  // If there is any state to be erased, report an error. We don't have yet any
  // tests that need this implemented.
  FXL_NOTIMPLEMENTED();
  callback(cloud_provider::Status::INTERNAL_ERROR);
}

}  // namespace ledger
