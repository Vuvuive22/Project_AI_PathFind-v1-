// handheld_calculator.cpp
// Single-file Win32 calculator (simple handheld style)
// Build with Visual Studio (cl) or MinGW (g++)
// Example:
//  cl /EHsc handheld_calculator.cpp user32.lib gdi32.lib comctl32.lib
//  or
//  g++ -municode -mwindows handheld_calculator.cpp -o calc.exe -lcomctl32

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <string>
#include <sstream>
#include <cmath>

const wchar_t CLASS_NAME[] = L"HandheldCalcClass";

// control IDs
enum {
    ID_DISPLAY = 1000,
    ID_BTN_0, ID_BTN_1, ID_BTN_2, ID_BTN_3, ID_BTN_4,
    ID_BTN_5, ID_BTN_6, ID_BTN_7, ID_BTN_8, ID_BTN_9,
    ID_BTN_DOT, ID_BTN_EQ, ID_BTN_ADD, ID_BTN_SUB,
    ID_BTN_MUL, ID_BTN_DIV, ID_BTN_CLR, ID_BTN_BS
};

// app state
struct CalcState {
    std::wstring display = L"0";
    double operand = 0.0;          // stored operand
    wchar_t op = 0;                // '+','-','*','/' or 0
    bool entering = true;          // are we entering a number now?
    bool justEvaluated = false;    // last action was evaluation
} g_state;

HWND g_hDisplay = NULL;

void UpdateDisplay() {
    if (g_hDisplay) SetWindowTextW(g_hDisplay, g_state.display.c_str());
}

void ResetCalc() {
    g_state.display = L"0";
    g_state.operand = 0.0;
    g_state.op = 0;
    g_state.entering = true;
    g_state.justEvaluated = false;
    UpdateDisplay();
}

double ToDouble(const std::wstring &s) {
    std::wistringstream iss(s);
    double v = 0.0;
    iss >> v;
    return v;
}

std::wstring ToWString(double v) {
    std::wostringstream oss;
    // avoid scientific notation for typical calculator range
    oss.setf(std::ios::fmtflags(0), std::ios::floatfield);
    oss << v;
    std::wstring s = oss.str();
    // trim trailing zeros after decimal point
    if (s.find(L'.') != std::wstring::npos) {
        while (!s.empty() && s.back() == L'0') s.pop_back();
        if (!s.empty() && s.back() == L'.') s.pop_back();
        if (s.empty()) s = L"0";
    }
    return s;
}

void PressDigit(int d) {
    if (g_state.justEvaluated) {
        // start new entry
        g_state.display = L"0";
        g_state.justEvaluated = false;
    }
    if (!g_state.entering) {
        g_state.display = L"0";
        g_state.entering = true;
    }
    if (g_state.display == L"0") g_state.display.clear();
    g_state.display.push_back((wchar_t)(L'0' + d));
    UpdateDisplay();
}

void PressDot() {
    if (g_state.justEvaluated) {
        g_state.display = L"0";
        g_state.justEvaluated = false;
        g_state.entering = true;
    }
    if (g_state.display.find(L'.') == std::wstring::npos) {
        g_state.display.push_back(L'.');
        UpdateDisplay();
    }
}

void ApplyPendingOp() {
    double current = ToDouble(g_state.display);
    double result = current;
    switch (g_state.op) {
    case '+': result = g_state.operand + current; break;
    case '-': result = g_state.operand - current; break;
    case '*': result = g_state.operand * current; break;
    case '/':
        if (fabs(current) < 1e-15) {
            g_state.display = L"Error";
            g_state.op = 0;
            g_state.entering = false;
            g_state.justEvaluated = true;
            UpdateDisplay();
            return;
        } else {
            result = g_state.operand / current;
        }
        break;
    default:
        result = current;
    }
    g_state.display = ToWString(result);
    g_state.operand = result;
}

void PressOp(wchar_t op) {
    if (g_state.op && g_state.entering) {
        // there is a pending operation; evaluate first
        ApplyPendingOp();
    } else if (!g_state.op) {
        g_state.operand = ToDouble(g_state.display);
    }
    g_state.op = op;
    g_state.entering = false;
    g_state.justEvaluated = false;
    UpdateDisplay();
}

void PressEqual() {
    if (g_state.op) {
        ApplyPendingOp();
        g_state.op = 0;
        g_state.entering = false;
        g_state.justEvaluated = true;
        UpdateDisplay();
    }
}

void PressBackspace() {
    if (g_state.justEvaluated) return; // no-op after evaluation
    if (!g_state.entering) return;
    if (g_state.display.size() <= 1) {
        g_state.display = L"0";
    } else {
        g_state.display.pop_back();
        if (g_state.display == L"-" || g_state.display == L"" ) g_state.display = L"0";
    }
    UpdateDisplay();
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // create a simple display static control
        g_hDisplay = CreateWindowExW(0, L"EDIT", L"0",
            WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_READONLY | WS_BORDER,
            8, 8, 260, 40, hwnd, (HMENU)ID_DISPLAY, GetModuleHandle(NULL), NULL);
        // create buttons in grid (4 cols x 5 rows)
        const int btnW = 60, btnH = 40, margin = 8;
        int left = 8, top = 58;
        // row 1: 7 8 9 /
        HWND h;
        h = CreateWindowW(L"BUTTON", L"7", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, left, top, btnW, btnH, hwnd, (HMENU)ID_BTN_7, GetModuleHandle(NULL), NULL);
        h = CreateWindowW(L"BUTTON", L"8", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, left+btnW+margin, top, btnW, btnH, hwnd, (HMENU)ID_BTN_8, GetModuleHandle(NULL), NULL);
        h = CreateWindowW(L"BUTTON", L"9", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, left+2*(btnW+margin), top, btnW, btnH, hwnd, (HMENU)ID_BTN_9, GetModuleHandle(NULL), NULL);
        h = CreateWindowW(L"BUTTON", L"/", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, left+3*(btnW+margin), top, btnW, btnH, hwnd, (HMENU)ID_BTN_DIV, GetModuleHandle(NULL), NULL);
        // row 2: 4 5 6 *
        top += btnH + margin;
        h = CreateWindowW(L"BUTTON", L"4", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, left, top, btnW, btnH, hwnd, (HMENU)ID_BTN_4, GetModuleHandle(NULL), NULL);
        h = CreateWindowW(L"BUTTON", L"5", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, left+btnW+margin, top, btnW, btnH, hwnd, (HMENU)ID_BTN_5, GetModuleHandle(NULL), NULL);
        h = CreateWindowW(L"BUTTON", L"6", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, left+2*(btnW+margin), top, btnW, btnH, hwnd, (HMENU)ID_BTN_6, GetModuleHandle(NULL), NULL);
        h = CreateWindowW(L"BUTTON", L"*", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, left+3*(btnW+margin), top, btnW, btnH, hwnd, (HMENU)ID_BTN_MUL, GetModuleHandle(NULL), NULL);
        // row 3: 1 2 3 -
        top += btnH + margin;
        h = CreateWindowW(L"BUTTON", L"1", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, left, top, btnW, btnH, hwnd, (HMENU)ID_BTN_1, GetModuleHandle(NULL), NULL);
        h = CreateWindowW(L"BUTTON", L"2", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, left+btnW+margin, top, btnW, btnH, hwnd, (HMENU)ID_BTN_2, GetModuleHandle(NULL), NULL);
        h = CreateWindowW(L"BUTTON", L"3", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, left+2*(btnW+margin), top, btnW, btnH, hwnd, (HMENU)ID_BTN_3, GetModuleHandle(NULL), NULL);
        h = CreateWindowW(L"BUTTON", L"-", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, left+3*(btnW+margin), top, btnW, btnH, hwnd, (HMENU)ID_BTN_SUB, GetModuleHandle(NULL), NULL);
        // row 4: 0 . = +
        top += btnH + margin;
        h = CreateWindowW(L"BUTTON", L"0", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, left, top, btnW, btnH, hwnd, (HMENU)ID_BTN_0, GetModuleHandle(NULL), NULL);
        h = CreateWindowW(L"BUTTON", L".", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, left+btnW+margin, top, btnW, btnH, hwnd, (HMENU)ID_BTN_DOT, GetModuleHandle(NULL), NULL);
        h = CreateWindowW(L"BUTTON", L"=", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, left+2*(btnW+margin), top, btnW, btnH, hwnd, (HMENU)ID_BTN_EQ, GetModuleHandle(NULL), NULL);
        h = CreateWindowW(L"BUTTON", L"+", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, left+3*(btnW+margin), top, btnW, btnH, hwnd, (HMENU)ID_BTN_ADD, GetModuleHandle(NULL), NULL);
        // extra controls: Clear and Backspace
        top += btnH + margin;
        h = CreateWindowW(L"BUTTON", L"C", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, left, top, btnW, btnH, hwnd, (HMENU)ID_BTN_CLR, GetModuleHandle(NULL), NULL);
        h = CreateWindowW(L"BUTTON", L"BS", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, left+btnW+margin, top, btnW, btnH, hwnd, (HMENU)ID_BTN_BS, GetModuleHandle(NULL), NULL);
        // set font for controls
        HFONT hf = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        SendMessageW(g_hDisplay, WM_SETFONT, (WPARAM)hf, TRUE);
        // increase display font
        LOGFONTW lf;
        GetObjectW(GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
        lf.lfHeight = 22;
        HFONT hFontDisplay = CreateFontIndirectW(&lf);
        SendMessageW(g_hDisplay, WM_SETFONT, (WPARAM)hFontDisplay, TRUE);
        // set font on buttons
        for (HWND child = GetWindow(hwnd, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
            SendMessageW(child, WM_SETFONT, (WPARAM)hf, TRUE);
        }
        break;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        switch (id) {
        case ID_BTN_0: PressDigit(0); break;
        case ID_BTN_1: PressDigit(1); break;
        case ID_BTN_2: PressDigit(2); break;
        case ID_BTN_3: PressDigit(3); break;
        case ID_BTN_4: PressDigit(4); break;
        case ID_BTN_5: PressDigit(5); break;
        case ID_BTN_6: PressDigit(6); break;
        case ID_BTN_7: PressDigit(7); break;
        case ID_BTN_8: PressDigit(8); break;
        case ID_BTN_9: PressDigit(9); break;
        case ID_BTN_DOT: PressDot(); break;
        case ID_BTN_ADD: PressOp(L'+'); break;
        case ID_BTN_SUB: PressOp(L'-'); break;
        case ID_BTN_MUL: PressOp(L'*'); break;
        case ID_BTN_DIV: PressOp(L'/'); break;
        case ID_BTN_EQ: PressEqual(); break;
        case ID_BTN_CLR: ResetCalc(); break;
        case ID_BTN_BS: PressBackspace(); break;
        default: break;
        }
        break;
    }
    case WM_KEYDOWN: {
        // handle numeric keyboard and operators
        switch (wParam) {
        case VK_NUMPAD0: case '0': PressDigit(0); break;
        case VK_NUMPAD1: case '1': PressDigit(1); break;
        case VK_NUMPAD2: case '2': PressDigit(2); break;
        case VK_NUMPAD3: case '3': PressDigit(3); break;
        case VK_NUMPAD4: case '4': PressDigit(4); break;
        case VK_NUMPAD5: case '5': PressDigit(5); break;
        case VK_NUMPAD6: case '6': PressDigit(6); break;
        case VK_NUMPAD7: case '7': PressDigit(7); break;
        case VK_NUMPAD8: case '8': PressDigit(8); break;
        case VK_NUMPAD9: case '9': PressDigit(9); break;
        case VK_ADD: case '+': PressOp(L'+'); break;
        case VK_SUBTRACT: case '-': PressOp(L'-'); break;
        case VK_MULTIPLY: case '*': PressOp(L'*'); break;
        case VK_DIVIDE: case '/': PressOp(L'/'); break;
        case VK_DECIMAL: case '.': PressDot(); break;
        case VK_RETURN: case VK_OEM_PLUS: PressEqual(); break;
        case VK_BACK: PressBackspace(); break;
        case VK_ESCAPE: ResetCalc(); break;
        default: break;
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    WNDCLASSW wc = { };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"Handheld Calculator",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 360, NULL, NULL, hInstance, NULL);

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    ResetCalc();

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
