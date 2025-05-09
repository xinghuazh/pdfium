// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/cxfa_ffline.h"

#include "core/fxcrt/notreached.h"
#include "xfa/fgas/graphics/cfgas_gecolor.h"
#include "xfa/fgas/graphics/cfgas_gegraphics.h"
#include "xfa/fgas/graphics/cfgas_gepath.h"
#include "xfa/fxfa/parser/cxfa_edge.h"
#include "xfa/fxfa/parser/cxfa_line.h"
#include "xfa/fxfa/parser/cxfa_value.h"

namespace {

CFX_GraphStateData::LineCap LineCapToFXGE(XFA_AttributeValue iLineCap) {
  switch (iLineCap) {
    case XFA_AttributeValue::Round:
      return CFX_GraphStateData::LineCap::kRound;
    case XFA_AttributeValue::Butt:
      return CFX_GraphStateData::LineCap::kButt;
    default:
      return CFX_GraphStateData::LineCap::kSquare;
  }
}

}  // namespace

CXFA_FFLine::CXFA_FFLine(CXFA_Node* pNode) : CXFA_FFWidget(pNode) {}

CXFA_FFLine::~CXFA_FFLine() = default;

void CXFA_FFLine::GetRectFromHand(CFX_RectF& rect,
                                  XFA_AttributeValue iHand,
                                  float fLineWidth) {
  float fHalfWidth = fLineWidth / 2.0f;
  if (rect.height < 1.0f) {
    switch (iHand) {
      case XFA_AttributeValue::Left:
        rect.top -= fHalfWidth;
        break;
      case XFA_AttributeValue::Right:
        rect.top += fHalfWidth;
        break;
      case XFA_AttributeValue::Even:
        break;
      default:
        NOTREACHED();
    }
  } else if (rect.width < 1.0f) {
    switch (iHand) {
      case XFA_AttributeValue::Left:
        rect.left += fHalfWidth;
        break;
      case XFA_AttributeValue::Right:
        rect.left += fHalfWidth;
        break;
      case XFA_AttributeValue::Even:
        break;
      default:
        NOTREACHED();
    }
  } else {
    switch (iHand) {
      case XFA_AttributeValue::Left:
        rect.Inflate(fHalfWidth, fHalfWidth);
        break;
      case XFA_AttributeValue::Right:
        rect.Deflate(fHalfWidth, fHalfWidth);
        break;
      case XFA_AttributeValue::Even:
        break;
      default:
        NOTREACHED();
    }
  }
}

void CXFA_FFLine::RenderWidget(CFGAS_GEGraphics* pGS,
                               const CFX_Matrix& matrix,
                               HighlightOption highlight) {
  if (!HasVisibleStatus()) {
    return;
  }

  CXFA_Value* value = node_->GetFormValueIfExists();
  if (!value) {
    return;
  }

  FX_ARGB lineColor = 0xFF000000;
  float fLineWidth = 1.0f;
  XFA_AttributeValue iStrokeType = XFA_AttributeValue::Unknown;
  XFA_AttributeValue iCap = XFA_AttributeValue::Unknown;
  CXFA_Line* line = value->GetLineIfExists();
  if (line) {
    CXFA_Edge* edge = line->GetEdgeIfExists();
    if (edge && !edge->IsVisible()) {
      return;
    }

    if (edge) {
      lineColor = edge->GetColor();
      iStrokeType = edge->GetStrokeType();
      fLineWidth = edge->GetThickness();
      iCap = edge->GetCapType();
    }
  }

  CFX_Matrix mtRotate = GetRotateMatrix();
  mtRotate.Concat(matrix);

  CFX_RectF rtLine = GetRectWithoutRotate();
  CXFA_Margin* margin = node_->GetMarginIfExists();
  XFA_RectWithoutMargin(&rtLine, margin);

  GetRectFromHand(rtLine, line ? line->GetHand() : XFA_AttributeValue::Left,
                  fLineWidth);
  CFGAS_GEPath linePath;
  if (line && line->GetSlope() && rtLine.right() > 0.0f &&
      rtLine.bottom() > 0.0f) {
    linePath.AddLine(rtLine.TopRight(), rtLine.BottomLeft());
  } else {
    linePath.AddLine(rtLine.TopLeft(), rtLine.BottomRight());
  }

  CFGAS_GEGraphics::StateRestorer restorer(pGS);
  pGS->SetLineWidth(fLineWidth);
  pGS->EnableActOnDash();
  XFA_StrokeTypeSetLineDash(pGS, iStrokeType, iCap);

  pGS->SetStrokeColor(CFGAS_GEColor(lineColor));
  pGS->SetLineCap(LineCapToFXGE(iCap));
  pGS->StrokePath(linePath, mtRotate);
}
