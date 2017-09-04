// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/ledger/src/glue/crypto/crypto_test_util.h"

#include "lib/ftl/strings/string_number_conversions.h"

namespace glue {

std::string FromHex(ftl::StringView data) {
  std::string result;
  result.reserve(data.size() / 2);
  while (!data.empty()) {
    result.push_back(
        ftl::StringToNumber<uint8_t>(data.substr(0, 2), ftl::Base::k16));
    data = data.substr(2);
  }
  return result;
}

}  // namespace glue
