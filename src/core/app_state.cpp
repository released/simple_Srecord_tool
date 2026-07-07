#include "app_state.h"

namespace srecord_tool::core {
namespace {

std::wstring BoolText(bool value) {
    return value ? L"1" : L"0";
}

bool TextBool(const std::wstring& value, bool fallback) {
    if (value == L"1" || value == L"true" || value == L"TRUE" || value == L"yes" || value == L"YES") {
        return true;
    }
    if (value == L"0" || value == L"false" || value == L"FALSE" || value == L"no" || value == L"NO") {
        return false;
    }
    return fallback;
}

std::wstring GetValue(const mfc_tool::config::IniData& data,
                      const std::wstring& section,
                      const std::wstring& key,
                      const std::wstring& fallback) {
    auto sec_it = data.find(section);
    if (sec_it == data.end()) {
        return fallback;
    }
    auto key_it = sec_it->second.find(key);
    if (key_it == sec_it->second.end()) {
        return fallback;
    }
    return key_it->second;
}

ConvertOperation ParseOperation(const std::wstring& text, ConvertOperation fallback) {
    if (text == L"HexToBin" || text == L"HEX_TO_BIN" || text == L"2") {
        return ConvertOperation::HexToBin;
    }
    if (text == L"BinToHex" || text == L"BIN_TO_HEX" || text == L"1") {
        return ConvertOperation::BinToHex;
    }
    if (text == L"BinToBin" || text == L"BIN_TO_BIN" || text == L"0") {
        return ConvertOperation::BinToBin;
    }
    return fallback;
}

std::wstring OperationText(ConvertOperation op) {
    if (op == ConvertOperation::HexToBin) {
        return L"HexToBin";
    }
    if (op == ConvertOperation::BinToHex) {
        return L"BinToHex";
    }
    return L"BinToBin";
}

std::wstring FileNameOnly(const std::wstring& path) {
    size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return path;
    }
    return path.substr(pos + 1);
}

} // namespace

AppState AppState::Default() {
    AppState state;
    state.srecord.srec_exe = L"srec_cat";
    state.srecord.operation = ConvertOperation::BinToBin;
    state.srecord.output_name_tag = DefaultOutputNameTag();
    state.srecord.hex_copy_enabled = true;
    state.srecord.base_address = L"0x0000";
    state.srecord.offset_enabled = false;
    state.srecord.offset = L"0x0000";
    state.srecord.pad_enabled = true;
    state.srecord.image_size = L"0x20000";
    state.srecord.fill_byte = L"0xFF";
    state.srecord.checksum_algorithm = DefaultChecksumAlgorithm();
    state.srecord.checksum_enabled = false;
    return state;
}

mfc_tool::config::IniData AppState::ToIniData(const std::wstring& ini_path) const {
    mfc_tool::config::IniData out;
    out[L"APP"] = {
        {L"ini_path", FileNameOnly(ini_path)}
    };

    out[L"UI"] = {
        {L"save_log_checked", BoolText(ui.save_log_checked)}
    };

    out[L"SRECORD"] = {
        {L"srec_exe", srecord.srec_exe},
        {L"input_path", srecord.input_path},
        {L"output_path", srecord.output_path},
        {L"output_name_tag", srecord.output_name_tag},
        {L"hex_copy_enabled", BoolText(srecord.hex_copy_enabled)},
        {L"hex_output_path", srecord.hex_output_path},
        {L"operation", OperationText(srecord.operation)},
        {L"base_address", srecord.base_address},
        {L"offset_enabled", BoolText(srecord.offset_enabled)},
        {L"offset", srecord.offset},
        {L"pad_enabled", BoolText(srecord.pad_enabled)},
        {L"image_size", srecord.image_size},
        {L"fill_byte", srecord.fill_byte},
        {L"checksum_algorithm", srecord.checksum_algorithm},
        {L"checksum_enabled", BoolText(srecord.checksum_enabled)}
    };

    return out;
}

void AppState::ApplyIniData(const mfc_tool::config::IniData& data) {
    ui.save_log_checked = TextBool(GetValue(data, L"UI", L"save_log_checked", BoolText(ui.save_log_checked)), ui.save_log_checked);

    srecord.srec_exe = GetValue(data, L"SRECORD", L"srec_exe", srecord.srec_exe);
    srecord.input_path = GetValue(data, L"SRECORD", L"input_path", srecord.input_path);
    srecord.output_path = GetValue(data, L"SRECORD", L"output_path", srecord.output_path);
    srecord.output_name_tag = GetValue(data, L"SRECORD", L"output_name_tag", srecord.output_name_tag);
    if (srecord.output_name_tag.empty()) {
        srecord.output_name_tag = DefaultOutputNameTag();
    }
    srecord.hex_copy_enabled = TextBool(GetValue(data, L"SRECORD", L"hex_copy_enabled", BoolText(srecord.hex_copy_enabled)), srecord.hex_copy_enabled);
    srecord.hex_output_path = GetValue(data, L"SRECORD", L"hex_output_path", srecord.hex_output_path);
    srecord.operation = ParseOperation(GetValue(data, L"SRECORD", L"operation", OperationText(srecord.operation)), srecord.operation);
    srecord.base_address = GetValue(data, L"SRECORD", L"base_address", srecord.base_address);
    srecord.offset_enabled = TextBool(GetValue(data, L"SRECORD", L"offset_enabled", BoolText(srecord.offset_enabled)), srecord.offset_enabled);
    srecord.offset = GetValue(data, L"SRECORD", L"offset", srecord.offset);
    srecord.pad_enabled = TextBool(GetValue(data, L"SRECORD", L"pad_enabled", BoolText(srecord.pad_enabled)), srecord.pad_enabled);
    srecord.image_size = GetValue(data, L"SRECORD", L"image_size", srecord.image_size);
    srecord.fill_byte = GetValue(data, L"SRECORD", L"fill_byte", srecord.fill_byte);
    srecord.checksum_algorithm = GetValue(data, L"SRECORD", L"checksum_algorithm", srecord.checksum_algorithm);
    if (!IsSupportedChecksumAlgorithm(srecord.checksum_algorithm)) {
        srecord.checksum_algorithm = DefaultChecksumAlgorithm();
    }
    srecord.checksum_enabled = TextBool(GetValue(data, L"SRECORD", L"checksum_enabled", BoolText(srecord.checksum_enabled)), srecord.checksum_enabled);
}

} // namespace srecord_tool::core
