// TemperatureCPP.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "TemperatureCPP.h"
#include <cmath>
#include <vector>
#include <climits>
#include <cstdio>

#define MAX_LOADSTRING 100

#define IDC_EDIT_A          1001
#define IDC_EDIT_B          1002
#define IDC_EDIT_F          1003
#define IDC_EDIT_G          1004
#define IDC_EDIT_TSTART     1005
#define IDC_EDIT_TEND       1006
#define IDC_EDIT_LAMBDA     1007
#define IDC_EDIT_N          1008
#define IDC_EDIT_DELTAX     1009
#define IDC_EDIT_EPSILON    1010
#define IDC_EDIT_TMAX       1011
#define IDC_BTN_START       1012
#define IDC_CHK_WATCHTMAX   1013
#define IDC_CHK_DRAWABLE    1014
#define IDC_RADIO_TRIPLE    1015
#define IDC_RADIO_QUAD      1016
#define IDC_RADIO_QUINT     1017
#define IDC_LABEL_STEPS     1018
#define IDC_STATIC_BASE     1200

#define DRAW_X 280
#define DRAW_Y 10
#define DRAW_W 660
#define DRAW_H 620

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

HWND hEditA, hEditB, hEditF, hEditG;
HWND hEditTStart, hEditTEnd, hEditLambda, hEditN, hEditDeltaX, hEditEpsilon, hEditTMax;
HWND hBtnStart;
HWND hChkWatchTMax, hChkDrawable;
HWND hRadioTriple, hRadioQuad, hRadioQuint;
HWND hLabelSteps;

std::vector<double> g_pieces;
bool g_hasResult = false;
int  g_lastSteps = 0;
bool g_updating = false;

ATOM                MyRegisterClass(HINSTANCE);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

static void CreateControls(HWND hWnd, HINSTANCE hInstance);
static void DoCalculation(HWND hWnd);
static void PaintBars(HWND hWnd, HDC hdc);

static bool GetDbl(HWND h, double& out) {
    wchar_t buf[64];
    GetWindowTextW(h, buf, 64);
    wchar_t* end = nullptr;
    out = wcstod(buf, &end);
    return end != buf;
}

static bool GetInt(HWND h, int& out) {
    wchar_t buf[64];
    GetWindowTextW(h, buf, 64);
    wchar_t* end = nullptr;
    long v = wcstol(buf, &end, 10);
    if (end == buf) return false;
    out = (int)v;
    return true;
}

static void SetDbl(HWND h, double v) {
    wchar_t buf[64];
    swprintf_s(buf, L"%.6g", v);
    SetWindowTextW(h, buf);
}

static double FindC(double a, double b, double f, double g, double T) {
    double c = a * T * T * T + b * T * T + f * T + g;
    return c > 1e-10 ? c : 1e-10;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TEMPERATURECPP, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TEMPERATURECPP));
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}





ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex{};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TEMPERATURECPP));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);   // как в WinForms
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_TEMPERATURECPP);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassExW(&wcex);
}

static void CreateControls(HWND hWnd, HINSTANCE hInstance)
{
    struct FieldDef {
        const wchar_t* label;
        int            editId;
        HWND* edit;
        const wchar_t* defVal;
    };

    FieldDef fields[] = {
        { L"a:",        IDC_EDIT_A,       &hEditA,       L"0"      },
        { L"b:",        IDC_EDIT_B,       &hEditB,       L"0.00001"},
        { L"f:",        IDC_EDIT_F,       &hEditF,       L"0.003"  },
        { L"g:",        IDC_EDIT_G,       &hEditG,       L"10"     },
        { L"T start:",  IDC_EDIT_TSTART,  &hEditTStart,  L"300"    },
        { L"T end:",    IDC_EDIT_TEND,    &hEditTEnd,    L"400"    },
        { L"lambda:",   IDC_EDIT_LAMBDA,  &hEditLambda,  L"1"      },
        { L"n:",        IDC_EDIT_N,       &hEditN,       L"10"     },
        { L"deltaX:",   IDC_EDIT_DELTAX,  &hEditDeltaX,  L"0.1"    },
        { L"epsilon:",  IDC_EDIT_EPSILON, &hEditEpsilon, L"0.01" },
        { L"t max:",    IDC_EDIT_TMAX,    &hEditTMax,    L"1000000"},
    };

    int y = 10;
    for (auto& fd : fields) {
        CreateWindowW(L"STATIC", fd.label,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, y + 4, 110, 20, hWnd, nullptr, hInstance, nullptr);

        *fd.edit = CreateWindowW(L"EDIT", fd.defVal,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_LEFT,
            130, y, 120, 22, hWnd,
            (HMENU)(INT_PTR)fd.editId, hInstance, nullptr);
        y += 28;
    }

    // Радиокнопки
    y += 8;
    hRadioTriple = CreateWindowW(L"BUTTON", L"deltaT = 0.001",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        10, y, 240, 22, hWnd, (HMENU)IDC_RADIO_TRIPLE, hInstance, nullptr);
    y += 25;
    hRadioQuad = CreateWindowW(L"BUTTON", L"deltaT = 0.0001",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        10, y, 240, 22, hWnd, (HMENU)IDC_RADIO_QUAD, hInstance, nullptr);
    y += 25;
    hRadioQuint = CreateWindowW(L"BUTTON", L"deltaT = 0.00001",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        10, y, 240, 22, hWnd, (HMENU)IDC_RADIO_QUINT, hInstance, nullptr);
    y += 30;

    // Чекбоксы
    hChkWatchTMax = CreateWindowW(L"BUTTON", L"Ограничить t max",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_GROUP,
        10, y, 240, 22, hWnd, (HMENU)IDC_CHK_WATCHTMAX, hInstance, nullptr);
    y += 25;
    hChkDrawable = CreateWindowW(L"BUTTON", L"Рисовать результат",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        10, y, 240, 22, hWnd, (HMENU)IDC_CHK_DRAWABLE, hInstance, nullptr);
    SendMessage(hChkDrawable, BM_SETCHECK, BST_CHECKED, 0);
    y += 35;

    // Кнопка «Старт»
    hBtnStart = CreateWindowW(L"BUTTON", L"Старт",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, y, 120, 32, hWnd, (HMENU)IDC_BTN_START, hInstance, nullptr);
    y += 40;

    // Счётчик шагов
    CreateWindowW(L"STATIC", L"Шагов:",
        WS_CHILD | WS_VISIBLE,
        10, y + 2, 60, 20, hWnd, nullptr, hInstance, nullptr);
    hLabelSteps = CreateWindowW(L"STATIC", L"0",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        75, y + 2, 200, 20, hWnd, (HMENU)IDC_LABEL_STEPS, hInstance, nullptr);

    // Радиокнопки по умолчанию
    SendMessage(hRadioTriple, BM_SETCHECK, BST_CHECKED, 0);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle,
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, 0, 1020, 720,
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return FALSE;

    CreateControls(hWnd, hInstance);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

static void DoCalculation(HWND hWnd)
{
    double a, b, f, g, temperatureStart, temperatureEnd, lambda, deltaX, accuracy;
    int intervalCount = 0;

    if (!GetDbl(hEditA, a) || !GetDbl(hEditB, b) || !GetDbl(hEditF, f) ||
        !GetDbl(hEditG, g) || !GetDbl(hEditTStart, temperatureStart) ||
        !GetDbl(hEditTEnd, temperatureEnd) || !GetDbl(hEditLambda, lambda) ||
        !GetDbl(hEditDeltaX, deltaX) || !GetDbl(hEditEpsilon, accuracy) ||
        !GetInt(hEditN, intervalCount) || intervalCount <= 1 || deltaX <= 0.0) {
        MessageBoxW(hWnd, L"Некорректные входные данные.", L"Ошибка",
            MB_OK | MB_ICONERROR);
        return;
    }

    double deltaT = 0.0;
    if (IsDlgButtonChecked(hWnd, IDC_RADIO_TRIPLE) == BST_CHECKED)      deltaT = 0.001;
    else if (IsDlgButtonChecked(hWnd, IDC_RADIO_QUAD) == BST_CHECKED) deltaT = 0.0001;
    else if (IsDlgButtonChecked(hWnd, IDC_RADIO_QUINT) == BST_CHECKED) deltaT = 0.00001;
    else {
        MessageBoxW(hWnd, L"Не выбрана точность по времени (deltaT).", L"Ошибка",
            MB_OK | MB_ICONERROR);
        return;
    }

    double C_min = a * temperatureStart * temperatureStart * temperatureStart
        + b * temperatureStart * temperatureStart
        + f * temperatureStart + g;
    if (C_min <= 1e-10) C_min = 1e-10;

    double deltaT_max = 0.5 * C_min * deltaX * deltaX / lambda;
    if (deltaT > deltaT_max) {
        wchar_t msg[512];
        swprintf_s(msg,
            L"Внимание! Выбранный deltaT = %g превышает максимально допустимый для устойчивости: %g.\nРешение может быть неустойчивым.",
            deltaT, deltaT_max);
        MessageBoxW(hWnd, msg, L"Предупреждение", MB_OK | MB_ICONWARNING);
    }

    std::vector<double> pieces(intervalCount + 1);   // +1
    pieces[0] = temperatureEnd;
    for (int i = 1; i <= intervalCount; ++i) pieces[i] = temperatureStart;

    int tMax = INT_MAX;
    if (IsDlgButtonChecked(hWnd, IDC_CHK_WATCHTMAX) == BST_CHECKED) {
        int tmp = 1000000;
        GetInt(hEditTMax, tmp);
        tMax = tmp;
    }

    int t = 0;
    while (fabs(pieces[intervalCount] - temperatureEnd) > accuracy && tMax >= t) {
        for (int i = 1; i < intervalCount; ++i) {
            double C = FindC(a, b, f, g, pieces[i]);
            pieces[i] += lambda * deltaT * (pieces[i + 1] - 2 * pieces[i] + pieces[i - 1]) / (deltaX * deltaX * C);
        }
        int last = intervalCount;
        double C = FindC(a, b, f, g, pieces[last]);
        pieces[last] += deltaT * lambda * 2 * (pieces[last - 1] - pieces[last]) / (deltaX * deltaX * C);
        ++t;
    }

    g_pieces = std::move(pieces);
    g_hasResult = true;
    g_lastSteps = t;

    wchar_t buf[64];
    swprintf_s(buf, L"%d", t);
    SetWindowTextW(hLabelSteps, buf);

    InvalidateRect(hWnd, nullptr, TRUE);
}


static void PaintBars(HWND hWnd, HDC hdc)
{
    if (!g_hasResult || g_pieces.empty()) return;
    if (IsDlgButtonChecked(hWnd, IDC_CHK_DRAWABLE) != BST_CHECKED) return;

    double temperatureStart = 0.0, temperatureEnd = 0.0;
    GetDbl(hEditTStart, temperatureStart);
    GetDbl(hEditTEnd, temperatureEnd);

    RECT area = { DRAW_X, DRAW_Y, DRAW_X + DRAW_W, DRAW_Y + DRAW_H };
    HBRUSH hWhite = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hdc, &area, hWhite);
    DeleteObject(hWhite);

    int n = (int)g_pieces.size();
    double denom = (temperatureEnd - temperatureStart);
    double stepSize = (double)DRAW_W / n;

    for (int i = 0; i < n; ++i) {
        double value = denom != 0.0
            ? (g_pieces[i] - temperatureStart) / denom
            : 0.0;

        int r, g, b;
        if (value < 0.33333) {
            r = 0;
            g = (int)(255.0 * value * 3.0);
            b = 255;
        }
        else if (value < 0.66666) {
            r = (int)(255.0 * (value - 0.33333) * 3.0);
            g = 255;
            b = (int)(255.0 * (1.0 - (value - 0.33333) * 3.0));
        }
        else {
            r = 255;
            g = (int)(255.0 * (1.0 - (value - 0.66666) * 3.0));
            b = 0;
        }

        if (r < 0) r = 0; if (r > 255) r = 255;
        if (g < 0) g = 0; if (g > 255) g = 255;
        if (b < 0) b = 0; if (b > 255) b = 255;

        RECT rc;
        rc.left = DRAW_X + (int)(stepSize * i);
        rc.right = DRAW_X + (int)(stepSize * (i + 1));
        rc.top = DRAW_Y;
        rc.bottom = DRAW_Y + DRAW_H;

        HBRUSH br = CreateSolidBrush(RGB(r, g, b));
        FillRect(hdc, &rc, br);
        DeleteObject(br);
    }

    // Рамка вокруг графика
    FrameRect(hdc, &area, (HBRUSH)GetStockObject(BLACK_BRUSH));
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        int wmEvent = HIWORD(wParam);

        if (wmId == IDC_BTN_START && wmEvent == BN_CLICKED) {
            DoCalculation(hWnd);
            return 0;
        }

        if (wmId == IDC_EDIT_N && wmEvent == EN_CHANGE && !g_updating) {
            g_updating = true;
            double n;
            if (GetDbl(hEditN, n) && n != 0.0)
                SetDbl(hEditDeltaX, 1.0 / n);
            else
                SetWindowTextW(hEditDeltaX, L"###");
            g_updating = false;
            return 0;
        }

        if (wmId == IDC_EDIT_DELTAX && wmEvent == EN_CHANGE && !g_updating) {
            g_updating = true;
            double dx;
            if (GetDbl(hEditDeltaX, dx) && dx != 0.0)
                SetDbl(hEditN, 1.0 / dx);
            else
                SetWindowTextW(hEditN, L"###");
            g_updating = false;
            return 0;
        }

        if (wmId == IDC_CHK_DRAWABLE && wmEvent == BN_CLICKED) {
            InvalidateRect(hWnd, nullptr, TRUE);
            return 0;
        }

        switch (wmId) {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        PaintBars(hWnd, hdc);
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Неявную схему реализовать
// Сравнить что происходит при c || lambda = const и c || lambda != const
// Сохранять результат каждого шага для будущей отрисовки на графике абсциса=t, ордината=T по конкретной точке