// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_MODULAR_SRC_APPLICATION_MANAGER_APPLICATION_LOADER_H_
#define APPS_MODULAR_SRC_APPLICATION_MANAGER_APPLICATION_LOADER_H_

#include <functional>
#include <string>
#include <tuple>
#include <vector>

#include <mx/vmo.h>

#include "lib/ftl/macros.h"

namespace modular {

// TODO(jeffbrown): This should probably turn into a service which the
// environment host can implement or extend.  eg. to load from other sources.
class ApplicationLoader {
 public:
  explicit ApplicationLoader(std::vector<std::string> path);
  ~ApplicationLoader();

  // Opens the specified URL.
  // Callback receives a VMO and the URL.
  void Open(const std::string& url,
            const std::function<void(mx::vmo, std::string)>& callback);

 private:
  std::vector<std::string> path_;

  FTL_DISALLOW_COPY_AND_ASSIGN(ApplicationLoader);
};

}  // namespace modular

#endif  // APPS_MODULAR_SRC_APPLICATION_MANAGER_APPLICATION_LOADER_H_
