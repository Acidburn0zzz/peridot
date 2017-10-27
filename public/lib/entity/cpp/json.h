// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "rapidjson/document.h"

namespace modular {

// Returns a JSON string that can be used with EntityReferenceFromJson().
std::string EntityReferenceToJson(const std::string& ref);
// Like above but returns a rapidjson::Document.
rapidjson::Document EntityReferenceToJsonDoc(const std::string& ref);

// Returns false if |json| does not represent an Entity reference.
bool EntityReferenceFromJson(const std::string& json, std::string* ref);
// Like above but operates on a rapidjson::Value.
bool EntityReferenceFromJson(const rapidjson::Value& value, std::string* ref);

}  // namespace modular