// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <memory>
#include <string>

#include "application/lib/app/application_context.h"
#include "application/lib/app/connect.h"
#include "application/services/application_launcher.fidl.h"
#include "application/services/service_provider.fidl.h"
#include "apps/modular/lib/fidl/app_client.h"
#include "apps/modular/lib/fidl/array_to_string.h"
#include "apps/modular/lib/util/filesystem.h"
#include "apps/modular/services/auth/account_provider.fidl.h"
#include "apps/modular/services/config/config.fidl.h"
#include "apps/modular/services/device/device_runner_monitor.fidl.h"
#include "apps/modular/services/device/device_shell.fidl.h"
#include "apps/modular/services/device/user_provider.fidl.h"
#include "apps/modular/src/device_runner/user_provider_impl.h"
#include "apps/mozart/services/presentation/presenter.fidl.h"
#include "apps/mozart/services/views/view_provider.fidl.h"
#include "apps/mozart/services/views/view_token.fidl.h"
#include "apps/tracing/lib/trace/provider.h"
#include "lib/fidl/cpp/bindings/array.h"
#include "lib/fidl/cpp/bindings/binding.h"
#include "lib/fidl/cpp/bindings/interface_handle.h"
#include "lib/fidl/cpp/bindings/interface_request.h"
#include "lib/fidl/cpp/bindings/string.h"
#include "lib/ftl/command_line.h"
#include "lib/ftl/logging.h"
#include "lib/ftl/macros.h"
#include "lib/mtl/tasks/message_loop.h"

namespace modular {

namespace {

class Settings {
 public:
  explicit Settings(const ftl::CommandLine& command_line) {
    device_shell.url = command_line.GetOptionValueWithDefault(
        "device_shell", "file:///system/apps/userpicker_device_shell");
    story_shell.url = command_line.GetOptionValueWithDefault(
        "story_shell", "file:///system/apps/mondrian");
    user_runner.url = command_line.GetOptionValueWithDefault(
        "user_runner", "file:///system/apps/user_runner");
    user_shell.url = command_line.GetOptionValueWithDefault(
        "user_shell", "file:///system/apps/armadillo_user_shell");

    ignore_monitor = command_line.HasOption("ignore_monitor");
    no_minfs = command_line.HasOption("no_minfs");
    test = command_line.HasOption("test");

    ParseShellArgs(
        command_line.GetOptionValueWithDefault("device_shell_args", ""),
        &device_shell.args);

    ParseShellArgs(
        command_line.GetOptionValueWithDefault("story_shell_args", ""),
        &story_shell.args);

    ParseShellArgs(
        command_line.GetOptionValueWithDefault("user_runner_args", ""),
        &user_runner.args);

    ParseShellArgs(
        command_line.GetOptionValueWithDefault("user_shell_args", ""),
        &user_shell.args);

    if (test) {
      device_shell.args.push_back("--test");
      story_shell.args.push_back("--test");
      user_runner.args.push_back("--test");
      user_shell.args.push_back("--test");
      test_name = FindTestName(user_shell.url, user_shell.args);
      ignore_monitor = true;
      no_minfs = true;
    }
  }

  static std::string GetUsage() {
    return R"USAGE(device_runner
      --device_shell=DEVICE_SHELL
      --device_shell_args=SHELL_ARGS
      --user_shell=USER_SHELL
      --user_shell_args=SHELL_ARGS
      --story_shell=STORY_SHELL
      --story_shell_args=SHELL_ARGS
      --ignore_monitor
      --no_minfs
      --test
    DEVICE_NAME: Name which user shell uses to identify this device.
    DEVICE_SHELL: URL of the device shell to run.
                Defaults to 'file:///system/apps/userpicker_device_shell'.
                For integration testing use "dev_device_shell".
    USER_RUNNER: URL of the user runner to run.
                Defaults to 'file:///system/apps/user_runner'.
    USER_SHELL: URL of the user shell to run.
                Defaults to 'file:///system/apps/armadillo_user_shell'.
                For integration testing use "dev_user_shell".
    STORY_SHELL: URL of the story shell to run.
                Defaults to 'file:///system/apps/mondrian'.
                For integration testing use "dev_story_shell".
    SHELL_ARGS: Comma separated list of arguments. Backslash escapes comma.)USAGE";
  }

  AppConfig device_shell;
  AppConfig story_shell;
  AppConfig user_runner;
  AppConfig user_shell;

  std::string test_name;
  bool ignore_monitor;
  bool no_minfs;
  bool test;

 private:
  void ParseShellArgs(const std::string& value,
                      fidl::Array<fidl::String>* args) {
    bool escape = false;
    std::string arg;
    for (char i : value) {
      if (escape) {
        arg.push_back(i);
        escape = false;
        continue;
      }

      if (i == '\\') {
        escape = true;
        continue;
      }

      if (i == ',') {
        args->push_back(arg);
        arg.clear();
        continue;
      }

      arg.push_back(i);
    }

    if (!arg.empty()) {
      args->push_back(arg);
    }
  }

  // Extract the test name using knowledge of how Modular structures its
  // command lines for testing.
  static std::string FindTestName(
      const fidl::String& user_shell,
      const fidl::Array<fidl::String>& user_shell_args) {
    const std::string kRootModule = "--root_module";
    std::string result = user_shell;

    for (const auto& user_shell_arg : user_shell_args) {
      const auto& arg = user_shell_arg.get();
      if (arg.substr(0, kRootModule.size()) == kRootModule) {
        result = arg.substr(kRootModule.size());
      }
    }

    const auto index = result.find_last_of('/');
    if (index == std::string::npos) {
      return result;
    }

    return result.substr(index + 1);
  }

  FTL_DISALLOW_COPY_AND_ASSIGN(Settings);
};

class DeviceRunnerApp : DeviceShellContext, auth::AccountProviderContext {
 public:
  explicit DeviceRunnerApp(const Settings& settings)
      : settings_(settings),
        app_context_(
            app::ApplicationContext::CreateFromStartupInfoNotChecked()),
        device_shell_context_binding_(this),
        account_provider_context_binding_(this) {
    // 0a. Check if environment handle / services have been initialized.
    if (!app_context_->has_environment_services()) {
      FTL_LOG(ERROR) << "Failed to receive services from the environment.";
      exit(1);
    }

    // 0b. Connect to the device runner monitor and check this
    // instance is the only one running, unless the command line asks
    // to ignore the monitor check.
    if (settings.ignore_monitor) {
      Start();

    } else {
      app_context_->ConnectToEnvironmentService(monitor_.NewRequest());

      monitor_.set_connection_error_handler([] {
        FTL_LOG(ERROR) << "No device runner monitor found.";
        exit(1);
      });

      monitor_->GetConnectionCount([this](uint32_t count) {
        if (count != 1) {
          FTL_LOG(ERROR) << "Another device runner is running."
                         << " Please use that one, or shut it down first.";
          exit(1);
        }

        Start();
      });
    }
  }

 private:
  void Start() {
    // 0. Initialize tracing
    tracing::InitializeTracer(app_context_.get(), {"device_runner"});

    if (settings_.test) {
      FTL_LOG(INFO)
          << std::endl
          << std::endl
          << "======================== Starting Test [" << settings_.test_name
          << "]" << std::endl
          << "============================================================"
          << std::endl;
    }

    // 1. Start the device shell. This also connects the root view of the device
    // to the device shell. This is done first so that we can show some UI until
    // other things come up.
    device_shell_ = std::make_unique<AppClient<DeviceShell>>(
        app_context_->launcher().get(), settings_.device_shell.Clone());

    mozart::ViewProviderPtr device_shell_view_provider;
    ConnectToService(device_shell_->services(),
                     device_shell_view_provider.NewRequest());

    // We still need to pass a request for root view to device shell since
    // dev_device_shell (which mimics flutter behavior) blocks until it receives
    // the root view request.
    fidl::InterfaceHandle<mozart::ViewOwner> root_view;
    device_shell_view_provider->CreateView(root_view.NewRequest(), nullptr);
    if (!settings_.test) {
      app_context_->ConnectToEnvironmentService<mozart::Presenter>()->Present(
          std::move(root_view));
    }

    device_shell_->primary_service()->Initialize(
        device_shell_context_binding_.NewBinding());

    // 2. Wait for persistent data to come up.
    if (!settings_.no_minfs) {
      WaitForMinfs();
    }

    // 3. Start OAuth Token Manager App.
    AppConfigPtr token_manager_config = AppConfig::New();
    token_manager_config->url = "file:///system/apps/oauth_token_manager";
    token_manager_ = std::make_unique<AppClient<auth::AccountProvider>>(
        app_context_->launcher().get(), std::move(token_manager_config),
        "/data/modular/ACCOUNT_MANAGER");
    token_manager_->SetAppErrorHandler([] {
      FTL_CHECK(false) << "Token manager crashed. Stopping device runner.";
    });
    token_manager_->primary_service()->Initialize(
        account_provider_context_binding_.NewBinding());

    // 4. Setup user provider.
    user_provider_impl_ = std::make_unique<UserProviderImpl>(
        app_context_, settings_.user_runner, settings_.user_shell,
        settings_.story_shell, token_manager_->primary_service().get());
  }

  // |DeviceShellContext|
  void GetUserProvider(fidl::InterfaceRequest<UserProvider> request) override {
    user_provider_impl_->Connect(std::move(request));
  }

  // |DeviceShellContext|
  void Shutdown() override {
    // TODO(mesch): Some of these could be done in parallel too. UserProvider
    // must go first, but the order after user provider is for now rather
    // arbitrary. We terminate device shell last so that in tests
    // testing::Teardown() is invoked at the latest possible time. Right now it
    // just demonstrates that AppTerminate() works as we like it to.
    FTL_DLOG(INFO) << "DeviceShellContext::Shutdown()";

    if (settings_.test) {
      FTL_LOG(INFO)
          << std::endl
          << "============================================================"
          << std::endl
          << "======================== [" << settings_.test_name << "] Done";
    }

    user_provider_impl_->Teardown([this] {
      FTL_DLOG(INFO) << "- UserProvider down";
      token_manager_->AppTerminate([this] {
        FTL_DLOG(INFO) << "- AuthProvider down";
        device_shell_->AppTerminate([this] {
          FTL_DLOG(INFO) << "- DeviceShell down";
          FTL_LOG(INFO) << "Clean Shutdown";
          mtl::MessageLoop::GetCurrent()->QuitNow();
        });
      });
    });
  }

  // |AccountProviderContext|
  void GetAuthenticationContext(
      const fidl::String& account_id,
      fidl::InterfaceRequest<AuthenticationContext> request) override {
    device_shell_->primary_service()->GetAuthenticationContext(
        account_id, std::move(request));
  }

  const Settings& settings_;  // Not owned nor copied.
  std::unique_ptr<UserProviderImpl> user_provider_impl_;

  std::shared_ptr<app::ApplicationContext> app_context_;
  DeviceRunnerMonitorPtr monitor_;

  fidl::Binding<DeviceShellContext> device_shell_context_binding_;
  fidl::Binding<auth::AccountProviderContext> account_provider_context_binding_;

  std::unique_ptr<AppClient<auth::AccountProvider>> token_manager_;
  std::unique_ptr<AppClient<DeviceShell>> device_shell_;

  FTL_DISALLOW_COPY_AND_ASSIGN(DeviceRunnerApp);
};

}  // namespace
}  // namespace modular

int main(int argc, const char** argv) {
  auto command_line = ftl::CommandLineFromArgcArgv(argc, argv);
  if (command_line.HasOption("help")) {
    std::cout << modular::Settings::GetUsage() << std::endl;
    return 0;
  }

  modular::Settings settings(command_line);

  mtl::MessageLoop loop;
  modular::DeviceRunnerApp app(settings);
  loop.Run();
  return 0;
}
