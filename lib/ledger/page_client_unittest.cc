// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/modular/lib/fidl/array_to_string.h"
#include "apps/modular/lib/ledger/ledger_client.h"
#include "apps/modular/lib/ledger/page_client.h"
#include "apps/modular/lib/testing/ledger_repository_for_testing.h"
#include "apps/modular/lib/testing/test_with_message_loop.h"
#include "gtest/gtest.h"
#include "lib/ftl/macros.h"

namespace modular {
namespace testing {
namespace {

class PageClientImpl : PageClient {
 public:
  PageClientImpl(LedgerClient* ledger_client, LedgerPageId page_id,
                 const char* const prefix)
      : PageClient("PageClientImpl", ledger_client, std::move(page_id), prefix) {}

  ~PageClientImpl() override = default;

  void OnPageChange(const std::string& key, const std::string& value) override {
    ++change_count_;

    values_[key] = value;

    FTL_LOG(INFO) << "OnPageChange " << prefix()
                  << " " << change_count_
                  << " " << key << " " << value;
  }

  void OnPageConflict(Conflict* const conflict) override {
    ++conflict_count_;

    FTL_LOG(INFO) << "OnPageConflict " << prefix()
                  << " " << conflict_count_
                  << " " << conflict->key
                  << " " << conflict->left
                  << " " << conflict->right;

    conflict->resolution = MERGE;
    conflict->merged = "value3";
  }

  int change_count() const { return change_count_; }
  int conflict_count() const { return conflict_count_; }

  const std::string& value(const std::string& key) { return values_[key]; }
  ledger::Page* page() { return PageClient::page(); }

 private:
  std::map<std::string, std::string> values_;
  int change_count_{};
  int conflict_count_{};
};

class PageClientTest : public TestWithMessageLoop {
 public:
  PageClientTest() : page_id_(to_array("0123456789123456")) {}

  ~PageClientTest() = default;

  void SetUp() override {
    ledger_repository_app_ =
        std::make_unique<LedgerRepositoryForTesting>("page_client_unittest");

    ledger_client_.reset(new LedgerClient(ledger_repository_app_->ledger_repository(),
                                          __FILE__, [] {
                                            ASSERT_TRUE(false);
                                          }));

    // TODO(mesch): Registration order matters for overlapping prefixes. This
    // should not be like that.
    page_client_a_.reset(new PageClientImpl(ledger_client_.get(), page_id_.Clone(), "a/"));
    page_client_b_.reset(new PageClientImpl(ledger_client_.get(), page_id_.Clone(), "b/"));
    page_client_.reset(new PageClientImpl(ledger_client_.get(), page_id_.Clone(), nullptr));

    ledger_client_->ledger()->GetPage(
        page_id_.Clone(), page_.NewRequest(),
        [](ledger::Status status) { ASSERT_EQ(ledger::Status::OK, status); });
  }

  void TearDown() override {
    page_client_.reset();
    page_client_a_.reset();
    page_client_b_.reset();
    ledger_client_.reset();

    bool repo_deleted = false;
    ledger_repository_app_->Reset([&repo_deleted] { repo_deleted = true; });
    if (!repo_deleted) {
      RunLoopUntil([&repo_deleted] { return repo_deleted; });
    }

    bool terminated = false;
    ledger_repository_app_->Terminate([&terminated] { terminated = true; });
    if (!terminated) {
      RunLoopUntil([&terminated] { return terminated; });
    }

    ledger_repository_app_.reset();
  }

  PageClientImpl* page_client() { return page_client_.get(); }
  ledger::Page* page() { return page_.get(); }

  PageClientImpl* page_client_a() { return page_client_a_.get(); }
  PageClientImpl* page_client_b() { return page_client_b_.get(); }

  // Shorthand for the two connections to the page, and the two clients on the
  // first connection.
  ledger::Page* page1() { return page_client_->page(); }
  ledger::Page* page1a() { return page_client_a_->page(); }
  ledger::Page* page1b() { return page_client_b_->page(); }
  ledger::Page* page2() { return page_.get(); }

  // Factory for a ledger callback function that just logs errors.
  std::function<void(ledger::Status)> log(std::string context) {
    return [context](ledger::Status status) {
      EXPECT_EQ(ledger::Status::OK, status) << context;
    };
  }

 private:
  const LedgerPageId page_id_;

  std::unique_ptr<LedgerRepositoryForTesting> ledger_repository_app_;

  std::unique_ptr<LedgerClient> ledger_client_;

  // Separate page clients for different prefixes all for the same page. They
  // share the same page connection since they are created on the same ledger
  // client.
  std::unique_ptr<PageClientImpl> page_client_;
  std::unique_ptr<PageClientImpl> page_client_a_;
  std::unique_ptr<PageClientImpl> page_client_b_;

  // A separate connection to the same page as page_client_, used to create
  // conflicts.
  ledger::PagePtr page_;

  FTL_DISALLOW_COPY_AND_ASSIGN(PageClientTest);
};

TEST_F(PageClientTest, SimpleWrite) {
  page1()->Put(to_array("key"), to_array("value"), log("Put"));

  RunLoopUntil([this] {
      return page_client()->change_count() >= 1;
    });

  EXPECT_EQ(0, page_client()->conflict_count());
  EXPECT_EQ("value", page_client()->value("key"));
}

TEST_F(PageClientTest, PrefixWrite) {
  page2()->Put(to_array("a/key"), to_array("value"), log("Put"));
  page2()->Put(to_array("b/key"), to_array("value"), log("Put"));

  RunLoopUntil([this] {
      return page_client()->change_count() >= 2;
    });

  EXPECT_EQ(0, page_client()->conflict_count());
  EXPECT_EQ(0, page_client_a()->conflict_count());
  EXPECT_EQ(0, page_client_b()->conflict_count());
  EXPECT_EQ("value", page_client_a()->value("a/key"));
  EXPECT_EQ("value", page_client_b()->value("b/key"));
}

TEST_F(PageClientTest, ConcurrentWrite) {
  page1()->Put(to_array("key1"), to_array("value1"), log("Put key1"));
  page2()->Put(to_array("key2"), to_array("value2"), log("Put key2"));

  RunLoopUntil([this] {
      return page_client()->change_count() >= 2;
    });

  EXPECT_EQ(0, page_client()->conflict_count());
  EXPECT_EQ("value1", page_client()->value("key1"));
  EXPECT_EQ("value2", page_client()->value("key2"));
}


TEST_F(PageClientTest, ConflictWrite) {
  page1()->StartTransaction([this](ledger::Status status) {
      EXPECT_EQ(ledger::Status::OK, status);
      page1()->Put(to_array("key"), to_array("value1"), [this](ledger::Status status) {
          EXPECT_EQ(ledger::Status::OK, status);
          page2()->StartTransaction([this](ledger::Status status) {
              EXPECT_EQ(ledger::Status::OK, status);
              page2()->Put(to_array("key"), to_array("value2"), [this](ledger::Status status) {
                  EXPECT_EQ(ledger::Status::OK, status);
                  page2()->Commit([this](ledger::Status status) {
                      EXPECT_EQ(ledger::Status::OK, status);
                      page1()->Commit([this](ledger::Status status) {
                          EXPECT_EQ(ledger::Status::OK, status);
                        });
                    });
                });
            });
        });
    });

  RunLoopUntil([this] {
      return page_client()->change_count() >= 2;
    });

  EXPECT_EQ(1, page_client()->conflict_count());
  EXPECT_EQ("value3", page_client()->value("key"));
}

TEST_F(PageClientTest, ConflictPrefixWrite) {
  page1()->StartTransaction([this](ledger::Status status) {
      EXPECT_EQ(ledger::Status::OK, status);
      page1()->Put(to_array("a/key"), to_array("value1"), [this](ledger::Status status) {
          EXPECT_EQ(ledger::Status::OK, status);
          page2()->StartTransaction([this](ledger::Status status) {
              EXPECT_EQ(ledger::Status::OK, status);
              page2()->Put(to_array("a/key"), to_array("value2"), [this](ledger::Status status) {
                  EXPECT_EQ(ledger::Status::OK, status);
                  page2()->Commit([this](ledger::Status status) {
                      EXPECT_EQ(ledger::Status::OK, status);
                      page1()->Commit([this](ledger::Status status) {
                          EXPECT_EQ(ledger::Status::OK, status);
                        });
                    });
                });
            });
        });
    });

  RunLoopUntil([this] {
      return page_client_a()->change_count() >= 2;
    });

  EXPECT_EQ(1, page_client_a()->conflict_count());
  EXPECT_EQ(0, page_client_b()->conflict_count());
  EXPECT_EQ("value3", page_client_a()->value("a/key"));
}

TEST_F(PageClientTest, ConcurrentConflictWrite) {
  page1()->StartTransaction([this](ledger::Status status) {
      EXPECT_EQ(ledger::Status::OK, status);
      page1()->Put(to_array("key1"), to_array("value1"), log("Put 1 key1"));
      page1()->Put(to_array("key"), to_array("value1"), [this](ledger::Status status) {
          EXPECT_EQ(ledger::Status::OK, status);
          page2()->StartTransaction([this](ledger::Status status) {
              EXPECT_EQ(ledger::Status::OK, status);
              page2()->Put(to_array("key2"), to_array("value2"), log("Put 2 key2"));
              page2()->Put(to_array("key"), to_array("value2"), [this](ledger::Status status) {
                  EXPECT_EQ(ledger::Status::OK, status);
                  page2()->Commit([this](ledger::Status status) {
                      EXPECT_EQ(ledger::Status::OK, status);
                      page1()->Commit([this](ledger::Status status) {
                          EXPECT_EQ(ledger::Status::OK, status);
                        });
                    });
                });
            });
        });
    });

  RunLoopUntil([this] {
      return page_client()->change_count() >= 4;
    });

  EXPECT_EQ(1, page_client()->conflict_count());
  EXPECT_EQ("value1", page_client()->value("key1"));
  EXPECT_EQ("value2", page_client()->value("key2"));
  EXPECT_EQ("value3", page_client()->value("key"));
}

}  // namespace
}  // namespace testing
}  // namespace modular
