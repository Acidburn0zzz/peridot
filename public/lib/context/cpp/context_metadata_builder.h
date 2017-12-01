// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIB_CONTEXT_CPP_CONTEXT_METADATA_BUILDER_H_
#define LIB_CONTEXT_CPP_CONTEXT_METADATA_BUILDER_H_

#include "lib/context/fidl/metadata.fidl.h"

namespace maxwell {

class ContextMetadataBuilder {
 public:
  ContextMetadataBuilder();
  explicit ContextMetadataBuilder(ContextMetadataPtr initial_value);

  ContextMetadataBuilder& SetStoryId(const fidl::String& story_id);
  ContextMetadataBuilder& SetStoryFocused(bool focused);

  ContextMetadataBuilder& SetModuleUrl(const fidl::String& url);
  ContextMetadataBuilder& SetModulePath(const fidl::Array<fidl::String>& path);

  ContextMetadataBuilder& SetEntityTopic(const fidl::String& topic);
  ContextMetadataBuilder& AddEntityType(const fidl::String& type);
  ContextMetadataBuilder& SetEntityTypes(
      const fidl::Array<fidl::String>& types);

  ContextMetadataBuilder& SetLinkPath(
      const fidl::Array<fidl::String>& module_path,
      const fidl::String& name);

  // Build() can be called only once, as it moves |m_|.
  ContextMetadataPtr Build();

 private:
  StoryMetadataPtr& StoryMetadata();
  ModuleMetadataPtr& ModuleMetadata();
  EntityMetadataPtr& EntityMetadata();
  LinkMetadataPtr& LinkMetadata();

  ContextMetadataPtr m_;
};

}  // namespace maxwell

#endif  // LIB_CONTEXT_CPP_CONTEXT_METADATA_BUILDER_H_
