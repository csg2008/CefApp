#include <set>
#include <regex>
#include <string>
#include <fstream>
#include <filesystem>

#include "libcef/include/cef_parser.h"
#include "libcef/include/cef_stream.h"
#include "libcef/include/wrapper/cef_byte_read_handler.h"
#include "libcef/include/wrapper/cef_stream_resource_handler.h"

#include "misc.h"
#include "V8HandlerEx.h"
#include "ResourceUtil.h"

// Provider that returns string data for specific pages. Used in combination with LoadStringResourcePage().
class PagesResourceProvider : public CefResourceManager::Provider {
public:
  PagesResourceProvider(const std::vector< std::pair<std::string, std::string> >& redirect, const std::string& path, bool isDebug) : debug(isDebug), basePath(path) {
    DCHECK(!path.empty());

    if (!debug && std::filesystem::is_directory(std::filesystem::status(path))) {
      buildPagesList(path, path);
    }

    if (redirect.size() > 0) {
      try {
        for(size_t i = 0; i < redirect.size(); i++) {
          redirect_regex.push_back(std::make_pair(std::regex(redirect[i].first, std::regex::icase), redirect[i].second));
        }
      } catch (const std::exception& e) {
        LOG(INFO) << "redirect error: " << e.what();
      }
    }
  }

  bool OnRequest(scoped_refptr<CefResourceManager::Request> request) override {
    CEF_REQUIRE_IO_THREAD();

    std::string value, domain;
    CefResponse::HeaderMap headers;
    bool isExists = false;
    std::string url = request->url();
    std::string page = url.substr(url.find("//")+2);

    if (std::string::npos != url.find("devtools://")) {
      return false;
    }

    if (std::string::npos != page.find("?")) {
      page = page.substr(0, page.find("?"));
    }
    if (std::string::npos != page.find("#")) {
      page = page.substr(0, page.find("#"));
    }
    if (std::string::npos != page.find(":")) {
      page = page.erase(page.find(":"), page.find("/") - page.find(":"));
    }
    if (page.rfind("/") + 1 == page.size()) {
      page = page + "index.html";
    }

    for(size_t i = 0; i < redirect_regex.size(); i++) {
      if (std::regex_match(page, redirect_regex[i].first)) {
        page = std::regex_replace(page, redirect_regex[i].first, redirect_regex[i].second);
        break;
      }
    }

    if (debug) {
      isExists = std::filesystem::is_regular_file(std::filesystem::status(basePath + "/" + page));
      if (!isExists) {
        domain = GetURIDomain(page);
        if (!domain.empty()) {
          page = domain + page.substr(page.find("/"));
          isExists = std::filesystem::is_regular_file(std::filesystem::status(basePath + "/" + page));
        }
      }
      if (isExists) {
        std::ifstream inputFile(basePath + "/" + page, std::ios::binary);
        if (inputFile.good()) {
          value.assign(std::istreambuf_iterator<char>(inputFile), std::istreambuf_iterator<char>());
          inputFile.close();
        }
      } else {
        return false;
      }
    } else {
      // check page is exists
      if (pages.find(page) == pages.end()) {
        domain = GetURIDomain(page);
        if (!domain.empty()) {
          page = domain + page.substr(page.find("/"));
          isExists = pages.find(page) != pages.end();
        }

        if (!isExists) {
          return false;
        }
      }

      std::map<std::string, std::string>::const_iterator it = content.find(page);
      if (it != content.end()) {
        value = it->second;
      } else if (std::filesystem::is_regular_file(std::filesystem::status(basePath + "/" + page))) {
        std::ifstream inputFile(basePath + "/" + page, std::ios::binary);
        if (inputFile.good()) {
          value.assign(std::istreambuf_iterator<char>(inputFile), std::istreambuf_iterator<char>());
          inputFile.close();

          content[page] = value;
        }
      }
    }

    if (value.empty()) {
      value = " ";
    }

    CefRefPtr<CefStreamReader> response = CefStreamReader::CreateForData(static_cast<void*>(const_cast<char*>(value.c_str())), value.size());

    request->Continue(new CefStreamResourceHandler(200, "OK", request->mime_type_resolver().Run(url), headers, response));
    return true;
  }

  void buildPagesList(const std::string& base, const std::string& path) {
    std::string file;
    std::size_t len = base.size();
    auto it = std::filesystem::directory_iterator(path);
    for(; it != std::filesystem::directory_iterator(); ++it) {
      if (it->is_directory()) {
        buildPagesList(base, (*it).path().generic_string());
      } else if (it->is_regular_file()) {
        file = (*it).path().generic_string().substr(len+1);
        pages.insert(file);
      }
    }
  }

  std::string GetURIDomain(const std::string& uri) {
    std::string suffix = "";
    std::vector<std::string> domain = split(uri.substr(0, uri.find("/")), ".");
    if (3 == domain.size() && ("www" == domain[0] || "ftp" == domain[0] || "git" == domain[0] || "raw" == domain[0])) {
      return domain[1]  + "." + domain[2];
    } else if (domain.size() >= 2) {
      for(size_t i = domain.size() - 1; i >= 0 && i < domain.size(); i--) {
        if (domain[i].size() > 3) {
          return domain[i]  + "." + suffix;
        } else {
          if (suffix.empty()) {
            suffix = domain[i];
          } else {
            suffix = domain[i] + "." + suffix;
          }
        }
      }

      return domain[domain.size()-2]  + "." + domain[domain.size()-1];
    }

    return "";
  }

private:
  bool debug;
  std::string basePath;
  std::set<std::string> pages;
  std::map<std::string, std::string> content;
  std::vector< std::pair<std::regex, std::string> > redirect_regex;

  DISALLOW_COPY_AND_ASSIGN(PagesResourceProvider);
};

// Provider that dumps the request contents.
class RequestDumpResourceProvider : public CefResourceManager::Provider {
 public:
  RequestDumpResourceProvider() = default;

  bool OnRequest(scoped_refptr<CefResourceManager::Request> request) override {
    CEF_REQUIRE_IO_THREAD();

    CefRefPtr<CefResourceHandler> handler;

    bool ready = false;
    const std::string dumpHeader = "/_/dump/header.html";
    const std::string& url = request->url();
    if (url.ends_with(dumpHeader)) {
      ready = true;
      handler = GetDumpHeader(request->request());
    }
    if (ready) {
      DCHECK(handler);

      request->Continue(handler);
    }

    return ready;
  }

 private:
  CefRefPtr<CefResourceHandler> GetDumpResponse(CefRefPtr<CefRequest> request) {
    std::string origin;
    CefResponse::HeaderMap response_headers;

    // Extract the origin request header, if any. It will be specified for cross-origin requests.
    {
      CefRequest::HeaderMap requestMap;
      request->GetHeaderMap(requestMap);

      CefRequest::HeaderMap::const_iterator it = requestMap.begin();
      for (; it != requestMap.end(); ++it) {
        const std::string& key = LowerString(it->first);
        if (key == "origin") {
          origin = it->second;
          break;
        }
      }
    }

    if (!origin.empty()) {
      // Allow cross-origin XMLHttpRequests from test origins.
      response_headers.insert(std::make_pair("Access-Control-Allow-Origin", origin));

      // Allow the custom header from the xmlhttprequest.html example.
      response_headers.insert(std::make_pair("Access-Control-Allow-Headers", "My-Custom-Header"));
	  }

    const std::string& dump = DumpRequestContents(request);
    std::string str = "<html><body bgcolor=\"white\"><pre>" + dump + "</pre></body></html>";
    CefRefPtr<CefStreamReader> stream = CefStreamReader::CreateForData(static_cast<void*>(const_cast<char*>(str.c_str())), str.size());
    CefRefPtr<CefResourceHandler> handler  = new CefStreamResourceHandler(200, "OK", "text/html", response_headers, stream);

    return handler;
  }

  CefRefPtr<CefResourceHandler> GetDumpHeader(CefRefPtr<CefRequest> request) {
    std::string str;
    std::stringstream ss;
    CefRequest::HeaderMap headerMap;

    request->GetHeaderMap(headerMap);
    if (headerMap.size() > 0) {
      ss << "[";
      CefRequest::HeaderMap::const_iterator it = headerMap.begin();
      for (; it != headerMap.end(); ++it) {
        ss << "{\"" << std::string((*it).first) << "\": \"" << std::string((*it).second) << "\"},";
      }
      str = ss.str();
      str.at(str.size()-1) = ']';
    } else {
      str = "";
    }

    CefResponse::HeaderMap response_headers;
    response_headers.insert(std::make_pair("Access-Control-Allow-Origin", "*"));

    CefRefPtr<CefStreamReader> stream = CefStreamReader::CreateForData(static_cast<void*>(const_cast<char*>(str.c_str())), str.size());
    CefRefPtr<CefResourceHandler> handler  = new CefStreamResourceHandler(200, "OK", "application/json", response_headers, stream);

    return handler;
  }

  std::string DumpRequestContents(CefRefPtr<CefRequest> request) {
    std::stringstream ss;

    ss << "URL: " << std::string(request->GetURL());
    ss << "\nMethod: " << std::string(request->GetMethod());

    CefRequest::HeaderMap headerMap;
    request->GetHeaderMap(headerMap);
    if (headerMap.size() > 0) {
      ss << "\nHeaders:";
      CefRequest::HeaderMap::const_iterator it = headerMap.begin();
      for (; it != headerMap.end(); ++it) {
        ss << "\n\t" << std::string((*it).first) << ": " << std::string((*it).second);
      }
    }

    CefRefPtr<CefPostData> postData = request->GetPostData();
    if (postData.get()) {
      CefPostData::ElementVector elements;
      postData->GetElements(elements);
      if (elements.size() > 0) {
        ss << "\nPost Data:";
        CefRefPtr<CefPostDataElement> element;
        CefPostData::ElementVector::const_iterator it = elements.begin();
        for (; it != elements.end(); ++it) {
          element = (*it);
          if (element->GetType() == PDE_TYPE_BYTES) {
            // the element is composed of bytes
            ss << "\n\tBytes: ";
            if (element->GetBytesCount() == 0) {
              ss << "(empty)";
            } else {
              // retrieve the data.
              size_t size = element->GetBytesCount();
              char* bytes = new char[size];
              element->GetBytes(size, bytes);
              ss << std::string(bytes, size);
              delete[] bytes;
            }
          } else if (element->GetType() == PDE_TYPE_FILE) {
            ss << "\n\tFile: " << std::string(element->GetFile());
          }
        }
      }
    }

    return ss.str();
  }

  DISALLOW_COPY_AND_ASSIGN(RequestDumpResourceProvider);
};

void SetupResourceManager(CefRefPtr<CefResourceManager> resource_manager, const std::vector< std::pair<std::string, std::string> >& redirect, const std::string& appPath, bool debug) {
  if (!CefCurrentlyOn(TID_IO)) {
    // Execute on the browser IO thread.
    CefPostTask(TID_IO, base::BindOnce(SetupResourceManager, resource_manager, redirect, appPath, debug));
    return;
  }

  // Add provider for page file resources.
  resource_manager->AddProvider(new PagesResourceProvider(redirect, appPath + "\\page", debug), 0, std::string());

  // Add provider for resource dumps.
  resource_manager->AddProvider(new RequestDumpResourceProvider(), 0, std::string());

  // Read resources from a directory on disk.
  resource_manager->AddDirectoryProvider("https://localhost/", appPath + "\\data", 100, std::string());
}