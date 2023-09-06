// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include <windows.h>

#include "libcef/include/cef_command_line.h"
#include "libcef/include/cef_sandbox_win.h"

#include "misc.h"
#include "simple_app.h"
#include "ClientSwitches.h"

// When generating projects with CMake the CEF_USE_SANDBOX value will be defined
// automatically if using the required compiler version. Pass -DUSE_SANDBOX=OFF
// to the CMake command-line to disable use of the sandbox.
// Uncomment this line to manually enable sandbox support.
// #define CEF_USE_SANDBOX 1

#if defined(CEF_USE_SANDBOX)
// The cef_sandbox.lib static library may not link successfully with all VS
// versions.
#pragma comment(lib, "cef_sandbox.lib")
#endif

HINSTANCE g_hInstance = 0;

// Entry point function for all processes.
int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPTSTR lpCmdLine,
                      int nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  g_hInstance = hInstance;

  // Enable High-DPI support on Windows 7 or newer.
  CefEnableHighDPISupport();

  void* sandbox_info = nullptr;

#if defined(CEF_USE_SANDBOX)
  // Manage the life span of the sandbox information object. This is necessary
  // for sandbox support on Windows. See cef_sandbox_win.h for complete details.
  CefScopedSandboxInfo scoped_sandbox;
  sandbox_info = scoped_sandbox.sandbox_info();
#endif

  // Specify CEF global settings here.
  CefSettings settings;

  // Provide CEF with command-line arguments.
  CefMainArgs main_args(hInstance);

  // Parse command-line arguments for use in this method.
  CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
  command_line->InitFromString(::GetCommandLineW());

  // SimpleApp implements application-level callbacks for the browser process.
  // It will create the first browser instance in OnContextInitialized() after
  // CEF has initialized.
	CefRefPtr<CefApp> app;
	ProcessType process_type = ClientApp::GetProcessType(command_line);
	if (process_type == BrowserProcess)
		app = new ClientAppBrowser(command_line);
	else if (process_type == RendererProcess)
		app = new ClientAppRenderer(command_line);
	else if (process_type == OtherProcess)
		app = new ClientAppOther(command_line);

  // CEF applications have multiple sub-processes (render, plugin, GPU, etc)
  // that share the same executable. This function checks the command-line and,
  // if this is a sub-process, executes the appropriate logic.
  int exit_code = CefExecuteProcess(main_args, app.get(), sandbox_info);
  if (exit_code >= 0) {
    // The sub-process has completed so return here.
    return exit_code;
  }

  if (command_line->HasSwitch("enable-chrome-runtime")) {
    // Enable experimental Chrome runtime. See issue #2969 for details.
    settings.chrome_runtime = true;
  }

  // cache_path
  std::string cache_path;
  if (command_line->HasSwitch(Switches::kCachePath)) {
    cache_path = GetAbsolutePath(command_line->GetSwitchValue(Switches::kCachePath).ToString());
  } else {
    cache_path = GetAbsolutePath("webcache");
  }

  // log_file
  std::string chrome_log_file;
  if (command_line->HasSwitch(Switches::kLogFile)) {
    chrome_log_file = command_line->GetSwitchValue(Switches::kLogFile);
  } else {
    chrome_log_file = cache_path + "\\chrome.log";
  }

  // log_severity
  std::string chrome_log_severity;
  if (command_line->HasSwitch(Switches::kLogSeverity)) {
    chrome_log_severity = command_line->GetSwitchValue(Switches::kLogSeverity);
  } else {
    #if defined(_DEBUG) || defined(DEBUG)
    chrome_log_severity = "verbose";
    #else
    chrome_log_severity = "disable";
    #endif
  }
  if (!chrome_log_severity.empty()) {
    cef_log_severity_t log_severity = LOGSEVERITY_DISABLE;
    if (chrome_log_severity == "verbose") {
      log_severity = LOGSEVERITY_VERBOSE;
    } else if (chrome_log_severity == "info") {
      log_severity = LOGSEVERITY_INFO;
    } else if (chrome_log_severity == "warning") {
      log_severity = LOGSEVERITY_WARNING;
    } else if (chrome_log_severity == "error") {
      log_severity = LOGSEVERITY_ERROR;
    } else if (chrome_log_severity == "disable") {
      log_severity = LOGSEVERITY_DISABLE;
    }

    settings.log_severity = log_severity;
  }

  // locale
  if (command_line->HasSwitch(Switches::kLocale)) {
    CefString(&settings.locale) = command_line->GetSwitchValue(Switches::kLocale);
  } else {
    CefString(&settings.locale).FromASCII("zh-CN");
  }

  // remote_debugging_port
  // A value of -1 will disable remote debugging.
  if (command_line->HasSwitch(Switches::kRemoteDebugingPort)) {
    int remote_debugging_port = std::stoi(command_line->GetSwitchValue(Switches::kRemoteDebugingPort).ToString(), 0, 10);
    if (remote_debugging_port > 0 && remote_debugging_port < 65535) {
      settings.remote_debugging_port = remote_debugging_port;
    }
  }

  CefString(&settings.cache_path) = cache_path;
  CefString(&settings.cache_path) = cache_path;
  CefString(&settings.log_file) = chrome_log_file;

#if !defined(CEF_USE_SANDBOX)
  settings.no_sandbox = true;
#endif

  // Initialize CEF.
  CefInitialize(main_args, settings, app.get(), sandbox_info);

  // Run the CEF message loop. This will block until CefQuitMessageLoop() is
  // called.
  CefRunMessageLoop();

  // Shut down CEF.
  CefShutdown();

  return 0;
}
