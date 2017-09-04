// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_LEDGER_SRC_GLUE_CRYPTO_HMAC_H_
#define APPS_LEDGER_SRC_GLUE_CRYPTO_HMAC_H_

#include "lib/ftl/strings/string_view.h"

namespace glue {
// Compute the HMAC defined by RFC 2104 using SHA-256 for the hash algorithm.
// |key| must be at least 256 bits long.
std::string SHA256HMAC(ftl::StringView key, ftl::StringView data);
}  // namespace glue

#endif  // APPS_LEDGER_SRC_GLUE_CRYPTO_HMAC_H_
