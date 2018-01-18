// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERIDOT_BIN_LEDGER_STORAGE_PUBLIC_CONSTANTS_H_
#define PERIDOT_BIN_LEDGER_STORAGE_PUBLIC_CONSTANTS_H_

#include <stdint.h>

#include "lib/fxl/strings/string_view.h"

namespace storage {

// The size of a commit id in number of bytes.
constexpr uint64_t kCommitIdSize = 32;

// The id of the first commit of a page.
constexpr char kFirstPageCommitIdArray[kCommitIdSize] = {0};
constexpr const fxl::StringView kFirstPageCommitId(kFirstPageCommitIdArray,
                                                   kCommitIdSize);
// The default encryption values. Only used until real encryption is
// implemented: LE-286
//
// Use max_int32 for key_index as it will never be used in practice as it is not
// expected that any user will change its key 2^32 times.
constexpr uint32_t kDefaultKeyIndex = std::numeric_limits<uint32_t>::max();
// Use max_int32 - 1 for default deletion scoped id. max_int32 has a special
// meaning in the specification and is used to have per object deletion scope.
constexpr uint32_t kDefaultDeletionScopeId =
    std::numeric_limits<uint32_t>::max() - 1;

// The serialization version of the ledger.
constexpr const fxl::StringView kSerializationVersion = "21";

}  // namespace storage

#endif  // PERIDOT_BIN_LEDGER_STORAGE_PUBLIC_CONSTANTS_H_
