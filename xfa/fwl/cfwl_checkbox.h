// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FWL_CFWL_CHECKBOX_H_
#define XFA_FWL_CFWL_CHECKBOX_H_

#include "xfa/fwl/cfwl_event.h"
#include "xfa/fwl/cfwl_widget.h"

namespace pdfium {

#define FWL_STYLEEXT_CKB_3State (1L << 6)
#define FWL_STYLEEXT_CKB_RadioButton (1L << 7)
#define FWL_STYLEEXT_CKB_SignShapeCheck 0
#define FWL_STYLEEXT_CKB_SignShapeCircle (1L << 10)
#define FWL_STYLEEXT_CKB_SignShapeCross (2L << 10)
#define FWL_STYLEEXT_CKB_SignShapeDiamond (3L << 10)
#define FWL_STYLEEXT_CKB_SignShapeSquare (4L << 10)
#define FWL_STYLEEXT_CKB_SignShapeStar (5L << 10)
#define FWL_STYLEEXT_CKB_SignShapeMask (7L << 10)
#define FWL_STATE_CKB_Hovered (1 << FWL_STATE_WGT_MAX)
#define FWL_STATE_CKB_Pressed (1 << (FWL_STATE_WGT_MAX + 1))
#define FWL_STATE_CKB_Unchecked 0
#define FWL_STATE_CKB_Checked (1 << (FWL_STATE_WGT_MAX + 2))
#define FWL_STATE_CKB_Neutral (2 << (FWL_STATE_WGT_MAX + 2))
#define FWL_STATE_CKB_CheckMask (3L << (FWL_STATE_WGT_MAX + 2))

class CFWL_MessageKey;
class CFWL_MessageMouse;

class CFWL_CheckBox final : public CFWL_Widget {
 public:
  CONSTRUCT_VIA_MAKE_GARBAGE_COLLECTED;
  ~CFWL_CheckBox() override;

  // CFWL_Widget
  FWL_Type GetClassID() const override;
  void Update() override;
  void DrawWidget(CFGAS_GEGraphics* pGraphics,
                  const CFX_Matrix& matrix) override;

  void OnProcessMessage(CFWL_Message* pMessage) override;
  void OnDrawWidget(CFGAS_GEGraphics* pGraphics,
                    const CFX_Matrix& matrix) override;

  void SetBoxSize(float fHeight);

 private:
  explicit CFWL_CheckBox(CFWL_App* pApp);

  void SetCheckState(int32_t iCheck);
  void Layout();
  Mask<CFWL_PartState> GetPartStates() const;
  void UpdateTextOutStyles();
  void NextStates();
  void OnFocusGained();
  void OnFocusLost();
  void OnLButtonDown();
  void OnLButtonUp(CFWL_MessageMouse* pMsg);
  void OnMouseMove(CFWL_MessageMouse* pMsg);
  void OnMouseLeave();
  void OnKeyDown(CFWL_MessageKey* pMsg);

  CFX_RectF client_rect_;
  CFX_RectF box_rect_;
  CFX_RectF caption_rect_;
  CFX_RectF focus_rect_;
  FDE_TextStyle tto_styles_;
  FDE_TextAlignment tto_align_ = FDE_TextAlignment::kCenter;
  bool btn_down_ = false;
  float box_height_ = 16.0f;
};

}  // namespace pdfium

// TODO(crbug.com/42271761): Remove.
using pdfium::CFWL_CheckBox;

#endif  // XFA_FWL_CFWL_CHECKBOX_H_
