// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_LEDGER_SRC_GLUE_CRYPTO_KDF_H_
#define APPS_LEDGER_SRC_GLUE_CRYPTO_KDF_H_

#include <string>

#include "lib/ftl/strings/string_view.h"

namespace glue {
// Compute the key derivation function defined by RFC 5869 using HMAC-256 and
// the given |length|. The usual salt and info are omitted due to the fact that
// our scheme always passes unique data to the KDF.
std::string HMAC256KDF(ftl::StringView data, size_t length);
}  // namespace glue

#endif  // APPS_LEDGER_SRC_GLUE_CRYPTO_KDF_H_
