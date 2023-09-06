// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFSIMPLE_SIMPLE_APP_H_
#define CEF_TESTS_CEFSIMPLE_SIMPLE_APP_H_

#include <set>
#include <vector>

#include "V8HandlerEx.h"
#include "libcef/include/cef_app.h"
#include "libcef/include/cef_command_line.h"
#include "libcef/include/wrapper/cef_message_router.h"

// These flags must match the Chromium values.
const char kProcessType[] = "type";
const char kRendererProcess[] = "renderer";
const char kZygoteProcess[] = "zygote";

enum ProcessType {
  BrowserProcess,
  RendererProcess,
  ZygoteProcess,
  OtherProcess,
};

class ClientApp : public CefApp {
public:
  ClientApp(CefRefPtr<CefCommandLine> command_line);

	static ProcessType GetProcessType(CefRefPtr<CefCommandLine> command_line) {
		// The command-line flag won't be specified for the browser process.
		if (!command_line->HasSwitch(kProcessType))
			return BrowserProcess;

		const std::string& process_type = command_line->GetSwitchValue(kProcessType);
		if (process_type == kRendererProcess)
			return RendererProcess;
		#if defined(OS_LINUX)
		else if (process_type == kZygoteProcess)
			return ZygoteProcess;
		#endif

		return OtherProcess;
	}

  virtual void PrepareStartupMeta();

  // CefApp methods:
  virtual void OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line) override;

  class Delegate : public virtual CefBaseRefCounted {};

  public:
    bool debug = false;
    std::string appPath;
    std::string appV8Fn;
    std::map<std::string, std::string> meta;
    std::vector< std::pair<std::string, std::string> > replace;
    std::vector< std::pair<std::string, std::string> > redirect;
    CefRefPtr<CefCommandLine> m_command_line;
private:
  DISALLOW_COPY_AND_ASSIGN(ClientApp);
};

class ClientAppOther : public ClientApp {
public:
  ClientAppOther(CefRefPtr<CefCommandLine> command_line) : ClientApp(command_line) {};

private:
  IMPLEMENT_REFCOUNTING(ClientAppOther);
  DISALLOW_COPY_AND_ASSIGN(ClientAppOther);
};


class ClientAppBrowser : public ClientApp, public CefBrowserProcessHandler {
public:
  ClientAppBrowser(CefRefPtr<CefCommandLine> command_line) : ClientApp(command_line), width(GetSystemMetrics(SM_CXMAXIMIZED) * 0.7), height(GetSystemMetrics(SM_CYMAXIMIZED) * 0.7) {};

  // CefApp methods.
  virtual void OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line) override;

  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override{return this;}

  // CefBrowserProcessHandler methods.
  void OnContextInitialized() override;
  CefRefPtr<CefClient> GetDefaultClient() override;
  void OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line) override;
  //void OnRenderProcessThreadCreated(CefRefPtr<CefListValue> extra_info) override;
private:
  int width;
  int height;
  IMPLEMENT_REFCOUNTING(ClientAppBrowser);
  DISALLOW_COPY_AND_ASSIGN(ClientAppBrowser);
};

class ClientAppRenderer : public ClientApp , public CefRenderProcessHandler {
public:
  ClientAppRenderer(CefRefPtr<CefCommandLine> command_line) : ClientApp(command_line) {};

private:
  // CefApp methods.
  CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override{return this;}

  // CefRenderProcessHandler methods.
  virtual void OnWebKitInitialized() override;
  virtual void OnBrowserCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDictionaryValue> extra_info) override;
  virtual void OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) override;
  virtual void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) override;
  virtual void OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) override;
  virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message) override;

private:
  // Handles the renderer side of query routing.
  CefRefPtr<CefMessageRouterRendererSide> message_router_;

  IMPLEMENT_REFCOUNTING(ClientAppRenderer);
  DISALLOW_COPY_AND_ASSIGN(ClientAppRenderer);
};

#endif