#ifndef __MISC_H__
#define __MISC_H__
#pragma once

#include <map>
#include <string>
#include <vector>
#include <tchar.h>
#include <wchar.h>
#include <windows.h>
#include <shellapi.h>

#ifndef MAX_PATH_SIZE
    #define MAX_PATH_SIZE 1024
#endif

void Utf8ToWide(const char* utf8String, wchar_t* wideString, int wideSize);
std::wstring Utf8ToWide(const char* utf8String);
std::wstring Utf8ToWide(const std::string& utf8String);
void WideToUtf8(const wchar_t* wideString, char* utf8String, int utf8Size);
std::string WideToUtf8(const wchar_t* wideString);
std::string WideToUtf8(const std::wstring& wideString);
std::string LowerString(std::string str);
std::string UpperString(std::string str);
std::string ReplaceString(std::string subject, const std::string& search, const std::string& replace);
size_t replace(std::string& str, const std::string& from, const std::string& to);
std::vector<std::string> split(const std::string& str, const std::string& pattern);
std::string trim(const std::string& str, const std::string& whitespace = "\r\n\t ");
std::string GetFileContents(std::string file);
std::string GetExecutablePath();
std::string GetExecutableDirectory();
std::string GetRealPath(std::string path);
std::string GetAbsolutePath(std::string path);
bool IsProcessExist (std::string file);
std::map<DWORD, std::string> GetProcessList();
SHELLEXECUTEINFOA exec(std::string exeFile, int num = 3, bool show = true, bool wait = false, bool asAdmin = false, DWORD* code = nullptr);
std::string DosExec(std::string exeFile);

#endif