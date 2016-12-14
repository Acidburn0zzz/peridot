// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include <memory>
#include <string>

#include "apps/modular/examples/hello_world_cpp/hello.fidl.h"
#include "apps/modular/lib/app/application_context.h"
#include "apps/modular/lib/app/connect.h"
#include "lib/ftl/command_line.h"
#include "lib/ftl/macros.h"
#include "lib/mtl/tasks/message_loop.h"

using examples::HelloPtr;

namespace {

class HelloAppParent {
 public:
  HelloAppParent(ftl::CommandLine& command_line)
      : context_(modular::ApplicationContext::CreateFromStartupInfo()) {
    auto launch_info = modular::ApplicationLaunchInfo::New();
    const std::vector<std::string>& args = command_line.positional_args();
    if (args.empty()) {
      launch_info->url = "file:///system/apps/hello_app_child";
    } else {
      launch_info->url = args[0];
      for (size_t i = 1; i < args.size(); ++i)
        launch_info->arguments.push_back(args[i]);
    }
    launch_info->services = child_services_.NewRequest();
    context_->launcher()->CreateApplication(std::move(launch_info),
                                            child_.NewRequest());

    modular::ConnectToService(child_services_.get(), hello_.NewRequest());

    DoIt("hello");
    DoIt("goodbye");
  }

 private:
  void DoIt(const std::string& request) {
    hello_->Say(request, [request](const fidl::String& response) {
      printf("%s --> %s\n", request.c_str(), response.get().c_str());
    });
  }

  std::unique_ptr<modular::ApplicationContext> context_;

  modular::ApplicationControllerPtr child_;
  modular::ServiceProviderPtr child_services_;
  HelloPtr hello_;

  FTL_DISALLOW_COPY_AND_ASSIGN(HelloAppParent);
};

}  // namespace

int main(int argc, const char** argv) {
  auto command_line = ftl::CommandLineFromArgcArgv(argc, argv);

  mtl::MessageLoop loop;
  HelloAppParent app(command_line);
  loop.Run();
  return 0;
}
