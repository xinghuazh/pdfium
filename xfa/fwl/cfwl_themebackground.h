// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FWL_CFWL_THEMEBACKGROUND_H_
#define XFA_FWL_CFWL_THEMEBACKGROUND_H_

#include "core/fxcrt/fx_memory.h"
#include "core/fxcrt/unowned_ptr.h"
#include "xfa/fwl/cfwl_themepart.h"

class CFGAS_GEGraphics;
class CFGAS_GEPath;

namespace pdfium {

class CFWL_ThemeBackground final : public CFWL_ThemePart {
 public:
  FX_STACK_ALLOCATED();

  CFWL_ThemeBackground(Part iPart,
                       CFWL_Widget* pWidget,
                       CFGAS_GEGraphics* pGraphics);
  ~CFWL_ThemeBackground();

  CFGAS_GEGraphics* GetGraphics() const { return graphics_; }
  const CFGAS_GEPath* GetPath() const { return path_; }
  void SetPath(const CFGAS_GEPath* pPath) { path_ = pPath; }

 private:
  UnownedPtr<const CFGAS_GEPath> path_;
  UnownedPtr<CFGAS_GEGraphics> const graphics_;
};

}  // namespace pdfium

// TODO(crbug.com/42271761): Remove.
using pdfium::CFWL_ThemeBackground;

#endif  // XFA_FWL_CFWL_THEMEBACKGROUND_H_
