#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace srecord_tool::core {

enum class ConvertOperation {
    BinToBin = 0,
    BinToHex = 1,
    HexToBin = 2
};

struct SRecordOptions {
    std::wstring srec_exe = L"srec_cat";
    std::wstring input_path;
    std::wstring output_path;
    std::wstring output_name_tag = L"crc";
    bool hex_copy_enabled = true;
    std::wstring hex_output_path;
    ConvertOperation operation = ConvertOperation::BinToBin;
    bool offset_enabled = false;
    bool pad_enabled = true;
    bool checksum_enabled = false;
    std::wstring base_address = L"0x0000";
    std::wstring offset = L"0x0000";
    std::wstring image_size = L"0x20000";
    std::wstring fill_byte = L"0xFF";
    std::wstring checksum_algorithm = L"-crc32-l-e";
};

struct SRecordDerived {
    uint64_t base_address = 0;
    uint64_t offset = 0;
    uint64_t image_size = 0;
    uint64_t image_end = 0;
    uint64_t checksum_address = 0;
    uint64_t checksum_end = 0;
    uint64_t checksum_offset_in_input = 0;
    uint64_t app_binary_size = 0;
    uint64_t fill_byte = 0xFF;
};

struct SRecordCommand {
    std::vector<std::wstring> args;
    std::wstring command_line;
    SRecordDerived derived;
};

const wchar_t* OperationName(ConvertOperation op);
const wchar_t* InputFormatArg(ConvertOperation op);
const wchar_t* OutputFormatArg(ConvertOperation op);
const wchar_t* DefaultChecksumAlgorithm();
bool IsSupportedChecksumAlgorithm(const std::wstring& algorithm);
const wchar_t* DefaultOutputNameTag();
std::wstring DefaultOutputPath(const std::wstring& input_path, ConvertOperation op);
std::wstring DefaultOutputPath(const std::wstring& input_path, ConvertOperation op, const std::wstring& output_name_tag);
bool InferOperationFromPath(const std::wstring& path, ConvertOperation* op);
bool BuildConvertCommand(const SRecordOptions& options, SRecordCommand* command, std::wstring* error = nullptr);
bool BuildChecksumDumpCommand(const SRecordOptions& options, const SRecordDerived& derived, SRecordCommand* command, std::wstring* error = nullptr);
bool BuildHexCopyCommand(const SRecordOptions& options, const SRecordDerived& derived, SRecordCommand* command, std::wstring* error = nullptr);
std::wstring DescribeDerived(const SRecordOptions& options, const SRecordDerived& derived);

} // namespace srecord_tool::core
