#ifndef __RESOURCE_UTIL_H__
#define __RESOURCE_UTIL_H__
#pragma once

#include "libcef/include/cef_stream.h"
#include "libcef/include/wrapper/cef_resource_manager.h"
#include "libcef/include/wrapper/cef_stream_resource_handler.h"

// Set up the resource manager for tests.
void SetupResourceManager(CefRefPtr<CefResourceManager> resource_manager, const std::vector< std::pair<std::string, std::string> >& redirect, const std::string& appPath, bool debug);

#endif
