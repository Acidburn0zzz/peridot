// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERIDOT_BIN_ACQUIRERS_STORY_INFO_STORY_INFO_H_
#define PERIDOT_BIN_ACQUIRERS_STORY_INFO_STORY_INFO_H_

#include <map>

#include "lib/agent/fidl/agent.fidl.h"
#include "lib/app_driver/cpp/agent_driver.h"
#include "lib/context/fidl/context_writer.fidl.h"
#include "lib/fidl/cpp/bindings/binding.h"
#include "lib/fxl/macros.h"
#include "lib/story/fidl/story_controller.fidl.h"
#include "lib/story/fidl/story_provider.fidl.h"
#include "lib/svc/cpp/service_namespace.h"
#include "lib/user/fidl/focus.fidl.h"
#include "peridot/bin/acquirers/story_info/initializer.fidl.h"

namespace maxwell {

class StoryWatcherImpl;

// This class pulls info about Stories from Framework and stores it in
// the Context service.
//
// It maintains a hierarchy of context values to represent:
// Stories -> Modules
//         -> Link Entities
//
// TODO(thatguy): Add Link value types to the Context engine and use them here.
// Then update the resulting published value to remove its added JSON
// structure, since it will all be represented in the metadata of the value.
class StoryInfoAcquirer : public modular::VisibleStoriesWatcher,
                          public modular::StoryProviderWatcher,
                          public modular::FocusWatcher,
                          public StoryInfoInitializer {
 public:
  StoryInfoAcquirer(modular::AgentHost* agent_host);
  ~StoryInfoAcquirer() override;

  // Called by AgentDriver.
  void Connect(fidl::InterfaceRequest<app::ServiceProvider> services);

  // Called by AgentDriver.
  void RunTask(const fidl::String& task_id,
               const modular::Agent::RunTaskCallback& callback);

  // Called by AgentDriver.
  void Terminate(const std::function<void()>& done);

  // Used by StoryWatcherImpl.
  void DropStoryWatcher(const std::string& story_id);

 private:
  // |StoryInfoInitializer|
  void Initialize(fidl::InterfaceHandle<modular::StoryProvider> story_provider,
                  fidl::InterfaceHandle<modular::FocusProvider> focus_provider,
                  fidl::InterfaceHandle<modular::VisibleStoriesProvider>
                      visible_stories_provider) override;

  // |FocusWatcher|
  void OnFocusChange(modular::FocusInfoPtr info) override;

  // |VisibleStoriesWatcher|
  void OnVisibleStoriesChange(fidl::Array<fidl::String> ids) override;

  // |StoryProviderWatcher|
  void OnChange(modular::StoryInfoPtr info, modular::StoryState state) override;
  void OnDelete(const fidl::String& story_id) override;

  ContextWriterPtr context_writer_;
  ContextReaderPtr context_reader_;
  modular::StoryProviderPtr story_provider_;
  modular::FocusProviderPtr focus_provider_;

  fidl::Binding<StoryInfoInitializer> initializer_binding_;
  fidl::Binding<modular::VisibleStoriesWatcher>
      visible_stories_watcher_binding_;
  fidl::Binding<modular::StoryProviderWatcher> story_provider_watcher_binding_;
  fidl::Binding<modular::FocusWatcher> focus_watcher_binding_;

  // Local state.
  // story id -> context value id
  std::map<fidl::String, fidl::String> story_value_ids_;
  fidl::String focused_story_id_;
  std::set<fidl::String> visible_story_ids_;

  // A collection of all active stories we watch. Keys are story IDs, Values are
  // the StoryWatcher instances.
  std::map<std::string, std::unique_ptr<StoryWatcherImpl>> stories_;

  app::ServiceNamespace agent_services_;

  FXL_DISALLOW_COPY_AND_ASSIGN(StoryInfoAcquirer);
};

}  // namespace maxwell

#endif  // PERIDOT_BIN_ACQUIRERS_STORY_INFO_STORY_INFO_H_
