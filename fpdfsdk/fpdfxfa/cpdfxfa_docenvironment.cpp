// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fpdfsdk/fpdfxfa/cpdfxfa_docenvironment.h"

#include <utility>

#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/cpdf_stream_acc.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/retain_ptr.h"
#include "fpdfsdk/cpdfsdk_formfillenvironment.h"
#include "fpdfsdk/cpdfsdk_helpers.h"
#include "fpdfsdk/cpdfsdk_interactiveform.h"
#include "fpdfsdk/cpdfsdk_pageview.h"
#include "fpdfsdk/fpdfxfa/cpdfxfa_context.h"
#include "fpdfsdk/fpdfxfa/cpdfxfa_page.h"
#include "xfa/fxfa/cxfa_ffdocview.h"
#include "xfa/fxfa/cxfa_ffwidget.h"
#include "xfa/fxfa/cxfa_ffwidgethandler.h"
#include "xfa/fxfa/cxfa_readynodeiterator.h"
#include "xfa/fxfa/parser/cxfa_node.h"
#include "xfa/fxfa/parser/cxfa_submit.h"

#define IDS_XFA_Validate_Input                                          \
  "At least one required field was empty. Please fill in the required " \
  "fields\r\n(highlighted) before continuing."

// submit
#define FXFA_CONFIG 0x00000001
#define FXFA_TEMPLATE 0x00000010
#define FXFA_LOCALESET 0x00000100
#define FXFA_DATASETS 0x00001000
#define FXFA_XMPMETA 0x00010000
#define FXFA_XFDF 0x00100000
#define FXFA_FORM 0x01000000
#define FXFA_PDF 0x10000000
#define FXFA_XFA_ALL 0x01111111

// Although there isn't direct casting between these types at present,
// keep the internal and exernal types in sync.
static_assert(FXFA_PAGEVIEWEVENT_POSTADDED ==
                  static_cast<int>(CXFA_FFDoc::PageViewEvent::kPostAdded),
              "kPostAdded mismatch");
static_assert(FXFA_PAGEVIEWEVENT_POSTREMOVED ==
                  static_cast<int>(CXFA_FFDoc::PageViewEvent::kPostRemoved),
              "kPostRemoved mismatch");

CPDFXFA_DocEnvironment::CPDFXFA_DocEnvironment(CPDFXFA_Context* pContext)
    : context_(pContext) {
  DCHECK(context_);
}

CPDFXFA_DocEnvironment::~CPDFXFA_DocEnvironment() = default;

void CPDFXFA_DocEnvironment::SetChangeMark(CXFA_FFDoc* hDoc) {
  if (hDoc == context_->GetXFADoc() && context_->GetFormFillEnv()) {
    context_->GetFormFillEnv()->SetChangeMark();
  }
}

void CPDFXFA_DocEnvironment::InvalidateRect(CXFA_FFPageView* pPageView,
                                            const CFX_RectF& rt) {
  if (!context_->GetXFADoc() || !context_->GetFormFillEnv()) {
    return;
  }

  if (context_->GetFormType() != FormType::kXFAFull) {
    return;
  }

  RetainPtr<CPDFXFA_Page> pPage = context_->GetXFAPage(pPageView);
  if (!pPage) {
    return;
  }

  CPDFSDK_FormFillEnvironment* pFormFillEnv = context_->GetFormFillEnv();
  if (!pFormFillEnv) {
    return;
  }

  pFormFillEnv->Invalidate(pPage.Get(), rt.ToFloatRect().ToFxRect());
}

void CPDFXFA_DocEnvironment::DisplayCaret(CXFA_FFWidget* hWidget,
                                          bool bVisible,
                                          const CFX_RectF* pRtAnchor) {
  if (!hWidget || !pRtAnchor || !context_->GetXFADoc() ||
      !context_->GetFormFillEnv() || !context_->GetXFADocView()) {
    return;
  }

  if (context_->GetFormType() != FormType::kXFAFull) {
    return;
  }

  CXFA_FFWidgetHandler* pWidgetHandler =
      context_->GetXFADocView()->GetWidgetHandler();
  if (!pWidgetHandler) {
    return;
  }

  CXFA_FFPageView* pPageView = hWidget->GetPageView();
  if (!pPageView) {
    return;
  }

  RetainPtr<CPDFXFA_Page> pPage = context_->GetXFAPage(pPageView);
  if (!pPage) {
    return;
  }

  CPDFSDK_FormFillEnvironment* pFormFillEnv = context_->GetFormFillEnv();
  if (!pFormFillEnv) {
    return;
  }

  CFX_FloatRect rcCaret = pRtAnchor->ToFloatRect();
  pFormFillEnv->DisplayCaret(pPage.Get(), bVisible, rcCaret.left, rcCaret.top,
                             rcCaret.right, rcCaret.bottom);
}

bool CPDFXFA_DocEnvironment::GetPopupPos(CXFA_FFWidget* hWidget,
                                         float fMinPopup,
                                         float fMaxPopup,
                                         const CFX_RectF& rtAnchor,
                                         CFX_RectF* pPopupRect) {
  if (!hWidget) {
    return false;
  }

  CXFA_FFPageView* pXFAPageView = hWidget->GetPageView();
  if (!pXFAPageView) {
    return false;
  }

  RetainPtr<CPDFXFA_Page> pPage = context_->GetXFAPage(pXFAPageView);
  if (!pPage) {
    return false;
  }

  CPDFSDK_FormFillEnvironment* pFormFillEnv = context_->GetFormFillEnv();
  if (!pFormFillEnv) {
    return false;
  }

  FS_RECTF page_view_rect = pFormFillEnv->GetPageViewRect(pPage.Get());
  int nRotate = hWidget->GetNode()->GetRotate();

  int space_available_below_anchor;
  int space_available_above_anchor;
  switch (nRotate) {
    case 0:
    default: {
      space_available_below_anchor =
          static_cast<int>(page_view_rect.bottom - rtAnchor.bottom());
      space_available_above_anchor =
          static_cast<int>(rtAnchor.top - page_view_rect.top);

      if (rtAnchor.left < page_view_rect.left) {
        pPopupRect->left += page_view_rect.left - rtAnchor.left;
      }
      if (rtAnchor.right() > page_view_rect.right) {
        pPopupRect->left -= rtAnchor.right() - page_view_rect.right;
      }
      break;
    }
    case 90: {
      space_available_below_anchor =
          static_cast<int>(page_view_rect.right - rtAnchor.right());
      space_available_above_anchor =
          static_cast<int>(rtAnchor.left - page_view_rect.left);

      if (rtAnchor.bottom() > page_view_rect.bottom) {
        pPopupRect->left += rtAnchor.bottom() - page_view_rect.bottom;
      }
      if (rtAnchor.top < page_view_rect.top) {
        pPopupRect->left -= page_view_rect.top - rtAnchor.top;
      }
      break;
    }
    case 180: {
      space_available_below_anchor =
          static_cast<int>(rtAnchor.top - page_view_rect.top);
      space_available_above_anchor =
          static_cast<int>(page_view_rect.bottom - rtAnchor.bottom());

      if (rtAnchor.right() > page_view_rect.right) {
        pPopupRect->left += rtAnchor.right() - page_view_rect.right;
      }
      if (rtAnchor.left < page_view_rect.left) {
        pPopupRect->left -= page_view_rect.left - rtAnchor.left;
      }
      break;
    }
    case 270: {
      space_available_below_anchor =
          static_cast<int>(rtAnchor.left - page_view_rect.left);
      space_available_above_anchor =
          static_cast<int>(page_view_rect.right - rtAnchor.right());

      if (rtAnchor.top < page_view_rect.top) {
        pPopupRect->left += page_view_rect.top - rtAnchor.top;
      }
      if (rtAnchor.bottom() > page_view_rect.bottom) {
        pPopupRect->left -= rtAnchor.bottom() - page_view_rect.bottom;
      }
      break;
    }
  }

  // If there is no space on either side, the popup can't be rendered.
  if (space_available_below_anchor <= 0 && space_available_above_anchor <= 0) {
    return false;
  }

  // Determine whether to draw above or below the anchor.
  bool draw_below_anchor;
  if (space_available_below_anchor <= 0) {
    draw_below_anchor = false;
  } else if (space_available_above_anchor <= 0) {
    draw_below_anchor = true;
  } else if (space_available_below_anchor > space_available_above_anchor) {
    draw_below_anchor = true;
  } else {
    draw_below_anchor = false;
  }

  int space_available = (draw_below_anchor ? space_available_below_anchor
                                           : space_available_above_anchor);

  // Set the popup height and y position according to what was decided above.
  float popup_height;
  if (space_available < fMinPopup) {
    popup_height = fMinPopup;
  } else if (space_available > fMaxPopup) {
    popup_height = fMaxPopup;
  } else {
    popup_height = static_cast<float>(space_available);
  }

  switch (nRotate) {
    case 0:
    case 180: {
      if (draw_below_anchor) {
        pPopupRect->top = rtAnchor.height;
      } else {
        pPopupRect->top = -popup_height;
      }
      break;
    }
    case 90:
    case 270: {
      if (draw_below_anchor) {
        pPopupRect->top = rtAnchor.width;
      } else {
        pPopupRect->top = -popup_height;
      }
      break;
    }
    default:
      break;
  }

  pPopupRect->height = popup_height;
  return true;
}

bool CPDFXFA_DocEnvironment::PopupMenu(CXFA_FFWidget* hWidget,
                                       const CFX_PointF& ptPopup) {
  if (!hWidget) {
    return false;
  }

  CXFA_FFPageView* pXFAPageView = hWidget->GetPageView();
  if (!pXFAPageView) {
    return false;
  }

  RetainPtr<CPDFXFA_Page> pPage = context_->GetXFAPage(pXFAPageView);
  if (!pPage) {
    return false;
  }

  CPDFSDK_FormFillEnvironment* pFormFillEnv = context_->GetFormFillEnv();
  if (!pFormFillEnv) {
    return false;
  }

  int menuFlag = 0;
  if (hWidget->CanUndo()) {
    menuFlag |= FXFA_MENU_UNDO;
  }
  if (hWidget->CanRedo()) {
    menuFlag |= FXFA_MENU_REDO;
  }
  if (hWidget->CanPaste()) {
    menuFlag |= FXFA_MENU_PASTE;
  }
  if (hWidget->CanCopy()) {
    menuFlag |= FXFA_MENU_COPY;
  }
  if (hWidget->CanCut()) {
    menuFlag |= FXFA_MENU_CUT;
  }
  if (hWidget->CanSelectAll()) {
    menuFlag |= FXFA_MENU_SELECTALL;
  }

  return pFormFillEnv->PopupMenu(pPage.Get(), menuFlag, ptPopup);
}

void CPDFXFA_DocEnvironment::OnPageViewEvent(CXFA_FFPageView* pPageView,
                                             CXFA_FFDoc::PageViewEvent eEvent) {
  CPDFSDK_FormFillEnvironment* pFormFillEnv = context_->GetFormFillEnv();
  if (!pFormFillEnv) {
    return;
  }

  if (context_->GetLoadStatus() == CPDFXFA_Context::LoadStatus::kLoading ||
      context_->GetLoadStatus() == CPDFXFA_Context::LoadStatus::kClosing ||
      eEvent != CXFA_FFDoc::PageViewEvent::kStopLayout) {
    return;
  }
  int nNewCount = context_->GetPageCount();
  if (nNewCount == context_->GetOriginalPageCount()) {
    return;
  }

  CXFA_FFDocView* pXFADocView = context_->GetXFADocView();
  if (!pXFADocView) {
    return;
  }

  for (int i = 0; i < context_->GetOriginalPageCount(); ++i) {
    RetainPtr<CPDFXFA_Page> pPage = context_->GetXFAPage(i);
    if (!pPage) {
      continue;
    }

    context_->GetFormFillEnv()->RemovePageView(pPage.Get());
    pPage->SetXFAPageViewIndex(i);
  }

  int flag = (nNewCount < context_->GetOriginalPageCount())
                 ? FXFA_PAGEVIEWEVENT_POSTREMOVED
                 : FXFA_PAGEVIEWEVENT_POSTADDED;
  int count = abs(nNewCount - context_->GetOriginalPageCount());
  context_->SetOriginalPageCount(nNewCount);
  pFormFillEnv->PageEvent(count, flag);
}

void CPDFXFA_DocEnvironment::WidgetPostAdd(CXFA_FFWidget* hWidget) {
  if (context_->GetFormType() != FormType::kXFAFull) {
    return;
  }

  CXFA_FFPageView* pPageView = hWidget->GetPageView();
  if (!pPageView) {
    return;
  }

  RetainPtr<CPDFXFA_Page> pXFAPage = context_->GetXFAPage(pPageView);
  if (!pXFAPage) {
    return;
  }

  auto* formfill = context_->GetFormFillEnv();
  formfill->GetOrCreatePageView(pXFAPage.Get())->AddAnnotForFFWidget(hWidget);
}

void CPDFXFA_DocEnvironment::WidgetPreRemove(CXFA_FFWidget* hWidget) {
  if (context_->GetFormType() != FormType::kXFAFull) {
    return;
  }

  CXFA_FFPageView* pPageView = hWidget->GetPageView();
  if (!pPageView) {
    return;
  }

  RetainPtr<CPDFXFA_Page> pXFAPage = context_->GetXFAPage(pPageView);
  if (!pXFAPage) {
    return;
  }

  CPDFSDK_PageView* pSdkPageView =
      context_->GetFormFillEnv()->GetOrCreatePageView(pXFAPage.Get());
  pSdkPageView->DeleteAnnotForFFWidget(hWidget);
}

int32_t CPDFXFA_DocEnvironment::CountPages(const CXFA_FFDoc* hDoc) const {
  if (hDoc == context_->GetXFADoc() && context_->GetFormFillEnv()) {
    return context_->GetPageCount();
  }
  return 0;
}

int32_t CPDFXFA_DocEnvironment::GetCurrentPage(const CXFA_FFDoc* hDoc) const {
  if (hDoc != context_->GetXFADoc() || !context_->GetFormFillEnv()) {
    return -1;
  }

  if (context_->GetFormType() != FormType::kXFAFull) {
    return -1;
  }

  CPDFSDK_FormFillEnvironment* pFormFillEnv = context_->GetFormFillEnv();
  return pFormFillEnv ? pFormFillEnv->GetCurrentPageIndex() : -1;
}

void CPDFXFA_DocEnvironment::SetCurrentPage(CXFA_FFDoc* hDoc,
                                            int32_t iCurPage) {
  if (hDoc != context_->GetXFADoc() || !context_->GetFormFillEnv() ||
      !context_->ContainsExtensionForm() || iCurPage < 0 ||
      iCurPage >= context_->GetFormFillEnv()->GetPageCount()) {
    return;
  }

  CPDFSDK_FormFillEnvironment* pFormFillEnv = context_->GetFormFillEnv();
  if (!pFormFillEnv) {
    return;
  }

  pFormFillEnv->SetCurrentPage(iCurPage);
}

bool CPDFXFA_DocEnvironment::IsCalculationsEnabled(
    const CXFA_FFDoc* hDoc) const {
  if (hDoc != context_->GetXFADoc() || !context_->GetFormFillEnv()) {
    return false;
  }
  auto* pForm = context_->GetFormFillEnv()->GetInteractiveForm();
  return pForm->IsXfaCalculateEnabled();
}

void CPDFXFA_DocEnvironment::SetCalculationsEnabled(CXFA_FFDoc* hDoc,
                                                    bool bEnabled) {
  if (hDoc != context_->GetXFADoc() || !context_->GetFormFillEnv()) {
    return;
  }
  context_->GetFormFillEnv()->GetInteractiveForm()->XfaEnableCalculate(
      bEnabled);
}

WideString CPDFXFA_DocEnvironment::GetTitle(const CXFA_FFDoc* hDoc) const {
  if (hDoc != context_->GetXFADoc()) {
    return WideString();
  }

  CPDF_Document* pPDFDoc = context_->GetPDFDoc();
  if (!pPDFDoc) {
    return WideString();
  }

  RetainPtr<const CPDF_Dictionary> pInfoDict = pPDFDoc->GetInfo();
  if (!pInfoDict) {
    return WideString();
  }

  ByteString csTitle = pInfoDict->GetByteStringFor("Title");
  return WideString::FromDefANSI(csTitle.AsStringView());
}

void CPDFXFA_DocEnvironment::SetTitle(CXFA_FFDoc* hDoc,
                                      const WideString& wsTitle) {
  if (hDoc != context_->GetXFADoc()) {
    return;
  }

  CPDF_Document* pPDFDoc = context_->GetPDFDoc();
  if (!pPDFDoc) {
    return;
  }

  RetainPtr<CPDF_Dictionary> pInfoDict = pPDFDoc->GetInfo();
  if (pInfoDict) {
    pInfoDict->SetNewFor<CPDF_String>("Title", wsTitle.AsStringView());
  }
}

void CPDFXFA_DocEnvironment::ExportData(CXFA_FFDoc* hDoc,
                                        const WideString& wsFilePath,
                                        bool bXDP) {
  if (hDoc != context_->GetXFADoc()) {
    return;
  }

  if (!context_->ContainsExtensionForm()) {
    return;
  }

  CPDFSDK_FormFillEnvironment* pFormFillEnv = context_->GetFormFillEnv();
  if (!pFormFillEnv) {
    return;
  }

  int fileType = bXDP ? FXFA_SAVEAS_XDP : FXFA_SAVEAS_XML;
  ByteString bs = wsFilePath.ToUTF16LE();
  if (wsFilePath.IsEmpty()) {
    if (!pFormFillEnv->GetFormFillInfo() ||
        !pFormFillEnv->GetFormFillInfo()->m_pJsPlatform) {
      return;
    }

    WideString filepath = pFormFillEnv->JS_fieldBrowse();
    bs = filepath.ToUTF16LE();
  }
  FPDF_FILEHANDLER* pFileHandler = pFormFillEnv->OpenFile(
      bXDP ? FXFA_SAVEAS_XDP : FXFA_SAVEAS_XML, AsFPDFWideString(&bs), "wb");
  if (!pFileHandler) {
    return;
  }

  RetainPtr<IFX_SeekableStream> fileWrite = MakeSeekableStream(pFileHandler);
  if (fileType == FXFA_SAVEAS_XML) {
    fileWrite->WriteString("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n");
    CXFA_FFDoc* ffdoc = context_->GetXFADocView()->GetDoc();
    ffdoc->SavePackage(
        ToNode(ffdoc->GetXFADoc()->GetXFAObject(XFA_HASHCODE_Data)), fileWrite);
  } else if (fileType == FXFA_SAVEAS_XDP) {
    if (!context_->GetPDFDoc()) {
      return;
    }

    const CPDF_Dictionary* pRoot = context_->GetPDFDoc()->GetRoot();
    if (!pRoot) {
      return;
    }

    RetainPtr<const CPDF_Dictionary> pAcroForm = pRoot->GetDictFor("AcroForm");
    if (!pAcroForm) {
      return;
    }

    RetainPtr<const CPDF_Array> pArray =
        ToArray(pAcroForm->GetObjectFor("XFA"));
    if (!pArray) {
      return;
    }

    for (size_t i = 1; i < pArray->size(); i += 2) {
      RetainPtr<const CPDF_Object> pPDFObj = pArray->GetObjectAt(i);
      RetainPtr<const CPDF_Object> pPrePDFObj = pArray->GetObjectAt(i - 1);
      if (!pPrePDFObj->IsString()) {
        continue;
      }
      if (!pPDFObj->IsReference()) {
        continue;
      }

      RetainPtr<const CPDF_Stream> pStream = ToStream(pPDFObj->GetDirect());
      if (!pStream) {
        continue;
      }
      if (pPrePDFObj->GetString() == "form") {
        CXFA_FFDoc* ffdoc = context_->GetXFADocView()->GetDoc();
        ffdoc->SavePackage(
            ToNode(ffdoc->GetXFADoc()->GetXFAObject(XFA_HASHCODE_Form)),
            fileWrite);
        continue;
      }
      if (pPrePDFObj->GetString() == "datasets") {
        CXFA_FFDoc* ffdoc = context_->GetXFADocView()->GetDoc();
        ffdoc->SavePackage(
            ToNode(ffdoc->GetXFADoc()->GetXFAObject(XFA_HASHCODE_Datasets)),
            fileWrite);
        continue;
      }
      if (i == pArray->size() - 1) {
        WideString wPath = WideString::FromUTF16LE(bs.unsigned_span());
        ByteString bPath = wPath.ToUTF8();
        static const char kFormat[] =
            "\n<pdf href=\"%s\" xmlns=\"http://ns.adobe.com/xdp/pdf/\"/>";
        ByteString content = ByteString::Format(kFormat, bPath.c_str());
        fileWrite->WriteString(content.AsStringView());
      }
      auto pAcc = pdfium::MakeRetain<CPDF_StreamAcc>(std::move(pStream));
      pAcc->LoadAllDataFiltered();
      fileWrite->WriteBlock(pAcc->GetSpan());
    }
  }
  fileWrite->Flush();
}

void CPDFXFA_DocEnvironment::GotoURL(CXFA_FFDoc* hDoc,
                                     const WideString& wsURL) {
  if (hDoc != context_->GetXFADoc()) {
    return;
  }

  if (context_->GetFormType() != FormType::kXFAFull) {
    return;
  }

  CPDFSDK_FormFillEnvironment* pFormFillEnv = context_->GetFormFillEnv();
  if (!pFormFillEnv) {
    return;
  }

  pFormFillEnv->GotoURL(wsURL);
}

bool CPDFXFA_DocEnvironment::IsValidationsEnabled(
    const CXFA_FFDoc* hDoc) const {
  if (hDoc != context_->GetXFADoc() || !context_->GetFormFillEnv()) {
    return false;
  }

  auto* pForm = context_->GetFormFillEnv()->GetInteractiveForm();
  return pForm->IsXfaValidationsEnabled();
}

void CPDFXFA_DocEnvironment::SetValidationsEnabled(CXFA_FFDoc* hDoc,
                                                   bool bEnabled) {
  if (hDoc != context_->GetXFADoc() || !context_->GetFormFillEnv()) {
    return;
  }

  context_->GetFormFillEnv()->GetInteractiveForm()->XfaSetValidationsEnabled(
      bEnabled);
}

void CPDFXFA_DocEnvironment::SetFocusWidget(CXFA_FFDoc* hDoc,
                                            CXFA_FFWidget* hWidget) {
  if (hDoc != context_->GetXFADoc()) {
    return;
  }

  if (!hWidget) {
    ObservedPtr<CPDFSDK_Annot> pNull;
    context_->GetFormFillEnv()->SetFocusAnnot(pNull);
    return;
  }

  int pageViewCount = context_->GetFormFillEnv()->GetPageViewCount();
  for (int i = 0; i < pageViewCount; i++) {
    CPDFSDK_PageView* pPageView =
        context_->GetFormFillEnv()->GetPageViewAtIndex(i);
    if (!pPageView) {
      continue;
    }

    ObservedPtr<CPDFSDK_Annot> pAnnot(pPageView->GetAnnotForFFWidget(hWidget));
    if (pAnnot) {
      context_->GetFormFillEnv()->SetFocusAnnot(pAnnot);
      break;
    }
  }
}

void CPDFXFA_DocEnvironment::Print(CXFA_FFDoc* hDoc,
                                   int32_t nStartPage,
                                   int32_t nEndPage,
                                   Mask<XFA_PrintOpt> dwOptions) {
  if (hDoc != context_->GetXFADoc()) {
    return;
  }

  CPDFSDK_FormFillEnvironment* pFormFillEnv = context_->GetFormFillEnv();
  if (!pFormFillEnv || !pFormFillEnv->GetFormFillInfo() ||
      !pFormFillEnv->GetFormFillInfo()->m_pJsPlatform ||
      !pFormFillEnv->GetFormFillInfo()->m_pJsPlatform->Doc_print) {
    return;
  }

  pFormFillEnv->GetFormFillInfo()->m_pJsPlatform->Doc_print(
      pFormFillEnv->GetFormFillInfo()->m_pJsPlatform,
      !!(dwOptions & XFA_PrintOpt::kShowDialog), nStartPage, nEndPage,
      !!(dwOptions & XFA_PrintOpt::kCanCancel),
      !!(dwOptions & XFA_PrintOpt::kShrinkPage),
      !!(dwOptions & XFA_PrintOpt::kAsImage),
      !!(dwOptions & XFA_PrintOpt::kReverseOrder),
      !!(dwOptions & XFA_PrintOpt::kPrintAnnot));
}

FX_ARGB CPDFXFA_DocEnvironment::GetHighlightColor(
    const CXFA_FFDoc* hDoc) const {
  if (hDoc != context_->GetXFADoc() || !context_->GetFormFillEnv()) {
    return 0;
  }

  CPDFSDK_InteractiveForm* pForm =
      context_->GetFormFillEnv()->GetInteractiveForm();
  return AlphaAndColorRefToArgb(pForm->GetHighlightAlpha(),
                                pForm->GetHighlightColor(FormFieldType::kXFA));
}

IJS_Runtime* CPDFXFA_DocEnvironment::GetIJSRuntime(
    const CXFA_FFDoc* hDoc) const {
  if (hDoc != context_->GetXFADoc()) {
    return nullptr;
  }

  CPDFSDK_FormFillEnvironment* pFormFillEnv = context_->GetFormFillEnv();
  return pFormFillEnv ? pFormFillEnv->GetIJSRuntime() : nullptr;
}

CFX_XMLDocument* CPDFXFA_DocEnvironment::GetXMLDoc() const {
  return context_->GetXMLDoc();
}

RetainPtr<IFX_SeekableReadStream> CPDFXFA_DocEnvironment::OpenLinkedFile(
    CXFA_FFDoc* hDoc,
    const WideString& wsLink) {
  CPDFSDK_FormFillEnvironment* pFormFillEnv = context_->GetFormFillEnv();
  if (!pFormFillEnv) {
    return nullptr;
  }

  ByteString bs = wsLink.ToUTF16LE();
  FPDF_FILEHANDLER* pFileHandler =
      pFormFillEnv->OpenFile(0, AsFPDFWideString(&bs), "rb");
  if (!pFileHandler) {
    return nullptr;
  }

  return MakeSeekableStream(pFileHandler);
}

#ifdef PDF_XFA_ELEMENT_SUBMIT_ENABLED
bool CPDFXFA_DocEnvironment::Submit(CXFA_FFDoc* hDoc, CXFA_Submit* submit) {
  if (!OnBeforeNotifySubmit() || !context_->GetXFADocView()) {
    return false;
  }

  context_->GetXFADocView()->UpdateDocView();
  bool ret = SubmitInternal(hDoc, submit);
  OnAfterNotifySubmit();
  return ret;
}

bool CPDFXFA_DocEnvironment::MailToInfo(WideString& csURL,
                                        WideString& csToAddress,
                                        WideString& csCCAddress,
                                        WideString& csBCCAddress,
                                        WideString& csSubject,
                                        WideString& csMsg) {
  WideString srcURL = csURL;
  srcURL.TrimWhitespaceFront();
  if (!srcURL.Left(7).EqualsASCIINoCase("mailto:")) {
    return false;
  }
  auto pos = srcURL.Find(L'?');
  {
    WideString tmp;
    if (!pos.has_value()) {
      pos = srcURL.Find(L'@');
      if (!pos.has_value()) {
        return false;
      }

      tmp = srcURL.Right(csURL.GetLength() - 7);
    } else {
      tmp = srcURL.Left(pos.value());
      tmp = tmp.Right(tmp.GetLength() - 7);
    }
    tmp.TrimWhitespace();
    csToAddress = std::move(tmp);
  }

  srcURL = srcURL.Right(srcURL.GetLength() - (pos.value() + 1));
  while (!srcURL.IsEmpty()) {
    srcURL.TrimWhitespace();
    pos = srcURL.Find(L'&');
    WideString tmp = (!pos.has_value()) ? srcURL : srcURL.Left(pos.value());
    tmp.TrimWhitespace();
    if (tmp.GetLength() >= 3 && tmp.Left(3).EqualsASCIINoCase("cc=")) {
      tmp = tmp.Right(tmp.GetLength() - 3);
      if (!csCCAddress.IsEmpty()) {
        csCCAddress += L';';
      }
      csCCAddress += tmp;
    } else if (tmp.GetLength() >= 4 && tmp.Left(4).EqualsASCIINoCase("bcc=")) {
      tmp = tmp.Right(tmp.GetLength() - 4);
      if (!csBCCAddress.IsEmpty()) {
        csBCCAddress += L';';
      }
      csBCCAddress += tmp;
    } else if (tmp.GetLength() >= 8 &&
               tmp.Left(8).EqualsASCIINoCase("subject=")) {
      tmp = tmp.Right(tmp.GetLength() - 8);
      csSubject += tmp;
    } else if (tmp.GetLength() >= 5 && tmp.Left(5).EqualsASCIINoCase("body=")) {
      tmp = tmp.Right(tmp.GetLength() - 5);
      csMsg += tmp;
    }
    srcURL = pos.has_value()
                 ? srcURL.Right(csURL.GetLength() - (pos.value() + 1))
                 : WideString();
  }
  csToAddress.Replace(L",", L";");
  csCCAddress.Replace(L",", L";");
  csBCCAddress.Replace(L",", L";");
  return true;
}

bool CPDFXFA_DocEnvironment::ExportSubmitFile(FPDF_FILEHANDLER* pFileHandler,
                                              int fileType,
                                              FPDF_DWORD encodeType,
                                              FPDF_DWORD flag) {
  if (!context_->GetXFADocView()) {
    return false;
  }

  CPDFSDK_FormFillEnvironment* pFormFillEnv = context_->GetFormFillEnv();
  if (!pFormFillEnv) {
    return false;
  }

  CXFA_FFDoc* ffdoc = context_->GetXFADocView()->GetDoc();
  RetainPtr<IFX_SeekableStream> fileStream = MakeSeekableStream(pFileHandler);
  if (fileType == FXFA_SAVEAS_XML) {
    fileStream->WriteString("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n");
    ffdoc->SavePackage(
        ToNode(ffdoc->GetXFADoc()->GetXFAObject(XFA_HASHCODE_Data)),
        fileStream);
    return true;
  }

  if (fileType != FXFA_SAVEAS_XDP) {
    return true;
  }

  if (!flag) {
    flag = FXFA_CONFIG | FXFA_TEMPLATE | FXFA_LOCALESET | FXFA_DATASETS |
           FXFA_XMPMETA | FXFA_XFDF | FXFA_FORM;
  }
  if (!context_->GetPDFDoc()) {
    fileStream->Flush();
    return false;
  }

  const CPDF_Dictionary* pRoot = context_->GetPDFDoc()->GetRoot();
  if (!pRoot) {
    fileStream->Flush();
    return false;
  }

  RetainPtr<const CPDF_Dictionary> pAcroForm = pRoot->GetDictFor("AcroForm");
  if (!pAcroForm) {
    fileStream->Flush();
    return false;
  }

  RetainPtr<const CPDF_Array> pArray = ToArray(pAcroForm->GetObjectFor("XFA"));
  if (!pArray) {
    fileStream->Flush();
    return false;
  }

  for (size_t i = 1; i < pArray->size(); i += 2) {
    RetainPtr<const CPDF_Object> pPDFObj = pArray->GetObjectAt(i);
    RetainPtr<const CPDF_Object> pPrePDFObj = pArray->GetObjectAt(i - 1);
    if (!pPrePDFObj->IsString()) {
      continue;
    }
    if (!pPDFObj->IsReference()) {
      continue;
    }

    RetainPtr<const CPDF_Object> pDirectObj = pPDFObj->GetDirect();
    if (!pDirectObj->IsStream()) {
      continue;
    }
    ByteString bsType = pPrePDFObj->GetString();
    if (bsType == "config" && !(flag & FXFA_CONFIG)) {
      continue;
    }
    if (bsType == "template" && !(flag & FXFA_TEMPLATE)) {
      continue;
    }
    if (bsType == "localeSet" && !(flag & FXFA_LOCALESET)) {
      continue;
    }
    if (bsType == "datasets" && !(flag & FXFA_DATASETS)) {
      continue;
    }
    if (bsType == "xmpmeta" && !(flag & FXFA_XMPMETA)) {
      continue;
    }
    if (bsType == "xfdf" && !(flag & FXFA_XFDF)) {
      continue;
    }
    if (bsType == "form" && !(flag & FXFA_FORM)) {
      continue;
    }

    if (bsType == "form") {
      ffdoc->SavePackage(
          ToNode(ffdoc->GetXFADoc()->GetXFAObject(XFA_HASHCODE_Form)),
          fileStream);
    } else if (pPrePDFObj->GetString() == "datasets") {
      ffdoc->SavePackage(
          ToNode(ffdoc->GetXFADoc()->GetXFAObject(XFA_HASHCODE_Datasets)),
          fileStream);
    }
  }
  return true;
}

void CPDFXFA_DocEnvironment::ToXFAContentFlags(WideString csSrcContent,
                                               FPDF_DWORD& flag) {
  if (csSrcContent.Contains(L" config ")) {
    flag |= FXFA_CONFIG;
  }
  if (csSrcContent.Contains(L" template ")) {
    flag |= FXFA_TEMPLATE;
  }
  if (csSrcContent.Contains(L" localeSet ")) {
    flag |= FXFA_LOCALESET;
  }
  if (csSrcContent.Contains(L" datasets ")) {
    flag |= FXFA_DATASETS;
  }
  if (csSrcContent.Contains(L" xmpmeta ")) {
    flag |= FXFA_XMPMETA;
  }
  if (csSrcContent.Contains(L" xfdf ")) {
    flag |= FXFA_XFDF;
  }
  if (csSrcContent.Contains(L" form ")) {
    flag |= FXFA_FORM;
  }
  if (flag == 0) {
    flag = FXFA_CONFIG | FXFA_TEMPLATE | FXFA_LOCALESET | FXFA_DATASETS |
           FXFA_XMPMETA | FXFA_XFDF | FXFA_FORM;
  }
}

bool CPDFXFA_DocEnvironment::OnBeforeNotifySubmit() {
  if (!context_->ContainsXFAForm()) {
    return true;
  }

  CXFA_FFDocView* docView = context_->GetXFADocView();
  if (!docView) {
    return true;
  }

  CXFA_FFWidgetHandler* pWidgetHandler = docView->GetWidgetHandler();
  if (!pWidgetHandler) {
    return true;
  }

  auto it = docView->CreateReadyNodeIterator();
  if (it) {
    CXFA_EventParam Param;
    Param.type_ = XFA_EVENT_PreSubmit;
    while (CXFA_Node* pNode = it->MoveToNext()) {
      pWidgetHandler->ProcessEvent(pNode, &Param);
    }
  }

  it = docView->CreateReadyNodeIterator();
  if (!it) {
    return true;
  }

  (void)it->MoveToNext();
  CXFA_Node* pNode = it->MoveToNext();

  while (pNode) {
    if (pNode->ProcessValidate(docView, -1) == XFA_EventError::kError) {
      CPDFSDK_FormFillEnvironment* pFormFillEnv = context_->GetFormFillEnv();
      if (!pFormFillEnv) {
        return false;
      }

      pFormFillEnv->JS_appAlert(WideString::FromDefANSI(IDS_XFA_Validate_Input),
                                WideString(), JSPLATFORM_ALERT_BUTTON_OK,
                                JSPLATFORM_ALERT_ICON_WARNING);
      return false;
    }
    pNode = it->MoveToNext();
  }

  docView->UpdateDocView();
  return true;
}

void CPDFXFA_DocEnvironment::OnAfterNotifySubmit() {
  if (!context_->ContainsXFAForm()) {
    return;
  }

  if (!context_->GetXFADocView()) {
    return;
  }

  CXFA_FFWidgetHandler* pWidgetHandler =
      context_->GetXFADocView()->GetWidgetHandler();
  if (!pWidgetHandler) {
    return;
  }

  auto it = context_->GetXFADocView()->CreateReadyNodeIterator();
  if (!it) {
    return;
  }

  CXFA_EventParam Param;
  Param.type_ = XFA_EVENT_PostSubmit;
  CXFA_Node* pNode = it->MoveToNext();
  while (pNode) {
    pWidgetHandler->ProcessEvent(pNode, &Param);
    pNode = it->MoveToNext();
  }
  context_->GetXFADocView()->UpdateDocView();
}

bool CPDFXFA_DocEnvironment::SubmitInternal(CXFA_FFDoc* hDoc,
                                            CXFA_Submit* submit) {
  CPDFSDK_FormFillEnvironment* pFormFillEnv = context_->GetFormFillEnv();
  if (!pFormFillEnv) {
    return false;
  }

  WideString csURL = submit->GetSubmitTarget();
  if (csURL.IsEmpty()) {
    pFormFillEnv->JS_appAlert(WideString::FromDefANSI("Submit cancelled."),
                              WideString(), JSPLATFORM_ALERT_BUTTON_OK,
                              JSPLATFORM_ALERT_ICON_ASTERISK);
    return false;
  }

  FPDF_FILEHANDLER* pFileHandler = nullptr;
  int fileFlag = -1;
  switch (submit->GetSubmitFormat()) {
    case XFA_AttributeValue::Xdp: {
      WideString csContent = submit->GetSubmitXDPContent();
      csContent.TrimWhitespace();

      WideString space = WideString::FromDefANSI(" ");
      csContent = space + csContent + space;
      FPDF_DWORD flag = 0;
      if (submit->IsSubmitEmbedPDF()) {
        flag |= FXFA_PDF;
      }

      ToXFAContentFlags(csContent, flag);
      pFileHandler = pFormFillEnv->OpenFile(FXFA_SAVEAS_XDP, nullptr, "wb");
      fileFlag = FXFA_SAVEAS_XDP;
      ExportSubmitFile(pFileHandler, FXFA_SAVEAS_XDP, 0, flag);
      break;
    }
    case XFA_AttributeValue::Xml:
      pFileHandler = pFormFillEnv->OpenFile(FXFA_SAVEAS_XML, nullptr, "wb");
      fileFlag = FXFA_SAVEAS_XML;
      ExportSubmitFile(pFileHandler, FXFA_SAVEAS_XML, 0, FXFA_XFA_ALL);
      break;
    case XFA_AttributeValue::Pdf:
      break;
    case XFA_AttributeValue::Urlencoded:
      pFileHandler = pFormFillEnv->OpenFile(FXFA_SAVEAS_XML, nullptr, "wb");
      fileFlag = FXFA_SAVEAS_XML;
      ExportSubmitFile(pFileHandler, FXFA_SAVEAS_XML, 0, FXFA_XFA_ALL);
      break;
    default:
      return false;
  }
  if (!pFileHandler) {
    return false;
  }

  if (csURL.Left(7).EqualsASCIINoCase("mailto:")) {
    WideString csToAddress;
    WideString csCCAddress;
    WideString csBCCAddress;
    WideString csSubject;
    WideString csMsg;
    if (!MailToInfo(csURL, csToAddress, csCCAddress, csBCCAddress, csSubject,
                    csMsg)) {
      return false;
    }
    ByteString bsTo = WideString(csToAddress).ToUTF16LE();
    ByteString bsCC = WideString(csCCAddress).ToUTF16LE();
    ByteString bsBcc = WideString(csBCCAddress).ToUTF16LE();
    ByteString bsSubject = WideString(csSubject).ToUTF16LE();
    ByteString bsMsg = WideString(csMsg).ToUTF16LE();
    pFormFillEnv->EmailTo(pFileHandler, AsFPDFWideString(&bsTo),
                          AsFPDFWideString(&bsSubject), AsFPDFWideString(&bsCC),
                          AsFPDFWideString(&bsBcc), AsFPDFWideString(&bsMsg));
    return true;
  }

  // HTTP or FTP
  ByteString bs = csURL.ToUTF16LE();
  pFormFillEnv->UploadTo(pFileHandler, fileFlag, AsFPDFWideString(&bs));
  return true;
}
#endif  // PDF_XFA_ELEMENT_SUBMIT_ENABLED
