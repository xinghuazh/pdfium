// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/page/cpdf_pathobject.h"

CPDF_PathObject::CPDF_PathObject(int32_t content_stream)
    : CPDF_PageObject(content_stream) {}

CPDF_PathObject::CPDF_PathObject() : CPDF_PathObject(kNoContentStream) {}

CPDF_PathObject::~CPDF_PathObject() = default;

CPDF_PageObject::Type CPDF_PathObject::GetType() const {
  return Type::kPath;
}

void CPDF_PathObject::Transform(const CFX_Matrix& matrix) {
  matrix_.Concat(matrix);
  CalcBoundingBox();
  SetDirty(true);
}

bool CPDF_PathObject::IsPath() const {
  return true;
}

CPDF_PathObject* CPDF_PathObject::AsPath() {
  return this;
}

const CPDF_PathObject* CPDF_PathObject::AsPath() const {
  return this;
}

void CPDF_PathObject::CalcBoundingBox() {
  if (!path_.HasRef()) {
    return;
  }
  CFX_FloatRect rect;
  float width = graph_state().GetLineWidth();
  if (stroke_ && width != 0) {
    rect =
        path_.GetBoundingBoxForStrokePath(width, graph_state().GetMiterLimit());
  } else {
    rect = path_.GetBoundingBox();
  }
  rect = matrix_.TransformRect(rect);

  if (width == 0 && stroke_) {
    rect.Inflate(0.5f, 0.5f);
  }
  SetRect(rect);
}

void CPDF_PathObject::SetPathMatrix(const CFX_Matrix& matrix) {
  matrix_ = matrix;
  CalcBoundingBox();
}
