// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "misc.h"
#include "CefQuery.h"
#include "ClientSwitches.h"

#include <string>

#include "libcef/include/cef_parser.h"
#include "libcef/include/cef_browser.h"
#include "libcef/include/wrapper/cef_helpers.h"
#include "libcef/include/cef_devtools_message_observer.h"

namespace {

const char kFileOpenMessageName[] = "FileOpen";
const char kFileOpenMultipleMessageName[] = "FileOpenMultiple";
const char kFileOpenFolderMessageName[] = "FileOpenFolder";
const char kFileSaveMessageName[] = "FileSave";
const char kBindingTestMessageName[] = "BindingTest";

// Store persistent dialog state information.
class DialogState : public base::RefCountedThreadSafe<DialogState> {
 public:
  DialogState() : mode_(FILE_DIALOG_OPEN), pending_(false) {}

  cef_file_dialog_mode_t mode_;
  CefString last_file_;
  bool pending_;

  DISALLOW_COPY_AND_ASSIGN(DialogState);
};

// Callback executed when the file dialog is dismissed.
class DialogCallback : public CefRunFileDialogCallback {
 public:
  DialogCallback(
      CefRefPtr<CefMessageRouterBrowserSide::Callback> router_callback,
      scoped_refptr<DialogState> dialog_state)
      : router_callback_(router_callback), dialog_state_(dialog_state) {}

  virtual void OnFileDialogDismissed(
      const std::vector<CefString>& file_paths) override {
    CEF_REQUIRE_UI_THREAD();
    DCHECK(dialog_state_->pending_);

    if (!file_paths.empty()) {
      dialog_state_->last_file_ = file_paths[0];
      if (dialog_state_->mode_ == FILE_DIALOG_OPEN_FOLDER) {
        std::string last_file = dialog_state_->last_file_;
        if (last_file[last_file.length() - 1] != Switches::kPathSep) {
          // Add a trailing slash so we know it's a directory. Otherwise, file
          // dialogs will think the last path component is a file name.
          last_file += Switches::kPathSep;
          dialog_state_->last_file_ = last_file;
        }
      }
    }

    // Send a message back to the render process with the list of file paths.
    std::string response;
    for (int i = 0; i < static_cast<int>(file_paths.size()); ++i) {
      if (!response.empty())
        response += "|";  // Use a delimiter disallowed in file paths.
      response += file_paths[i];
    }

    router_callback_->Success(response);
    router_callback_ = nullptr;

    dialog_state_->pending_ = false;
    dialog_state_ = nullptr;
  }

 private:
  CefRefPtr<CefMessageRouterBrowserSide::Callback> router_callback_;
  scoped_refptr<DialogState> dialog_state_;

  IMPLEMENT_REFCOUNTING(DialogCallback);
  DISALLOW_COPY_AND_ASSIGN(DialogCallback);
};

class PdfPrinterCallback : public CefPdfPrintCallback {
  public:
    explicit PdfPrinterCallback(CefRefPtr<CefMessageRouterBrowserSide::Callback> callback, CefRefPtr<CefBrowser> browser, std::vector<std::string> params) : callback_(callback), browser_(browser), params_(params) {
        CefPdfPrintSettings settings;

        // Show the URL in the footer.
        settings.display_header_footer = 0;
        //CefString(&settings.header_footer_url) = browser_->GetMainFrame()->GetURL();

        if (params_.size() >= 3 && params_[2] == "horizontal") {
          settings.landscape = 1;
        }

        if (params_.size() >= 4 && params_[3] == "y") {
          settings.print_background = 1;
        }

        if (params_.size() >= 5) {
          settings.scale = std::stoi(params_[4]);
        }

        if (params_.size() >= 6) {
          settings.paper_width = std::stoi(params_[5]);
        }

        if (params_.size() >= 7) {
          settings.paper_height = std::stoi(params_[6]);
        }

        // Print to the selected PDF file.
        browser_->GetHost()->PrintToPDF(params_[1], settings, this);
    }

    void OnPdfPrintFinished(const CefString& path, bool ok) override {
      if (ok) {
        callback_->Success(path);
      } else {
        callback_->Failure(999, "create pdf failed");
      }
    }

  private:
    std::vector<std::string> params_;
    CefRefPtr<CefMessageRouterBrowserSide::Callback> callback_;
    CefRefPtr<CefBrowser> browser_;

    IMPLEMENT_REFCOUNTING(PdfPrinterCallback);
};


class DevToolsMessageHandler : public virtual CefBaseRefCounted {
 public:
  DevToolsMessageHandler(CefRefPtr<CefBrowser> browser) : browser_(browser) {
    // STEP 1: Add the DevTools observer. Wait for the 1st load.
    registration_ = browser->GetHost()->AddDevToolsMessageObserver(
        new TestMessageObserver(this));
  }

  struct MethodResult {
    int message_id;
    bool success;
    std::string result;
  };

  struct Event {
    std::string method;
    std::string params;
  };

  class TestMessageObserver : public CefDevToolsMessageObserver {
   public:
    explicit TestMessageObserver(DevToolsMessageHandler* handler)
        : handler_(handler) {}

    //virtual ~TestMessageObserver() { handler_->observer_destroyed_.yes(); }

    bool OnDevToolsMessage(CefRefPtr<CefBrowser> browser,
                           const void* message,
                           size_t message_size) override {
      //EXPECT_TRUE(browser.get());
      //EXPECT_EQ(handler_->GetBrowserId(), browser->GetIdentifier());
      handler_->message_ct_++;
      return false;
    }

    void OnDevToolsMethodResult(CefRefPtr<CefBrowser> browser,
                                int message_id,
                                bool success,
                                const void* result,
                                size_t result_size) override {
      //EXPECT_TRUE(browser.get());
      //EXPECT_EQ(handler_->GetBrowserId(), browser->GetIdentifier());
      handler_->result_ct_++;

      MethodResult mr;
      mr.message_id = message_id;
      mr.success = success;
      if (result) {
        // Intentionally truncating at small size.
        mr.result = std::string(static_cast<const char*>(result), std::min(static_cast<int>(result_size), 80));
      }
      handler_->OnMethodResult(mr);
    }

    void OnDevToolsEvent(CefRefPtr<CefBrowser> browser,
                         const CefString& method,
                         const void* params,
                         size_t params_size) override {
      //EXPECT_TRUE(browser.get());
      //EXPECT_EQ(handler_->GetBrowserId(), browser->GetIdentifier());
      handler_->event_ct_++;

      Event ev;
      ev.method = method;
      if (params) {
        // Intentionally truncating at small size.
        ev.params = std::string(static_cast<const char*>(params), std::min(static_cast<int>(params_size), 80));
      }
      handler_->OnEvent(ev);
    }

    void OnDevToolsAgentAttached(CefRefPtr<CefBrowser> browser) override {
      //EXPECT_TRUE(browser.get());
      //EXPECT_EQ(handler_->GetBrowserId(), browser->GetIdentifier());
      handler_->attached_ct_++;
    }

    void OnDevToolsAgentDetached(CefRefPtr<CefBrowser> browser) override {
      //EXPECT_TRUE(browser.get());
      //EXPECT_EQ(handler_->GetBrowserId(), browser->GetIdentifier());
      handler_->detached_ct_++;
    }

   private:
    DevToolsMessageHandler* handler_;

    IMPLEMENT_REFCOUNTING(TestMessageObserver);
    DISALLOW_COPY_AND_ASSIGN(TestMessageObserver);
  };

  // Execute a DevTools method. Expected results will be verified in
  // OnMethodResult, and |next_step| will then be executed.
  // |expected_result| can be a fragment that the result should start with.
  bool ExecuteMethod(const std::string& method, const std::string& params) {
    CHECK(!method.empty());

    bool ret = false;
    int message_id = next_message_id_++;

    std::stringstream message;
    message << "{\"id\":" << message_id << ",\"method\":\"" << method << "\"";
    if (!params.empty()) {
      message << ",\"params\":" << params;
    }
    message << "}";

    // Set expected result state.
    pending_message_ = message.str();
    //pending_result_next_ = std::move(next_step);
    //pending_result_ = {message_id, expected_success, expected_result};

    if (true) {
      // Use the less structured method.
      method_send_ct_++;
      ret = browser_->GetHost()->SendDevToolsMessage(pending_message_.data(), pending_message_.size());
    } else {
      // Use the more structured method.
      method_execute_ct_++;
      CefRefPtr<CefDictionaryValue> dict;
      if (!params.empty()) {
        CefRefPtr<CefValue> value =
            CefParseJSON(params.data(), params.size(), JSON_PARSER_RFC);
        if (value && value->GetType() == VTYPE_DICTIONARY) {
          dict = value->GetDictionary();
        }
      }
      ret = browser_->GetHost()->ExecuteDevToolsMethod(message_id, method, dict) > 0;
    }

    return ret;
  }

  // Every call to ExecuteMethod should result in a single call to this method
  // with the same message_id.
  void OnMethodResult(const MethodResult& result) {
    //EXPECT_EQ(pending_result_.message_id, result.message_id)
    //    << "with message=" << pending_message_;
    //if (result.message_id != pending_result_.message_id)
    //  return;

    //EXPECT_EQ(pending_result_.success, result.success)
    //    << "with message=" << pending_message_;

    //EXPECT_TRUE(result.result.find(pending_result_.result) == 0)
    //    << "with message=" << pending_message_
    //    << "\nand actual result=" << result.result
    //    << "\nand expected result=" << pending_result_.result;

    //last_result_id_ = result.message_id;

    // Continue asynchronously to allow the callstack to unwind.
    //CefPostTask(TID_UI, std::move(pending_result_next_));

    // Clear expected result state.
    //pending_message_.clear();
    //pending_result_ = {};
    CefRefPtr<CefFrame> frame = browser_->GetMainFrame();
    frame->ExecuteJavaScript("console.log('OnMethodResult: " + result.result + "');", frame->GetURL(), 0);
  }

  void OnEvent(const Event& event) {
    //if (event.method != pending_event_.method)
    //  return;

    //EXPECT_TRUE(event.params.find(pending_event_.params) == 0)
    //    << "with method=" << event.method
    //    << "\nand actual params=" << event.params
    //    << "\nand expected params=" << pending_event_.params;

    // Continue asynchronously to allow the callstack to unwind.
    // CefPostTask(TID_UI, std::move(pending_event_next_));

    // Clear expected result state.
    //pending_event_ = {};
      // Execute a JavaScript alert().
    CefRefPtr<CefFrame> frame = browser_->GetMainFrame();
    frame->ExecuteJavaScript("console.log('OnEvent: " + event.method + "');", frame->GetURL(), 0);
  }

  // void Navigate() {
  //   pending_event_ = {"Page.frameNavigated", "{\"frame\":"};
  //   pending_event_next_ =
  //       base::BindOnce(&DevToolsMessageTestHandler::AfterNavigate, this);

  //   std::stringstream params;
  //   params << "{\"url\":\"http://www.baidu.com/\"}";

  //   // STEP 3: Page domain notifications are enabled. Now start a new
  //   // navigation (but do nothing on method result) and wait for the
  //   // "Page.frameNavigated" event.
  //   ExecuteMethod("Page.navigate", params.str(), base::DoNothing(),
  //                 /*expected_result=*/"{\"frameId\":");
  // }

  // void AfterNavigate() {
  //   // STEP 4: Got the "Page.frameNavigated" event. Now disable page domain
  //   // notifications.
  //   ExecuteMethod(
  //       "Page.disable", "",
  //       base::BindOnce(&DevToolsMessageTestHandler::AfterPageDisabled, this));
  // }

  // void AfterPageDisabled() {
  //   // STEP 5: Got the the "Page.disable" method result. Now call a non-existant
  //   // method to verify an error result, and then destroy the test when done.
  //   ExecuteMethod(
  //       "Foo.doesNotExist", "",
  //       base::BindOnce(&DevToolsMessageTestHandler::MaybeDestroyTest, this),
  //       /*expected_result=*/
  //       "{\"code\":-32601,\"message\":\"'Foo.doesNotExist' wasn't found\"}",
  //       /*expected_success=*/false);
  // }


  // Total # of times we're planning to call ExecuteMethod.
  const int expected_method_ct_ = 4;

  // Total # of times we're expecting OnLoadingStateChange(isLoading=false) to
  // be called.
  const int expected_load_ct_ = 2;

  // In ExecuteMethod:
  //   Next message ID to use.
  int next_message_id_ = 1;
  //   Last message that was sent (used for debug messages only).
  std::string pending_message_;
  //   SendDevToolsMessage call count.
  int method_send_ct_ = 0;
  //   ExecuteDevToolsMethod call count.
  int method_execute_ct_ = 0;

  // Expect |pending_result_.message_id| in OnMethodResult.
  // The result should start with the |pending_result_.result| fragment.
  MethodResult pending_result_;
  //   Tracks the last message ID received.
  int last_result_id_ = -1;
  //   When received, execute this callback.
  base::OnceClosure pending_result_next_;

  // Wait for |pending_event_.method| in OnEvent.
  // The params should start with the |pending_event_.params| fragment.
  Event pending_event_;
  //   When received, execute this callback.
  base::OnceClosure pending_event_next_;

  CefRefPtr<CefRegistration> registration_;
  CefRefPtr<CefBrowser> browser_;
  // Observer callback count.
  int message_ct_ = 0;
  int result_ct_ = 0;
  int event_ct_ = 0;
  int attached_ct_ = 0;
  int detached_ct_ = 0;

  // OnLoadingStateChange(isLoading=false) count.
  int load_ct_ = 0;

  //TrackCallback observer_destroyed_;

  IMPLEMENT_REFCOUNTING(DevToolsMessageHandler);
  DISALLOW_COPY_AND_ASSIGN(DevToolsMessageHandler);
};

// Handle messages in the browser process.
class QueryHandler : public CefMessageRouterBrowserSide::Handler {
 public:
  QueryHandler() {}

  // Called due to cefQuery execution in binding.html.
  virtual bool OnQuery(CefRefPtr<CefBrowser> browser,
                       CefRefPtr<CefFrame> frame,
                       int64 query_id,
                       const CefString& request,
                       bool persistent,
                       CefRefPtr<Callback> callback) override {
    const size_t pos = request.ToString().find_first_of("||");
    if (pos != std::string::npos) {
      const std::string prefix = request.ToString().substr(0, pos);
      if (prefix == "Dialog") {
        return OnDialogQuery(browser, frame, query_id, request, persistent, callback);
      } else if (prefix == "PrintToPDF") {
        return OnPrintToPDF(browser, frame, query_id, request, persistent, callback);
      } else if (prefix == "DevToolsMessage") {
        return OnDevToolsMessage(browser, frame, query_id, request, persistent, callback);
      } else if (prefix == "Binding") {
        return OnBindingQuery(browser, frame, query_id, request, persistent, callback);
      }
    }

    return false;
  }

  //void OnPrintToPDF(CefRefPtr<CefBrowser> browser, std::string path = "output.pdf") {
  bool OnPrintToPDF(CefRefPtr<CefBrowser> browser,
                       CefRefPtr<CefFrame> frame,
                       int64 query_id,
                       const CefString& request,
                       bool persistent,
                       CefRefPtr<Callback> callback) {

    std::string pattern = "||";
    std::vector<std::string> msgItems = split(request, pattern);

    if (msgItems.size() < 2) {
      callback->Failure(1, "param is empty");
      return false;
    }

    new PdfPrinterCallback(callback, browser, msgItems);

    return true;
  }

  bool OnDevToolsMessage(CefRefPtr<CefBrowser> browser,
                       CefRefPtr<CefFrame> frame,
                       int64 query_id,
                       const CefString& request,
                       bool persistent,
                       CefRefPtr<Callback> callback) {

    std::string pattern = "||";
    std::vector<std::string> msgItems = split(request, pattern);

    if (msgItems.size() < 3) {
      callback->Failure(1, "param is empty");
      return false;
    }

    //std::string msg1 = "{\"mobile\": false, \"width\": 1024, \"height\": 1024, \"deviceScaleFactor\": 1, \"screenOrientation\": {\"angle\": 0, \"type\": \"portraitPrimary\"}}";
    //std::string msg2 = "{\"path\":\"D:\\\\Work\\\\ssh\\\\cefapp.20230327\\\\app\\\\data\\\\output\\\\screenshot.png\", \"captureBeyondViewport\": true, \"format\":\"png\"}";

    CefRefPtr<DevToolsMessageHandler> handler = new DevToolsMessageHandler(browser);
    //handler -> ExecuteMethod("Emulation.setDeviceMetricsOverride", msg1);
    //handler -> ExecuteMethod("Page.captureScreenshot", msg2);

    if (handler -> ExecuteMethod(msgItems[1], msgItems[2])) {
      callback->Success("ok");
    } else {
      callback->Failure(2, "unknown error");
    }

    return true;
  }

  // Called due to cefQuery execution in dialogs.html.
  bool OnDialogQuery(CefRefPtr<CefBrowser> browser,
                       CefRefPtr<CefFrame> frame,
                       int64 query_id,
                       const CefString& request,
                       bool persistent,
                       CefRefPtr<Callback> callback) {
    CEF_REQUIRE_UI_THREAD();

    if (!dialog_state_.get())
      dialog_state_ = new DialogState;

    // Make sure we're only running one dialog at a time.
    DCHECK(!dialog_state_->pending_);

    std::string title;
    std::string pattern = "||";
    std::vector<CefString> accept_filters;
    std::vector<std::string> msgItems = split(request, pattern);

    if (msgItems.size() < 3) {
      return false;
    }
    if (msgItems[1] == kFileOpenMessageName) {
      dialog_state_->mode_ = FILE_DIALOG_OPEN;
      title = msgItems[2];

      if (msgItems.size() > 3) {
        accept_filters.push_back(msgItems[3]);
      }
    } else if (msgItems[1] == kFileOpenMultipleMessageName) {
      dialog_state_->mode_ = FILE_DIALOG_OPEN_MULTIPLE;
      title = msgItems[2];

      if (msgItems.size() > 3) {
        accept_filters.push_back(msgItems[3]);
      }
    } else if (msgItems[1] == kFileOpenFolderMessageName) {
      dialog_state_->mode_ = FILE_DIALOG_OPEN_FOLDER;
      title = msgItems[2];
    } else if (msgItems[1] == kFileSaveMessageName) {
      dialog_state_->mode_ = FILE_DIALOG_SAVE;
      title = msgItems[2];
    } else {
      NOTREACHED();
      return true;
    }

    if (accept_filters.empty() &&
        dialog_state_->mode_ != FILE_DIALOG_OPEN_FOLDER) {
      // Build filters based on mime time.
      accept_filters.push_back("text/*");

      // Build filters based on file extension.
      accept_filters.push_back(".log");
      accept_filters.push_back(".patch");

      // Add specific filters as-is.
      accept_filters.push_back("Excel Files|.xls;.xlsx");
      accept_filters.push_back("Document Files|.doc;.odt;.docx");
      accept_filters.push_back("Image Files|.png;.jpg;.jpeg;.gif");
      accept_filters.push_back("PDF Files|.pdf");
    }

    dialog_state_->pending_ = true;

    browser->GetHost()->RunFileDialog(
        dialog_state_->mode_, title, dialog_state_->last_file_, accept_filters,
        new DialogCallback(callback, dialog_state_));

    return true;
  }

  // Called due to cefQuery execution in binding.html.
  bool OnBindingQuery(CefRefPtr<CefBrowser> browser,
                       CefRefPtr<CefFrame> frame,
                       int64 query_id,
                       const CefString& request,
                       bool persistent,
                       CefRefPtr<Callback> callback) {

    const std::string& message_name = request;
    if (message_name.find(kBindingTestMessageName) == 0) {
      // Reverse the string and return.
      std::string result = message_name.substr(sizeof(kBindingTestMessageName));
      std::reverse(result.begin(), result.end());
      callback->Success(result);
      return true;
    }

    return false;
  }

 private:
  scoped_refptr<DialogState> dialog_state_;

  DISALLOW_COPY_AND_ASSIGN(QueryHandler);
};

}  // namespace

void CreateMessageHandlers(MessageHandlerSet& handlers) {
  handlers.insert(new QueryHandler());
}