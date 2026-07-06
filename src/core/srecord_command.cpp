#include "srecord_command.h"

#include <limits>
#include <sstream>

#include "text_utils.h"

namespace srecord_tool::core {
namespace {

constexpr const wchar_t* kDefaultChecksumAlgorithm = L"-crc32-l-e";
constexpr const wchar_t* kDefaultOutputNameTag = L"crc";

const wchar_t* kSupportedChecksumAlgorithms[] = {
    L"-crc32-l-e",
    L"-crc32-b-e",
    L"-checksum-n-l-e",
    L"-checksum-n-b-e",
    L"-checksum-p-l-e",
    L"-checksum-p-b-e"
};

bool ParseOptions(const SRecordOptions& options, SRecordDerived* derived, std::wstring* error) {
    uint64_t base = 0;
    uint64_t offset = 0;
    uint64_t image_size = 0;
    uint64_t fill = 0;
    std::wstring parse_error;

    if (!ParseU64(options.base_address, &base, &parse_error)) {
        if (error != nullptr) {
            *error = L"Base address: " + parse_error;
        }
        return false;
    }
    if (!ParseU64(options.offset, &offset, &parse_error)) {
        if (error != nullptr) {
            *error = L"Offset/App start: " + parse_error;
        }
        return false;
    }
    if (!ParseU64(options.image_size, &image_size, &parse_error)) {
        if (error != nullptr) {
            *error = L"Image size: " + parse_error;
        }
        return false;
    }
    if (!ParseU64(options.fill_byte, &fill, &parse_error)) {
        if (error != nullptr) {
            *error = L"Fill byte: " + parse_error;
        }
        return false;
    }

    if (fill > 0xFFu) {
        if (error != nullptr) {
            *error = L"Fill byte must be 0x00..0xFF.";
        }
        return false;
    }
    if (image_size < 4u) {
        if (error != nullptr) {
            *error = L"Image size must be at least 4 bytes.";
        }
        return false;
    }
    if (base > std::numeric_limits<uint64_t>::max() - image_size) {
        if (error != nullptr) {
            *error = L"Base address + image size overflows.";
        }
        return false;
    }

    const uint64_t image_end = base + image_size;
    const uint64_t checksum_address = image_end - 4u;
    if (options.offset_enabled && offset > checksum_address) {
        if (error != nullptr) {
            *error = L"Offset/App start is beyond checksum address.";
        }
        return false;
    }

    const uint64_t app_start = options.offset_enabled ? offset : base;
    if (app_start > checksum_address) {
        if (error != nullptr) {
            *error = L"App start is beyond checksum address.";
        }
        return false;
    }

    derived->base_address = base;
    derived->offset = app_start;
    derived->image_size = image_size;
    derived->image_end = image_end;
    derived->checksum_address = checksum_address;
    derived->checksum_end = image_end;
    derived->checksum_offset_in_input = checksum_address - app_start;
    derived->app_binary_size = derived->checksum_offset_in_input + 4u;
    derived->fill_byte = fill;
    return true;
}

void AppendInput(std::vector<std::wstring>* args, const SRecordOptions& options) {
    args->push_back(options.input_path);
    args->push_back(InputFormatArg(options.operation));
    if (options.operation != ConvertOperation::BinToBin &&
        options.offset_enabled && !options.offset.empty() && options.offset != L"0" && options.offset != L"0x0" && options.offset != L"0x0000") {
        uint64_t offset = 0;
        std::wstring ignored;
        if (ParseU64(options.offset, &offset, &ignored) && offset != 0) {
            args->push_back(L"-offset");
            args->push_back(FormatHex(offset));
        }
    }
}

void AppendOutput(std::vector<std::wstring>* args, const SRecordOptions& options) {
    args->push_back(L"-o");
    args->push_back(options.output_path);
    args->push_back(OutputFormatArg(options.operation));
}

bool ChecksumAlgorithmNeedsSize(const std::wstring& algorithm) {
    return algorithm.find(L"-checksum-") == 0u;
}

std::wstring OutputNameSuffix(const std::wstring& output_name_tag) {
    std::wstring tag = Trim(output_name_tag);
    if (tag.empty()) {
        tag = kDefaultOutputNameTag;
    }
    if (!tag.empty() && tag.front() == L'_') {
        return tag;
    }
    return L"_" + tag;
}

} // namespace

const wchar_t* OperationName(ConvertOperation op) {
    switch (op) {
    case ConvertOperation::BinToBin:
        return L"BIN -> BIN";
    case ConvertOperation::BinToHex:
        return L"BIN -> HEX";
    case ConvertOperation::HexToBin:
        return L"HEX -> BIN";
    default:
        return L"Unknown";
    }
}

const wchar_t* InputFormatArg(ConvertOperation op) {
    return op == ConvertOperation::HexToBin ? L"-Intel" : L"-binary";
}

const wchar_t* OutputFormatArg(ConvertOperation op) {
    return op == ConvertOperation::BinToHex ? L"-Intel" : L"-binary";
}

const wchar_t* DefaultChecksumAlgorithm() {
    return kDefaultChecksumAlgorithm;
}

bool IsSupportedChecksumAlgorithm(const std::wstring& algorithm) {
    for (const wchar_t* supported : kSupportedChecksumAlgorithms) {
        if (algorithm == supported) {
            return true;
        }
    }
    return false;
}

const wchar_t* DefaultOutputNameTag() {
    return kDefaultOutputNameTag;
}

std::wstring DefaultOutputPath(const std::wstring& input_path, ConvertOperation op) {
    return DefaultOutputPath(input_path, op, DefaultOutputNameTag());
}

std::wstring DefaultOutputPath(const std::wstring& input_path, ConvertOperation op, const std::wstring& output_name_tag) {
    std::wstring dir = DirectoryOf(input_path);
    std::wstring stem = FileNameWithoutExtension(input_path);
    std::wstring ext = op == ConvertOperation::BinToHex ? L".hex" : L".bin";
    std::wstring suffix = op == ConvertOperation::BinToBin ? OutputNameSuffix(output_name_tag) : L"_srecord";
    if (stem.empty()) {
        stem = L"output";
    }
    return CombinePath(dir, stem + suffix + ext);
}

bool InferOperationFromPath(const std::wstring& path, ConvertOperation* op) {
    if (op == nullptr) {
        return false;
    }

    const std::wstring ext = ExtensionOf(path);
    if (ext == L".bin") {
        *op = ConvertOperation::BinToBin;
        return true;
    }
    if (ext == L".hex" || ext == L".ihx") {
        *op = ConvertOperation::HexToBin;
        return true;
    }
    return false;
}

bool BuildConvertCommand(const SRecordOptions& options, SRecordCommand* command, std::wstring* error) {
    SRecordDerived derived = {};
    if (command == nullptr) {
        if (error != nullptr) {
            *error = L"Internal command error: command pointer is null.";
        }
        return false;
    }
    command->args.clear();
    command->command_line.clear();

    if (Trim(options.srec_exe).empty()) {
        if (error != nullptr) {
            *error = L"SRecord executable is empty.";
        }
        return false;
    }
    if (Trim(options.input_path).empty()) {
        if (error != nullptr) {
            *error = L"Input path is empty.";
        }
        return false;
    }
    if (Trim(options.output_path).empty()) {
        if (error != nullptr) {
            *error = L"Output path is empty.";
        }
        return false;
    }
    if (!FileExists(options.input_path)) {
        if (error != nullptr) {
            *error = L"Input file does not exist: " + options.input_path;
        }
        return false;
    }
    if (!ParseOptions(options, &derived, error)) {
        return false;
    }
    const std::wstring checksum_algorithm = Trim(options.checksum_algorithm).empty()
                                               ? DefaultChecksumAlgorithm()
                                               : Trim(options.checksum_algorithm);
    if (!IsSupportedChecksumAlgorithm(checksum_algorithm)) {
        if (error != nullptr) {
            *error = L"Unsupported checksum algorithm: " + checksum_algorithm;
        }
        return false;
    }

    std::vector<std::wstring> args;
    args.push_back(options.srec_exe);
    AppendInput(&args, options);

    if (options.operation == ConvertOperation::BinToBin && options.checksum_enabled) {
        args.push_back(L"-fill");
        args.push_back(FormatHex(derived.fill_byte, 2));
        args.push_back(L"0x0000");
        args.push_back(FormatHex(derived.app_binary_size));
        args.push_back(L"-crop");
        args.push_back(L"0x0000");
        args.push_back(FormatHex(derived.checksum_offset_in_input));
        args.push_back(checksum_algorithm);
        args.push_back(FormatHex(derived.checksum_offset_in_input));
        if (ChecksumAlgorithmNeedsSize(checksum_algorithm)) {
            args.push_back(L"4");
        }
    } else if (options.operation == ConvertOperation::BinToBin && options.pad_enabled) {
        args.push_back(L"-fill");
        args.push_back(FormatHex(derived.fill_byte, 2));
        args.push_back(L"0x0000");
        args.push_back(FormatHex(derived.app_binary_size));
        args.push_back(L"-crop");
        args.push_back(L"0x0000");
        args.push_back(FormatHex(derived.app_binary_size));
    } else if (options.checksum_enabled) {
        args.push_back(L"-fill");
        args.push_back(FormatHex(derived.fill_byte, 2));
        args.push_back(FormatHex(derived.base_address));
        args.push_back(FormatHex(derived.checksum_address));
        args.push_back(L"-crop");
        args.push_back(FormatHex(derived.base_address));
        args.push_back(FormatHex(derived.checksum_address));
        args.push_back(checksum_algorithm);
        args.push_back(FormatHex(derived.checksum_address));
        if (ChecksumAlgorithmNeedsSize(checksum_algorithm)) {
            args.push_back(L"4");
        }
    } else if (options.pad_enabled) {
        args.push_back(L"-fill");
        args.push_back(FormatHex(derived.fill_byte, 2));
        args.push_back(FormatHex(derived.base_address));
        args.push_back(FormatHex(derived.image_end));
        args.push_back(L"-crop");
        args.push_back(FormatHex(derived.base_address));
        args.push_back(FormatHex(derived.image_end));
    }

    AppendOutput(&args, options);

    command->args = args;
    command->command_line = JoinCommandLine(args);
    command->derived = derived;
    return true;
}

bool BuildChecksumDumpCommand(const SRecordOptions& options, const SRecordDerived& derived, SRecordCommand* command, std::wstring* error) {
    if (command == nullptr) {
        if (error != nullptr) {
            *error = L"Internal command error: command pointer is null.";
        }
        return false;
    }
    if (Trim(options.output_path).empty()) {
        if (error != nullptr) {
            *error = L"Output path is empty.";
        }
        return false;
    }

    const bool output_is_binary = options.operation != ConvertOperation::BinToHex;
    const uint64_t crop_start = options.operation == ConvertOperation::BinToBin
                                    ? derived.checksum_offset_in_input
                                    : (output_is_binary ? derived.checksum_address - derived.base_address : derived.checksum_address);
    const uint64_t crop_end = crop_start + 4u;

    std::vector<std::wstring> args;
    args.push_back(options.srec_exe);
    args.push_back(options.output_path);
    args.push_back(OutputFormatArg(options.operation));
    args.push_back(L"-crop");
    args.push_back(FormatHex(crop_start));
    args.push_back(FormatHex(crop_end));
    args.push_back(L"-o");
    args.push_back(L"-");
    args.push_back(L"-HEX_Dump");

    command->args = args;
    command->command_line = JoinCommandLine(args);
    command->derived = derived;
    return true;
}

bool BuildHexCopyCommand(const SRecordOptions& options, const SRecordDerived& derived, SRecordCommand* command, std::wstring* error) {
    if (command == nullptr) {
        if (error != nullptr) {
            *error = L"Internal command error: command pointer is null.";
        }
        return false;
    }
    if (options.operation != ConvertOperation::BinToBin) {
        if (error != nullptr) {
            *error = L"HEX copy is only available for BIN -> BIN.";
        }
        return false;
    }
    if (Trim(options.output_path).empty()) {
        if (error != nullptr) {
            *error = L"Patched BIN output path is empty.";
        }
        return false;
    }
    if (Trim(options.hex_output_path).empty()) {
        if (error != nullptr) {
            *error = L"HEX copy path is empty.";
        }
        return false;
    }

    std::vector<std::wstring> args;
    args.push_back(options.srec_exe);
    args.push_back(options.output_path);
    args.push_back(L"-binary");
    if (derived.offset != 0u) {
        args.push_back(L"-offset");
        args.push_back(FormatHex(derived.offset));
    }
    args.push_back(L"-o");
    args.push_back(options.hex_output_path);
    args.push_back(L"-Intel");

    command->args = args;
    command->command_line = JoinCommandLine(args);
    command->derived = derived;
    return true;
}

std::wstring DescribeDerived(const SRecordOptions& options, const SRecordDerived& derived) {
    std::wstringstream ss;
    ss << L"Operation: " << OperationName(options.operation)
       << L" | Base: " << FormatHex(derived.base_address)
       << L" | Offset/App start: " << FormatHex(derived.offset)
       << L" | Image end: " << FormatHex(derived.image_end)
       << L" | Checksum " << (Trim(options.checksum_algorithm).empty() ? DefaultChecksumAlgorithm() : Trim(options.checksum_algorithm))
       << L" @ " << FormatHex(derived.checksum_address)
       << L".." << FormatHex(derived.checksum_end)
       << L" | Checksum offset in BIN: " << FormatHex(derived.checksum_offset_in_input)
       << L" | Output BIN size: " << FormatHex(derived.app_binary_size);
    return ss.str();
}

} // namespace srecord_tool::core
