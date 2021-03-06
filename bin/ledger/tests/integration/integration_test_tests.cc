// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "lib/fidl/cpp/bindings/binding.h"
#include "lib/fsl/tasks/message_loop.h"
#include "lib/fxl/functional/make_copyable.h"
#include "lib/fxl/macros.h"
#include "lib/fxl/strings/string_printf.h"
#include "lib/ledger/fidl/ledger.fidl.h"
#include "peridot/bin/ledger/tests/integration/integration_test.h"
#include "peridot/bin/ledger/tests/integration/test_utils.h"
#include "peridot/lib/convert/convert.h"

namespace test {
namespace integration {
namespace {

class IntegrationTestTests : public IntegrationTest {};

TEST_F(IntegrationTestTests, MultipleLedgerAppInstances) {
  auto instance1 = NewLedgerAppInstance();
  auto instance2 = NewLedgerAppInstance();

  EXPECT_TRUE(instance1->GetTestLedger());
  EXPECT_TRUE(instance2->GetTestLedger());
}

}  // namespace
}  // namespace integration
}  // namespace test
