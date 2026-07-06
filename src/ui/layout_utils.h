#pragma once

#include <afxcmn.h>
#include <afxwin.h>

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <string>

namespace mfc_tool::ui {

inline int MeasureTextWidth(CWnd& wnd, const std::wstring& text, int padding = 8) {
    if (!::IsWindow(wnd.GetSafeHwnd())) {
        return static_cast<int>(text.size()) * 8 + padding;
    }
    CClientDC dc(&wnd);
    CFont* font = wnd.GetFont();
    CFont* old_font = nullptr;
    if (font != nullptr) {
        old_font = dc.SelectObject(font);
    }
    const CSize sz = dc.GetTextExtent(text.c_str(), static_cast<int>(text.size()));
    if (font != nullptr && old_font != nullptr) {
        dc.SelectObject(old_font);
    }
    return sz.cx + padding;
}

inline int MeasureControlTextWidth(CWnd& wnd, int padding = 8) {
    if (!::IsWindow(wnd.GetSafeHwnd())) {
        return padding;
    }
    CString text;
    wnd.GetWindowTextW(text);
    return MeasureTextWidth(wnd, std::wstring(text.GetString()), padding);
}

inline int MeasureButtonMinWidth(CWnd& wnd, int padding = 20) {
    int width = MeasureControlTextWidth(wnd, padding);
    const LONG style = static_cast<LONG>(::GetWindowLongW(wnd.GetSafeHwnd(), GWL_STYLE));
    if ((style & BS_AUTOCHECKBOX) == BS_AUTOCHECKBOX ||
        (style & BS_CHECKBOX) == BS_CHECKBOX ||
        (style & BS_AUTORADIOBUTTON) == BS_AUTORADIOBUTTON ||
        (style & BS_RADIOBUTTON) == BS_RADIOBUTTON) {
        width += 18;
    }
    return width;
}

inline void SafeEnableWindow(CWnd& control, BOOL enable) {
    if (::IsWindow(control.GetSafeHwnd())) {
        control.EnableWindow(enable);
    }
}

inline void SafeMoveWindow(CWnd& control, int x, int y, int width, int height, BOOL repaint = TRUE) {
    if (::IsWindow(control.GetSafeHwnd())) {
        control.MoveWindow(x, y, width, height, repaint);
    }
}

inline void SafeMoveWindow(CWnd& control, const RECT& rect, BOOL repaint = TRUE) {
    if (::IsWindow(control.GetSafeHwnd())) {
        control.MoveWindow(&rect, repaint);
    }
}

inline int PlaceLabel(CWnd& label, int x, int y, int padding = 8, int height = 18) {
    const int width = MeasureControlTextWidth(label, padding);
    SafeMoveWindow(label, x, y, width, height);
    return width;
}

inline int PlaceLabelAndControl(CWnd& label,
                                CWnd& control,
                                int x,
                                int label_y,
                                int control_y,
                                int control_width,
                                int control_height,
                                int gap = 6,
                                int label_padding = 8,
                                int label_height = 18) {
    const int label_width = PlaceLabel(label, x, label_y, label_padding, label_height);
    const int control_x = x + label_width + gap;
    SafeMoveWindow(control, control_x, control_y, control_width, control_height);
    return control_x + control_width;
}

} // namespace mfc_tool::ui
