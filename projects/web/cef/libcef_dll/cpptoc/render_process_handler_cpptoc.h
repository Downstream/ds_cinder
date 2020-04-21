// Copyright (c) 2019 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.
//
// ---------------------------------------------------------------------------
//
// This file was generated by the CEF translator tool. If making changes by
// hand only do so within the body of existing method and function
// implementations. See the translator.README.txt file in the tools directory
// for more information.
//
// $hash=c1339e8481d3e1d9823b19580f84082a3e307977$
//

#ifndef CEF_LIBCEF_DLL_CPPTOC_RENDER_PROCESS_HANDLER_CPPTOC_H_
#define CEF_LIBCEF_DLL_CPPTOC_RENDER_PROCESS_HANDLER_CPPTOC_H_
#pragma once

#if !defined(WRAPPING_CEF_SHARED)
#error This file can be included wrapper-side only
#endif

#include "include/capi/cef_render_process_handler_capi.h"
#include "include/cef_render_process_handler.h"
#include "libcef_dll/cpptoc/cpptoc_ref_counted.h"

// Wrap a C++ class with a C structure.
// This class may be instantiated and accessed wrapper-side only.
class CefRenderProcessHandlerCppToC
    : public CefCppToCRefCounted<CefRenderProcessHandlerCppToC,
                                 CefRenderProcessHandler,
                                 cef_render_process_handler_t> {
 public:
  CefRenderProcessHandlerCppToC();
  virtual ~CefRenderProcessHandlerCppToC();
};

#endif  // CEF_LIBCEF_DLL_CPPTOC_RENDER_PROCESS_HANDLER_CPPTOC_H_
