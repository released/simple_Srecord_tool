#pragma once

#include <string>

#include "../config/ini_manager.h"
#include "srecord_command.h"

namespace srecord_tool::core {

struct UiState {
    bool save_log_checked = false;
};

struct AppState {
    UiState ui;
    SRecordOptions srecord;

    static AppState Default();

    mfc_tool::config::IniData ToIniData(const std::wstring& ini_path) const;
    void ApplyIniData(const mfc_tool::config::IniData& data);
};

} // namespace srecord_tool::core
