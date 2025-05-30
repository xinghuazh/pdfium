// Copyright 2018 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "fxjs/cfx_globaldata.h"

#include <stdint.h>

#include <utility>
#include <vector>

#include "core/fxcrt/data_vector.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class TestDelegate : public CFX_GlobalData::Delegate {
 public:
  TestDelegate() = default;
  ~TestDelegate() override = default;

  bool StoreBuffer(pdfium::span<const uint8_t> buffer) override {
    last_buffer_ = DataVector<uint8_t>(buffer.begin(), buffer.end());
    return true;
  }
  std::optional<pdfium::span<uint8_t>> LoadBuffer() override {
    return pdfium::span<uint8_t>(last_buffer_);
  }
  void BufferDone() override {
    // Catch misuse after done.
    last_buffer_ = DataVector<uint8_t>();
  }

  DataVector<uint8_t> last_buffer_;
};

}  // namespace

TEST(CFXGlobalData, GetSafety) {
  CFX_GlobalData* pInstance = CFX_GlobalData::GetRetainedInstance(nullptr);
  EXPECT_FALSE(pInstance->GetGlobalVariable("nonesuch"));
  EXPECT_FALSE(pInstance->GetAt(-1));
  EXPECT_FALSE(pInstance->GetAt(0));
  EXPECT_FALSE(pInstance->GetAt(1));

  pInstance->SetGlobalVariableNumber("double", 2.0);
  pInstance->SetGlobalVariableString("string", "clams");

  EXPECT_FALSE(pInstance->GetGlobalVariable("nonesuch"));
  EXPECT_FALSE(pInstance->GetAt(-1));
  EXPECT_EQ(pInstance->GetGlobalVariable("double"), pInstance->GetAt(0));
  EXPECT_EQ(pInstance->GetGlobalVariable("string"), pInstance->GetAt(1));
  EXPECT_FALSE(pInstance->GetAt(2));

  ASSERT_TRUE(pInstance->Release());
}

TEST(CFXGlobalData, StoreReload) {
  TestDelegate delegate;
  std::vector<std::unique_ptr<CFX_KeyValue>> array;
  CFX_GlobalData* pInstance = CFX_GlobalData::GetRetainedInstance(&delegate);
  pInstance->SetGlobalVariableNumber("double", 2.0);
  pInstance->SetGlobalVariableString("string", "clams");
  pInstance->SetGlobalVariableBoolean("boolean", true);
  pInstance->SetGlobalVariableNull("null");
  pInstance->SetGlobalVariableObject("array", std::move(array));
  EXPECT_TRUE(pInstance->SetGlobalVariablePersistent("double", true));
  EXPECT_TRUE(pInstance->SetGlobalVariablePersistent("string", true));
  EXPECT_TRUE(pInstance->SetGlobalVariablePersistent("boolean", true));
  EXPECT_TRUE(pInstance->SetGlobalVariablePersistent("null", true));
  EXPECT_TRUE(pInstance->SetGlobalVariablePersistent("array", true));
  ASSERT_TRUE(pInstance->Release());

  pInstance = CFX_GlobalData::GetRetainedInstance(&delegate);
  auto* element = pInstance->GetAt(0);
  ASSERT_TRUE(element);
  EXPECT_EQ("double", element->data.sKey);
  EXPECT_EQ(CFX_Value::DataType::kNumber, element->data.nType);
  EXPECT_EQ(2.0, element->data.dData);

  element = pInstance->GetAt(1);
  ASSERT_TRUE(element);
  EXPECT_EQ("string", element->data.sKey);
  EXPECT_EQ(CFX_Value::DataType::kString, element->data.nType);
  EXPECT_EQ("clams", element->data.sData);

  element = pInstance->GetAt(2);
  ASSERT_TRUE(element);
  EXPECT_EQ("boolean", element->data.sKey);
  EXPECT_EQ(CFX_Value::DataType::kBoolean, element->data.nType);
  EXPECT_EQ(true, element->data.bData);

  element = pInstance->GetAt(3);
  ASSERT_TRUE(element);
  EXPECT_EQ("null", element->data.sKey);
  EXPECT_EQ(CFX_Value::DataType::kNull, element->data.nType);

  // Arrays don't get persisted.
  element = pInstance->GetAt(4);
  ASSERT_FALSE(element);

  ASSERT_TRUE(pInstance->Release());
}

TEST(CFXGlobalData, ResetValues) {
  CFX_GlobalData* pInstance = CFX_GlobalData::GetRetainedInstance(nullptr);
  pInstance->SetGlobalVariableString("double", "bogus!!!");
  pInstance->SetGlobalVariableString("string", "bogus!!!");
  pInstance->SetGlobalVariableString("boolean", "bogus!!!");
  pInstance->SetGlobalVariableString("null", "bogus!!!");

  pInstance->SetGlobalVariableNumber("double", 2.0);
  pInstance->SetGlobalVariableString("string", "clams");
  pInstance->SetGlobalVariableBoolean("boolean", true);
  pInstance->SetGlobalVariableNull("null");

  auto* element = pInstance->GetAt(0);
  ASSERT_TRUE(element);
  EXPECT_EQ("double", element->data.sKey);
  EXPECT_EQ(CFX_Value::DataType::kNumber, element->data.nType);
  EXPECT_EQ(2.0, element->data.dData);

  element = pInstance->GetAt(1);
  ASSERT_TRUE(element);
  EXPECT_EQ("string", element->data.sKey);
  EXPECT_EQ(CFX_Value::DataType::kString, element->data.nType);
  EXPECT_EQ("clams", element->data.sData);

  element = pInstance->GetAt(2);
  ASSERT_TRUE(element);
  EXPECT_EQ("boolean", element->data.sKey);
  EXPECT_EQ(CFX_Value::DataType::kBoolean, element->data.nType);
  EXPECT_EQ(true, element->data.bData);

  element = pInstance->GetAt(3);
  ASSERT_TRUE(element);
  EXPECT_EQ("null", element->data.sKey);
  EXPECT_EQ(CFX_Value::DataType::kNull, element->data.nType);

  ASSERT_TRUE(pInstance->Release());
}

TEST(CFXGlobalData, DeleteValues) {
  CFX_GlobalData* pInstance = CFX_GlobalData::GetRetainedInstance(nullptr);
  pInstance->SetGlobalVariableNumber("double", 2.0);
  pInstance->SetGlobalVariableString("string", "clams");
  pInstance->SetGlobalVariableBoolean("boolean", true);
  pInstance->SetGlobalVariableNull("null");
  EXPECT_EQ(4, pInstance->GetSize());

  EXPECT_FALSE(pInstance->DeleteGlobalVariable("nonesuch"));
  EXPECT_EQ(4, pInstance->GetSize());

  EXPECT_TRUE(pInstance->DeleteGlobalVariable("boolean"));
  EXPECT_EQ(3, pInstance->GetSize());

  EXPECT_TRUE(pInstance->DeleteGlobalVariable("string"));
  EXPECT_EQ(2, pInstance->GetSize());

  EXPECT_TRUE(pInstance->DeleteGlobalVariable("double"));
  EXPECT_EQ(1, pInstance->GetSize());

  EXPECT_TRUE(pInstance->DeleteGlobalVariable("null"));
  EXPECT_EQ(0, pInstance->GetSize());

  ASSERT_TRUE(pInstance->Release());
}
