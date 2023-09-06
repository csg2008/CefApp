// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFSIMPLE_SIMPLE_HANDLER_H_
#define CEF_TESTS_CEFSIMPLE_SIMPLE_HANDLER_H_

#include "CefQuery.h"

#include "libcef/include/cef_client.h"
#include "libcef/include/wrapper/cef_resource_manager.h"

#include <list>
#include <string>

std::string GetDataURI(const std::string& data, const std::string& mime_type);

class SimpleHandler : public CefClient,
                      public CefCommandHandler,
                      public CefContextMenuHandler,
                      public CefDisplayHandler,
                      public CefDownloadHandler,
                      public CefDragHandler,
                      public CefFocusHandler,
                      public CefKeyboardHandler,
                      public CefLifeSpanHandler,
                      public CefLoadHandler,
                      public CefPermissionHandler,
                      public CefRequestHandler,
                      public CefResourceRequestHandler {
 public:
  explicit SimpleHandler(
    const bool use_views,
    const bool debug,
    const std::string& path,
    const std::vector< std::pair<std::string, std::string> >& replace,
    const std::vector< std::pair<std::string, std::string> >& redirect
    );
  ~SimpleHandler();

  // Provide access to the single global instance of this object.
  static SimpleHandler* GetInstance();

  // CefClient methods:
  virtual CefRefPtr<CefDisplayHandler>  GetDisplayHandler()  override { return this; }
  virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
  virtual CefRefPtr<CefLoadHandler>     GetLoadHandler()     override { return this; }
  virtual CefRefPtr<CefRequestHandler>  GetRequestHandler()  override { return this; }
  virtual CefRefPtr<CefDownloadHandler>	GetDownloadHandler() override { return this; }
  virtual CefRefPtr<CefKeyboardHandler>	GetKeyboardHandler() override { return this; }
  virtual CefRefPtr<CefCommandHandler>  GetCommandHandler()  override { return this; }
  virtual CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override { return this; }
  virtual CefRefPtr<CefDragHandler> GetDragHandler() override { return this; }
  virtual CefRefPtr<CefFocusHandler> GetFocusHandler() override { return this; }
  virtual CefRefPtr<CefPermissionHandler> GetPermissionHandler() override { return this; }

  bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                CefProcessId source_process,
                                CefRefPtr<CefProcessMessage> message) override;

  // CefDisplayHandler methods:
  virtual void OnTitleChange(CefRefPtr<CefBrowser> browser,
                             const CefString& title) override;

  // CefLifeSpanHandler methods:
  virtual bool OnBeforePopup(
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
    bool* no_javascript_access) override;
  virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
  virtual bool DoClose(CefRefPtr<CefBrowser> browser) override;
  virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;
  virtual bool OnConsoleMessage(
    CefRefPtr<CefBrowser> browser,
    cef_log_severity_t level,
    const CefString& message,
    const CefString& source,
    int line) override;

  // CefLoadHandler methods:
  virtual void OnLoadError(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame,
                           ErrorCode errorCode,
                           const CefString& errorText,
                           const CefString& failedUrl) override;

  // CefRequestHandler method:
  virtual CefRefPtr<CefResponseFilter> GetResourceResponseFilter(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefRequest> request,
    CefRefPtr<CefResponse> response) override;

  // CefRequestHandler methods
  bool OnBeforeBrowse(CefRefPtr<CefBrowser> browser,
                      CefRefPtr<CefFrame> frame,
                      CefRefPtr<CefRequest> request,
                      bool user_gesture,
                      bool is_redirect) override;
  virtual CefRefPtr<CefResourceRequestHandler> GetResourceRequestHandler(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefRequest> request,
    bool is_navigation,
    bool is_download,
    const CefString& request_initiator,
    bool& disable_default_handling) override;
  virtual cef_return_value_t OnBeforeResourceLoad(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      CefRefPtr<CefRequest> request,
      CefRefPtr<CefCallback> callback) override;
  virtual CefRefPtr<CefResourceHandler> GetResourceHandler(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      CefRefPtr<CefRequest> request) override;
  bool OnCertificateError(CefRefPtr<CefBrowser> browser,
      ErrorCode cert_error,
      const CefString& request_url,
      CefRefPtr<CefSSLInfo> ssl_info,
      CefRefPtr<CefCallback> callback) override;
  void OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser,
                                 TerminationStatus status) override;

  // CefDownloadHandler methods
  void OnBeforeDownload(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefDownloadItem> download_item,
    const CefString& suggested_name,
    CefRefPtr<CefBeforeDownloadCallback> callback) override;
  void OnDownloadUpdated(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefDownloadItem> download_item,
    CefRefPtr<CefDownloadItemCallback> callback) override;

  // CefKeyboardHandler methods:
  virtual bool OnKeyEvent(CefRefPtr<CefBrowser> browser,
              const CefKeyEvent& event,
              CefEventHandle os_event) override;

  // Request that all existing browser windows close.
  void CloseAllBrowsers(bool force_close);

  bool IsClosing() const { return is_closing_; }

  // Returns true if the Chrome runtime is enabled.
  static bool IsChromeRuntimeEnabled();

  CefRefPtr<CefRequestContext> GetRequestContext();

  bool ShowDevTools(CefRefPtr<CefBrowser> browser,
        const CefPoint& inspect_element_at);

 private:
  // Platform-specific implementation.
  void PlatformTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title);

  // Manages the registration and delivery of resources.
  CefRefPtr<CefResourceManager> m_ResourceManager;

  // True if the application is using the Views framework.
  const bool use_views_;

  // List of existing browser windows. Only accessed on the CEF UI thread.
  typedef std::list<CefRefPtr<CefBrowser>> BrowserList;
  BrowserList browser_list_;

  bool is_closing_;

  bool isDebug;

  std::string appPath;

  std::vector< std::pair<std::string, std::string> > replace_;

  std::vector< std::pair<std::string, std::string> > redirect_;

  // Handles the browser side of query routing. The renderer side is handled
  // in client_renderer.cc.
  CefRefPtr<CefMessageRouterBrowserSide> message_router_;

  // Set of Handlers registered with the message router.
  MessageHandlerSet message_handler_set_;

  // Include the default reference counting implementation.
  IMPLEMENT_REFCOUNTING(SimpleHandler);
};

#endif  // CEF_TESTS_CEFSIMPLE_SIMPLE_HANDLER_H_
