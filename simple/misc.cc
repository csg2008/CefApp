#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <shlobj.h>
#include <comutil.h>
#include <WbemIdl.h>
#include <TlHelp32.h>
#include <shellapi.h>
#include <windows.h>

#include "misc.h"

void Utf8ToWide(const char* utf8String, wchar_t* wideString, int wideSize) {
    int copiedCharacters = MultiByteToWideChar(CP_UTF8, 0, utf8String, -1, wideString, wideSize);
    if (copiedCharacters == 0)
        wideString[0] = 0;
}
std::wstring Utf8ToWide(const char* utf8String) {
    int requiredSize = MultiByteToWideChar(CP_UTF8, 0, utf8String, -1, 0, 0);
    wchar_t* wideString = new wchar_t[requiredSize];
    int copiedCharacters = MultiByteToWideChar(CP_UTF8, 0, utf8String, -1, wideString, requiredSize);
    std::wstring returnedString(wideString, copiedCharacters - 1);
    delete[] wideString;
    wideString = 0;
    return returnedString;
}
std::wstring Utf8ToWide(const std::string& utf8String) {
    int requiredSize = MultiByteToWideChar(CP_UTF8, 0, utf8String.c_str(), -1, 0, 0);
    wchar_t* wideString = new wchar_t[requiredSize];
    int copiedCharacters = MultiByteToWideChar(CP_UTF8, 0, utf8String.c_str(), -1, wideString, requiredSize);
    std::wstring returnedString(wideString, copiedCharacters - 1);
    delete[] wideString;
    wideString = 0;
    return returnedString;
}
void WideToUtf8(const wchar_t* wideString, char* utf8String, int utf8Size) {
    int copiedBytes = WideCharToMultiByte(CP_UTF8, 0, wideString, -1, utf8String, utf8Size, NULL, NULL);
    if (copiedBytes == 0)
        utf8String[0] = 0;
}
std::string WideToUtf8(const wchar_t* wideString) {
    int requiredSize = WideCharToMultiByte(CP_UTF8, 0, wideString, -1, 0, 0, NULL, NULL);
    char* utf8String = new char[requiredSize];
    int copiedBytes = WideCharToMultiByte(CP_UTF8, 0, wideString, -1, utf8String, requiredSize, NULL, NULL);
    std::string returnedString(utf8String, copiedBytes - 1);
    delete[] utf8String;
    utf8String = 0;
    return returnedString;
}
std::string WideToUtf8(const std::wstring& wideString) {
    int requiredSize = WideCharToMultiByte(CP_UTF8, 0, wideString.c_str(), -1, 0, 0, NULL, NULL);
    char* utf8String = new char[requiredSize];
    int copiedBytes = WideCharToMultiByte(CP_UTF8, 0, wideString.c_str(), -1, utf8String, requiredSize, NULL, NULL);
    std::string returnedString(utf8String, copiedBytes - 1);
    delete[] utf8String;
    utf8String = 0;
    return returnedString;
}
std::string LowerString(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}
std::string UpperString(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}
std::string ReplaceString(std::string subject, const std::string& search, const std::string& replace) {
    size_t pos = 0;
    while((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
    return subject;
}
/**
 * Replaces occurrences of string with another string.
 * Example:  string "Hello "World". from: World, to: "El Mundo"
 *        replace(str, from, to) --> string eq. "Hello El Mundo";
 *
 * WARNING: This function is not necessarily efficient.
 * @param str to change
 * @param from substring to match
 * @param to replace the matching substring with
 * @return number of replacements made
**/
size_t replace(std::string& str, const std::string& from, const std::string& to) {
    if (from.empty() || str.empty()) {
        return 0;
    }

    size_t pos = 0;
    size_t count = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.length(), to);
        pos += to.length();
        ++count;
    }

    return count;
}

/** Trims a string's end and beginning from specified characters
*        such as tab, space and newline i.e. "\t \n" (tab space newline)
* @param str to clean
* @param whitespace by default space and tab character
* @return the cleaned string */
std::string trim(const std::string& str, const std::string& whitespace) {
    const auto& begin = str.find_first_not_of(whitespace);
    if (std::string::npos == begin) {
        return {};
    }

    const auto& end = str.find_last_not_of(whitespace);
    const auto& range = end - begin + 1;
    return str.substr(begin, range);
}

// split string to vector
std::vector<std::string> split(const std::string& str, const std::string& pattern) {
    std::vector<std::string> result;
    std::string::size_type pos1, pos2;

    pos1 = 0;
    pos2 = str.find(pattern);

    while(std::string::npos != pos2) {
        result.push_back(str.substr(pos1, pos2-pos1));

        pos1 = pos2 + pattern.size();
        pos2 = str.find(pattern, pos1);
    }
    if(pos1 != str.length())
        result.push_back(str.substr(pos1));

    return result;
}

std::string GetFileContents(std::string file) {
    std::ifstream inFile;
    inFile.open(Utf8ToWide(file).c_str(), std::ios::in);
    if (!inFile) {
        return "";
    }

    std::string contents;
    inFile.seekg(0, std::ios::end);
    contents.resize(static_cast<unsigned int>(inFile.tellg()));
    inFile.seekg(0, std::ios::beg);
    inFile.read(&contents[0], contents.size());
    inFile.close();
    return contents;
}

std::string GetExecutablePath() {
    wchar_t path[MAX_PATH_SIZE];
    std::wstring wideString;
    if (GetModuleFileName(NULL, path, _countof(path))) {
        if (GetLongPathName(path, path, _countof(path))){
            wideString = path;
        }
    }

    return WideToUtf8(wideString);
}

std::string GetExecutableDirectory() {
    std::string path = GetExecutablePath();
    wchar_t drive[_MAX_DRIVE];
    wchar_t directory[_MAX_DIR];
    wchar_t filename[_MAX_FNAME];
    wchar_t extension[_MAX_EXT];
    _wsplitpath_s(Utf8ToWide(path).c_str(),
            drive, _countof(drive),
            directory, _countof(directory),
            filename, _countof(filename),
            extension, _countof(extension));
    char utf8Drive[_MAX_DRIVE * 2];
    WideToUtf8(drive, utf8Drive, _countof(utf8Drive));
    char utf8Directory[_MAX_DIR * 2];
    WideToUtf8(directory, utf8Directory, _countof(utf8Directory));
    path.assign(utf8Drive).append(utf8Directory);
    if (path.length()) {
        char lastCharacter = path[path.length() - 1];
        if (lastCharacter == '\\' || lastCharacter == '/') {
            path.erase(path.length() - 1);
        }
    }

    return path;
}

std::string GetRealPath(std::string path) {
    wchar_t realPath[MAX_PATH_SIZE];
    GetFullPathName(Utf8ToWide(path).c_str(), _countof(realPath), realPath, NULL);
    return WideToUtf8(realPath);
}

std::string GetAbsolutePath(std::string path) {
    if (path.length() && path.find(":") == std::string::npos) {
        path = GetExecutableDirectory() + "\\" + path;
        path = GetRealPath(path);
    }

    return path;
}

SHELLEXECUTEINFOA exec(std::string exeFile, int num, bool show, bool wait, bool asAdmin, DWORD* code) {
    int pid = 0;
    int times = 0;
    std::string::size_type pos = LowerString(exeFile).find(".exe");

    SHELLEXECUTEINFOA seInfo = { 0 };;
    seInfo.cbSize = sizeof(seInfo);
    seInfo.lpDirectory = GetExecutableDirectory().c_str();
    seInfo.fMask = SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_DDEWAIT | SEE_MASK_UNICODE;
    seInfo.hwnd = 0;
    seInfo.hInstApp = 0;

    if (asAdmin)
        seInfo.lpVerb = "runas";                // Operation to perform
    else
        seInfo.lpVerb = "open";

    if (std::string::npos == pos || pos + 4 >= exeFile.length()) {
        seInfo.lpFile = exeFile.c_str();
        seInfo.lpParameters = "";                  // Additional parameters
    } else {
        seInfo.lpFile = exeFile.substr(0, pos+4).c_str();
        seInfo.lpParameters = exeFile.substr(pos+4).c_str();
    }

    if (show) {
        seInfo.nShow = SW_SHOW;
    } else {
        seInfo.nShow = SW_HIDE;
    }

    while (times < num) {
        if (ShellExecuteExA(&seInfo)) {
            pid = GetProcessId(seInfo.hProcess);

            if (wait) {
                WaitForSingleObject(seInfo.hProcess, INFINITE);
                if (code != nullptr) {
                    ::GetExitCodeProcess(seInfo.hProcess, code);
                }
            }

            break;
        } else {
            times++;

            if (code != nullptr) {
                *code = GetLastError();
            }
        }
    }

    return seInfo;
}

/* Execute a DOS command.

 If the function succeeds, the return value is a non-NULL pointer to the output of the invoked command.
 Command will produce a 8-bit characters stream using OEM code-page.

 As charset depends on OS config (ex: CP437 [OEM-US/latin-US], CP850 [OEM 850/latin-1]),
 before being returned, output is converted to a wide-char string with function OEMtoUNICODE.

 Resulting buffer is allocated with LocalAlloc.
 It is the caller's responsibility to free the memory used by the argument list when it is no longer needed.
 To free the memory, use a single call to LocalFree function.
*/
std::string DosExec(std::string exeFile){
    // Allocate 1Mo to store the output (final buffer will be sized to actual output)
    // If output exceeds that size, it will be truncated
    const SIZE_T RESULT_SIZE = sizeof(char)*1024*1024;
    char* output = (char*) LocalAlloc(LPTR, RESULT_SIZE);

    HANDLE readPipe, writePipe;
    SECURITY_ATTRIBUTES security;
    STARTUPINFOW        start;
    PROCESS_INFORMATION processInfo;

    security.nLength = sizeof(SECURITY_ATTRIBUTES);
    security.bInheritHandle = true;
    security.lpSecurityDescriptor = NULL;

    LPWSTR exeFilePtr = const_cast<LPWSTR>(Utf8ToWide(exeFile).c_str());
    if ( CreatePipe(
                    &readPipe,  // address of variable for read handle
                    &writePipe, // address of variable for write handle
                    &security,  // pointer to security attributes
                    0           // number of bytes reserved for pipe
                    ) ){


        GetStartupInfoW(&start);
        start.hStdOutput  = writePipe;
        start.hStdError   = writePipe;
        start.hStdInput   = readPipe;
        start.dwFlags     = STARTF_USESTDHANDLES + STARTF_USESHOWWINDOW;
        start.wShowWindow = SW_SHOW;

        // We have to start the DOS app the same way cmd.exe does (using the current Win32 ANSI code-page).
        // So, we use the "ANSI" version of createProcess, to be able to pass a LPSTR (single/multi-byte character string)
        // instead of a LPWSTR (wide-character string) and we use the UNICODEtoANSI function to convert the given command
        if (CreateProcessW(NULL,                    // pointer to name of executable module
                           exeFilePtr,  // pointer to command line string
                           &security,               // pointer to process security attributes
                           &security,               // pointer to thread security attributes
                           TRUE,                    // handle inheritance flag
                           NORMAL_PRIORITY_CLASS,   // creation flags
                           NULL,                    // pointer to new environment block
                           NULL,                    // pointer to current directory name
                           &start,                  // pointer to STARTUPINFO
                           &processInfo             // pointer to PROCESS_INFORMATION
                         )){

            // wait for the child process to start
            for(UINT state = WAIT_TIMEOUT; state == WAIT_TIMEOUT; state = WaitForSingleObject(processInfo.hProcess, 100) );

            DWORD bytesRead = 0, count = 0;
            const int BUFF_SIZE = 1024;
            char* buffer = (char*) malloc(sizeof(char)*BUFF_SIZE+1);
            strcpy(output, "");
            do {
                DWORD dwAvail = 0;
                if (!PeekNamedPipe(readPipe, NULL, 0, NULL, &dwAvail, NULL)) {
                    // error, the child process might have ended
                    break;
                }
                if (!dwAvail) {
                    // no data available in the pipe
                    break;
                }
                ReadFile(readPipe, buffer, BUFF_SIZE, &bytesRead, NULL);
                buffer[bytesRead] = '\0';
                if((count+bytesRead) > RESULT_SIZE) break;
                strcat(output, buffer);
                count += bytesRead;
            } while (bytesRead >= BUFF_SIZE);
            free(buffer);
        }
    }

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    CloseHandle(writePipe);
    CloseHandle(readPipe);

    std::string outStr = std::string(output);

    LocalFree(output);

    return outStr;
}

std::map<DWORD, std::string> GetProcessList () {
    HANDLE hSnapShot = ::CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe32 { sizeof (PROCESSENTRY32) };
    std::map<DWORD, std::string> m;
    if (::Process32First (hSnapShot, &pe32)) {
        do {
            m[pe32.th32ProcessID] = WideToUtf8(pe32.szExeFile);
        } while (::Process32Next (hSnapShot, &pe32));
    }
    ::CloseHandle (hSnapShot);
    return m;
}

bool IsProcessExist (std::string file) {
    HANDLE hSnapShot = ::CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe32 { sizeof (PROCESSENTRY32) };
    std::map<DWORD, std::string> m;
    if (::Process32First (hSnapShot, &pe32)) {
        do {
            std::string _path = WideToUtf8(pe32.szExeFile);
            size_t p = _path.rfind (_T ('\\'));
            if (_path.substr (p + 1) == file) {
                ::CloseHandle (hSnapShot);
                return true;
            }
        } while (::Process32Next (hSnapShot, &pe32));
    }
    ::CloseHandle (hSnapShot);
    return false;
}
