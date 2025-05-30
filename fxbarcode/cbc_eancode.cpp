// Copyright 2018 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxbarcode/cbc_eancode.h"

#include <utility>

#include "fxbarcode/BC_Library.h"
#include "fxbarcode/oned/BC_OnedEANWriter.h"

CBC_EANCode::CBC_EANCode(std::unique_ptr<CBC_OneDimEANWriter> pWriter)
    : CBC_OneCode(std::move(pWriter)) {}

CBC_EANCode::~CBC_EANCode() = default;

CBC_OneDimEANWriter* CBC_EANCode::GetOneDimEANWriter() {
  return static_cast<CBC_OneDimEANWriter*>(bc_writer_.get());
}

bool CBC_EANCode::Encode(WideStringView contents) {
  auto* pWriter = GetOneDimEANWriter();
  if (!pWriter->CheckContentValidity(contents)) {
    return false;
  }

  render_contents_ = Preprocess(contents);
  ByteString str = render_contents_.ToUTF8();
  pWriter->InitEANWriter();
  return pWriter->RenderResult(render_contents_.AsStringView(),
                               pWriter->Encode(str));
}

bool CBC_EANCode::RenderDevice(CFX_RenderDevice* device,
                               const CFX_Matrix& matrix) {
  return GetOneDimEANWriter()->RenderDeviceResult(
      device, matrix, render_contents_.AsStringView());
}

WideString CBC_EANCode::Preprocess(WideStringView contents) {
  auto* pWriter = GetOneDimEANWriter();
  WideString encoded_contents = pWriter->FilterContents(contents);
  size_t length = encoded_contents.GetLength();
  size_t max_length = GetMaxLength();
  if (length <= max_length) {
    for (size_t i = 0; i < max_length - length; i++) {
      encoded_contents.InsertAtFront(L'0');
    }

    ByteString str = encoded_contents.ToUTF8();
    int32_t checksum = pWriter->CalcChecksum(str);
    str += '0' + checksum;
    encoded_contents = WideString::FromUTF8(str.AsStringView());
  } else {
    encoded_contents = encoded_contents.First(max_length + 1);
  }

  return encoded_contents;
}
