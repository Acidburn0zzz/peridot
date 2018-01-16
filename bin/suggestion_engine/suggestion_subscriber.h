// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERIDOT_BIN_SUGGESTION_ENGINE_SUGGESTION_SUBSCRIBER_H_
#define PERIDOT_BIN_SUGGESTION_ENGINE_SUGGESTION_SUBSCRIBER_H_

#include "lib/fidl/cpp/bindings/binding.h"
#include "lib/fxl/functional/closure.h"
#include "lib/suggestion/fidl/suggestion_provider.fidl.h"
#include "peridot/bin/suggestion_engine/ranked_suggestion.h"

namespace maxwell {

class SuggestionSubscriber {
 public:
  SuggestionSubscriber(fidl::InterfaceHandle<SuggestionListener> listener);
  virtual ~SuggestionSubscriber();

  // Send the current initial set of suggestions
  virtual void OnSubscribe() = 0;

  // TODO(jwnichols): Why did we change the terminology here?  Seems like it
  // should be OnRemoveAllSuggestions().
  virtual void Invalidate() = 0;

  virtual void OnProcessingChange(bool processing) = 0;

  // FIDL methods, for use with BoundSet without having to expose listener_.

  bool is_bound() { return listener_.is_bound(); }

  void set_connection_error_handler(const fxl::Closure& error_handler) {
    listener_.set_connection_error_handler(error_handler);
  }

  // End FIDL methods.

 protected:
  SuggestionListener* listener() const { return listener_.get(); }

 private:
  SuggestionListenerPtr listener_;
};

}  // namespace maxwell

#endif  // PERIDOT_BIN_SUGGESTION_ENGINE_SUGGESTION_SUBSCRIBER_H_
