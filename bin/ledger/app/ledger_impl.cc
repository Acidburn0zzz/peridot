// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "peridot/bin/ledger/app/ledger_impl.h"

#include <memory>
#include <string>
#include <utility>

#include <trace/event.h>

#include "lib/fxl/logging.h"
#include "lib/fxl/random/rand.h"
#include "lib/ledger/fidl/ledger.fidl.h"
#include "peridot/bin/ledger/app/constants.h"
#include "peridot/bin/ledger/app/page_impl.h"
#include "peridot/lib/callback/trace_callback.h"

namespace ledger {

namespace {

void GenerateRandomId(fidl::Array<uint8_t>* id) {
  id->resize(kPageIdSize);
  fxl::RandBytes(id->data(), kPageIdSize);
}

}  // namespace

LedgerImpl::LedgerImpl(Delegate* delegate) : delegate_(delegate) {}

LedgerImpl::~LedgerImpl() {}

// GetRootPage(Page& page) => (Status status);
void LedgerImpl::GetRootPage(fidl::InterfaceRequest<Page> page_request,
                             const GetRootPageCallback& callback) {
  delegate_->GetPage(
      kRootPageId, std::move(page_request),
      TRACE_CALLBACK(callback, "ledger", "ledger_get_root_page"));
}

// GetPage(array<uint8, 16>? id, Page& page) => (Status status);
void LedgerImpl::GetPage(fidl::Array<uint8_t> id,
                         fidl::InterfaceRequest<Page> page_request,
                         const GetPageCallback& callback) {
  if (!id) {
    GenerateRandomId(&id);
  }
  delegate_->GetPage(id, std::move(page_request),
                     TRACE_CALLBACK(callback, "ledger", "ledger_get_page"));
}

// DeletePage(array<uint8> id) => (Status status);
void LedgerImpl::DeletePage(fidl::Array<uint8_t> id,
                            const DeletePageCallback& callback) {
  TRACE_DURATION("ledger", "ledger_delete_page");

  callback(delegate_->DeletePage(id));
}

// SetConflictResolverFactory(ConflictResolverFactory? factory)
//     => (Status status);
void LedgerImpl::SetConflictResolverFactory(
    fidl::InterfaceHandle<ConflictResolverFactory> factory,
    const SetConflictResolverFactoryCallback& callback) {
  TRACE_DURATION("ledger", "ledger_set_conflict_resolver_factory");

  delegate_->SetConflictResolverFactory(std::move(factory));
  callback(Status::OK);
}

}  // namespace ledger
