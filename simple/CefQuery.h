// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_QUERY_H_
#define CEF_QUERY_H_
#pragma once

#include <set>
#include <string>

#include "libcef/include/cef_browser.h"
#include "libcef/include/cef_request.h"
#include "libcef/include/wrapper/cef_message_router.h"
#include "libcef/include/wrapper/cef_resource_manager.h"

// Create all CefMessageRouterBrowserSide::Handler objects. They will be
// deleted when the ClientHandler is destroyed.
typedef std::set<CefMessageRouterBrowserSide::Handler *> MessageHandlerSet;
void CreateMessageHandlers(MessageHandlerSet &handlers);

#endif // CEF_QUERY_H_
