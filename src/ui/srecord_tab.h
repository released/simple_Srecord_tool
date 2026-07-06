#pragma once

#include <afxcmn.h>
#include <afxwin.h>

#include <functional>
#include <string>

#include "../core/app_state.h"

class CSRecordTab : public CWnd {
public:
    using LogCallback = std::function<void(const std::wstring&)>;

    BOOL Create(CWnd* parent, const RECT& rect, UINT id);
    void Bind(LogCallback log_cb);
    void LoadState(const srecord_tool::core::AppState& state);
    void SaveState(srecord_tool::core::AppState* state) const;

protected:
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnBrowseInput();
    afx_msg void OnReloadInput();
    afx_msg void OnBrowseOutput();
    afx_msg void OnBrowseHexCopy();
    afx_msg void OnBrowseSrec();
    afx_msg void OnConvert();
    afx_msg void OnCopyCommand();
    afx_msg void OnOpenOutputFolder();
    afx_msg void OnOptionChanged();
    afx_msg void OnOutputNameTagChanged();
    afx_msg void OnOperationChanged();
    afx_msg void OnOffsetPresetChanged();
    afx_msg void OnChecksumAlgorithmChanged();
    afx_msg void OnResetDefaults();

    DECLARE_MESSAGE_MAP()

private:
    void CreateControls();
    void LayoutControls(int cx, int cy);
    void SetFonts();
    void Log(const std::wstring& text) const;
    void UpdateInputInfo();
    void UpdateDerivedPreview();
    void UpdateOptionEnable();
    void AutoSelectOperationAndOutput(const std::wstring& path);
    void UpdateDefaultOutputPaths(const std::wstring& input, srecord_tool::core::ConvertOperation op);
    void SetOperation(srecord_tool::core::ConvertOperation op);
    srecord_tool::core::ConvertOperation GetOperation() const;
    void SetChecksumAlgorithm(const std::wstring& algorithm);
    std::wstring GetChecksumAlgorithm() const;
    srecord_tool::core::SRecordOptions ReadOptions() const;
    void WriteOptions(const srecord_tool::core::SRecordOptions& options);
    std::wstring GetText(const CWnd& wnd) const;
    void SetText(CWnd& wnd, const std::wstring& text);
    static void ShowErrorBox(const std::wstring& title, const std::wstring& message);

private:
    CFont* font_ = nullptr;
    bool created_ = false;
    bool busy_ = false;
    LogCallback log_cb_;

    CStatic input_label_;
    CEdit input_edit_;
    CButton input_browse_btn_;
    CButton input_reload_btn_;
    CStatic input_info_;

    CStatic operation_label_;
    CComboBox operation_combo_;
    CStatic output_label_;
    CEdit output_edit_;
    CButton output_browse_btn_;
    CStatic output_name_tag_label_;
    CEdit output_name_tag_edit_;
    CButton hex_copy_check_;
    CStatic hex_copy_label_;
    CEdit hex_copy_edit_;
    CButton hex_copy_browse_btn_;

    CStatic srec_label_;
    CEdit srec_edit_;
    CButton srec_browse_btn_;

    CStatic address_section_label_;
    CStatic base_label_;
    CEdit base_edit_;
    CButton offset_check_;
    CEdit offset_edit_;
    CComboBox offset_preset_;
    CStatic image_section_label_;
    CButton pad_check_;
    CStatic size_label_;
    CEdit size_edit_;
    CStatic fill_label_;
    CEdit fill_edit_;
    CStatic checksum_section_label_;
    CButton checksum_check_;
    CStatic checksum_algo_label_;
    CComboBox checksum_algo_combo_;
    CButton reset_defaults_btn_;

    CStatic derived_info_;
    CStatic command_label_;
    CEdit command_preview_;
    CButton convert_btn_;
    CButton copy_command_btn_;
    CButton open_output_btn_;
};
