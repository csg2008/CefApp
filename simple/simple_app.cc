// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include <regex>
#include <string>
#include <filesystem>

#include "cpuid.h"
#include "libcef/include/cef_browser.h"
#include "libcef/include/cef_origin_whitelist.h"
#include "libcef/include/base/cef_logging.h"
#include "libcef/include/views/cef_window.h"
#include "libcef/include/views/cef_browser_view.h"
#include "libcef/include/wrapper/cef_helpers.h"

#include "misc.h"
#include "V8HandlerEx.h"
#include "simple_app.h"
#include "simple_handler.h"
#include "ClientSwitches.h"

namespace {

// When using the Views framework this object provides the delegate
// implementation for the CefWindow that hosts the Views-based browser.
class SimpleWindowDelegate : public CefWindowDelegate {
 public:
  explicit SimpleWindowDelegate(CefRefPtr<CefBrowserView> browser_view, int wx, int hy) : browser_view_(browser_view), width(wx), height(hy) {}

  void OnWindowCreated(CefRefPtr<CefWindow> window) override {
    // Add the browser view and show the window.
    window->AddChildView(browser_view_);
    window->Show();

    // Give keyboard focus to the browser view.
    // browser_view_->RequestFocus();
  }

  void OnWindowDestroyed(CefRefPtr<CefWindow> window) override {
    browser_view_ = nullptr;
  }

  bool CanClose(CefRefPtr<CefWindow> window) override {
    // Allow the window to close if the browser says it's OK.
    CefRefPtr<CefBrowser> browser = browser_view_->GetBrowser();
    if (browser)
      return browser->GetHost()->TryCloseBrowser();
    return true;
  }

  CefSize GetPreferredSize(CefRefPtr<CefView> view) override {
    return CefSize(width, height);
  }

 private:
  int width;
  int height;
  CefRefPtr<CefBrowserView> browser_view_;

  IMPLEMENT_REFCOUNTING(SimpleWindowDelegate);
  DISALLOW_COPY_AND_ASSIGN(SimpleWindowDelegate);
};

class SimpleBrowserViewDelegate : public CefBrowserViewDelegate {
 public:
  SimpleBrowserViewDelegate(int wx, int hy) : width(wx), height(hy) {}

  bool OnPopupBrowserViewCreated(CefRefPtr<CefBrowserView> browser_view, CefRefPtr<CefBrowserView> popup_browser_view, bool is_devtools) override {
    // Create a new top-level Window for the popup. It will show itself after
    // creation.
    CefWindow::CreateTopLevelWindow(new SimpleWindowDelegate(popup_browser_view, width, height));

    // We created the Window.
    return true;
  }

 private:
  int width;
  int height;
  IMPLEMENT_REFCOUNTING(SimpleBrowserViewDelegate);
  DISALLOW_COPY_AND_ASSIGN(SimpleBrowserViewDelegate);
};

}  // namespace

ClientApp::ClientApp(CefRefPtr<CefCommandLine> command_line) : m_command_line(command_line) {
  if (m_command_line->HasSwitch(Switches::kAppV8Fn)) {
    appV8Fn = m_command_line->GetSwitchValue(Switches::kAppV8Fn);
  }
  if (m_command_line->HasSwitch(Switches::kAppPath)) {
    appPath = m_command_line->GetSwitchValue(Switches::kAppPath);
    if (!appPath.empty()) {
      appPath = GetAbsolutePath(appPath);
    }
  }
  if (appPath.empty()) {
    appPath = GetExecutableDirectory() + "\\app";
  }

  PrepareStartupMeta();

  debug = m_command_line->HasSwitch(Switches::kDebug);
}

void ClientApp::PrepareStartupMeta() {
  std::string startupPage = appPath + "\\page\\startup.html";

  if (std::filesystem::is_regular_file(std::filesystem::status(startupPage))) {
    std::string content = GetFileContents(startupPage);
    std::regex partten("<meta\\s+name\\s*=\\s*\"(.+)\"\\s+content\\s*=\\s*\"(.+)\"", std::regex::icase);

    std::smatch sm;
    std::string val;
    std::string data = content;
    while (std::regex_search(data, sm, partten)) {
      if (3 == sm.size()) {
        val = sm[2];
        meta[sm[1]] = val;

        std::vector<std::string> pair = split(sm[1], "|");
        if (pair.size() >= 2) {
          if ("cli" == pair[0]) {
            if ("appV8Fn" == pair[1]) {
              appV8Fn = val;
            } else if (!m_command_line->HasSwitch(pair[1])) {
              if ("true" == val) {
                m_command_line->AppendSwitch(pair[1]);
              } else {
                m_command_line->AppendSwitchWithValue(pair[1], val);
              }
            }
          } else if ("cors" == pair[0]) {
            if (3 == pair.size()) {
              CefAddCrossOriginWhitelistEntry(val, pair[2], pair[1], true);
            } else {
              CefAddCrossOriginWhitelistEntry(val, "ws", pair[1], true);
              CefAddCrossOriginWhitelistEntry(val, "wss", pair[1], true);
              CefAddCrossOriginWhitelistEntry(val, "http", pair[1], true);
              CefAddCrossOriginWhitelistEntry(val, "https", pair[1], true);
            }
          } else if ("replace" == pair[0]) {
            replace.push_back(std::make_pair(val, pair[1]));
          } else if ("redirect" == pair[0]) {
            redirect.push_back(std::make_pair(val, pair[1]));
          }
        }
      }

      data = sm.suffix().str();
    }
  }
}

// ----------------------------------------------------------------------------
// CefApp methods
// ----------------------------------------------------------------------------
///
// Provides an opportunity to view and/or modify command-line arguments before
// processing by CEF and Chromium. The |process_type| value will be empty for
// the browser process. Do not keep a reference to the CefCommandLine object
// passed to this method. The CefSettings.command_line_args_disabled value
// can be used to start with an empty command-line object. Any values
// specified in CefSettings that equate to command-line arguments will be set
// before this method is called. Be cautious when using this method to modify
// command-line arguments for non-browser processes as this may result in
// undefined behavior including crashes.
///
/*--cef(optional_param=process_type)--*/
void ClientApp::OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line) {

}

//////////////////////////////////////////////////////////////////////////////////////////
// CefApp methods.
///
// Provides an opportunity to view and/or modify command-line arguments before
// processing by CEF and Chromium. The |process_type| value will be empty for
// the browser process. Do not keep a reference to the CefCommandLine object
// passed to this method. The CefSettings.command_line_args_disabled value
// can be used to start with an empty command-line object. Any values
// specified in CefSettings that equate to command-line arguments will be set
// before this method is called. Be cautious when using this method to modify
// command-line arguments for non-browser processes as this may result in
// undefined behavior including crashes.
///
/*--cef(optional_param=process_type)--*/
//查看更多的命令行 http://peter.sh/experiments/chromium-command-line-switches/#disable-gpu-process-prelaunch
void ClientAppBrowser::OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line) {
  ClientApp::OnBeforeCommandLineProcessing(process_type, command_line);

  // Pass additional command-line flags to the browser process.
  if (process_type.empty()) {
    for (const auto &iter : meta) {
      std::string key;
      std::string name = iter.first;
			std::string value = iter.second;

      if (name.size() > 4 && name.substr(0, 4) == "cli|") {
        key = name.substr(4);
        if (!command_line->HasSwitch(key)) {
          if ("true" == value) {
            command_line->AppendSwitch(key);
          } else {
            command_line->AppendSwitchWithValue(key, value);
          }
        }
      }
    }

    if (command_line->HasSwitch("startup-process")) {
      std::string exeFile;
      std::string process = command_line->GetSwitchValue("startup-process");

      if (process.size() > 0) {
        if (std::string::npos == process.find(" ")) {
          exeFile = process;
        } else {
          exeFile = process.substr(0, process.find(" "));
        }
        if (std::string::npos != exeFile.find("/")) {
          exeFile = exeFile.substr(exeFile.find_last_of("/")+1);
        }
        if (std::string::npos != exeFile.find("\\")) {
          exeFile = exeFile.substr(exeFile.find_last_of("\\")+1);
        }

        if (!IsProcessExist(exeFile)) {
          /*SHELLEXECUTEINFO shellInfo = */exec(process, 3, false);
        }
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////////////
// CefBrowserProcessHandler methods.
///
// Called on the browser process UI thread immediately after the CEF context has been initialized.
///
void ClientAppBrowser::OnContextInitialized() {
  CEF_REQUIRE_UI_THREAD();

#if defined(OS_WIN) || defined(OS_LINUX)
  // Create the browser using the Views framework if "--use-views" is specified
  // via the command-line. Otherwise, create the browser using the native
  // platform framework. The Views framework is currently only supported on
  // Windows and Linux.
  const bool use_views = m_command_line->HasSwitch(Switches::kUseViews);
#else
  const bool use_views = false;
#endif

  // SimpleHandler implements browser-level callbacks.
  CefRefPtr<SimpleHandler> handler(new SimpleHandler(use_views, debug, appPath, replace, redirect));

  // Specify CEF browser settings here.
  CefBrowserSettings browser_settings;
  browser_settings.javascript = STATE_ENABLED;
  browser_settings.javascript_close_windows = STATE_ENABLED;
  browser_settings.javascript_access_clipboard = STATE_ENABLED;

  std::string url;

  // Check if a "--url=" value was provided via the command-line. If so, use
  // that instead of the default URL.
  url = m_command_line->GetSwitchValue("url");
  if (url.empty()) {
    std::string startupPage = appPath + "\\page\\startup.html";

    if (std::filesystem::is_regular_file(std::filesystem::status(startupPage))) {
      if (meta.find("homepage") == meta.end()) {
        url = "file:///" + startupPage;
      } else {
        url = meta["homepage"];
      }
    } else {
      std::string ss = "<html><head><title>WebAssistant</title></head><body bgcolor=\"white\">";
      ss = ss + "<h2>WebAssistant</h2>";
      ss = ss + "<div>Youth, is doomed to be bumpy, with sweat and tears, have a grievance, unwilling and failure</div>";
      ss = ss + "<div>update: " + __DATE__ + " " + __TIME__;
      ss = ss + "</body></html>";
      url = GetDataURI(ss, "text/html");
    }
  }

  if (use_views) {
    // Create the BrowserView.
    CefRefPtr<CefBrowserView> browser_view = CefBrowserView::CreateBrowserView(handler, url, browser_settings, nullptr, nullptr, new SimpleBrowserViewDelegate(width, height));

    // Create the Window. It will show itself after creation.
    CefWindow::CreateTopLevelWindow(new SimpleWindowDelegate(browser_view, width, height));
  } else {
    // Information used when creating the native window.
    CefWindowInfo windowInfo;

#if defined(OS_WIN)
    // On Windows we need to specify certain flags that will be passed to
    // CreateWindowEx().
    windowInfo.SetAsPopup(nullptr, "cefsimple");
#endif

    // Create the first browser window.
    CefBrowserHost::CreateBrowser(windowInfo, handler, url, browser_settings, nullptr, nullptr);
  }
}

///
/// Return the default client for use with a newly created browser window. If
/// null is returned the browser will be unmanaged (no callbacks will be
/// executed for that browser) and application shutdown will be blocked until
/// the browser window is closed manually. This method is currently only used
/// with the chrome runtime.
///
/*--cef()--*/
CefRefPtr<CefClient> ClientAppBrowser::GetDefaultClient() {
  // Called when a new browser window is created via the Chrome runtime UI.
  return SimpleHandler::GetInstance();
}

///
// Called before a child process is launched. Will be called on the browser
// process UI thread when launching a render process and on the browser
// process IO thread when launching a GPU or plugin process. Provides an
// opportunity to modify the child process command line. Do not keep a
// reference to |command_line| outside of this method.
///
/*--cef()--*/
void ClientAppBrowser::OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line) {
  if (command_line->HasSwitch("type")) {
    std::string processType = command_line->GetSwitchValue("type");

    if ("renderer" == processType)  {
      command_line->AppendSwitchWithValue(Switches::kAppPath, appPath);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////////////
// CefRenderProcessHandler methods.
///
/// Called after WebKit has been initialized.
///
/*--cef()--*/
void ClientAppRenderer::OnWebKitInitialized() {
  // Create the renderer-side router for query handling.
  CefMessageRouterConfig config;
  message_router_ = CefMessageRouterRendererSide::Create(config);
}

///
/// Called immediately before the V8 context for a frame is released. No
/// references to the context should be kept after this method is called.
///
/*--cef()--*/
void ClientAppRenderer::OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) {
  message_router_->OnContextReleased(browser, frame, context);
}

///
// Called after a browser has been created. When browsing cross-origin a new
// browser will be created before the old browser with the same identifier is
// destroyed. |extra_info| is a read-only value originating from
// CefBrowserHost::CreateBrowser(), CefBrowserHost::CreateBrowserSync(),
// CefLifeSpanHandler::OnBeforePopup() or CefBrowserView::CreateBrowserView().
///
/*--cef()--*/
void ClientAppRenderer::OnBrowserCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDictionaryValue> extra_info) {

}

///
// Called before a browser is destroyed.
///
/*--cef()--*/
void ClientAppRenderer::OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) {

}

///
// Called immediately after the V8 context for a frame has been created. To
// retrieve the JavaScript 'window' object use the CefV8Context::GetGlobal()
// method. V8 handles can only be accessed from the thread on which they are
// created. A task runner for posting tasks on the associated thread can be
// retrieved via the CefV8Context::GetTaskRunner() method.
///
/*--cef()--*/
void ClientAppRenderer::OnContextCreated(CefRefPtr<CefBrowser> browser,	CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) {
  CefRefPtr<CefV8Value> window = context->GetGlobal();
  CefRefPtr<CefV8Handler> handler = new V8HandlerEx(this, appPath, debug);

  message_router_->OnContextCreated(browser, frame, context);

  if (!handler.get()) {
    LOG(ERROR) << "GetJavascriptApi() failed in OnContextCreated()";
    return;
  }

  // Javascript bindings.
  if (!appV8Fn.empty()) {
    CefRefPtr<CefV8Value> webAssistant = CefV8Value::CreateObject(nullptr, nullptr);
    window->SetValue("WebAssistant", webAssistant, V8_PROPERTY_ATTRIBUTE_READONLY);
    webAssistant->SetValue("debug", CefV8Value::CreateBool(debug), V8_PROPERTY_ATTRIBUTE_READONLY);
    webAssistant->SetValue("cpuId", CefV8Value::CreateString(cpuId()), V8_PROPERTY_ATTRIBUTE_READONLY);
    webAssistant->SetValue("computer", CefV8Value::CreateString(computer()), V8_PROPERTY_ATTRIBUTE_READONLY);
    webAssistant->SetValue("exePath", CefV8Value::CreateString(GetExecutableDirectory()), V8_PROPERTY_ATTRIBUTE_READONLY);
    webAssistant->SetValue("appPath", CefV8Value::CreateString(appPath), V8_PROPERTY_ATTRIBUTE_READONLY);
    webAssistant->SetValue("dataPath", CefV8Value::CreateString(appPath + "\\data"), V8_PROPERTY_ATTRIBUTE_READONLY);

    // Methods.
    const char* methods[] = {"GetVersion", "GetAppPath", "Sleep", "IsFile", "IsDir", "MkDir", "GetDirFiles", "FileGetContents", "FilePutContents", "FileSize", "FileRename", "FileRemove", "Execute", nullptr};
    for (int i = 0; nullptr != methods[i]; i++) {
      CefRefPtr<CefV8Value> method = CefV8Value::CreateFunction(methods[i], handler);
      webAssistant->SetValue(method->GetFunctionName(), method, V8_PROPERTY_ATTRIBUTE_READONLY);
    }
  }
}

///
// Called when a new message is received from a different process. Return true
// if the message was handled or false otherwise. Do not keep a reference to
// or attempt to access the message outside of this callback.
///
/*--cef()--*/
bool ClientAppRenderer::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message) {
  DCHECK_EQ(source_process, PID_BROWSER);

  LOG(INFO) << "renderer[" << browser->GetIdentifier() << "] OnProcessMessageReceived: " << message->GetName().ToString();

  return message_router_->OnProcessMessageReceived(browser, frame, source_process, message);
}