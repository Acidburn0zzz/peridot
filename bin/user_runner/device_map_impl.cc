// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/modular/src/user_runner/device_map_impl.h"

#include <limits.h>
#include <unistd.h>

#include "apps/modular/lib/fidl/array_to_string.h"
#include "apps/modular/lib/fidl/json_xdr.h"
#include "apps/modular/lib/ledger/operations.h"
#include "apps/modular/lib/ledger/storage.h"
#include "apps/modular/lib/rapidjson/rapidjson.h"
#include "lib/fidl/cpp/bindings/array.h"
#include "lib/ftl/time/time_point.h"
#include "lib/mtl/vmo/strings.h"

namespace modular {

namespace {

void XdrDeviceData(XdrContext* const xdr, DeviceMapEntry* const data) {
  xdr->Field("name", &data->name);
  xdr->Field("device_id", &data->device_id);
  xdr->Field("profile", &data->profile);
  xdr->Field("hostname", &data->hostname);
}

std::string LoadHostname() {
  char host_name_buffer[HOST_NAME_MAX + 1];
  int result = gethostname(host_name_buffer, sizeof(host_name_buffer));

  if (result < 0) {
    FTL_LOG(ERROR) << "unable to get hostname. errno " << errno;
    return "fuchsia";
  }

  return host_name_buffer;
}

}  // namespace

DeviceMapImpl::DeviceMapImpl(const std::string& device_name,
                             const std::string& device_id,
                             const std::string& device_profile,
                             ledger::Page* const page)
    : PageClient("DeviceMapImpl", page, kDeviceKeyPrefix), page_(page) {
  current_device_.name = device_name;
  current_device_.device_id = device_id;
  current_device_.profile = device_profile;
  current_device_.hostname = LoadHostname();

  new WriteDataCall<DeviceMapEntry, fidl::InlinedStructPtr<DeviceMapEntry>>(
      &operation_queue_, page, MakeDeviceKey(device_id), XdrDeviceData,
      current_device_.Clone(), [] {});
}

DeviceMapImpl::~DeviceMapImpl() = default;

void DeviceMapImpl::Connect(fidl::InterfaceRequest<DeviceMap> request) {
  bindings_.AddBinding(this, std::move(request));
}

void DeviceMapImpl::Query(const QueryCallback& callback) {
  new ReadAllDataCall<DeviceMapEntry, fidl::InlinedStructPtr<DeviceMapEntry>>(
      &operation_queue_, page_, kDeviceKeyPrefix, XdrDeviceData, callback);
}

void DeviceMapImpl::GetCurrentDevice(const GetCurrentDeviceCallback& callback) {
  callback(current_device_.Clone());
}

void DeviceMapImpl::OnPageChange(const std::string& key,
                                 const std::string& /*value*/) {
  FTL_LOG(INFO) << "New Device: " << key;
}

void DeviceMapImpl::OnPageDelete(const std::string& key) {
  FTL_LOG(INFO) << "Deleted Device: " << key;
}

}  // namespace modular
