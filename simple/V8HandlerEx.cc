#include <string>
#include <thread>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <filesystem>

#include "misc.h"
#include "V8HandlerEx.h"

#include "libcef/include/cef_task.h"
#include "libcef/include/base/cef_callback.h"
#include "libcef/include/wrapper/cef_closure_task.h"
#include "libcef/include/wrapper/cef_stream_resource_handler.h"

bool V8HandlerEx::Execute(const CefString& name,
    CefRefPtr<CefV8Value> object,
    const CefV8ValueList& arguments,
    CefRefPtr<CefV8Value>& retval,
    CefString& exception) {

    namespace fs = std::filesystem;

    if (name == "GetVersion") {
        retval = CefV8Value::CreateString("1.0.0." + getDateFromMacro(__DATE__, __TIME__));
        return true;
    } else if (name == "GetAppPath") {
        retval = CefV8Value::CreateString(appPath);
        return true;
    } else if (name == "Sleep") {
        if (arguments.size() >= 1 && arguments[0]->IsInt() && arguments[0]->GetIntValue() > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(arguments[0]->GetIntValue()));
        } else {
            exception = "sleep time be greate than 0";
        }

        return true;
    } else if (name == "IsFile") {
        if (1 == arguments.size() && arguments[0]->IsString()) {
            std::string param = arguments.at(0)->GetStringValue().ToString();

            if (0 == param.find(dataPath) && std::string::npos == param.find("..")) {
                bool isOk = fs::is_regular_file(fs::status(param));
                retval = CefV8Value::CreateBool(isOk);
            } else {
                exception = "file path must in data path";
            }
        } else {
            exception = "file path is empty or not a string";
        }

        return true;
    } else if (name == "IsDir") {
        if (1 == arguments.size() && arguments[0]->IsString()) {
            std::string param = arguments.at(0)->GetStringValue().ToString();

            if (0 == param.find(dataPath) && std::string::npos == param.find("..")) {
                bool isOk = fs::is_directory(fs::status(param));
                retval = CefV8Value::CreateBool(isOk);
            } else {
                exception = "directory path must in data path";
            }
        } else {
            exception = "directory path is empty or not a string";
        }

        return true;
    } else if (name == "MkDir") {
        if (1 == arguments.size() && arguments[0]->IsString()) {
            std::string param = arguments.at(0)->GetStringValue().ToString();

            if (0 == param.find(dataPath) && std::string::npos == param.find("..")) {
                bool isOk = fs::create_directory(param);
                retval = CefV8Value::CreateBool(isOk);
            } else {
                exception = "directory path must in data path";
            }
        } else {
            exception = "directory path is empty or not a string";
        }

        return true;
    } else if (name == "GetDirFiles") {
        if (1 == arguments.size() && arguments[0]->IsString()) {
            int idx = 0, fileNum = 0;
            std::vector<std::string> files;
            std::string param = arguments.at(0)->GetStringValue().ToString();

            if (0 == param.find(dataPath) && std::string::npos == param.find("..")) {
                auto it = fs::directory_iterator(param);
                for(; it != fs::directory_iterator(); ++it) {
                    if (it->is_regular_file()) {
                        files.push_back((*it).path().generic_string());
                    }
                }

                fileNum = (int) files.size();
                retval = CefV8Value::CreateArray(fileNum);

                for (std::string v : files) {
                    retval->SetValue(idx, CefV8Value::CreateString(v));
                    idx++;
                }
            } else {
                exception = "directory path must in data path";
            }

        } else {
            exception = "directory path is empty or not a string";
        }

        return true;
    } else if (name == "FileGetContents") {
        if (arguments.size() > 0 && arguments[0]->IsString()) {
            std::string param = arguments.at(0)->GetStringValue().ToString();

            if (0 == param.find(dataPath) && std::string::npos == param.find("..")) {
                bool isOk = fs::is_regular_file(fs::status(param));

                if (isOk) {
                    int idx = 0;
                    std::string content;
                    std::ifstream inputFile(param, std::ios::binary);

                    if (inputFile.good()) {
                        content.assign(std::istreambuf_iterator<char>(inputFile), std::istreambuf_iterator<char>());
                        inputFile.close();

                        if (arguments.size() > 1 && arguments[1]->IsBool() && arguments.at(1)->GetBoolValue()) {
                            retval = CefV8Value::CreateArray((int) content.size());
                            for (char v : content) {
                                retval->SetValue(idx, CefV8Value::CreateUInt((uint8_t) v));
                                idx++;
                            }
                        } else {
                            retval = CefV8Value::CreateString(content);
                        }
                    } else {
                        exception = "file open failed";
                    }
                } else {
                    exception = "file is not exists";
                }
            } else {
                exception = "file must in data path";
            }
        } else {
            exception = "file path is empty or not a string";
        }

        return true;
    } else if (name == "FilePutContents") {
        if (arguments.size() >= 2 && arguments[0]->IsString() && arguments[1]->IsString()) {
            std::ofstream output;
            std::string param = arguments.at(0)->GetStringValue().ToString();

            if (0 == param.find(dataPath) && std::string::npos == param.find("..")) {
                if (3 == arguments.size() && arguments[2]->IsBool() && arguments.at(2)->GetBoolValue()) {
                    output = std::ofstream(param, std::ios_base::binary | std::ios_base::app);
                } else {
                    output = std::ofstream(param, std::ios_base::binary | std::ios_base::trunc);
                }

                if (output.good()) {
                    output << arguments.at(1)->GetStringValue().ToString();
                    if (output.good()) {
                        output.close();
                        retval = CefV8Value::CreateBool(true);
                    } else {
                        exception = "file save failed";
                    }
                } else {
                    exception = "file open failed";
                }
            } else {
                exception = "file path must in data path";
            }
        } else {
            exception = "file path is empty or not a string";
        }

        return true;
    } else if (name == "FileSize") {
        if (1 == arguments.size() && arguments[0]->IsString()) {
            int fileSize = 0;
            std::string param = arguments.at(0)->GetStringValue().ToString();

            if (0 == param.find(dataPath) && std::string::npos == param.find("..")) {
                bool isOk = fs::is_regular_file(fs::status(param));

                if (isOk) {
                    fileSize = (int) fs::file_size(fs::path(arguments.at(0)->GetStringValue().ToString()));
                }

                retval = CefV8Value::CreateInt(fileSize);
            } else {
                exception = "file path must in data path";
            }
        } else {
            exception = "file path is empty or not a string";
        }

        return true;
    } else if (name == "FileRename") {
        if (arguments.size() == 2 && arguments[0]->IsString() && arguments[1]->IsString()) {
            std::string source = arguments.at(0)->GetStringValue().ToString();
            std::string target = arguments.at(1)->GetStringValue().ToString();

            if (0 == source.find(dataPath) && 0 == target.find(dataPath) && std::string::npos == source.find("..") && std::string::npos == target.find("..")) {
                if (fs::is_regular_file(fs::status(source))) {
                    fs::rename(fs::path(), fs::path(target));

                    retval = CefV8Value::CreateBool(true);
                } else {
                    exception = "source file is not exists";
                }
            } else {
                exception = "file path must in data path";
            }
        } else {
            exception = "source file and target file is empty or not a string";
        }

        return true;
    } else if (name == "FileRemove") {
        if (1 == arguments.size() && arguments[0]->IsString()) {
            std::string param = arguments.at(0)->GetStringValue().ToString();

            if (0 == param.find(dataPath) && std::string::npos == param.find("..")) {
                bool isOk = fs::is_regular_file(fs::status(param));

                if (isOk) {
                    isOk = fs::remove(fs::path(arguments.at(0)->GetStringValue().ToString()));
                } else {
                    isOk = true;
                }

                retval = CefV8Value::CreateBool(isOk);
            } else {
                exception = "file path must in data path";
            }
        } else {
            exception = "file path is empty or not a string";
        }

        return true;
    } else if (name == "Execute") {
        if (arguments.size() > 0 && arguments[0]->IsString()) {
            DWORD code = 999999;
            std::string waitFile = "";
            std::string winOperator = "open";
            std::string param = arguments.at(0)->GetStringValue().ToString();

            SHELLEXECUTEINFOA shellInfo = exec(param, 3, false, true, false, &code);
            retval = CefV8Value::CreateUInt(code);
        } else {
            exception = "file path is empty or not a string";
        }

        return true;
    }

    // Function does not exist.
    return false;
}

std::string V8HandlerEx::getDateFromMacro(char const *date, char const *time) {
    char s_month[5];
    struct tm t = {0};
    int month, day, year, hour, minute, second;
    static const char monthNames[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

    sscanf(date, "%s %d %d", s_month, &day, &year);
    sscanf(time, "%d:%d:%d", &hour, &minute, &second);
    month = (strstr(monthNames, s_month)-monthNames)/3;

    t.tm_mon = month;
    t.tm_mday = day;
    t.tm_year = year - 1900;
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = second;
    t.tm_isdst = -1;

    return std::to_string(mktime(&t));
}