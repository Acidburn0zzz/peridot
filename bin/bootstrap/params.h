// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_MODULAR_SRC_BOOTSTRAP_PARAMS_H_
#define APPS_MODULAR_SRC_BOOTSTRAP_PARAMS_H_

#include <string>
#include <unordered_map>
#include <utility>

#include "application/services/application_launcher.fidl.h"
#include "lib/ftl/command_line.h"

namespace bootstrap {

class Params {
 public:
  using ServiceMap =
      std::unordered_map<std::string, app::ApplicationLaunchInfoPtr>;

  bool Setup(const ftl::CommandLine& command_line);

  std::string label() const { return label_; }

  ServiceMap TakeServices() { return std::move(services_); }
  app::ApplicationLaunchInfoPtr TakeInitialLaunch() {
    return std::move(initial_launch_);
  }

 private:
  ServiceMap services_;
  app::ApplicationLaunchInfoPtr initial_launch_;
  std::string label_;
};

}  // namespace bootstrap

#endif  // APPS_MODULAR_SRC_BOOTSTRAP_PARAMS_H_
