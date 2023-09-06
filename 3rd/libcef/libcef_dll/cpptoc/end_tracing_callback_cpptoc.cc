// Copyright (c) 2023 The Chromium Embedded Framework Authors. All rights
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
// $hash=d1cdc1747a3caa4b8aa4cc385c1164bc066bbefb$
//

#include "libcef_dll/cpptoc/end_tracing_callback_cpptoc.h"
#include "libcef_dll/shutdown_checker.h"

namespace {

// MEMBER FUNCTIONS - Body may be edited by hand.

void CEF_CALLBACK end_tracing_callback_on_end_tracing_complete(
    struct _cef_end_tracing_callback_t* self,
    const cef_string_t* tracing_file) {
  shutdown_checker::AssertNotShutdown();

  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  DCHECK(self);
  if (!self)
    return;
  // Verify param: tracing_file; type: string_byref_const
  DCHECK(tracing_file);
  if (!tracing_file)
    return;

  // Execute
  CefEndTracingCallbackCppToC::Get(self)->OnEndTracingComplete(
      CefString(tracing_file));
}

}  // namespace

// CONSTRUCTOR - Do not edit by hand.

CefEndTracingCallbackCppToC::CefEndTracingCallbackCppToC() {
  GetStruct()->on_end_tracing_complete =
      end_tracing_callback_on_end_tracing_complete;
}

// DESTRUCTOR - Do not edit by hand.

CefEndTracingCallbackCppToC::~CefEndTracingCallbackCppToC() {
  shutdown_checker::AssertNotShutdown();
}

template <>
CefRefPtr<CefEndTracingCallback> CefCppToCRefCounted<
    CefEndTracingCallbackCppToC,
    CefEndTracingCallback,
    cef_end_tracing_callback_t>::UnwrapDerived(CefWrapperType type,
                                               cef_end_tracing_callback_t* s) {
  NOTREACHED() << "Unexpected class type: " << type;
  return nullptr;
}

template <>
CefWrapperType CefCppToCRefCounted<CefEndTracingCallbackCppToC,
                                   CefEndTracingCallback,
                                   cef_end_tracing_callback_t>::kWrapperType =
    WT_END_TRACING_CALLBACK;
