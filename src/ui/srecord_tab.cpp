#include "srecord_tab.h"

#include <afxdlgs.h>
#include <shellapi.h>

#include <algorithm>
#include <cstring>
#include <utility>

#include "../core/process_runner.h"
#include "../core/srecord_command.h"
#include "../core/text_utils.h"
#include "../resource.h"
#include "layout_utils.h"

namespace {

constexpr int kRowH = 26;
constexpr int kGap = 6;
constexpr int kMargin = 8;

} // namespace

BEGIN_MESSAGE_MAP(CSRecordTab, CWnd)
    ON_WM_SIZE()
    ON_BN_CLICKED(ID_SREC_INPUT_BROWSE_BTN, &CSRecordTab::OnBrowseInput)
    ON_BN_CLICKED(ID_SREC_INPUT_RELOAD_BTN, &CSRecordTab::OnReloadInput)
    ON_BN_CLICKED(ID_SREC_OUTPUT_BROWSE_BTN, &CSRecordTab::OnBrowseOutput)
    ON_BN_CLICKED(ID_SREC_HEX_COPY_BROWSE_BTN, &CSRecordTab::OnBrowseHexCopy)
    ON_BN_CLICKED(ID_SREC_EXE_BROWSE_BTN, &CSRecordTab::OnBrowseSrec)
    ON_BN_CLICKED(ID_SREC_CONVERT_BTN, &CSRecordTab::OnConvert)
    ON_BN_CLICKED(ID_SREC_COPY_COMMAND_BTN, &CSRecordTab::OnCopyCommand)
    ON_BN_CLICKED(ID_SREC_OPEN_OUTPUT_BTN, &CSRecordTab::OnOpenOutputFolder)
    ON_BN_CLICKED(ID_SREC_OFFSET_CHECK, &CSRecordTab::OnOptionChanged)
    ON_BN_CLICKED(ID_SREC_PAD_CHECK, &CSRecordTab::OnOptionChanged)
    ON_BN_CLICKED(ID_SREC_CHECKSUM_CHECK, &CSRecordTab::OnOptionChanged)
    ON_BN_CLICKED(ID_SREC_HEX_COPY_CHECK, &CSRecordTab::OnOptionChanged)
    ON_BN_CLICKED(ID_SREC_RESET_DEFAULTS_BTN, &CSRecordTab::OnResetDefaults)
    ON_CBN_SELCHANGE(ID_SREC_OPERATION_COMBO, &CSRecordTab::OnOperationChanged)
    ON_CBN_SELCHANGE(ID_SREC_OFFSET_PRESET, &CSRecordTab::OnOffsetPresetChanged)
    ON_CBN_SELCHANGE(ID_SREC_CHECKSUM_ALGO_COMBO, &CSRecordTab::OnChecksumAlgorithmChanged)
    ON_EN_CHANGE(ID_SREC_INPUT_EDIT, &CSRecordTab::OnOptionChanged)
    ON_EN_CHANGE(ID_SREC_OUTPUT_EDIT, &CSRecordTab::OnOptionChanged)
    ON_EN_CHANGE(ID_SREC_OUTPUT_NAME_TAG_EDIT, &CSRecordTab::OnOutputNameTagChanged)
    ON_EN_CHANGE(ID_SREC_HEX_COPY_EDIT, &CSRecordTab::OnOptionChanged)
    ON_EN_CHANGE(ID_SREC_EXE_EDIT, &CSRecordTab::OnOptionChanged)
    ON_EN_CHANGE(ID_SREC_BASE_EDIT, &CSRecordTab::OnOptionChanged)
    ON_EN_CHANGE(ID_SREC_OFFSET_EDIT, &CSRecordTab::OnOptionChanged)
    ON_EN_CHANGE(ID_SREC_SIZE_EDIT, &CSRecordTab::OnOptionChanged)
    ON_EN_CHANGE(ID_SREC_FILL_EDIT, &CSRecordTab::OnOptionChanged)
END_MESSAGE_MAP()

BOOL CSRecordTab::Create(CWnd* parent, const RECT& rect, UINT id) {
    BOOL ok = CWnd::CreateEx(0, AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW),
                                                    reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), nullptr),
                             L"SRecordTab", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                             rect, parent, id);
    if (!ok) {
        return FALSE;
    }
    CreateControls();
    created_ = true;
    UpdateInputInfo();
    UpdateOptionEnable();
    UpdateDerivedPreview();
    return TRUE;
}

void CSRecordTab::Bind(LogCallback log_cb) {
    log_cb_ = std::move(log_cb);
}

void CSRecordTab::LoadState(const srecord_tool::core::AppState& state) {
    WriteOptions(state.srecord);
    UpdateInputInfo();
    UpdateOptionEnable();
    UpdateDerivedPreview();
}

void CSRecordTab::SaveState(srecord_tool::core::AppState* state) const {
    if (state == nullptr) {
        return;
    }
    state->srecord = ReadOptions();
}

void CSRecordTab::CreateControls() {
    auto create_or_log = [this](BOOL ok, const wchar_t* name) {
        if (!ok) {
            Log(std::wstring(L"Create control failed: ") + name);
        }
        return ok;
    };

    create_or_log(input_label_.Create(L"Input:", WS_CHILD | WS_VISIBLE, CRect(), this), L"input label");
    create_or_log(input_edit_.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(), this, ID_SREC_INPUT_EDIT), L"input edit");
    create_or_log(input_browse_btn_.Create(L"Browse", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, ID_SREC_INPUT_BROWSE_BTN), L"input browse");
    create_or_log(input_reload_btn_.Create(L"Reload", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, ID_SREC_INPUT_RELOAD_BTN), L"input reload");
    create_or_log(input_info_.Create(L"-", WS_CHILD | WS_VISIBLE | SS_PATHELLIPSIS, CRect(), this, ID_SREC_INPUT_INFO), L"input info");

    create_or_log(operation_label_.Create(L"Operation:", WS_CHILD | WS_VISIBLE, CRect(), this), L"operation label");
    create_or_log(operation_combo_.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST, CRect(), this, ID_SREC_OPERATION_COMBO), L"operation combo");
    operation_combo_.AddString(L"BIN -> BIN (checksum)");
    operation_combo_.AddString(L"BIN -> HEX");
    operation_combo_.AddString(L"HEX -> BIN");
    operation_combo_.SetCurSel(0);

    create_or_log(output_label_.Create(L"Output BIN:", WS_CHILD | WS_VISIBLE, CRect(), this), L"output label");
    create_or_log(output_edit_.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(), this, ID_SREC_OUTPUT_EDIT), L"output edit");
    create_or_log(output_browse_btn_.Create(L"Browse", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, ID_SREC_OUTPUT_BROWSE_BTN), L"output browse");
    create_or_log(output_name_tag_label_.Create(L"BIN suffix:", WS_CHILD | WS_VISIBLE, CRect(), this), L"output name tag label");
    create_or_log(output_name_tag_edit_.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(), this, ID_SREC_OUTPUT_NAME_TAG_EDIT), L"output name tag edit");
    create_or_log(hex_copy_check_.Create(L"HEX copy", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, CRect(), this, ID_SREC_HEX_COPY_CHECK), L"hex copy check");
    create_or_log(hex_copy_label_.Create(L"HEX path:", WS_CHILD | WS_VISIBLE, CRect(), this, ID_SREC_HEX_COPY_LABEL), L"hex copy label");
    create_or_log(hex_copy_edit_.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(), this, ID_SREC_HEX_COPY_EDIT), L"hex copy edit");
    create_or_log(hex_copy_browse_btn_.Create(L"Browse", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, ID_SREC_HEX_COPY_BROWSE_BTN), L"hex copy browse");
    hex_copy_check_.SetCheck(BST_CHECKED);

    create_or_log(srec_label_.Create(L"SRecord:", WS_CHILD | WS_VISIBLE, CRect(), this), L"srec label");
    create_or_log(srec_edit_.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(), this, ID_SREC_EXE_EDIT), L"srec edit");
    create_or_log(srec_browse_btn_.Create(L"Browse", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, ID_SREC_EXE_BROWSE_BTN), L"srec browse");

    create_or_log(address_section_label_.Create(L"Address", WS_CHILD | WS_VISIBLE, CRect(), this), L"address section label");
    create_or_log(base_label_.Create(L"Base:", WS_CHILD | WS_VISIBLE, CRect(), this), L"base label");
    create_or_log(base_edit_.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(), this, ID_SREC_BASE_EDIT), L"base edit");
    create_or_log(offset_check_.Create(L"Offset/App start", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, CRect(), this, ID_SREC_OFFSET_CHECK), L"offset check");
    create_or_log(offset_edit_.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(), this, ID_SREC_OFFSET_EDIT), L"offset edit");
    create_or_log(offset_preset_.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST, CRect(), this, ID_SREC_OFFSET_PRESET), L"offset preset");
    offset_preset_.AddString(L"0x0000");
    offset_preset_.AddString(L"0x0200");
    offset_preset_.AddString(L"0x0400");
    offset_preset_.AddString(L"0x1000");
    offset_preset_.AddString(L"0x3000");
    offset_preset_.AddString(L"0x4000");
    offset_preset_.AddString(L"0x4800");
    offset_preset_.SetCurSel(0);

    create_or_log(image_section_label_.Create(L"Image", WS_CHILD | WS_VISIBLE, CRect(), this), L"image section label");
    create_or_log(pad_check_.Create(L"Pad/Crop to image size", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, CRect(), this, ID_SREC_PAD_CHECK), L"pad check");
    create_or_log(size_label_.Create(L"Size:", WS_CHILD | WS_VISIBLE, CRect(), this), L"size label");
    create_or_log(size_edit_.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(), this, ID_SREC_SIZE_EDIT), L"size edit");
    create_or_log(fill_label_.Create(L"Fill:", WS_CHILD | WS_VISIBLE, CRect(), this), L"fill label");
    create_or_log(fill_edit_.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(), this, ID_SREC_FILL_EDIT), L"fill edit");
    create_or_log(checksum_section_label_.Create(L"Checksum", WS_CHILD | WS_VISIBLE, CRect(), this), L"checksum section label");
    create_or_log(checksum_check_.Create(L"Add checksum at last 4 bytes", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, CRect(), this, ID_SREC_CHECKSUM_CHECK), L"checksum check");
    create_or_log(checksum_algo_label_.Create(L"Algorithm:", WS_CHILD | WS_VISIBLE, CRect(), this), L"checksum algorithm label");
    create_or_log(checksum_algo_combo_.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST, CRect(), this, ID_SREC_CHECKSUM_ALGO_COMBO), L"checksum algorithm combo");
    checksum_algo_combo_.AddString(L"-crc32-l-e");
    checksum_algo_combo_.AddString(L"-crc32-b-e");
    checksum_algo_combo_.AddString(L"-checksum-n-l-e");
    checksum_algo_combo_.AddString(L"-checksum-n-b-e");
    checksum_algo_combo_.AddString(L"-checksum-p-l-e");
    checksum_algo_combo_.AddString(L"-checksum-p-b-e");
    checksum_algo_combo_.SetCurSel(0);
    create_or_log(reset_defaults_btn_.Create(L"Reset Defaults", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, ID_SREC_RESET_DEFAULTS_BTN), L"reset defaults");

    create_or_log(derived_info_.Create(L"-", WS_CHILD | WS_VISIBLE | SS_PATHELLIPSIS, CRect(), this, ID_SREC_DERIVED_INFO), L"derived info");
    create_or_log(command_label_.Create(L"Command Preview:", WS_CHILD | WS_VISIBLE, CRect(), this), L"command label");
    create_or_log(command_preview_.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, CRect(), this, ID_SREC_COMMAND_PREVIEW), L"command preview");

    create_or_log(convert_btn_.Create(L"Convert", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, CRect(), this, ID_SREC_CONVERT_BTN), L"convert");
    create_or_log(copy_command_btn_.Create(L"Copy Command", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, ID_SREC_COPY_COMMAND_BTN), L"copy command");
    create_or_log(open_output_btn_.Create(L"Open Folder", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, ID_SREC_OPEN_OUTPUT_BTN), L"open folder");

    SetText(srec_edit_, L"srec_cat");
    SetText(output_name_tag_edit_, srecord_tool::core::DefaultOutputNameTag());
    SetText(base_edit_, L"0x0000");
    SetText(offset_edit_, L"0x0000");
    SetText(size_edit_, L"0x20000");
    SetText(fill_edit_, L"0xFF");
    pad_check_.SetCheck(BST_CHECKED);
    SetFonts();
}

void CSRecordTab::SetFonts() {
    font_ = GetParent() != nullptr ? GetParent()->GetFont() : nullptr;
    if (font_ == nullptr) {
        return;
    }

    CWnd* controls[] = {
        &input_label_, &input_edit_, &input_browse_btn_, &input_reload_btn_, &input_info_,
        &operation_label_, &operation_combo_, &output_label_, &output_edit_, &output_browse_btn_,
        &output_name_tag_label_, &output_name_tag_edit_, &hex_copy_check_, &hex_copy_label_,
        &hex_copy_edit_, &hex_copy_browse_btn_, &srec_label_, &srec_edit_, &srec_browse_btn_,
        &address_section_label_, &base_label_, &base_edit_,
        &offset_check_, &offset_edit_, &offset_preset_, &image_section_label_, &pad_check_,
        &size_label_, &size_edit_, &fill_label_, &fill_edit_, &checksum_section_label_,
        &checksum_check_, &checksum_algo_label_, &checksum_algo_combo_, &reset_defaults_btn_,
        &derived_info_, &command_label_, &command_preview_, &convert_btn_, &copy_command_btn_,
        &open_output_btn_
    };
    for (CWnd* wnd : controls) {
        if (wnd != nullptr && ::IsWindow(wnd->GetSafeHwnd())) {
            wnd->SetFont(font_);
        }
    }
}

void CSRecordTab::OnSize(UINT nType, int cx, int cy) {
    CWnd::OnSize(nType, cx, cy);
    LayoutControls(cx, cy);
}

void CSRecordTab::LayoutControls(int cx, int cy) {
    if (!created_ || cx <= 0 || cy <= 0) {
        return;
    }

    int y = kMargin;
    int x = kMargin;
    const int browse_w = 78;
    const int reload_w = 74;
    const int label_w = 104;
    const int edit_right = cx - kMargin;
    const int path_btn_w = browse_w + kGap + reload_w;

    mfc_tool::ui::SafeMoveWindow(input_label_, x, y + 4, label_w, 18);
    mfc_tool::ui::SafeMoveWindow(input_edit_, x + label_w, y, edit_right - x - label_w - path_btn_w - kGap, kRowH);
    mfc_tool::ui::SafeMoveWindow(input_browse_btn_, edit_right - path_btn_w, y, browse_w, kRowH);
    mfc_tool::ui::SafeMoveWindow(input_reload_btn_, edit_right - reload_w, y, reload_w, kRowH);
    y += kRowH + kGap;
    mfc_tool::ui::SafeMoveWindow(input_info_, x + label_w, y + 3, edit_right - x - label_w, 18);
    y += 22 + kGap;

    mfc_tool::ui::SafeMoveWindow(operation_label_, x, y + 4, label_w, 18);
    mfc_tool::ui::SafeMoveWindow(operation_combo_, x + label_w, y, 250, 180);
    y += kRowH + kGap;

    mfc_tool::ui::SafeMoveWindow(output_label_, x, y + 4, label_w, 18);
    mfc_tool::ui::SafeMoveWindow(output_edit_, x + label_w, y, edit_right - x - label_w - browse_w - kGap, kRowH);
    mfc_tool::ui::SafeMoveWindow(output_browse_btn_, edit_right - browse_w, y, browse_w, kRowH);
    y += kRowH + kGap;

    mfc_tool::ui::SafeMoveWindow(output_name_tag_label_, x, y + 4, label_w, 18);
    mfc_tool::ui::SafeMoveWindow(output_name_tag_edit_, x + label_w, y, 160, kRowH);
    y += kRowH + kGap;

    mfc_tool::ui::SafeMoveWindow(hex_copy_check_, x + label_w, y + 3, 112, 20);
    mfc_tool::ui::SafeMoveWindow(hex_copy_label_, x + label_w + 126, y + 4, 82, 18);
    mfc_tool::ui::SafeMoveWindow(hex_copy_edit_, x + label_w + 212, y, edit_right - x - label_w - 212 - browse_w - kGap, kRowH);
    mfc_tool::ui::SafeMoveWindow(hex_copy_browse_btn_, edit_right - browse_w, y, browse_w, kRowH);
    y += kRowH + kGap;

    mfc_tool::ui::SafeMoveWindow(srec_label_, x, y + 4, label_w, 18);
    mfc_tool::ui::SafeMoveWindow(srec_edit_, x + label_w, y, edit_right - x - label_w - browse_w - kGap, kRowH);
    mfc_tool::ui::SafeMoveWindow(srec_browse_btn_, edit_right - browse_w, y, browse_w, kRowH);
    y += kRowH + kGap + 2;

    mfc_tool::ui::SafeMoveWindow(address_section_label_, x, y + 4, 120, 18);
    mfc_tool::ui::SafeMoveWindow(reset_defaults_btn_, edit_right - 122, y, 122, kRowH);
    y += kRowH + kGap;

    mfc_tool::ui::SafeMoveWindow(base_label_, x, y + 4, 70, 18);
    mfc_tool::ui::SafeMoveWindow(base_edit_, x + 70, y, 120, kRowH);
    mfc_tool::ui::SafeMoveWindow(offset_check_, x + 210, y + 3, 170, 20);
    mfc_tool::ui::SafeMoveWindow(offset_edit_, x + 390, y, 120, kRowH);
    mfc_tool::ui::SafeMoveWindow(offset_preset_, x + 518, y, 120, 180);
    y += kRowH + kGap;

    mfc_tool::ui::SafeMoveWindow(image_section_label_, x, y + 4, 120, 18);
    y += 22 + kGap;

    mfc_tool::ui::SafeMoveWindow(pad_check_, x, y + 3, 240, 20);
    mfc_tool::ui::SafeMoveWindow(size_label_, x + 258, y + 4, 54, 18);
    mfc_tool::ui::SafeMoveWindow(size_edit_, x + 312, y, 120, kRowH);
    mfc_tool::ui::SafeMoveWindow(fill_label_, x + 452, y + 4, 46, 18);
    mfc_tool::ui::SafeMoveWindow(fill_edit_, x + 498, y, 80, kRowH);
    y += kRowH + kGap;

    mfc_tool::ui::SafeMoveWindow(checksum_section_label_, x, y + 4, 120, 18);
    y += 22 + kGap;

    mfc_tool::ui::SafeMoveWindow(checksum_check_, x, y + 3, 272, 20);
    mfc_tool::ui::SafeMoveWindow(checksum_algo_label_, x + 292, y + 4, 82, 18);
    mfc_tool::ui::SafeMoveWindow(checksum_algo_combo_, x + 378, y, 190, 180);
    y += kRowH + kGap;

    mfc_tool::ui::SafeMoveWindow(derived_info_, x, y + 4, edit_right - x, 18);
    y += 22 + kGap;

    mfc_tool::ui::SafeMoveWindow(command_label_, x, y + 4, 150, 18);
    const int open_btn_w = 122;
    const int copy_btn_w = 138;
    const int convert_btn_w = 104;
    int button_x = edit_right - open_btn_w;
    mfc_tool::ui::SafeMoveWindow(open_output_btn_, button_x, y, open_btn_w, kRowH);
    button_x -= kGap + copy_btn_w;
    mfc_tool::ui::SafeMoveWindow(copy_command_btn_, button_x, y, copy_btn_w, kRowH);
    button_x -= kGap + convert_btn_w;
    mfc_tool::ui::SafeMoveWindow(convert_btn_, button_x, y, convert_btn_w, kRowH);
    y += kRowH + kGap;

    mfc_tool::ui::SafeMoveWindow(command_preview_, x, y, edit_right - x, (std::max)(80, cy - y - kMargin));
}

void CSRecordTab::Log(const std::wstring& text) const {
    if (log_cb_) {
        log_cb_(text);
    }
}

std::wstring CSRecordTab::GetText(const CWnd& wnd) const {
    CString text;
    const_cast<CWnd&>(wnd).GetWindowTextW(text);
    return text.GetString();
}

void CSRecordTab::SetText(CWnd& wnd, const std::wstring& text) {
    if (::IsWindow(wnd.GetSafeHwnd())) {
        wnd.SetWindowTextW(text.c_str());
    }
}

void CSRecordTab::ShowErrorBox(const std::wstring& title, const std::wstring& message) {
    ::MessageBoxW(nullptr, message.c_str(), title.c_str(), MB_ICONERROR | MB_OK);
}

void CSRecordTab::UpdateInputInfo() {
    const std::wstring path = GetText(input_edit_);
    uint64_t size = 0;
    if (path.empty()) {
        SetText(input_info_, L"No input selected.");
        return;
    }
    if (!srecord_tool::core::GetFileSizeBytes(path, &size)) {
        SetText(input_info_, L"Input not found or cannot read.");
        return;
    }
    SetText(input_info_, L"Size: " + std::to_wstring(size) + L" bytes (" + srecord_tool::core::FormatHex(size) +
                         L") | Modified: " + srecord_tool::core::LastWriteTimeText(path));
}

void CSRecordTab::UpdateDerivedPreview() {
    srecord_tool::core::SRecordCommand command;
    srecord_tool::core::SRecordCommand hex_command;
    std::wstring error;
    const srecord_tool::core::SRecordOptions options = ReadOptions();
    if (!srecord_tool::core::BuildConvertCommand(options, &command, &error)) {
        SetText(derived_info_, L"Preview error: " + error);
        SetText(command_preview_, L"");
        return;
    }

    SetText(derived_info_, srecord_tool::core::DescribeDerived(options, command.derived));
    std::wstring preview = command.command_line;
    if (options.operation == srecord_tool::core::ConvertOperation::BinToBin && options.hex_copy_enabled) {
        if (srecord_tool::core::BuildHexCopyCommand(options, command.derived, &hex_command, &error)) {
            preview += L"\r\n" + hex_command.command_line;
        } else {
            preview += L"\r\nHEX copy preview error: " + error;
        }
    }
    SetText(command_preview_, preview);
}

void CSRecordTab::UpdateOptionEnable() {
    const BOOL busy = busy_ ? FALSE : TRUE;
    const BOOL offset_enabled = (busy && offset_check_.GetCheck() == BST_CHECKED) ? TRUE : FALSE;
    const BOOL image_enabled = (busy && (pad_check_.GetCheck() == BST_CHECKED || checksum_check_.GetCheck() == BST_CHECKED)) ? TRUE : FALSE;
    const BOOL bin_to_bin = GetOperation() == srecord_tool::core::ConvertOperation::BinToBin ? TRUE : FALSE;
    const BOOL hex_copy_enabled = (busy && bin_to_bin && hex_copy_check_.GetCheck() == BST_CHECKED) ? TRUE : FALSE;

    const int hex_show = bin_to_bin ? SW_SHOW : SW_HIDE;
    hex_copy_check_.ShowWindow(hex_show);
    hex_copy_label_.ShowWindow(hex_show);
    hex_copy_edit_.ShowWindow(hex_show);
    hex_copy_browse_btn_.ShowWindow(hex_show);

    mfc_tool::ui::SafeEnableWindow(input_edit_, busy);
    mfc_tool::ui::SafeEnableWindow(input_browse_btn_, busy);
    mfc_tool::ui::SafeEnableWindow(input_reload_btn_, busy);
    mfc_tool::ui::SafeEnableWindow(operation_combo_, busy);
    mfc_tool::ui::SafeEnableWindow(output_edit_, busy);
    mfc_tool::ui::SafeEnableWindow(output_browse_btn_, busy);
    mfc_tool::ui::SafeEnableWindow(output_name_tag_label_, busy && bin_to_bin);
    mfc_tool::ui::SafeEnableWindow(output_name_tag_edit_, busy && bin_to_bin);
    mfc_tool::ui::SafeEnableWindow(hex_copy_check_, busy && bin_to_bin);
    mfc_tool::ui::SafeEnableWindow(hex_copy_label_, hex_copy_enabled);
    mfc_tool::ui::SafeEnableWindow(hex_copy_edit_, hex_copy_enabled);
    mfc_tool::ui::SafeEnableWindow(hex_copy_browse_btn_, hex_copy_enabled);
    mfc_tool::ui::SafeEnableWindow(srec_edit_, busy);
    mfc_tool::ui::SafeEnableWindow(srec_browse_btn_, busy);
    mfc_tool::ui::SafeEnableWindow(base_edit_, image_enabled);
    mfc_tool::ui::SafeEnableWindow(offset_check_, busy);
    mfc_tool::ui::SafeEnableWindow(offset_edit_, offset_enabled);
    mfc_tool::ui::SafeEnableWindow(offset_preset_, offset_enabled);
    mfc_tool::ui::SafeEnableWindow(pad_check_, busy);
    mfc_tool::ui::SafeEnableWindow(size_edit_, image_enabled);
    mfc_tool::ui::SafeEnableWindow(fill_edit_, image_enabled);
    mfc_tool::ui::SafeEnableWindow(checksum_check_, busy);
    mfc_tool::ui::SafeEnableWindow(checksum_algo_label_, busy && checksum_check_.GetCheck() == BST_CHECKED);
    mfc_tool::ui::SafeEnableWindow(checksum_algo_combo_, busy && checksum_check_.GetCheck() == BST_CHECKED);
    mfc_tool::ui::SafeEnableWindow(reset_defaults_btn_, busy);
    mfc_tool::ui::SafeEnableWindow(convert_btn_, busy);
    mfc_tool::ui::SafeEnableWindow(copy_command_btn_, busy);
    mfc_tool::ui::SafeEnableWindow(open_output_btn_, busy);
}

void CSRecordTab::AutoSelectOperationAndOutput(const std::wstring& path) {
    srecord_tool::core::ConvertOperation op = GetOperation();
    if (srecord_tool::core::InferOperationFromPath(path, &op)) {
        SetOperation(op);
    }
    UpdateDefaultOutputPaths(path, op);
}

void CSRecordTab::UpdateDefaultOutputPaths(const std::wstring& input, srecord_tool::core::ConvertOperation op) {
    if (input.empty()) {
        return;
    }
    SetText(output_edit_, srecord_tool::core::DefaultOutputPath(input, op, GetText(output_name_tag_edit_)));
    if (op == srecord_tool::core::ConvertOperation::BinToBin) {
        SetText(hex_copy_edit_, srecord_tool::core::DefaultOutputPath(input, srecord_tool::core::ConvertOperation::BinToHex));
    }
}

void CSRecordTab::SetOperation(srecord_tool::core::ConvertOperation op) {
    int index = 0;
    if (op == srecord_tool::core::ConvertOperation::BinToHex) {
        index = 1;
    } else if (op == srecord_tool::core::ConvertOperation::HexToBin) {
        index = 2;
    }
    operation_combo_.SetCurSel(index);
    SetText(output_label_, op == srecord_tool::core::ConvertOperation::BinToHex ? L"Output HEX:" : L"Output BIN:");
}

srecord_tool::core::ConvertOperation CSRecordTab::GetOperation() const {
    if (operation_combo_.GetCurSel() == 1) {
        return srecord_tool::core::ConvertOperation::BinToHex;
    }
    if (operation_combo_.GetCurSel() == 2) {
        return srecord_tool::core::ConvertOperation::HexToBin;
    }
    return srecord_tool::core::ConvertOperation::BinToBin;
}

void CSRecordTab::SetChecksumAlgorithm(const std::wstring& algorithm) {
    CString target(algorithm.empty() ? srecord_tool::core::DefaultChecksumAlgorithm() : algorithm.c_str());
    int index = checksum_algo_combo_.FindStringExact(-1, target);
    checksum_algo_combo_.SetCurSel(index >= 0 ? index : 0);
}

std::wstring CSRecordTab::GetChecksumAlgorithm() const {
    CString text;
    checksum_algo_combo_.GetWindowTextW(text);
    std::wstring value = text.GetString();
    if (value.empty()) {
        value = srecord_tool::core::DefaultChecksumAlgorithm();
    }
    return value;
}

srecord_tool::core::SRecordOptions CSRecordTab::ReadOptions() const {
    srecord_tool::core::SRecordOptions options;
    options.srec_exe = GetText(srec_edit_);
    options.input_path = GetText(input_edit_);
    options.output_path = GetText(output_edit_);
    options.output_name_tag = GetText(output_name_tag_edit_);
    options.hex_copy_enabled = hex_copy_check_.GetCheck() == BST_CHECKED;
    options.hex_output_path = GetText(hex_copy_edit_);
    options.operation = GetOperation();
    options.base_address = GetText(base_edit_);
    options.offset_enabled = offset_check_.GetCheck() == BST_CHECKED;
    options.offset = GetText(offset_edit_);
    options.pad_enabled = pad_check_.GetCheck() == BST_CHECKED;
    options.image_size = GetText(size_edit_);
    options.fill_byte = GetText(fill_edit_);
    options.checksum_enabled = checksum_check_.GetCheck() == BST_CHECKED;
    options.checksum_algorithm = GetChecksumAlgorithm();
    return options;
}

void CSRecordTab::WriteOptions(const srecord_tool::core::SRecordOptions& options) {
    SetText(srec_edit_, options.srec_exe.empty() ? L"srec_cat" : options.srec_exe);
    SetText(input_edit_, options.input_path);
    SetText(output_name_tag_edit_, options.output_name_tag.empty() ? srecord_tool::core::DefaultOutputNameTag() : options.output_name_tag);
    SetText(output_edit_, options.output_path);
    hex_copy_check_.SetCheck(options.hex_copy_enabled ? BST_CHECKED : BST_UNCHECKED);
    SetText(hex_copy_edit_, options.hex_output_path);
    SetOperation(options.operation);
    SetText(base_edit_, options.base_address.empty() ? L"0x0000" : options.base_address);
    offset_check_.SetCheck(options.offset_enabled ? BST_CHECKED : BST_UNCHECKED);
    SetText(offset_edit_, options.offset.empty() ? L"0x0000" : options.offset);
    pad_check_.SetCheck(options.pad_enabled ? BST_CHECKED : BST_UNCHECKED);
    SetText(size_edit_, options.image_size.empty() ? L"0x20000" : options.image_size);
    SetText(fill_edit_, options.fill_byte.empty() ? L"0xFF" : options.fill_byte);
    SetChecksumAlgorithm(options.checksum_algorithm);
    checksum_check_.SetCheck(options.checksum_enabled ? BST_CHECKED : BST_UNCHECKED);
}

void CSRecordTab::OnBrowseInput() {
    CFileDialog dlg(TRUE, nullptr, nullptr,
                    OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
                    L"Firmware Images (*.bin;*.hex;*.ihx)|*.bin;*.hex;*.ihx|Binary (*.bin)|*.bin|Intel HEX (*.hex;*.ihx)|*.hex;*.ihx|All Files (*.*)|*.*||",
                    this);
    if (dlg.DoModal() != IDOK) {
        return;
    }

    const std::wstring path = dlg.GetPathName().GetString();
    SetText(input_edit_, path);
    AutoSelectOperationAndOutput(path);
    UpdateInputInfo();
    UpdateDerivedPreview();
    Log(L"Input selected: " + path);
}

void CSRecordTab::OnReloadInput() {
    UpdateInputInfo();
    UpdateDerivedPreview();
    Log(L"Input reloaded: " + GetText(input_edit_));
}

void CSRecordTab::OnBrowseOutput() {
    const bool to_hex = GetOperation() == srecord_tool::core::ConvertOperation::BinToHex;
    CFileDialog dlg(FALSE, to_hex ? L"hex" : L"bin", nullptr,
                    OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
                    to_hex ? L"Intel HEX (*.hex)|*.hex|All Files (*.*)|*.*||" :
                             L"Binary (*.bin)|*.bin|All Files (*.*)|*.*||",
                    this);
    if (dlg.DoModal() == IDOK) {
        SetText(output_edit_, dlg.GetPathName().GetString());
        UpdateDerivedPreview();
    }
}

void CSRecordTab::OnBrowseHexCopy() {
    CFileDialog dlg(FALSE, L"hex", nullptr,
                    OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
                    L"Intel HEX (*.hex)|*.hex|All Files (*.*)|*.*||",
                    this);
    if (dlg.DoModal() == IDOK) {
        SetText(hex_copy_edit_, dlg.GetPathName().GetString());
        UpdateDerivedPreview();
    }
}

void CSRecordTab::OnBrowseSrec() {
    CFileDialog dlg(TRUE, L"exe", nullptr,
                    OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
                    L"Executable (*.exe)|*.exe|All Files (*.*)|*.*||",
                    this);
    if (dlg.DoModal() == IDOK) {
        SetText(srec_edit_, dlg.GetPathName().GetString());
        UpdateDerivedPreview();
    }
}

void CSRecordTab::OnConvert() {
    srecord_tool::core::SRecordCommand command;
    srecord_tool::core::ProcessResult result;
    std::wstring error;
    srecord_tool::core::SRecordOptions options = ReadOptions();

    if (!srecord_tool::core::BuildConvertCommand(options, &command, &error)) {
        ShowErrorBox(L"SRecord Command", error);
        UpdateDerivedPreview();
        return;
    }

    busy_ = true;
    UpdateOptionEnable();
    Log(L"Running: " + command.command_line);

    const std::wstring cwd = srecord_tool::core::DirectoryOf(options.input_path);
    bool ok = srecord_tool::core::RunProcessCapture(command.args, cwd, &result, &error);
    busy_ = false;
    UpdateOptionEnable();

    if (!ok) {
        ShowErrorBox(L"SRecord Execute", error);
        Log(L"Execute failed: " + error);
        return;
    }
    if (!result.output.empty()) {
        Log(result.output);
    }
    if (result.exit_code != 0) {
        const std::wstring msg = L"srec_cat failed with exit code " + std::to_wstring(result.exit_code);
        ShowErrorBox(L"SRecord Execute", msg);
        Log(msg);
        return;
    }

    Log(L"Output generated: " + options.output_path);
    if (options.checksum_enabled) {
        srecord_tool::core::SRecordCommand dump_command;
        srecord_tool::core::ProcessResult dump_result;
        if (srecord_tool::core::BuildChecksumDumpCommand(options, command.derived, &dump_command, &error)) {
            Log(L"Checksum dump: " + dump_command.command_line);
            if (srecord_tool::core::RunProcessCapture(dump_command.args, srecord_tool::core::DirectoryOf(options.output_path), &dump_result, &error) &&
                dump_result.exit_code == 0) {
                Log(dump_result.output.empty() ? L"Checksum dump is empty." : dump_result.output);
            } else {
                Log(L"Checksum dump failed: " + error);
            }
        }
    }
    if (options.operation == srecord_tool::core::ConvertOperation::BinToBin && options.hex_copy_enabled) {
        srecord_tool::core::SRecordCommand hex_command;
        srecord_tool::core::ProcessResult hex_result;
        if (!srecord_tool::core::BuildHexCopyCommand(options, command.derived, &hex_command, &error)) {
            ShowErrorBox(L"HEX Copy", error);
            Log(L"HEX copy command failed: " + error);
            return;
        }
        Log(L"Running HEX copy: " + hex_command.command_line);
        if (!srecord_tool::core::RunProcessCapture(hex_command.args, srecord_tool::core::DirectoryOf(options.output_path), &hex_result, &error)) {
            ShowErrorBox(L"HEX Copy", error);
            Log(L"HEX copy execute failed: " + error);
            return;
        }
        if (!hex_result.output.empty()) {
            Log(hex_result.output);
        }
        if (hex_result.exit_code != 0) {
            const std::wstring msg = L"HEX copy failed with exit code " + std::to_wstring(hex_result.exit_code);
            ShowErrorBox(L"HEX Copy", msg);
            Log(msg);
            return;
        }
        Log(L"HEX copy generated: " + options.hex_output_path);
    }

    UpdateInputInfo();
    UpdateDerivedPreview();
}

void CSRecordTab::OnCopyCommand() {
    srecord_tool::core::SRecordCommand command;
    std::wstring error;
    const srecord_tool::core::SRecordOptions options = ReadOptions();
    if (!srecord_tool::core::BuildConvertCommand(options, &command, &error)) {
        ShowErrorBox(L"Copy Command", error);
        return;
    }
    if (options.operation == srecord_tool::core::ConvertOperation::BinToBin && options.hex_copy_enabled) {
        srecord_tool::core::SRecordCommand hex_command;
        if (srecord_tool::core::BuildHexCopyCommand(options, command.derived, &hex_command, &error)) {
            command.command_line += L"\r\n" + hex_command.command_line;
        }
    }

    if (!::OpenClipboard(GetSafeHwnd())) {
        ShowErrorBox(L"Copy Command", L"OpenClipboard failed.");
        return;
    }
    EmptyClipboard();
    const size_t bytes = (command.command_line.size() + 1u) * sizeof(wchar_t);
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (mem == nullptr) {
        CloseClipboard();
        ShowErrorBox(L"Copy Command", L"GlobalAlloc failed.");
        return;
    }
    void* ptr = GlobalLock(mem);
    if (ptr == nullptr) {
        GlobalFree(mem);
        CloseClipboard();
        ShowErrorBox(L"Copy Command", L"GlobalLock failed.");
        return;
    }
    memcpy(ptr, command.command_line.c_str(), bytes);
    GlobalUnlock(mem);
    SetClipboardData(CF_UNICODETEXT, mem);
    CloseClipboard();
    Log(L"Command copied to clipboard.");
}

void CSRecordTab::OnOpenOutputFolder() {
    const std::wstring output = GetText(output_edit_);
    const std::wstring folder = srecord_tool::core::DirectoryOf(output);
    if (folder.empty()) {
        return;
    }
    ShellExecuteW(GetSafeHwnd(), L"open", folder.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void CSRecordTab::OnOptionChanged() {
    if (!created_) {
        return;
    }
    UpdateOptionEnable();
    UpdateInputInfo();
    UpdateDerivedPreview();
}

void CSRecordTab::OnOutputNameTagChanged() {
    if (!created_) {
        return;
    }
    if (GetOperation() == srecord_tool::core::ConvertOperation::BinToBin) {
        UpdateDefaultOutputPaths(GetText(input_edit_), GetOperation());
    }
    UpdateOptionEnable();
    UpdateInputInfo();
    UpdateDerivedPreview();
}

void CSRecordTab::OnOperationChanged() {
    const std::wstring input = GetText(input_edit_);
    const srecord_tool::core::ConvertOperation op = GetOperation();
    SetOperation(op);
    UpdateDefaultOutputPaths(input, op);
    UpdateOptionEnable();
    UpdateDerivedPreview();
}

void CSRecordTab::OnOffsetPresetChanged() {
    CString text;
    offset_preset_.GetWindowTextW(text);
    SetText(offset_edit_, text.GetString());
    UpdateDerivedPreview();
}

void CSRecordTab::OnChecksumAlgorithmChanged() {
    UpdateDerivedPreview();
}

void CSRecordTab::OnResetDefaults() {
    SetText(base_edit_, L"0x0000");
    offset_check_.SetCheck(BST_UNCHECKED);
    SetText(offset_edit_, L"0x0000");
    offset_preset_.SetCurSel(0);
    SetText(fill_edit_, L"0xFF");
    SetChecksumAlgorithm(srecord_tool::core::DefaultChecksumAlgorithm());
    UpdateOptionEnable();
    UpdateDerivedPreview();
    Log(L"SRecord defaults restored: offset 0, fill 0xFF, checksum -crc32-l-e.");
}
