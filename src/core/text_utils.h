#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace srecord_tool::core {

std::wstring Trim(std::wstring text);
std::wstring ToLower(std::wstring text);
bool ParseU64(const std::wstring& text, uint64_t* value, std::wstring* error = nullptr);
std::wstring FormatHex(uint64_t value, int min_digits = 0);
std::wstring QuoteArg(const std::wstring& arg);
std::wstring JoinCommandLine(const std::vector<std::wstring>& args);
std::wstring DirectoryOf(const std::wstring& path);
std::wstring FileNameWithoutExtension(const std::wstring& path);
std::wstring ExtensionOf(const std::wstring& path);
std::wstring CombinePath(const std::wstring& dir, const std::wstring& name);
bool FileExists(const std::wstring& path);
bool GetFileSizeBytes(const std::wstring& path, uint64_t* size);
std::wstring LastWriteTimeText(const std::wstring& path);

} // namespace srecord_tool::core
