#include <afxwin.h>
#include <afxcmn.h>
#include "../src/main_frame.h"
#include "../src/resource.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;
CWinApp test_application;

namespace {
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
class FixtureFrame : public CMainFrame {
    void PostNcDestroy() override {} // Stack-owned fixture; production frame remains heap-owned.
};
bool PumpUntil(const std::function<bool()>& condition, DWORD limit = 6000) {
    const ULONGLONG start = GetTickCount64();
    do {
        MSG message = {};
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message != WM_QUIT) { TranslateMessage(&message); DispatchMessageW(&message); }
        }
        if (condition()) return true;
        Sleep(10);
    } while (GetTickCount64() - start < limit);
    return false;
}
void Set(FixtureFrame& frame, int id, const std::wstring& text) {
    auto* control = frame.GetDescendantWindow(id);
    Require(control && IsWindow(control->m_hWnd), "Missing native control");
    control->SetWindowTextW(text.c_str());
}
CString Text(FixtureFrame& frame, int id) {
    CString text;
    frame.GetDescendantWindow(id)->GetWindowTextW(text);
    return text;
}
void Command(FixtureFrame& frame, int id) {
    auto* control = frame.GetDescendantWindow(id);
    control->GetParent()->SendMessageW(WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), LPARAM(control->m_hWnd));
}
void Configure(FixtureFrame& frame, const fs::path& input, const fs::path& output, const std::wstring& engine) {
    Set(frame, ID_SREC_INPUT_EDIT, input.wstring());
    Set(frame, ID_SREC_OUTPUT_EDIT, output.wstring());
    Set(frame, ID_SREC_EXE_EDIT, engine);
    for (int id : {ID_SREC_HEX_COPY_CHECK, ID_SREC_PAD_CHECK, ID_SREC_CHECKSUM_CHECK})
        static_cast<CButton*>(frame.GetDescendantWindow(id))->SetCheck(BST_UNCHECKED);
    Command(frame, ID_SREC_PAD_CHECK);
}
void Run(const std::wstring& engine, const std::wstring& fake, const fs::path& root) {
    const auto input = root / L"ui-input.bin";
    { std::ofstream file(input, std::ios::binary); file << "native-ui-synthetic"; }
    FixtureFrame frame;
    Require(frame.Create(nullptr, L"SRecord execution fixture", WS_OVERLAPPEDWINDOW,
                          CRect(0, 0, 1200, 800)), "Fixture creation failed");
    auto* tab = static_cast<CSRecordTab*>(frame.GetDescendantWindow(ID_TAB_SRECORD));
    Require(tab != nullptr, "Missing tab owner");
    const auto output = root / L"ui-output.bin";
    Configure(frame, input, output, engine);
    Command(frame, ID_SREC_CONVERT_BTN);
    Require(tab->IsBusy(), "UI never entered busy state");
    Require(!frame.GetDescendantWindow(ID_TOP_LOAD_INI_BTN)->IsWindowEnabled(), "INI load remained enabled");
    Require(Text(frame, ID_SREC_CONVERT_BTN) == L"Cancel", "Cancel action missing");
    Require(PumpUntil([&]() { return !tab->IsBusy(); }), "UI completion did not drain");
    Require(fs::exists(output) && Text(frame, ID_SREC_CONVERT_BTN) == L"Convert", "UI success not restored");

    const auto waiting = root / L"wait.bin";
    { std::ofstream file(waiting, std::ios::binary); file << "wait"; }
    Configure(frame, waiting, root / L"ui-cancel.bin", fake);
    Command(frame, ID_SREC_CONVERT_BTN);
    Require(PumpUntil([&]() { return fs::exists(waiting.wstring() + L".ready"); }), "UI wait engine not ready");
    const ULONGLONG before = GetTickCount64();
    Command(frame, ID_SREC_CONVERT_BTN);
    Require(GetTickCount64() - before < 500 && tab->IsBusy(), "UI cancellation blocked/drained prematurely");
    Require(PumpUntil([&]() { return !tab->IsBusy(); }) && !fs::exists(root / L"ui-cancel.bin"), "UI cancel published output");

    Configure(frame, waiting, root / L"ui-close.bin", fake);
    Command(frame, ID_SREC_CONVERT_BTN);
    const HWND handle = frame.m_hWnd;
    frame.SendMessageW(WM_CLOSE);
    Require(IsWindow(handle) && tab->IsBusy(), "Close destroyed active worker owner synchronously");
    Require(PumpUntil([&]() { return !IsWindow(handle); }) && !fs::exists(root / L"ui-close.bin"), "Deferred close failed");
    std::cout << "PASS: native UI single-flight, Cancel label, INI lockout, completion drain, deferred close\n";
}
} // namespace

int wmain(int argc, wchar_t** argv) {
    try {
        Require(argc == 4, "Usage: UiExecutionRegression <engine> <fake engine> <fresh directory>");
        Require(AfxWinInit(GetModuleHandleW(nullptr), nullptr, GetCommandLineW(), 0) != FALSE, "MFC initialization failed");
        INITCOMMONCONTROLSEX controls = {sizeof(controls), ICC_WIN95_CLASSES};
        Require(InitCommonControlsEx(&controls) != FALSE, "Common controls initialization failed");
        Require(fs::create_directory(argv[3]), "UI fixtures must be fresh");
        Run(argv[1], argv[2], argv[3]);
        return 0;
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
