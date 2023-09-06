// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include <regex>
#include <string>
#include <sstream>
#include <filesystem>
#include <algorithm>

#include "libcef/include/base/cef_logging.h"
#include "libcef/include/cef_command_line.h"

#include "3rd/json.hpp"

#include "response_filter.h"
#include "V8HandlerEx.h"
#include "misc.h"

namespace response_filter {
  namespace {

  namespace fs = std::filesystem;

  // Helper for passing params to Write().
  #define WRITE_PARAMS data_out_ptr, data_out_size, data_out_written

  // Filter that writes out all of the contents unchanged.
  class InjectResponseFilter : public CefResponseFilter {
  public:
    InjectResponseFilter(
      CefRefPtr<CefFrame> frame,
      const std::vector< std::pair<std::string, std::string> >& replace,
      const std::string& url,
      const std::string& mime,
      const std::string path,
      const bool debug): appOk_(false), debug_(debug), url_(url), mime_(mime), appJS_(""), frame_(frame), appPath(path), replace_(replace) {
      std::string suffix = "";
      std::vector<std::string> domain;
      std::vector<std::string> uri = split(url, "/");

      if (std::string::npos != uri[2].find_last_of(":")) {
        uri[2] = uri[2].substr(0, uri[2].find_last_of(":"));
      }

      appJS_ = appPath + "\\page\\" + uri[2] + "\\script\\inject.js";
      appOk_ = fs::is_regular_file(fs::status(appJS_));
      if (!appOk_) {
        domain = split(uri[2], ".");
        for(size_t i = domain.size() - 1; i >= 1; i--) {
          if (domain[i].size() > 3) {
            appJS_ = appPath + "\\page\\" + domain[i]  + "." + suffix + "\\script\\inject.js";

            break;
          } else {
            if (suffix.empty()) {
              suffix = domain[i];
            } else {
              suffix = domain[i] + "." + suffix;
            }
          }
        }
        if (appJS_.empty()) {
          if (1 == domain.size()) {
            appJS_ = appPath + "\\page\\" + domain[0] + "\\script\\inject.js";
          } else if (domain.size() > 1) {
            appJS_ = appPath + "\\page\\" + domain[domain.size() - 2] + "." + domain[domain.size() - 1] + "\\script\\inject.js";
          }
        }

        appOk_ = fs::is_regular_file(fs::status(appJS_));
      }

      if (debug_) {
        LOG(INFO) << "filter:" << url_ << " " << mime_;
        LOG(INFO) << "appJS:" << appJS_ << (appOk_ ? " ok " : " not exists");
      }
    }

    bool InitFilter() override { return true; }

    FilterStatus Filter(void* data_in,
                        size_t data_in_size,
                        size_t& data_in_read,
                        void* data_out,
                        size_t data_out_size,
                        size_t& data_out_written) override {
      DCHECK((data_in_size == 0U && !data_in) || (data_in_size > 0U && data_in));
      DCHECK_EQ(data_in_read, 0U);
      DCHECK(data_out);
      DCHECK_GT(data_out_size, 0U);
      DCHECK_EQ(data_out_written, 0U);

      // All data will be read.
      data_in_read = data_in_size;

      // Write out the contents unchanged.
      data_out_written = std::min(data_in_read, data_out_size);
      if (data_out_written > 0) {
        std::string content;
        bool needContent = appOk_;
        const char* data_in_ptr = static_cast<char*>(data_in);
        if (replace_.size() > 0) {
          needContent = true;
        } else {
          memcpy(data_out, data_in, data_out_written);
        }
        if (needContent) {
          content = trim(std::string(data_in_ptr, data_out_written));
        }

        if (replace_.size() > 0) {
          for(auto it : replace_) {
            try {
              std::regex regexFrom(it.first, std::regex::icase);
              content = std::regex_replace(content, regexFrom, it.second);
            } catch (const std::exception& e) {
              LOG(INFO) << "replace error: " << e.what() << " regex: " << it.first;
            }
          }

          data_out_written = content.size();
          memcpy(data_out, content.data(), content.size());
        }

        if (appOk_) {
          if (std::string::npos != mime_.find("html")) {
            std::string command = GetFileContents(appJS_);
            frame_->ExecuteJavaScript(command.c_str(), frame_->GetURL(), 0);
          } else if (std::string::npos != mime_.find("json") || std::string::npos != mime_.find("xml")) {
            std::string msg = nlohmann::json(content).dump(-1, ' ', false, nlohmann::detail::error_handler_t::ignore);
            std::string command = "eventCall(\"" + url_ + "\"," + msg + ");";

            frame_->ExecuteJavaScript(command.c_str(), frame_->GetURL(), 0);

            if (debug_) {
              LOG(INFO) << "InjectResponseFilter: " << url_ << " " << command;
            }
          }
        }
      }

      return RESPONSE_FILTER_DONE;
    }

  private:
    bool debug_;
    bool appOk_;
    std::string url_;
    std::string mime_;
    std::string appJS_;
    std::string appPath;
    CefRefPtr<CefFrame> frame_;
    std::vector< std::pair<std::string, std::string> > replace_;
    IMPLEMENT_REFCOUNTING(InjectResponseFilter);
  };

  }  // namespace

  CefRefPtr<CefResponseFilter> GetResourceResponseFilter(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      CefRefPtr<CefRequest> request,
      CefRefPtr<CefResponse> response,
      const std::vector< std::pair<std::string, std::string> >& replace,
      const std::string& appPath,
      const bool debug) {
    // Use the find/replace filter on the test URL.
    const std::string& url = request->GetURL();
    std::string mime = response -> GetMimeType().ToString();

    if (0 == url.find("http") && (std::string::npos != mime.find("html") || std::string::npos != mime.find("json") || std::string::npos != mime.find("xml") || std::string::npos != mime.find("javascript") || std::string::npos != mime.find("text"))) {
     return new InjectResponseFilter(frame, replace, url, mime, appPath, debug);
    }

    return nullptr;
  }
}
