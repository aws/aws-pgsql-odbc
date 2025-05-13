// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef STRING_HELPER_H_
#define STRING_HELPER_H_

#ifdef WIN32
    #include <windows.h> // Required to include sql.h and sqlext.h below
    #include <tchar.h>
#else
    // On non-Windows platforms:
    #ifdef UNICODE
        #define TEXT(x) L##x
    #else
        #define TEXT(x) x
    #endif
#endif
#include <sql.h>

#include <cstring> // For narrow string functions like strlen, strncpy
#include <cwchar> // For wide string functions like wcslen, wcscpy
#include <cwctype> // For wide character functions like towupper
#include <locale>
#include <regex>
#include <string>

#define AS_SQLTCHAR(str) (const_cast<SQLTCHAR*>(reinterpret_cast<const SQLTCHAR*>(str)))
#define AS_CHAR(str) (reinterpret_cast<char*>(str))
#define AS_CONST_CHAR(str) (reinterpret_cast<const char*>(str))
#define AS_WCHAR(str) (reinterpret_cast<wchar_t*>(str))
#define AS_CONST_WCHAR(str) (reinterpret_cast<const wchar_t*>(str))
#define CONCATENATE(e1, e2) e1 ## e2

#ifdef UNICODE
    // Return L"s"
    #define CONSTRUCT_SQLSTR(s) CONCATENATE(L, s)
    typedef std::wstring SQLSTR;

    typedef wchar_t RDSCHAR;
    typedef std::wostringstream RDSSTRSTREAM;
    typedef std::wregex RDSREGEX;
    typedef std::wsmatch RDSSTRMATCH;
#else
    // No-op
    #define CONSTRUCT_SQLSTR(s) s
    typedef std::string SQLSTR;

    typedef char RDSCHAR;
    typedef std::ostringstream RDSSTRSTREAM;
    typedef std::regex RDSREGEX;
    typedef std::smatch RDSSTRMATCH;
#endif

#if defined(__APPLE__) || defined(__linux__)
#define strcmp_case_insensitive(str1, str2) strcasecmp(str1, str2)
#else
#define strcmp_case_insensitive(str1, str2) strcmpi(str1, str2)
#endif

class StringHelper {
public:
    /**
     * Converts a std::string to a std::wstring
     */
    static std::wstring ToWstring(const std::string& src) {
        if (src.empty()) {
            return std::wstring();
        }
        return std::wstring(src.begin(), src.end());
    }

    /**
     * Converts an SQLTCHAR array to a std::string
     */
    static std::string ToString(const SQLTCHAR *src) {
        #ifdef UNICODE
            std::wstring wstr = AS_CONST_WCHAR(src);
            return std::string(wstr.begin(), wstr.end());
        #else
            return std::string(AS_CONST_CHAR(src));
        #endif
    }

    /**
     * Converts a std::wstring to a std::string
     */
    static std::string ToString(const std::wstring& src) {
        if (src.empty()) {
            return std::string();
        }
        return std::string(src.begin(), src.end());
    }

    /**
     * Converts a std::string to a std::string (no-op)
     */
    static std::string ToString(const std::string& src) {
        return src;
    }

    /**
     * Converts an SQLTCHAR array to a SQLSTR (std::string for ANSI, std::wstring for Unicode)
     */
    static SQLSTR ToSQLSTR(const SQLTCHAR *src) {
        #ifdef UNICODE
            return std::wstring(AS_CONST_WCHAR(src));
        #else
            return std::string(AS_CONST_CHAR(src));
        #endif
    }

    /**
     * Converts a std::string to a SQLSTR (std::string for ANSI, std::wstring for Unicode)
     */
    static SQLSTR ToSQLSTR(const std::string &src) {
        #ifdef UNICODE
            if (src.empty()) {
                return std::wstring();
            }
            return std::wstring(src.begin(), src.end());
        #else
            return src;
        #endif
    }

    /**
     * Converts a std::wstring to a SQLSTR (std::string for ANSI, std::wstring for Unicode)
     */
    static SQLSTR ToSQLSTR(const std::wstring &src) {
        #ifdef UNICODE
            return src;
        #else
            if (src.empty()) {
                return std::string();
            }
            return std::string(src.begin(), src.end());
        #endif
    }

    /**
     * Returns the upper-case wide character for a given wide character, if one exists
     */
    static wchar_t ToUpper(wchar_t c) {
        return std::towupper(c);
    }

    /**
     * Returns the upper-case ASCII character for a given ASCII character, if one exists
     */
    static char ToUpper(char c) {
        return std::toupper(c);
    }

    /**
     * Merges two strings together, separated by a new line, if both aren't empty. Otherwise, returns the non-empty string, or an empty string if both are empty
     */
    static std::string MergeStrings(std::string a, std::string b) {
        if (a.empty()) return b;
        if (b.empty()) return a;
        return a + '\n' + b;
    }

    /**
     * Copies a string to a character array with an optional warning if it is truncated
     */
    static void CopyToCStr(std::string str, char *arr, size_t arr_size, const std::string &truncated_warning) {
        if (arr == nullptr) return;

        if (str.size() >= arr_size) {
            str = StringHelper::MergeStrings(truncated_warning, str);
        }

        strncpy(arr, str.c_str(), arr_size);
    }
};

#endif  // STRING_HELPER_H_
