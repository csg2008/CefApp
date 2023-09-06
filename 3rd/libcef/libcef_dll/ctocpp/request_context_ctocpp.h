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
// $hash=4a05f4583cacc076519f6505f8f8952c5e471a49$
//

#ifndef CEF_LIBCEF_DLL_CTOCPP_REQUEST_CONTEXT_CTOCPP_H_
#define CEF_LIBCEF_DLL_CTOCPP_REQUEST_CONTEXT_CTOCPP_H_
#pragma once

#if !defined(WRAPPING_CEF_SHARED)
#error This file can be included wrapper-side only
#endif

#include <vector>
#include "include/capi/cef_request_context_capi.h"
#include "include/capi/cef_request_context_handler_capi.h"
#include "include/capi/cef_scheme_capi.h"
#include "include/cef_request_context.h"
#include "include/cef_request_context_handler.h"
#include "include/cef_scheme.h"
#include "libcef_dll/ctocpp/ctocpp_ref_counted.h"

// Wrap a C structure with a C++ class.
// This class may be instantiated and accessed wrapper-side only.
class CefRequestContextCToCpp
    : public CefCToCppRefCounted<CefRequestContextCToCpp,
                                 CefRequestContext,
                                 cef_request_context_t> {
 public:
  CefRequestContextCToCpp();
  virtual ~CefRequestContextCToCpp();

  // CefRequestContext methods.
  bool IsSame(CefRefPtr<CefRequestContext> other) override;
  bool IsSharingWith(CefRefPtr<CefRequestContext> other) override;
  bool IsGlobal() override;
  CefRefPtr<CefRequestContextHandler> GetHandler() override;
  CefString GetCachePath() override;
  CefRefPtr<CefCookieManager> GetCookieManager(
      CefRefPtr<CefCompletionCallback> callback) override;
  bool RegisterSchemeHandlerFactory(
      const CefString& scheme_name,
      const CefString& domain_name,
      CefRefPtr<CefSchemeHandlerFactory> factory) override;
  bool ClearSchemeHandlerFactories() override;
  void ClearCertificateExceptions(
      CefRefPtr<CefCompletionCallback> callback) override;
  void ClearHttpAuthCredentials(
      CefRefPtr<CefCompletionCallback> callback) override;
  void CloseAllConnections(CefRefPtr<CefCompletionCallback> callback) override;
  void ResolveHost(const CefString& origin,
                   CefRefPtr<CefResolveCallback> callback) override;
  void LoadExtension(const CefString& root_directory,
                     CefRefPtr<CefDictionaryValue> manifest,
                     CefRefPtr<CefExtensionHandler> handler) override;
  bool DidLoadExtension(const CefString& extension_id) override;
  bool HasExtension(const CefString& extension_id) override;
  bool GetExtensions(std::vector<CefString>& extension_ids) override;
  CefRefPtr<CefExtension> GetExtension(const CefString& extension_id) override;
  CefRefPtr<CefMediaRouter> GetMediaRouter(
      CefRefPtr<CefCompletionCallback> callback) override;

  // CefPreferenceManager methods.
  bool HasPreference(const CefString& name) override;
  CefRefPtr<CefValue> GetPreference(const CefString& name) override;
  CefRefPtr<CefDictionaryValue> GetAllPreferences(
      bool include_defaults) override;
  bool CanSetPreference(const CefString& name) override;
  bool SetPreference(const CefString& name,
                     CefRefPtr<CefValue> value,
                     CefString& error) override;
};

#endif  // CEF_LIBCEF_DLL_CTOCPP_REQUEST_CONTEXT_CTOCPP_H_
