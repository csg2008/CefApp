// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_BROWSER_RESPONSE_FILTER_TEST_H_
#define CEF_TESTS_CEFCLIENT_BROWSER_RESPONSE_FILTER_TEST_H_
#pragma once

#include <string>

#include "libcef/include/cef_browser.h"
#include "libcef/include/cef_request.h"
#include "libcef/include/cef_response.h"
#include "libcef/include/cef_response_filter.h"

namespace response_filter {

// Create a resource response filter. Called from test_runner.cc.
CefRefPtr<CefResponseFilter> GetResourceResponseFilter(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefRequest> request,
    CefRefPtr<CefResponse> response,
    const std::vector< std::pair<std::string, std::string> >& replace,
    const std::string& appPath,
    const bool debug);

}  // namespace response_filter_test


#endif  // CEF_TESTS_CEFCLIENT_BROWSER_RESPONSE_FILTER_TEST_H_
