// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>

#include "application/lib/app/connect.h"
#include "application/services/service_provider.fidl.h"
#include "apps/maxwell/services/context/context_publisher.fidl.h"
#include "apps/maxwell/services/context/context_reader.fidl.h"
#include "apps/modular/lib/fidl/array_to_string.h"
#include "apps/modular/lib/fidl/single_service_view_app.h"
#include "apps/modular/lib/testing/component_base.h"
#include "apps/modular/lib/testing/reporting.h"
#include "apps/modular/lib/testing/testing.h"
#include "apps/modular/services/user/focus.fidl.h"
#include "apps/modular/services/user/user_shell.fidl.h"
#include "apps/mozart/services/views/view_manager.fidl.h"
#include "apps/mozart/services/views/view_provider.fidl.h"
#include "apps/test_runner/services/test_runner.fidl.h"
#include "lib/fidl/cpp/bindings/binding.h"
#include "lib/ftl/command_line.h"
#include "lib/ftl/functional/make_copyable.h"
#include "lib/ftl/logging.h"
#include "lib/ftl/macros.h"
#include "lib/ftl/tasks/task_runner.h"
#include "lib/ftl/time/time_delta.h"
#include "lib/mtl/tasks/message_loop.h"

namespace {

constexpr char kModuleUrl[] = "file:///system/apps/modular_tests/null_module";
constexpr char kTopic[] = "/location/home_work";

// A simple story provider watcher implementation. Just logs observed state
// transitions.
class StoryProviderWatcherImpl : modular::StoryProviderWatcher {
 public:
  StoryProviderWatcherImpl() : binding_(this) {}
  ~StoryProviderWatcherImpl() override = default;

  // Registers itself a watcher on the given story provider. Only one story
  // provider can be watched at a time.
  void Watch(modular::StoryProviderPtr* const story_provider) {
    (*story_provider)->Watch(binding_.NewBinding());
  }

  // Deregisters itself from the watched story provider.
  void Reset() { binding_.Close(); }

 private:
  // |StoryProviderWatcher|
  void OnDelete(const ::fidl::String& story_id) override {
    FTL_VLOG(1) << "TestApp::OnDelete() " << story_id;
  }

  // |StoryProviderWatcher|
  void OnChange(modular::StoryInfoPtr story_info,
                modular::StoryState /*story_state*/) override {
    FTL_VLOG(1) << "TestApp::OnChange() "
                << " id " << story_info->id << " url " << story_info->url;
  }

  fidl::Binding<modular::StoryProviderWatcher> binding_;
  FTL_DISALLOW_COPY_AND_ASSIGN(StoryProviderWatcherImpl);
};

// A simple story watcher implementation that invokes a "continue" callback when
// it sees the watched story transition to RUNNING state. Used to push the test
// sequence forward when the test story has started.
class StoryWatcherImpl : modular::StoryWatcher {
 public:
  StoryWatcherImpl() : binding_(this) {}
  ~StoryWatcherImpl() override = default;

  // Registers itself as a watcher on the given story. Only one story at a time
  // can be watched.
  void Watch(modular::StoryController* const story_controller) {
    story_controller->Watch(binding_.NewBinding());
  }

  // Deregisters itself from the watched story.
  void Reset() { binding_.Close(); }

  // Sets the function where to continue when the story is observed to be
  // running.
  void Continue(std::function<void()> at) { continue_ = at; }

 private:
  // |StoryWatcher|
  void OnStateChange(modular::StoryState state) override {
    FTL_VLOG(4) << "OnStateChange() " << state;
    if (state != modular::StoryState::RUNNING) {
      return;
    }

    continue_();
  }

  // |StoryWatcher|
  void OnModuleAdded(modular::ModuleDataPtr /*module_data*/) override {}

  fidl::Binding<modular::StoryWatcher> binding_;
  std::function<void()> continue_;
  FTL_DISALLOW_COPY_AND_ASSIGN(StoryWatcherImpl);
};

// A simple focus watcher implementation that invokes a "continue" callback when
// it sees the next focus change.
class FocusWatcherImpl : modular::FocusWatcher {
 public:
  FocusWatcherImpl() : binding_(this) {}
  ~FocusWatcherImpl() override = default;

  // Registers itself as a watcher on the focus provider.
  void Watch(modular::FocusProvider* const focus_provider) {
    focus_provider->Watch(binding_.NewBinding());
  }

  // Deregisters itself from the watched focus provider.
  void Reset() { binding_.Close(); }

  // Sets the function where to continue when the next focus change happens.
  void Continue(std::function<void()> at) { continue_ = at; }

 private:
  // |FocusWatcher|
  void OnFocusChange(modular::FocusInfoPtr info) override {
    FTL_VLOG(4) << "OnFocusChange() " << info->focused_story_id;
    continue_();
  }

  fidl::Binding<modular::FocusWatcher> binding_;
  std::function<void()> continue_;
  FTL_DISALLOW_COPY_AND_ASSIGN(FocusWatcherImpl);
};

// A context reader watcher implementation.
class ContextListenerForTopicsImpl : maxwell::ContextListenerForTopics {
 public:
  ContextListenerForTopicsImpl() : binding_(this) {
    handler_ = [](fidl::String, fidl::String) {};
  }

  ~ContextListenerForTopicsImpl() override = default;

  // Registers itself a watcher on the given story provider. Only one story
  // provider can be watched at a time.
  void Listen(maxwell::ContextReader* const context_reader) {
    auto query = maxwell::ContextQueryForTopics::New();
    query->topics.resize(0);
    context_reader->SubscribeToTopics(std::move(query), binding_.NewBinding());
    binding_.set_connection_error_handler(
        [] { FTL_LOG(ERROR) << "Lost connection to ContextReader."; });
  }

  using Handler = std::function<void(fidl::String, fidl::String)>;

  void Handle(const Handler& handler) { handler_ = handler; }

  // Deregisters itself from the watched story provider.
  void Reset() { binding_.Close(); }

 private:
  // |ContextListenerForTopics|
  void OnUpdate(maxwell::ContextUpdateForTopicsPtr update) override {
    FTL_VLOG(4) << "ContextListenerForTopicsImpl::OnUpdate()";
    const auto& values = update->values;
    for (auto i = values.cbegin(); i != values.cend(); ++i) {
      FTL_VLOG(4) << "ContextListenerForTopicsImpl::OnUpdate() " << i.GetKey()
                  << " " << i.GetValue();
      handler_(i.GetKey(), i.GetValue());
    }
  }

  fidl::Binding<maxwell::ContextListenerForTopics> binding_;
  Handler handler_;
  FTL_DISALLOW_COPY_AND_ASSIGN(ContextListenerForTopicsImpl);
};

// Tests the story importance machinery. We set context to home, start one
// story, then set context to work, start another story. The we compute story
// importance and verify that the importance of the first story is lower than
// the importance of the second story.
class TestApp : modular::testing::ComponentViewBase<modular::UserShell> {
 public:
  // The app instance must be dynamic, because it needs to do several things
  // after its own constructor is invoked. It accomplishes that by being able to
  // call delete this. Cf. Terminate().
  static void New() {
    new TestApp;  // will delete itself in Terminate().
  }

 private:
  TestApp() { TestInit(__FILE__); }

  ~TestApp() override = default;

  using TestPoint = modular::testing::TestPoint;

  TestPoint create_view_{"CreateView()"};

  // |SingleServiceViewApp|
  void CreateView(
      fidl::InterfaceRequest<mozart::ViewOwner> /*view_owner_request*/,
      fidl::InterfaceRequest<app::ServiceProvider> /*services*/) override {
    create_view_.Pass();
  }

  TestPoint initialize_{"Initialize()"};

  // |UserShell|
  void Initialize(fidl::InterfaceHandle<modular::UserShellContext>
                      user_shell_context) override {
    initialize_.Pass();

    user_shell_context_.Bind(std::move(user_shell_context));

    user_shell_context_->GetStoryProvider(story_provider_.NewRequest());
    story_provider_watcher_.Watch(&story_provider_);

    user_shell_context_->GetFocusController(focus_controller_.NewRequest());
    user_shell_context_->GetFocusProvider(focus_provider_.NewRequest());
    focus_watcher_.Watch(focus_provider_.get());

    user_shell_context_->GetContextPublisher(context_publisher_.NewRequest());
    user_shell_context_->GetContextReader(context_reader_.NewRequest());
    context_listener_.Listen(context_reader_.get());

    SetContextHome();
  }

  TestPoint set_context_home_{"SetContextHome()"};

  void SetContextHome() {
    context_listener_.Handle(
        [this](const fidl::String& key, const fidl::String& value) {
          GetContextHome(key, value);
        });
    context_publisher_->Publish(kTopic, "\"home\"");
    set_context_home_.Pass();
  }

  TestPoint get_context_home_{"GetContextHome()"};

  void GetContextHome(const fidl::String& topic, const fidl::String& value) {
    FTL_VLOG(4) << "Context " << topic << " " << value;
    if (topic == kTopic && value == "\"home\"" && !story1_context_) {
      story1_context_ = true;
      get_context_home_.Pass();
      CreateStory1();
    }
  }

  TestPoint create_story1_{"CreateStory1()"};

  void CreateStory1() {
    story_provider_->CreateStory(kModuleUrl,
                                 [this](const fidl::String& story_id) {
                                   story1_id_ = story_id;
                                   create_story1_.Pass();
                                   StartStory1();
                                 });
  }

  TestPoint start_story1_{"StartStory1()"};

  void StartStory1() {
    story_provider_->GetController(story1_id_, story1_controller_.NewRequest());

    story1_watcher_.Watch(story1_controller_.get());
    story1_watcher_.Continue([this] {
      start_story1_.Pass();
      SetContextWork();
    });

    // Start and show the new story.
    fidl::InterfaceHandle<mozart::ViewOwner> story_view;
    story1_controller_->Start(story_view.NewRequest());
  }

  TestPoint set_context_work_{"SetContextWork()"};

  void SetContextWork() {
    context_listener_.Handle(
        [this](const fidl::String& key, const fidl::String& value) {
          GetContextWork(key, value);
        });
    context_publisher_->Publish(kTopic, "\"work\"");
    set_context_work_.Pass();
  }

  TestPoint get_context_work_{"GetContextWork()"};

  void GetContextWork(const fidl::String& topic, const fidl::String& value) {
    if (topic == kTopic && value == "\"work\"" && !story2_context_) {
      story2_context_ = true;
      get_context_work_.Pass();
      CreateStory2();
    }
  }

  TestPoint create_story2_{"CreateStory2()"};

  void CreateStory2() {
    story_provider_->CreateStory(kModuleUrl,
                                 [this](const fidl::String& story_id) {
                                   story2_id_ = story_id;
                                   create_story2_.Pass();
                                   StartStory2();
                                 });
  }

  TestPoint start_story2_{"StartStory2()"};

  void StartStory2() {
    story_provider_->GetController(story2_id_, story2_controller_.NewRequest());

    story2_watcher_.Watch(story2_controller_.get());
    story2_watcher_.Continue([this] {
      start_story2_.Pass();
      GetImportance1();
    });

    // Start and show the new story.
    fidl::InterfaceHandle<mozart::ViewOwner> story_view;
    story2_controller_->Start(story_view.NewRequest());
  }

  TestPoint get_importance1_{"GetImportance1()"};

  void GetImportance1() {
    story_provider_->GetImportance(
        [this](fidl::Map<fidl::String, float> importance) {
          get_importance1_.Pass();

          if (importance.find(story1_id_) == importance.end()) {
            modular::testing::Fail("No importance for story1");
          } else {
            FTL_VLOG(4) << "Story1 importance " << importance[story1_id_];
          }

          if (importance.find(story2_id_) == importance.end()) {
            modular::testing::Fail("No importance for story2");
          } else {
            FTL_VLOG(4) << "Story2 importance " << importance[story2_id_];
          }

          if (importance[story1_id_] > 0.1f) {
            modular::testing::Fail("Wrong importance for story1 " +
                                   std::to_string(importance[story1_id_]));
          };

          if (importance[story2_id_] < 0.9f) {
            modular::testing::Fail("Wrong importance for story2 " +
                                   std::to_string(importance[story2_id_]));
          };

          Focus();
        });
  }

  void Focus() {
    focus_watcher_.Continue([this] { Focused(); });

    focus_controller_->Set(story1_id_);
  }

  TestPoint focused_{"Focused()"};

  void Focused() {
    focused_.Pass();
    GetImportance2();
  }

  TestPoint get_importance2_{"GetImportance2()"};

  void GetImportance2() {
    story_provider_->GetImportance(
        [this](fidl::Map<fidl::String, float> importance) {
          get_importance2_.Pass();

          if (importance.find(story1_id_) == importance.end()) {
            modular::testing::Fail("No importance for story1");
          } else {
            FTL_VLOG(4) << "Story1 importance " << importance[story1_id_];
          }

          if (importance[story1_id_] < 0.4f) {
            modular::testing::Fail("Wrong importance for story1 " +
                                   std::to_string(importance[story1_id_]));
          };

          Logout();
        });
  }

  void Logout() { user_shell_context_->Logout(); }

  TestPoint terminate_{"Terminate()"};

  // |UserShell|
  void Terminate() override {
    terminate_.Pass();
    DeleteAndQuit();
  }

  StoryProviderWatcherImpl story_provider_watcher_;

  modular::UserShellContextPtr user_shell_context_;
  modular::StoryProviderPtr story_provider_;

  modular::FocusControllerPtr focus_controller_;
  modular::FocusProviderPtr focus_provider_;
  FocusWatcherImpl focus_watcher_;

  bool story1_context_{};
  fidl::String story1_id_;
  modular::StoryControllerPtr story1_controller_;
  StoryWatcherImpl story1_watcher_;

  bool story2_context_{};
  fidl::String story2_id_;
  modular::StoryControllerPtr story2_controller_;
  StoryWatcherImpl story2_watcher_;

  maxwell::ContextPublisherPtr context_publisher_;
  maxwell::ContextReaderPtr context_reader_;
  ContextListenerForTopicsImpl context_listener_;

  FTL_DISALLOW_COPY_AND_ASSIGN(TestApp);
};

}  // namespace

int main(int argc, const char** argv) {
  auto command_line = ftl::CommandLineFromArgcArgv(argc, argv);

  mtl::MessageLoop loop;
  TestApp::New();
  loop.Run();
  return 0;
}
