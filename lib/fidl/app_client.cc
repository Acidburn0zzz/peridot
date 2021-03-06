// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "peridot/lib/fidl/app_client.h"

#include <fdio/limits.h>
#include <fdio/util.h>
#include <zircon/processargs.h>

#include <fcntl.h>

#include "lib/app/fidl/flat_namespace.fidl.h"
#include "lib/fxl/files/directory.h"
#include "lib/fxl/files/unique_fd.h"

namespace modular {
namespace {

zx::channel CloneChannel(int fd) {
  zx_handle_t handle[FDIO_MAX_HANDLES];
  uint32_t type[FDIO_MAX_HANDLES];

  zx_status_t r = fdio_clone_fd(fd, 0, handle, type);
  if (r < 0 || r == 0) {
    return zx::channel();
  }

  if (type[0] != PA_FDIO_REMOTE) {
    for (int i = 0; i < r; ++i) {
      zx_handle_close(handle[i]);
    }
    return zx::channel();
  }

  // Close any extra handles.
  for (int i = 1; i < r; ++i) {
    zx_handle_close(handle[i]);
  }

  return zx::channel(handle[0]);
}

}  // namespace

AppClientBase::AppClientBase(app::ApplicationLauncher* const launcher,
                             AppConfigPtr config,
                             std::string data_origin,
                             app::ServiceListPtr additional_services)
    : AsyncHolderBase(config->url) {
  auto launch_info = app::ApplicationLaunchInfo::New();
  launch_info->service_request = services_.NewRequest();
  launch_info->url = config->url;
  launch_info->arguments = config->args.Clone();

  if (!data_origin.empty()) {
    if (!files::CreateDirectory(data_origin)) {
      FXL_LOG(ERROR) << "Unable to create directory at " << data_origin;
      return;
    }
    launch_info->flat_namespace = app::FlatNamespace::New();
    launch_info->flat_namespace->paths.resize(1);
    launch_info->flat_namespace->paths[0] = "/data";
    launch_info->flat_namespace->directories.resize(1);

    fxl::UniqueFD dir(open(data_origin.c_str(), O_DIRECTORY | O_RDONLY));
    if (!dir.is_valid()) {
      FXL_LOG(ERROR) << "Unable to open directory at " << data_origin
                     << ". errno: " << errno;
      return;
    }

    launch_info->flat_namespace->directories[0] = CloneChannel(dir.get());
    if (!launch_info->flat_namespace->directories[0]) {
      FXL_LOG(ERROR) << "Unable create a handle from  " << data_origin;
      return;
    }
  }

  if (!additional_services.is_null()) {
    launch_info->additional_services = std::move(additional_services);
  }
  launcher->CreateApplication(std::move(launch_info), app_.NewRequest());
}

AppClientBase::~AppClientBase() = default;

void AppClientBase::ImplTeardown(std::function<void()> done) {
  ServiceTerminate(std::move(done));
}

void AppClientBase::ImplReset() {
  app_.Unbind();
  ServiceUnbind();
}

void AppClientBase::SetAppErrorHandler(
    const std::function<void()>& error_handler) {
  app_.set_error_handler(error_handler);
}

void AppClientBase::ServiceTerminate(const std::function<void()>& /* done */) {}

void AppClientBase::ServiceUnbind() {}

template <>
void AppClient<Lifecycle>::ServiceTerminate(const std::function<void()>& done) {
  SetAppErrorHandler(done);
  if (primary_service())
    primary_service()->Terminate();
}

}  // namespace modular
