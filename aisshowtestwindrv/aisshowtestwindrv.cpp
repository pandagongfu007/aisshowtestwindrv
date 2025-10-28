// main.cpp  -- PowerShell风格 PCIe 控制台GUI（CHR44X02 离散量卡命令）
// 要求：VS2022, x64, C++20 (/std:c++20), 子系统=Windows, Unicode
// 预处理器建议：定义 NOMINMAX（避免 <Windows.h> 污染 std::min/max）

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <windowsx.h>
#include <strsafe.h>
#include <commctrl.h>

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <cwctype>
#include <format>
#include <limits>
#include <chrono>
#include <filesystem>

#include "Lib/CHR44X02_lib.h"  // ← 按你的实际路径调整

// ===================== 全局状态（设备 / 触发） =====================
static HANDLE gDev        = nullptr;  // di open 后保存
static HANDLE gTrigEvent  = nullptr;  // trig in open/create 后保存
static BYTE   gWorkMode   = 0;        // 0=单板, 1=主卡, 2=从卡
static BYTE   gTrigLine   = 0;        // 0..7 (PXI_TRIG0..7)

// ===================== 控制台UI全局 =====================
static const wchar_t* kWndClass   = L"PcieShellWnd";
static HWND   g_hWndMain          = nullptr;
static HWND   g_hConsole          = nullptr;
static HFONT  g_hFont             = nullptr;
static size_t g_inputStart        = 0;                // 可编辑起点
static std::wstring g_prompt      = L"PS lcdtest:\\> ";     // 提示符
static HBRUSH g_hBrushBk          = nullptr;
static COLORREF g_textColor       = RGB(255,255,255); // 白字
static COLORREF g_backColor       = RGB(1,36,86);     // 深蓝

// ===================== 编辑框子类（复制/粘贴/保护历史） =====================
static LRESULT CALLBACK EditProc(HWND, UINT, WPARAM, LPARAM);
static WNDPROC g_OldEditProc = nullptr;

static bool TryParseUInt32(const std::wstring& s, DWORD& out) {
    if (s.empty()) return false;
    wchar_t* end = nullptr;
    unsigned long long v = std::wcstoull(s.c_str(), &end, 10);
    if (!end || *end != L'\0') return false;
    if (v > 0xFFFFFFFFull) return false;
    out = static_cast<DWORD>(v);
    return true;
}

// ===================== 简易工具函数 =====================
static std::vector<std::wstring> SplitWSV(const std::wstring& line) {
    std::vector<std::wstring> out; std::wstring cur;
    for (wchar_t c : line) {
        if (iswspace(c)) { if (!cur.empty()) { out.push_back(cur); cur.clear(); } }
        else cur.push_back(c);
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

static std::wstring ToLower(std::wstring s) {
    for (auto& ch : s) ch = (wchar_t)towlower(ch);
    return s;
}

static void ConsoleAppend(std::wstring_view text) {
    if (!g_hConsole) return;
    int len = GetWindowTextLengthW(g_hConsole);
    SendMessageW(g_hConsole, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageW(g_hConsole, EM_REPLACESEL, FALSE, (LPARAM)text.data());
}

static void ConsoleNewPrompt() {
    ConsoleAppend(g_prompt);
    int newLen = GetWindowTextLengthW(g_hConsole);
    g_inputStart = (size_t)newLen;
    SendMessageW(g_hConsole, EM_SETSEL, newLen, newLen);
}

static std::wstring ConsoleGetCurrentLine() {
    int totalLen = GetWindowTextLengthW(g_hConsole);
    std::wstring buf; buf.resize((size_t)totalLen);
    GetWindowTextW(g_hConsole, buf.data(), totalLen + 1);
    if (g_inputStart > (size_t)totalLen) return L"";
    std::wstring sub = buf.substr(g_inputStart);
    while (!sub.empty() && (sub.back() == L'\r' || sub.back() == L'\n')) sub.pop_back();
    return sub;
}

// ===================== 执行日志（UTF-8 文件） =====================
static std::wstring GetLogPath() {
    wchar_t exe[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    std::filesystem::path p(exe);
    p.replace_filename(L"pcie_shell.log");
    return p.wstring();
}
static void LogLineUTF8(std::wstring_view line) {
    // 时间戳
    SYSTEMTIME st{}; GetLocalTime(&st);
    std::wstring ts = std::format(L"[{:04}-{:02}-{:02} {:02}:{:02}:{:02}] ",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    std::wstring msg; msg.reserve(ts.size() + line.size() + 2);
    msg.append(ts);
    msg.append(line);
    msg.append(L"\r\n");

    int bytes = WideCharToMultiByte(CP_UTF8, 0, msg.c_str(), (int)msg.size(),
                                    nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) return;
    std::string utf8; utf8.resize(bytes);
    WideCharToMultiByte(CP_UTF8, 0, msg.c_str(), (int)msg.size(),
                        utf8.data(), bytes, nullptr, nullptr);

    std::wstring path = GetLogPath();
    HANDLE h = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return;
    DWORD written = 0;
    WriteFile(h, utf8.data(), (DWORD)utf8.size(), &written, nullptr);
    CloseHandle(h);
}
static void LogCmdResult(const std::wstring& cmd, const std::wstring& result) {
    // 只记录首行结果，避免日志爆长
    std::wstring first = result;
    auto pos = first.find_first_of(L"\r\n");
    if (pos != std::wstring::npos) first = first.substr(0, pos);
    LogLineUTF8(std::format(L"> {} | {}", cmd, first));
}

// ===================== help 文案 =====================
static std::wstring HelpText() {
    return
L"命令列表：\r\n"
L"\r\n"
L"[设备]\r\n"
L"  di open <cardId>        打开设备（cardId通常从0开始）\r\n"
L"  di reset                复位当前设备\r\n"
L"  di close                关闭设备（内部会先复位）\r\n"
L"\r\n"
L"[输入]\r\n"
L"  di get <ch>             读取单通道 DI（0..23）\r\n"
L"  di poll [ms]            轮询 24 路 DI 一次（可在脚本中循环调用）\r\n"
L"\r\n"
L"[输出]\r\n"
L"  do set <ch> <0|1>       设置单通道 DO（0..23）\r\n"
L"  do all <hexMask>        批量写 24 路（bit0→ch0 ... bit23→ch23）\r\n"
L"  do wave odd-even        演示：偶数=0、奇数=1（demo_0）\r\n"
L"\r\n"
L"[触发 / PXIe]\r\n"
L"  trig mode <0|1|2>       工作模式：0单板/1主卡/2从卡（PXIe常用2）\r\n"
L"  trig line <0..7>        选择 PXI_TRIG 线（0..7 对应 PXI_TRIG0..7）\r\n"
L"  trig in cfg <line> <edge> 配置触发输入：line=0..7, edge=参见头文件枚举\r\n"
L"  trig in open            显式创建触发事件句柄（可选）\r\n"
L"  trig in wait <timeout>  等待触发 <timeout_ms> 并读取状态\r\n"
L"  trig in close           关闭触发事件句柄\r\n"
L"\r\n"
L"[演示]\r\n"
L"  demo do                 执行 DO 奇偶演示（odd=1, even=0）\r\n"
L"  demo di                 一次性读取并打印 24 路 DI\r\n"
L"\r\n"
L"[其它]\r\n"
L"  help                    显示本帮助\r\n"
L"\r\n";
}

// ===================== CHR 命令执行器（核心） =====================
static std::wstring ExecuteChrCmd(const std::wstring& line) {
    auto tok = SplitWSV(line);
    if (tok.empty()) return L"";

    auto eq = [](const std::wstring& a, const wchar_t* b) {
        return _wcsicmp(a.c_str(), b) == 0;
    };

    // ---- help ----
    if (eq(tok[0], L"help")) {
        return HelpText();
    }

    // ---- di: 设备生命周期 & 输入 ----
    if (eq(tok[0], L"di")) {
       
		if (tok.size() >= 2 && eq(tok[1], L"open")) {
			if (tok.size() < 3) return L"[err] usage: di open <cardId>\r\n";
			DWORD id=0; if (!TryParseUInt32(tok[2], id)) return L"[err] cardId\r\n";
		
			if (gDev) { 
				CHR44X02_ResetDev(gDev); 
				CHR44X02_CloseDev(gDev); 
				gDev=nullptr; 
			}
		
			// 注意参数顺序：先 HANDLE*，再 BYTE
			int rc = CHR44X02_OpenDev(&gDev, (BYTE)id);
			if (rc != 0 || !gDev) 
				return std::format(L"[err] OpenDev rc={}\r\n", rc);
		
			return std::format(L"[ok] device opened: {}\r\n", id);
		}
		
        if (tok.size() >= 2 && eq(tok[1], L"reset")) {
            if (!gDev) return L"[err] device not open\r\n";
            int rc = CHR44X02_ResetDev(gDev);
            return rc==0 ? L"[ok]\r\n" : std::format(L"[err] Reset rc={}\r\n", rc);
        }
        if (tok.size() >= 2 && eq(tok[1], L"close")) {
            if (!gDev) return L"[ok] already closed\r\n";
            CHR44X02_ResetDev(gDev);
            if (gTrigEvent) { CHR44X02_TrigIn_CloseEvent(gDev, gTrigEvent); gTrigEvent=nullptr; }
            CHR44X02_CloseDev(gDev); gDev=nullptr;
            return L"[ok] device closed\r\n";
        }
        if (tok.size() >= 2 && eq(tok[1], L"get")) {
            if (!gDev) return L"[err] device not open\r\n";
            if (tok.size() < 3) return L"[err] usage: di get <ch>\r\n";
            DWORD ch=0; if (!TryParseUInt32(tok[2], ch) || ch>23) return L"[err] ch=0..23\r\n";
            BYTE st=0; int rc = CHR44X02_IO_GetInputStatus(gDev, (BYTE)ch, &st);
            if (rc!=0) return std::format(L"[err] GetInputStatus rc={}\r\n", rc);
            return std::format(L"DI[{:02}]={}\r\n", ch, st);
        }
        if (tok.size() >= 2 && eq(tok[1], L"poll")) {
            if (!gDev) return L"[err] device not open\r\n";
            DWORD ms=500; if (tok.size()>=3) TryParseUInt32(tok[2], ms);
            // 简化：本命令只打印一次（若要连续轮询，可在脚本/外层循环调用）
            std::wstring out;
            for (DWORD ch=0; ch<24; ++ch) {
                BYTE st=0; CHR44X02_IO_GetInputStatus(gDev, (BYTE)ch, &st);
                out.append(std::format(L"ch[{:02}]={} ", ch, st));
            }
            out.append(L"\r\n");
            return out;
        }
        return L"[err] di cmds: open|reset|close|get|poll\r\n";
    }

    // ---- do: 输出 ----
    if (eq(tok[0], L"do")) {
        if (!gDev) return L"[err] device not open\r\n";
        if (tok.size()>=2 && eq(tok[1], L"set")) {
            if (tok.size()<4) return L"[err] usage: do set <ch 0..23> <0|1>\r\n";
            DWORD ch=0,val=0;
            if (!TryParseUInt32(tok[2], ch) || ch>23) return L"[err] ch=0..23\r\n";
            if (!TryParseUInt32(tok[3], val) || (val!=0 && val!=1)) return L"[err] val=0|1\r\n";
            int rc = CHR44X02_IO_SetOutputStatus(gDev, (BYTE)ch, (BYTE)val);
            if (rc!=0) return std::format(L"[err] SetOutput rc={}\r\n", rc);
            return std::format(L"[ok] DO[{:02}]={}\r\n", ch, val);
        }
        if (tok.size()>=2 && eq(tok[1], L"all")) {
            if (tok.size()<3) return L"[err] usage: do all <hexMask>\r\n";
            unsigned long mask = wcstoul(tok[2].c_str(), nullptr, 16);
            for (int ch=0; ch<24; ++ch) {
                BYTE v = (mask >> ch) & 1 ? 1 : 0;
                CHR44X02_IO_SetOutputStatus(gDev, (BYTE)ch, v);
            }
            return std::format(L"[ok] mask=0x{:06X}\r\n", mask & 0xFFFFFF);
        }
        if (tok.size()>=3 && eq(tok[1], L"wave") && eq(tok[2], L"odd-even")) {
            for (int ch=0; ch<24; ++ch) {
                BYTE v = (ch%2)?1:0;
                CHR44X02_IO_SetOutputStatus(gDev, (BYTE)ch, v);
            }
            return L"[ok] even->0, odd->1 over 24 channels\r\n";
        }
        return L"[err] do cmds: set|all|wave odd-even\r\n";
    }

    // ---- trig: 工作模式 / 触发线 / 触发输入 ----
    if (eq(tok[0], L"trig")) {
        if (!gDev) return L"[err] device not open\r\n";

        if (tok.size()>=2 && eq(tok[1], L"mode")) {
            if (tok.size()<3) return L"[err] usage: trig mode <0|1|2>\r\n";
            DWORD m=0; if (!TryParseUInt32(tok[2], m) || m>2) return L"[err] mode=0|1|2\r\n";
            int rc = CHR44X02_SetWorkMode(gDev, (BYTE)m);
            if (rc!=0) return std::format(L"[err] SetWorkMode rc={}\r\n", rc);
            gWorkMode=(BYTE)m;
            return std::format(L"[ok] workmode={}\r\n", gWorkMode);
        }

        if (tok.size()>=2 && eq(tok[1], L"line")) {
            if (tok.size()<3) return L"[err] usage: trig line <0..7>\r\n";
            DWORD ln=0; if (!TryParseUInt32(tok[2], ln) || ln>7) return L"[err] line=0..7\r\n";
            int rc = CHR44X02_SetTrigLine(gDev, (BYTE)ln);
            if (rc!=0) return std::format(L"[err] SetTrigLine rc={}\r\n", rc);
            gTrigLine=(BYTE)ln;
            return std::format(L"[ok] trigline=PXI_TRIG{}\r\n", gTrigLine);
        }

        if (tok.size()>=4 && eq(tok[1], L"in") && eq(tok[2], L"cfg")) {
            DWORD ln=0, edge=0;
            if (!TryParseUInt32(tok[3], ln) || ln>7) return L"[err] line=0..7\r\n";
            if (tok.size()<5 || !TryParseUInt32(tok[4], edge)) return L"[err] edge(int)\r\n";
            int rc = CHR44X02_TrigIn_Config(gDev, (BYTE)ln, (BYTE)edge);
            if (rc!=0) return std::format(L"[err] TrigIn_Config rc={}\r\n", rc);
            return std::format(L"[ok] TrigIn: line=PXI_TRIG{} edge={}\r\n", ln, edge);
        }

        if (tok.size()>=3 && eq(tok[1], L"in") && eq(tok[2], L"open")) {
            if (gTrigEvent) return L"[ok] event exists\r\n";
            int rc = CHR44X02_TrigIn_CreateEvent(gDev, &gTrigEvent);
            if (rc!=0 || !gTrigEvent) return std::format(L"[err] CreateEvent rc={}\r\n", rc);
            return L"[ok] trig event created\r\n";
        }

        if (tok.size()>=4 && eq(tok[1], L"in") && eq(tok[2], L"wait")) {
            DWORD to=0; if (!TryParseUInt32(tok[3], to)) return L"[err] timeout_ms\r\n";
            if (!gTrigEvent) {
                int rc = CHR44X02_TrigIn_CreateEvent(gDev, &gTrigEvent);
                if (rc!=0 || !gTrigEvent) return std::format(L"[err] CreateEvent rc={}\r\n", rc);
            }
            DWORD dw = CHR44X02_TrigIn_WaitEvent(gDev, gTrigEvent, to);
            BYTE  st = 0;
            CHR44X02_TrigIn_GetStatus(gDev, &st);
            return std::format(L"[wait={}ms] event={} status={}\r\n", to, dw, st);
        }

        if (tok.size()>=3 && eq(tok[1], L"in") && eq(tok[2], L"close")) {
            if (!gTrigEvent) return L"[ok] no event\r\n";
            CHR44X02_TrigIn_CloseEvent(gDev, gTrigEvent);
            gTrigEvent=nullptr;
            return L"[ok] trig event closed\r\n";
        }

        return L"[err] trig cmds: mode|line|in cfg|in open|in wait|in close\r\n";
    }

    // ---- demo: 快捷演示 ----
    if (eq(tok[0], L"demo")) {
        if (!gDev) return L"[err] device not open\r\n";
        if (tok.size()>=2 && eq(tok[1], L"do")) {
            for (int ch=0; ch<24; ++ch) CHR44X02_IO_SetOutputStatus(gDev,(BYTE)ch,(BYTE)((ch%2)?1:0));
            return L"[ok] demo do: odd=1 even=0\r\n";
        }
        if (tok.size()>=2 && eq(tok[1], L"di")) {
            std::wstring out;
            for (int ch=0; ch<24; ++ch) { BYTE st=0; CHR44X02_IO_GetInputStatus(gDev,(BYTE)ch,&st);
                out.append(std::format(L"ch[{:02}]={} ", ch, st)); }
            out.append(L"\r\n");
            return out;
        }
        return L"[err] demo cmds: do|di\r\n";
    }

    // 未识别为 CHR 命令
    return L"";
}

// ===================== 命令执行入口（带日志） =====================
static void ExecuteAndPrint(const std::wstring& line) {
    // 先尝试 CHR 命令（di/do/trig/demo/help）
    std::wstring result = ExecuteChrCmd(line);
    if (result.empty()) {
        // 非法命令时提示用户：用 help 查看
        result = L"[err] 未知命令。输入 help 查看可用命令。\r\n";
    }
    ConsoleAppend(result);
    LogCmdResult(line, result);
}

// ===================== 子类过程 & 键鼠控制 =====================
static bool IsCtrlDown()  { return (GetKeyState(VK_CONTROL) & 0x8000) != 0; }
static bool IsShiftDown() { return (GetKeyState(VK_SHIFT)   & 0x8000) != 0; }
static bool IsPrintable(wchar_t ch) { return ch >= L' ' && ch != 0x7F; }

static void ClampSelectionToEditable(HWND hEdit) {
    DWORD s=0, e=0;
    SendMessageW(hEdit, EM_GETSEL, (WPARAM)&s, (LPARAM)&e);
    if (s < g_inputStart || e < g_inputStart) {
        s = e = (DWORD)g_inputStart;
        SendMessageW(hEdit, EM_SETSEL, s, e);
    }
}

static void ConsoleRunCurrentLine() {
    std::wstring line = ConsoleGetCurrentLine();
    ConsoleAppend(L"\r\n");
    ExecuteAndPrint(line);
    ConsoleNewPrompt();
}

static LRESULT CALLBACK EditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_RETURN:
            ConsoleRunCurrentLine();
            return 0;

        case VK_BACK:
        case VK_DELETE: {
            DWORD s=0, e=0;
            SendMessageW(hWnd, EM_GETSEL, (WPARAM)&s, (LPARAM)&e);
            if ((s == e && s <= g_inputStart) || (s < g_inputStart)) {
                SendMessageW(hWnd, EM_SETSEL, (WPARAM)g_inputStart, (LPARAM)g_inputStart);
                return 0;
            }
            break;
        }
        case VK_LEFT:
        case VK_HOME: {
            if (!IsCtrlDown()) {
                DWORD s=0, e=0;
                SendMessageW(hWnd, EM_GETSEL, (WPARAM)&s, (LPARAM)&e);
                if (IsShiftDown()) {
                    if (s < g_inputStart) s = (DWORD)g_inputStart;
                    if (e < g_inputStart) e = (DWORD)g_inputStart;
                    SendMessageW(hWnd, EM_SETSEL, s, e);
                } else {
                    if (s <= g_inputStart) {
                        SendMessageW(hWnd, EM_SETSEL, (WPARAM)g_inputStart, (LPARAM)g_inputStart);
                        return 0;
                    }
                }
            }
            break;
        }
        default: break;
        }
        break;

    case WM_CHAR:
        if (wParam == VK_RETURN) return 0;
        if (IsPrintable((wchar_t)wParam)) {
            DWORD s=0, e=0;
            SendMessageW(hWnd, EM_GETSEL, (WPARAM)&s, (LPARAM)&e);
            if (s < g_inputStart) {
                SendMessageW(hWnd, EM_SETSEL, (WPARAM)g_inputStart, (LPARAM)g_inputStart);
            }
        }
        break;

    case WM_PASTE:
        ClampSelectionToEditable(hWnd);
        break;

    default: break;
    }
    return CallWindowProcW(g_OldEditProc, hWnd, msg, wParam, lParam);
}

// ===================== 窗口过程 =====================
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
    {
        g_hBrushBk = CreateSolidBrush(g_backColor);

        LOGFONTW lf{}; lf.lfHeight = -32; // 字体放大
        StringCchCopyW(lf.lfFaceName, LF_FACESIZE, L"Consolas");
        g_hFont = CreateFontIndirectW(&lf);

        RECT rc; GetClientRect(hWnd, &rc);
        g_hConsole = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
            rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
            hWnd, (HMENU)1001, GetModuleHandleW(nullptr), nullptr);
        SendMessageW(g_hConsole, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        g_OldEditProc = (WNDPROC)SetWindowLongPtrW(g_hConsole, GWLP_WNDPROC, (LONG_PTR)EditProc);

        // 欢迎信息 + 帮助提示
        ConsoleAppend(L"PCIe Shell\r\n输入 help 查看命令。\r\n\r\n");
        ConsoleNewPrompt();
        return 0;
    }
    case WM_SIZE:
        if (g_hConsole) MoveWindow(g_hConsole, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        return 0;

    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, g_textColor);
        SetBkColor(hdc, g_backColor);
        return (LRESULT)g_hBrushBk;
    }

    case WM_DESTROY:
        if (g_hBrushBk) { DeleteObject(g_hBrushBk); g_hBrushBk=nullptr; }
        if (g_hFont)    { DeleteObject(g_hFont);    g_hFont=nullptr;   }
        // 程序退出前，确保资源清理
        if (gDev) {
            if (gTrigEvent) { CHR44X02_TrigIn_CloseEvent(gDev, gTrigEvent); gTrigEvent=nullptr; }
            CHR44X02_ResetDev(gDev);
            CHR44X02_CloseDev(gDev); gDev=nullptr;
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ===================== wWinMain =====================
int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow)
{
    // 高DPI
    if (auto shcore = LoadLibraryW(L"Shcore.dll")) {
        using SetDpiAwareFn = HRESULT (WINAPI*)(UINT);
        if (auto p = (SetDpiAwareFn)GetProcAddress(shcore, "SetProcessDpiAwareness")) p(2);
        FreeLibrary(shcore);
    }

    WNDCLASSEXW wc{ sizeof(wc) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursorW(nullptr, IDC_IBEAM);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName = kWndClass;
    wc.hIconSm = wc.hIcon;

    if (!RegisterClassExW(&wc)) {
        MessageBoxW(nullptr, L"RegisterClassEx 失败", L"错误", MB_ICONERROR);
        return -1;
    }

    g_hWndMain = CreateWindowExW(0, kWndClass, L"PCIe Shell - PowerShell Style",
                                 WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                 1000, 600, nullptr, nullptr, hInst, nullptr);
    if (!g_hWndMain) {
        MessageBoxW(nullptr, L"CreateWindowEx 失败", L"错误", MB_ICONERROR);
        return -1;
    }

    ShowWindow(g_hWndMain, nCmdShow);
    UpdateWindow(g_hWndMain);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}

