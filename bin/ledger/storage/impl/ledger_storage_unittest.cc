// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "peridot/bin/ledger/storage/impl/ledger_storage_impl.h"

#include <memory>

#include "gtest/gtest.h"
#include "lib/fsl/tasks/message_loop.h"
#include "lib/fxl/files/scoped_temp_dir.h"
#include "lib/fxl/macros.h"
#include "peridot/bin/ledger/coroutine/coroutine_impl.h"
#include "peridot/bin/ledger/encryption/fake/fake_encryption_service.h"
#include "peridot/lib/callback/capture.h"
#include "peridot/lib/callback/set_when_called.h"
#include "peridot/lib/gtest/test_with_message_loop.h"

namespace storage {
namespace {

class LedgerStorageTest : public gtest::TestWithMessageLoop {
 public:
  LedgerStorageTest()
      : encryption_service_(message_loop_.task_runner()),
        storage_(message_loop_.task_runner(),
                 &coroutine_service_,
                 &encryption_service_,
                 tmp_dir_.path(),
                 "test_app") {}

  ~LedgerStorageTest() override {}

 private:
  files::ScopedTempDir tmp_dir_;
  coroutine::CoroutineServiceImpl coroutine_service_;
  encryption::FakeEncryptionService encryption_service_;

 protected:
  LedgerStorageImpl storage_;

  FXL_DISALLOW_COPY_AND_ASSIGN(LedgerStorageTest);
};

TEST_F(LedgerStorageTest, CreateGetCreatePageStorage) {
  PageId page_id = "1234";
  Status status;
  std::unique_ptr<PageStorage> page_storage;
  bool called;
  storage_.GetPageStorage(
      page_id, callback::Capture(callback::SetWhenCalled(&called), &status,
                                 &page_storage));
  RunLoopUntilIdle();
  ASSERT_TRUE(called);
  EXPECT_EQ(Status::NOT_FOUND, status);
  EXPECT_EQ(nullptr, page_storage);

  storage_.CreatePageStorage(
      page_id, callback::Capture(callback::SetWhenCalled(&called), &status,
                                 &page_storage));
  RunLoopUntilIdle();
  ASSERT_TRUE(called);
  ASSERT_EQ(Status::OK, status);
  ASSERT_NE(nullptr, page_storage);
  ASSERT_EQ(page_id, page_storage->GetId());

  page_storage.reset();
  storage_.GetPageStorage(
      page_id, callback::Capture(callback::SetWhenCalled(&called), &status,
                                 &page_storage));
  RunLoopUntilIdle();
  ASSERT_TRUE(called);
  EXPECT_EQ(Status::OK, status);
  EXPECT_NE(nullptr, page_storage);
}

TEST_F(LedgerStorageTest, CreateDeletePageStorage) {
  PageId page_id = "1234";
  Status status;
  bool called;
  std::unique_ptr<PageStorage> page_storage;
  storage_.CreatePageStorage(
      page_id, callback::Capture(callback::SetWhenCalled(&called), &status,
                                 &page_storage));
  RunLoopUntilIdle();
  ASSERT_TRUE(called);
  ASSERT_EQ(Status::OK, status);
  ASSERT_NE(nullptr, page_storage);
  ASSERT_EQ(page_id, page_storage->GetId());
  page_storage.reset();

  storage_.GetPageStorage(
      page_id, callback::Capture(callback::SetWhenCalled(&called), &status,
                                 &page_storage));
  RunLoopUntilIdle();
  ASSERT_TRUE(called);
  EXPECT_EQ(Status::OK, status);
  EXPECT_NE(nullptr, page_storage);

  EXPECT_TRUE(storage_.DeletePageStorage(page_id));
  storage_.GetPageStorage(
      page_id, callback::Capture(callback::SetWhenCalled(&called), &status,
                                 &page_storage));
  RunLoopUntilIdle();
  ASSERT_TRUE(called);
  EXPECT_EQ(Status::NOT_FOUND, status);
  EXPECT_EQ(nullptr, page_storage);
}

}  // namespace
}  // namespace storage
