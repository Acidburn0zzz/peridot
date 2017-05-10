// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/modular/lib/testing/component_base.h"
#include "apps/modular/lib/testing/reporting.h"
#include "apps/modular/lib/testing/testing.h"
#include "apps/modular/services/agent/agent.fidl.h"
#include "lib/ftl/logging.h"
#include "lib/mtl/tasks/message_loop.h"

namespace {

class TestAgentApp : modular::testing::ComponentBase<modular::Agent> {
 public:
  static void New() {
    new TestAgentApp;
  }

 private:
  TestAgentApp() { TestInit(__FILE__); }
  ~TestAgentApp() override = default;

  // |Agent|
  void Initialize(fidl::InterfaceHandle<modular::AgentContext> agent_context,
                  const InitializeCallback& callback) override {
    callback();
  }

  // |Agent|
  void Connect(const fidl::String& requestor_url,
               fidl::InterfaceRequest<app::ServiceProvider> services) override {
    modular::testing::GetStore()->Put("test_agent2_connected", "", [] {});
  }

  // |Agent|
  void RunTask(const fidl::String& task_id,
               const RunTaskCallback& callback) override {}

  // |Agent|
  void Stop(const StopCallback& callback) override {
    TEST_PASS("Test agent2 exited");
    DeleteAndQuit(callback);
  }
};

}  // namespace

int main(int argc, const char** argv) {
  mtl::MessageLoop loop;
  TestAgentApp::New();
  loop.Run();
  return 0;
}
