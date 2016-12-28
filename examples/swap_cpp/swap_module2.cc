// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/modular/examples/swap_cpp/module.h"
#include "lib/mtl/tasks/message_loop.h"

int main(int argc, const char** argv) {
  mtl::MessageLoop loop;
  modular_example::ModuleApp app(
      [](auto view_manager, auto view_owner_request) {
        return new modular_example::ModuleView(
            std::move(view_manager), std::move(view_owner_request), 0xFFFF00FF);
      });
  loop.Run();
  return 0;
}
