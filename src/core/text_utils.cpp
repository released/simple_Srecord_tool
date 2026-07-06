#include "text_utils.h"

#include <windows.h>

#include <cwctype>
#include <iomanip>
#include <sstream>
#include <vector>

namespace srecord_tool::core {

std::wstring Trim(std::wstring text) {
    size_t b = 0;
    size_t e = text.size();
    while (b < text.size() && iswspace(text[b])) {
        ++b;
    }
    while (e > b && iswspace(text[e - 1])) {
        --e;
    }
    return text.substr(b, e - b);
}

std::wstring ToLower(std::wstring text) {
    for (wchar_t& ch : text) {
        ch = static_cast<wchar_t>(towlower(ch));
    }
    return text;
}

bool ParseU64(const std::wstring& text, uint64_t* value, std::wstring* error) {
    std::wstring s = Trim(text);
    uint64_t out = 0;
    int base = 10;
    size_t idx = 0;

    if (value == nullptr) {
        if (error != nullptr) {
            *error = L"Internal parse error: output pointer is null.";
        }
        return false;
    }
    if (s.empty()) {
        if (error != nullptr) {
            *error = L"Value is empty.";
        }
        return false;
    }
    if (s.size() > 2u && s[0] == L'0' && (s[1] == L'x' || s[1] == L'X')) {
        base = 16;
        idx = 2;
    }
    if (idx >= s.size()) {
        if (error != nullptr) {
            *error = L"Value has no digits.";
        }
        return false;
    }

    for (; idx < s.size(); ++idx) {
        wchar_t ch = s[idx];
        int digit = -1;
        if (ch >= L'0' && ch <= L'9') {
            digit = ch - L'0';
        } else if (base == 16 && ch >= L'a' && ch <= L'f') {
            digit = 10 + ch - L'a';
        } else if (base == 16 && ch >= L'A' && ch <= L'F') {
            digit = 10 + ch - L'A';
        } else {
            if (error != nullptr) {
                *error = L"Invalid number: " + text;
            }
            return false;
        }
        if (digit >= base) {
            if (error != nullptr) {
                *error = L"Invalid digit for number: " + text;
            }
            return false;
        }
        if (out > (UINT64_MAX - static_cast<uint64_t>(digit)) / static_cast<uint64_t>(base)) {
            if (error != nullptr) {
                *error = L"Number is too large: " + text;
            }
            return false;
        }
        out = out * static_cast<uint64_t>(base) + static_cast<uint64_t>(digit);
    }

    *value = out;
    return true;
}

std::wstring FormatHex(uint64_t value, int min_digits) {
    std::wstringstream ss;
    ss << L"0x" << std::uppercase << std::hex << std::setfill(L'0');
    if (min_digits > 0) {
        ss << std::setw(min_digits);
    }
    ss << value;
    return ss.str();
}

std::wstring QuoteArg(const std::wstring& arg) {
    if (arg.empty()) {
        return L"\"\"";
    }

    bool needs_quote = false;
    for (wchar_t ch : arg) {
        if (iswspace(ch) || ch == L'"') {
            needs_quote = true;
            break;
        }
    }
    if (!needs_quote) {
        return arg;
    }

    std::wstring out = L"\"";
    size_t slash_count = 0;
    for (wchar_t ch : arg) {
        if (ch == L'\\') {
            ++slash_count;
            continue;
        }
        if (ch == L'"') {
            out.append(slash_count * 2 + 1, L'\\');
            out.push_back(ch);
        } else {
            out.append(slash_count, L'\\');
            out.push_back(ch);
        }
        slash_count = 0;
    }
    out.append(slash_count * 2, L'\\');
    out.push_back(L'"');
    return out;
}

std::wstring JoinCommandLine(const std::vector<std::wstring>& args) {
    std::wstring out;
    for (const auto& arg : args) {
        if (!out.empty()) {
            out.push_back(L' ');
        }
        out += QuoteArg(arg);
    }
    return out;
}

std::wstring DirectoryOf(const std::wstring& path) {
    size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return L"";
    }
    return path.substr(0, pos);
}

std::wstring FileNameWithoutExtension(const std::wstring& path) {
    size_t slash = path.find_last_of(L"\\/");
    size_t start = (slash == std::wstring::npos) ? 0 : slash + 1;
    size_t dot = path.find_last_of(L'.');
    if (dot == std::wstring::npos || dot < start) {
        return path.substr(start);
    }
    return path.substr(start, dot - start);
}

std::wstring ExtensionOf(const std::wstring& path) {
    size_t slash = path.find_last_of(L"\\/");
    size_t start = (slash == std::wstring::npos) ? 0 : slash + 1;
    size_t dot = path.find_last_of(L'.');
    if (dot == std::wstring::npos || dot < start) {
        return L"";
    }
    return ToLower(path.substr(dot));
}

std::wstring CombinePath(const std::wstring& dir, const std::wstring& name) {
    if (dir.empty()) {
        return name;
    }
    if (dir.back() == L'\\' || dir.back() == L'/') {
        return dir + name;
    }
    return dir + L"\\" + name;
}

bool FileExists(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool GetFileSizeBytes(const std::wstring& path, uint64_t* size) {
    WIN32_FILE_ATTRIBUTE_DATA data = {};
    ULARGE_INTEGER uli = {};
    if (size == nullptr) {
        return false;
    }
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data)) {
        return false;
    }
    uli.HighPart = data.nFileSizeHigh;
    uli.LowPart = data.nFileSizeLow;
    *size = uli.QuadPart;
    return true;
}

std::wstring LastWriteTimeText(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA data = {};
    FILETIME local_ft = {};
    SYSTEMTIME st = {};
    wchar_t buf[64] = {};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data)) {
        return L"-";
    }
    if (!FileTimeToLocalFileTime(&data.ftLastWriteTime, &local_ft)) {
        return L"-";
    }
    if (!FileTimeToSystemTime(&local_ft, &st)) {
        return L"-";
    }
    swprintf_s(buf, L"%04u-%02u-%02u %02u:%02u:%02u",
               st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return buf;
}

} // namespace srecord_tool::core
