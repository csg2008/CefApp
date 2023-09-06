// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include <windows.h>
#include <sstream>
#include <string>

#include "libcef/include/cef_browser.h"
#include "libcef/include/base/cef_bind.h"
#include "libcef/include/cef_app.h"
#include "libcef/include/cef_parser.h"
#include "libcef/include/cef_request_context_handler.h"
#include "libcef/include/views/cef_browser_view.h"
#include "libcef/include/views/cef_window.h"
#include "libcef/include/wrapper/cef_closure_task.h"
#include "libcef/include/wrapper/cef_helpers.h"

#include "resource.h"
#include "ClientSwitches.h"
#include "ResourceUtil.h"
#include "simple_handler.h"
#include "response_filter.h"

extern HINSTANCE g_hInstance;

SimpleHandler* g_instance = nullptr;

namespace {

  class ClientRequestContextHandler : public CefRequestContextHandler {
  public:
    ClientRequestContextHandler(SimpleHandler* handler) : m_handler(handler) {}

  protected:
    SimpleHandler* m_handler;

  private:
    IMPLEMENT_REFCOUNTING(ClientRequestContextHandler);
    DISALLOW_COPY_AND_ASSIGN(ClientRequestContextHandler);
  };
}

// Returns a data: URI with the specified contents.
std::string GetDataURI(const std::string& data, const std::string& mime_type) {
  return "data:" + mime_type + ";base64," + CefURIEncode(CefBase64Encode(data.data(), data.size()), false).ToString();
}

SimpleHandler::SimpleHandler(
  const bool use_views, const bool debug,
  const std::string& path,
  const std::vector< std::pair<std::string, std::string> >& replace,
  const std::vector< std::pair<std::string, std::string> >& redirect) : use_views_(use_views), is_closing_(false), appPath(path), isDebug(debug), replace_(replace), redirect_(redirect) {
  DCHECK(!g_instance);
  g_instance = this;

  m_ResourceManager = new CefResourceManager();
  SetupResourceManager(m_ResourceManager, redirect, appPath, isDebug);
}

SimpleHandler::~SimpleHandler() {
  g_instance = nullptr;
}

// static
SimpleHandler* SimpleHandler::GetInstance() {
  return g_instance;
}

bool SimpleHandler::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message) {
  CEF_REQUIRE_UI_THREAD();

  if (message_router_->OnProcessMessageReceived(browser, frame, source_process,
                                                message)) {
    return true;
  }

  return false;
}

void SimpleHandler::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) {
  CEF_REQUIRE_UI_THREAD();

  if (use_views_) {
    // Set the title of the window using the Views framework.
    CefRefPtr<CefBrowserView> browser_view =
        CefBrowserView::GetForBrowser(browser);
    if (browser_view) {
      CefRefPtr<CefWindow> window = browser_view->GetWindow();
      if (window)
        window->SetTitle(title);
    }
  } else if (!IsChromeRuntimeEnabled()) {
    // Set the title of the window using platform APIs.
    PlatformTitleChange(browser, title);
  }
}

bool SimpleHandler::OnBeforePopup(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    const CefString& target_url,
    const CefString& target_frame_name,
    CefLifeSpanHandler::WindowOpenDisposition target_disposition,
    bool user_gesture,
    const CefPopupFeatures& popupFeatures,
    CefWindowInfo& windowInfo,
    CefRefPtr<CefClient>& client,
    CefBrowserSettings& settings,
    CefRefPtr<CefDictionaryValue>& extra_info,
    bool* no_javascript_access) {
  CEF_REQUIRE_UI_THREAD();
  LOG(INFO) << "SimpleHandler::OnBeforePopup() " << target_frame_name << " " << target_url;

  if (target_frame_name == "private") {
    // Information used when creating the native window.
		CefWindowInfo window_info;
		//window_info.SetAsChild(windowHandle_, rect);
    #if defined(OS_WIN)
    // On Windows we need to specify certain flags that will be passed to
    // CreateWindowEx().
    windowInfo.SetAsPopup(nullptr, "cefsimple");
    #endif
		// Specify CEF browser settings here.
		CefBrowserSettings browser_settings;
		// Create the first browser window.
		return CefBrowserHost::CreateBrowser(window_info, this, target_url, browser_settings, nullptr, GetRequestContext());
  }

  // Return true to cancel the popup window.
  return false;
}

void SimpleHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  if (!message_router_) {
    // Create the browser-side router for query handling.
    CefMessageRouterConfig config;
    message_router_ = CefMessageRouterBrowserSide::Create(config);

    // Register handlers with the router.
    CreateMessageHandlers(message_handler_set_);
    MessageHandlerSet::const_iterator it = message_handler_set_.begin();
    for (; it != message_handler_set_.end(); ++it)
      message_router_->AddHandler(*(it), false);
  }

  HICON smallIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_SMALL));
  if (smallIcon) {
    HWND cefHandle = browser->GetHost()->GetWindowHandle();
    SendMessage(cefHandle, WM_SETICON, ICON_SMALL, (LPARAM)smallIcon);
  }

  // Add to the list of existing browsers.
  browser_list_.push_back(browser);
}

bool SimpleHandler::OnBeforeBrowse(CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefFrame> frame,
                                   CefRefPtr<CefRequest> request,
                                   bool user_gesture,
                                   bool is_redirect) {
  CEF_REQUIRE_UI_THREAD();

  message_router_->OnBeforeBrowse(browser, frame);
  return false;
}

bool SimpleHandler::DoClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  // Closing the main window requires special handling. See the DoClose()
  // documentation in the CEF header for a detailed destription of this
  // process.
  if (browser_list_.size() == 1) {
    // Set a flag to indicate that the window close should be allowed.
    is_closing_ = true;
  }

  // Allow the close. For windowed browsers this will result in the OS close
  // event being sent.
  return false;
}

void SimpleHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  // Remove from the list of existing browsers.
  BrowserList::iterator bit = browser_list_.begin();
  for (; bit != browser_list_.end(); ++bit) {
    if ((*bit)->IsSame(browser)) {
      browser_list_.erase(bit);
      break;
    }
  }

  if (browser_list_.empty()) {
    // Remove and delete message router handlers.
    MessageHandlerSet::const_iterator it = message_handler_set_.begin();
    for (; it != message_handler_set_.end(); ++it) {
      message_router_->RemoveHandler(*(it));
      delete *(it);
    }
    message_handler_set_.clear();
    message_router_ = nullptr;

    // All browser windows have closed. Quit the application message loop.
    CefQuitMessageLoop();
  }
}

void SimpleHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString& errorText,
                                const CefString& failedUrl) {
  CEF_REQUIRE_UI_THREAD();

  // Allow Chrome to show the error page.
  if (IsChromeRuntimeEnabled())
    return;

  // Don't display an error for downloaded files.
  if (errorCode == ERR_ABORTED)
    return;

  // Display a load error message using a data: URI.
  std::stringstream ss;
  ss << "<html><body bgcolor=\"white\">"
        "<h2>Failed to load URL "
     << std::string(failedUrl) << " with error " << std::string(errorText)
     << " (" << errorCode << ").</h2></body></html>";

  frame->LoadURL(GetDataURI(ss.str(), "text/html"));
}

void SimpleHandler::CloseAllBrowsers(bool force_close) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute on the UI thread.
    CefPostTask(TID_UI, base::BindOnce(&SimpleHandler::CloseAllBrowsers, this, force_close));
    return;
  }

  if (browser_list_.empty())
    return;

  BrowserList::const_iterator it = browser_list_.begin();
  for (; it != browser_list_.end(); ++it)
    (*it)->GetHost()->CloseBrowser(force_close);
}

void SimpleHandler::PlatformTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) {
  CefWindowHandle hwnd = browser->GetHost()->GetWindowHandle();
  if (hwnd)
    SetWindowText(hwnd, std::wstring(title).c_str());
}

cef_return_value_t SimpleHandler::OnBeforeResourceLoad(
  CefRefPtr<CefBrowser> browser,
  CefRefPtr<CefFrame> frame,
  CefRefPtr<CefRequest> request,
  CefRefPtr<CefCallback> callback)
{
  CEF_REQUIRE_IO_THREAD();

  return m_ResourceManager->OnBeforeResourceLoad(browser, frame, request, callback);
}

CefRefPtr<CefResourceHandler> SimpleHandler::GetResourceHandler(
  CefRefPtr<CefBrowser> browser,
  CefRefPtr<CefFrame> frame,
  CefRefPtr<CefRequest> request)
{
  CEF_REQUIRE_IO_THREAD();

  return m_ResourceManager->GetResourceHandler(browser, frame, request);
}

void SimpleHandler::OnBeforeDownload(CefRefPtr<CefBrowser> browser,
  CefRefPtr<CefDownloadItem> download_item,
  const CefString& suggested_name,
  CefRefPtr<CefBeforeDownloadCallback> callback)
{
  CEF_REQUIRE_UI_THREAD();

  LOG(INFO) << "About to download a file: " << suggested_name.ToString();
  // Continue the download and show the "Save As" dialog.
  callback->Continue(suggested_name, true);
}

///
// Called when a download's status or progress information has been updated.
// This may be called multiple times before and after OnBeforeDownload().
// Execute |callback| either asynchronously or in this method to cancel the
// download if desired. Do not keep a reference to |download_item| outside of
// this method.
///
/*--cef()--*/
void SimpleHandler::OnDownloadUpdated(CefRefPtr<CefBrowser> browser,
  CefRefPtr<CefDownloadItem> download_item,
  CefRefPtr<CefDownloadItemCallback> callback)
{
  CEF_REQUIRE_UI_THREAD();

  if (download_item->IsComplete()) {
    LOG(INFO) << "Download completed, saved to: " << download_item->GetFullPath().ToString();
  } else if (download_item->IsCanceled()) {
    LOG(INFO) << "Download file " << download_item->GetFullPath().ToString() << " was cancelled";
  }
}

///
// Called on the IO thread to optionally filter resource response content. The
// |browser| and |frame| values represent the source of the request, and may
// be NULL for requests originating from service workers or CefURLRequest.
// |request| and |response| represent the request and response respectively
// and cannot be modified in this callback.
///
/*--cef(optional_param=browser,optional_param=frame)--*/
CefRefPtr<CefResponseFilter> SimpleHandler::GetResourceResponseFilter(
  CefRefPtr<CefBrowser> browser,
  CefRefPtr<CefFrame> frame,
  CefRefPtr<CefRequest> request,
  CefRefPtr<CefResponse> response) {
  CEF_REQUIRE_IO_THREAD();
  return response_filter::GetResourceResponseFilter(browser, frame, request, response, replace_, appPath, isDebug);
}

/*--cef(optional_param=browser,optional_param=frame, optional_param=request_initiator)--*/
CefRefPtr<CefResourceRequestHandler> SimpleHandler::GetResourceRequestHandler(
  CefRefPtr<CefBrowser> browser,
  CefRefPtr<CefFrame> frame,
  CefRefPtr<CefRequest> request,
  bool is_navigation,
  bool is_download,
  const CefString& request_initiator,
  bool& disable_default_handling) {
  CEF_REQUIRE_IO_THREAD();
  return this;
}

bool SimpleHandler::OnConsoleMessage(
  CefRefPtr<CefBrowser> browser,
  cef_log_severity_t level,
  const CefString& message,
  const CefString& source,
  int line) {
  CEF_REQUIRE_UI_THREAD();

  LOG(INFO) << "ConsoleMessage:" << source.ToString() << " Line: " << line << "\r\n" << message.ToString();

  return false;
}

bool SimpleHandler::OnKeyEvent(CefRefPtr<CefBrowser> cefBrowser, const CefKeyEvent& event, CefEventHandle os_event) {
  CEF_REQUIRE_UI_THREAD();

  if (event.windows_key_code == VK_F5 && event.type == KEYEVENT_RAWKEYDOWN) {
    cefBrowser->ReloadIgnoreCache();
    return false;
  } else if (event.windows_key_code == VK_F12 && event.type == KEYEVENT_RAWKEYDOWN) {
    if (!ShowDevTools(cefBrowser, CefPoint())) {
      return false;
    }
    return true;
  }

  return CefKeyboardHandler::OnKeyEvent(cefBrowser, event, os_event);
}

bool SimpleHandler::ShowDevTools(CefRefPtr<CefBrowser> browser, const CefPoint& inspect_element_at) {
  CefWindowInfo windowInfo;
  CefBrowserSettings settings;

  if (std::string::npos != browser -> GetMainFrame() -> GetURL().ToString().find("devtools://")) {
    return false;
  }

  if (browser->GetHost()->HasDevTools()) {
    browser->GetHost()->CloseDevTools();
  } else {
    #if defined(OS_WIN)
      windowInfo.SetAsPopup(nullptr, "DevTools");
    #endif

    browser->GetHost()->ShowDevTools(windowInfo, browser->GetHost()->GetClient(), settings, inspect_element_at);
  }

  return true;
}

bool SimpleHandler::OnCertificateError(CefRefPtr<CefBrowser> browser,
  ErrorCode cert_error,
  const CefString& request_url,
  CefRefPtr<CefSSLInfo> ssl_info,
  CefRefPtr<CefCallback> callback) {
    CEF_REQUIRE_UI_THREAD();

    callback->Continue();
    return true;
}

void SimpleHandler::OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser,
                                              TerminationStatus status) {
  CEF_REQUIRE_UI_THREAD();

  message_router_->OnRenderProcessTerminated(browser);
}

// static
bool SimpleHandler::IsChromeRuntimeEnabled() {
  static int value = -1;
  if (value == -1) {
    CefRefPtr<CefCommandLine> command_line = CefCommandLine::GetGlobalCommandLine();
    value = command_line->HasSwitch("enable-chrome-runtime") ? 1 : 0;
  }

  return value == 1;
}

CefRefPtr<CefRequestContext> SimpleHandler::GetRequestContext() {
  DCHECK(CefCurrentlyOn(TID_UI));
  CefRefPtr<CefCommandLine> command_line = CefCommandLine::GetGlobalCommandLine();
  DCHECK(command_line.get());
  bool request_context_shared_cache_ = command_line->HasSwitch(Switches::kRequestContextSharedCache);

  // Create a new request context for each browser.
  CefRequestContextSettings settings;

  if (command_line->HasSwitch(Switches::kCachePath)) {
    if (request_context_shared_cache_) {
      // Give each browser the same cache path. The resulting context objects
      // will share the same storage internally.
      CefString(&settings.cache_path) = command_line->GetSwitchValue(Switches::kCachePath);
    } else {
      // Give each browser a unique cache path. This will create completely
      // isolated context objects.
      std::stringstream ss;
      ss << command_line->GetSwitchValue(Switches::kCachePath).ToString() << Switches::kPathSep << time(NULL);
      CefString(&settings.cache_path) = ss.str();
    }
  }

  return CefRequestContext::CreateContext(settings, new ClientRequestContextHandler(this));
}