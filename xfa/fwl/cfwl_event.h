// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FWL_CFWL_EVENT_H_
#define XFA_FWL_CFWL_EVENT_H_

#include "core/fxcrt/unowned_ptr.h"
#include "v8/include/cppgc/macros.h"

namespace pdfium {

class CFWL_Widget;

class CFWL_Event {
  CPPGC_STACK_ALLOCATED();  // Allow Raw/Unowned pointers.

 public:
  enum class Type {
    CheckStateChanged,
    Click,
    Close,
    EditChanged,
    Mouse,
    PostDropDown,
    PreDropDown,
    Scroll,
    SelectChanged,
    TextWillChange,
    TextFull,
    Validate
  };

  explicit CFWL_Event(Type type);
  CFWL_Event(Type type, CFWL_Widget* pSrcTarget);
  CFWL_Event(Type type, CFWL_Widget* pSrcTarget, CFWL_Widget* pDstTarget);
  virtual ~CFWL_Event();

  Type GetType() const { return type_; }
  CFWL_Widget* GetSrcTarget() const { return src_target_; }
  CFWL_Widget* GetDstTarget() const { return dst_target_; }

 private:
  const Type type_;
  UnownedPtr<CFWL_Widget> const src_target_;
  UnownedPtr<CFWL_Widget> const dst_target_;
};

}  // namespace pdfium

// TODO(crbug.com/42271761): Remove.
using pdfium::CFWL_Event;

#endif  // XFA_FWL_CFWL_EVENT_H_
