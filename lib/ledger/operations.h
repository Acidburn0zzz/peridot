// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The file defines Operations commonly executed on Ledger pages.

#ifndef APPS_MODULAR_LIB_LEDGER_OPERATIONS_H_
#define APPS_MODULAR_LIB_LEDGER_OPERATIONS_H_

#include <string>

#include "apps/ledger/services/public/ledger.fidl.h"
#include "apps/modular/lib/fidl/array_to_string.h"
#include "apps/modular/lib/fidl/json_xdr.h"
#include "apps/modular/lib/fidl/operation.h"
#include "lib/fidl/cpp/bindings/array.h"
#include "lib/fidl/cpp/bindings/struct_ptr.h"
#include "lib/mtl/vmo/strings.h"

namespace modular {

template <typename Data,
          typename DataPtr = fidl::StructPtr<Data>,
          typename DataFilter = XdrFilterType<Data>>
class ReadDataCall : Operation<DataPtr> {
 public:
  using ResultCall = std::function<void(DataPtr)>;
  using FlowToken = typename Operation<DataPtr>::FlowToken;

  ReadDataCall(OperationContainer* const container,
               ledger::Page* const page,
               const std::string& key,
               const bool not_found_is_ok,
               DataFilter filter,
               ResultCall result_call)
      : Operation<DataPtr>(container, std::move(result_call)),
        page_(page),
        key_(key),
        not_found_is_ok_(not_found_is_ok),
        filter_(filter) {
    this->Ready();
  }

 private:
  std::string GetName() const override { return "ReadDataCall"; }

  void Run() override {
    FlowToken flow{this, &result_};

    page_->GetSnapshot(page_snapshot_.NewRequest(), nullptr, nullptr,
                       [this, flow](ledger::Status status) {
                         if (status != ledger::Status::OK) {
                           FTL_LOG(ERROR) << "ReadDataCall() " << key_
                                          << " Page.GetSnapshot() " << status;
                           return;
                         }

                         Cont(flow);
                       });
  }

  void Cont(FlowToken flow) {
    page_snapshot_->Get(
        to_array(key_), [this, flow](ledger::Status status, mx::vmo value) {
          if (status != ledger::Status::OK) {
            if (status != ledger::Status::KEY_NOT_FOUND || !not_found_is_ok_) {
              FTL_LOG(ERROR) << "ReadDataCall() " << key_
                             << " PageSnapshot.Get() " << status;
            }
            return;
          }

          if (!value) {
            FTL_LOG(ERROR) << "ReadDataCall() " << key_
                           << " PageSnapshot.Get() null vmo";
          }

          std::string value_as_string;
          if (!mtl::StringFromVmo(value, &value_as_string)) {
            FTL_LOG(ERROR) << "ReadDataCall() " << key_
                           << " Unable to extract data.";
            return;
          }

          if (!XdrRead(value_as_string, &result_, filter_)) {
            result_.reset();
            return;
          }

          FTL_DCHECK(!result_.is_null());
        });
  }

  ledger::Page* const page_;  // not owned
  const std::string key_;
  const bool not_found_is_ok_;
  DataFilter const filter_;
  ledger::PageSnapshotPtr page_snapshot_;
  DataPtr result_;

  FTL_DISALLOW_COPY_AND_ASSIGN(ReadDataCall);
};

template <typename Data,
          typename DataPtr = fidl::StructPtr<Data>,
          typename DataArray = fidl::Array<DataPtr>,
          typename DataFilter = XdrFilterType<Data>>
class ReadAllDataCall : Operation<DataArray> {
 public:
  using ResultCall = std::function<void(DataArray)>;
  using FlowToken = typename Operation<DataArray>::FlowToken;

  ReadAllDataCall(OperationContainer* const container,
                  ledger::Page* const page,
                  const char* const prefix,
                  DataFilter const filter,
                  ResultCall result_call)
      : Operation<DataArray>(container, std::move(result_call)),
        page_(page),
        prefix_(prefix),
        filter_(filter) {
    data_.resize(0);
    this->Ready();
  }

 private:
  std::string GetName() const override { return "ReadAllDataCall"; }

  void Run() override {
    FlowToken flow{this, &data_};

    page_->GetSnapshot(page_snapshot_.NewRequest(), to_array(prefix_), nullptr,
                       [this, flow](ledger::Status status) {
                         if (status != ledger::Status::OK) {
                           FTL_LOG(ERROR) << "ReadAllDataCall() "
                                          << "Page.GetSnapshot() " << status;
                           return;
                         }

                         Cont1(flow);
                       });
  }

  void Cont1(FlowToken flow) {
    GetEntries(page_snapshot_.get(), &entries_,
               [this, flow](ledger::Status status) {
                 if (status != ledger::Status::OK) {
                   FTL_LOG(ERROR) << "ReadAllDataCall() "
                                  << "GetEntries() " << status;
                   return;
                 }

                 Cont2(flow);
               });
  }

  void Cont2(FlowToken flow) {
    for (auto& entry : entries_) {
      std::string value_as_string;
      if (!mtl::StringFromVmo(entry->value, &value_as_string)) {
        FTL_LOG(ERROR) << "ReadAllDataCall() "
                       << "Unable to extract data.";
        continue;
      }

      DataPtr data;
      if (!XdrRead(value_as_string, &data, filter_)) {
        continue;
      }

      FTL_DCHECK(!data.is_null());

      data_.push_back(std::move(data));
    }
  }

  ledger::Page* page_;  // not owned
  ledger::PageSnapshotPtr page_snapshot_;
  const char* const prefix_;
  DataFilter const filter_;
  std::vector<ledger::EntryPtr> entries_;
  DataArray data_;

  FTL_DISALLOW_COPY_AND_ASSIGN(ReadAllDataCall);
};

template <typename Data,
          typename DataPtr = fidl::StructPtr<Data>,
          typename DataFilter = XdrFilterType<Data>>
class WriteDataCall : Operation<> {
 public:
  WriteDataCall(OperationContainer* const container,
                ledger::Page* const page,
                const std::string& key,
                DataFilter filter,
                DataPtr data,
                ResultCall result_call)
      : Operation(container, std::move(result_call)),
        page_(page),
        key_(key),
        filter_(filter),
        data_(std::move(data)) {
    Ready();
  }

 private:
  std::string GetName() const override { return "WriteDataCall"; }

  void Run() override {
    FlowToken flow{this};

    std::string json;
    XdrWrite(&json, &data_, filter_);

    page_->Put(to_array(key_), to_array(json),
               [this, flow](ledger::Status status) {
                 if (status != ledger::Status::OK) {
                   FTL_LOG(ERROR) << "WriteDataCall() key =" << key_
                                  << ", Page.Put() " << status;
                 }
               });
  }

  ledger::Page* const page_;  // not owned
  const std::string key_;
  DataFilter const filter_;
  DataPtr data_;

  FTL_DISALLOW_COPY_AND_ASSIGN(WriteDataCall);
};

}  // namespace modular

#endif  // APPS_MODULAR_LIB_LEDGER_OPERATIONS_H_
