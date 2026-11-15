// TemperatureCPP.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "TemperatureCPP.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <climits>
#include <cstdio>
#include <string>
#include <sstream>

#define MAX_LOADSTRING 100

#define IDC_EDIT_A          1001
#define IDC_EDIT_B          1002
#define IDC_EDIT_F          1003
#define IDC_EDIT_G          1004
#define IDC_EDIT_TSTART     1005
#define IDC_EDIT_TEND       1006
#define IDC_EDIT_LAMBDA     1007
#define IDC_EDIT_N          1008
#define IDC_EDIT_LENGHT     1009
#define IDC_EDIT_EPSILON    1010
#define IDC_EDIT_TMAX       1011
#define IDC_EDIT_XCOORD     1012
#define IDC_BTN_START       1013
#define IDC_BTN_DRAW        1014
#define IDC_BTN_DRAW_COORD  1015
#define IDC_CHK_WATCHTMAX   1016
#define IDC_CHK_JACOBI      1017
#define IDC_RADIO_TRIPLE    1018
#define IDC_RADIO_QUAD      1019
#define IDC_RADIO_QUINT     1020
#define IDC_LABEL_STEPS     1021
#define IDC_LABEL_DELTAX    1022
#define IDC_STATIC_BASE     1200

#define DRAW_X 280
#define DRAW_Y 10
#define DRAW_W 660
#define DRAW_H 620

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

HWND hEditA, hEditB, hEditF, hEditG;
HWND hEditTStart, hEditTEnd, hEditLambda, hEditN, hEditLength, hEditEpsilon, hEditTMax, hEditXCoord;
HWND hBtnStart, hBtnDraw, hBtnDrawCoord;
HWND hChkWatchTMax, hChkJacobi;
HWND hRadioTriple, hRadioQuad, hRadioQuint;
HWND hLabelSteps, hLabelDeltaX;

std::vector<double>      g_pieces;
bool                     g_hasResult = false;
int                      g_lastSteps = 0;
int                      g_maxSnapshots = 100;
int                      whichDraw = -1;
std::vector<
    std::vector<double>> g_snapshots;
std::vector<double>      g_snapshotTimes;

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

static double FindLambda(double constant, double T) {
    double lambda = constant * T;
    return lambda > 1e-10 ? lambda : 1e-10;
}

static void UpdateDeltaXLabel() {
    double n, len;
    if (GetDbl(hEditN, n) && GetDbl(hEditLength, len)) {
        wchar_t buf[64];
        swprintf_s(buf, L"%.6g м.", len / n);
        SetWindowTextW(hLabelDeltaX, buf);
    }
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
        { L"lenght:",   IDC_EDIT_LENGHT,  &hEditLength,  L"1"    },
        { L"epsilon:",  IDC_EDIT_EPSILON, &hEditEpsilon, L"0.01" },
        { L"t max:",    IDC_EDIT_TMAX,    &hEditTMax,    L"1000000"},
        { L"x coord:",  IDC_EDIT_XCOORD,  &hEditXCoord,  L"0.5"},
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
    hChkJacobi = CreateWindowW(L"BUTTON", L"Использовать явную схему",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        10, y, 240, 22, hWnd, (HMENU)IDC_CHK_JACOBI, hInstance, nullptr);
    y += 30;

    // Кнопки
    hBtnStart = CreateWindowW(L"BUTTON", L"Старт",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, y, 180, 32, hWnd, (HMENU)IDC_BTN_START, hInstance, nullptr);
    y += 37;
    hBtnDraw = CreateWindowW(L"BUTTON", L"Рисовать тепловую карту",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, y, 180, 32, hWnd, (HMENU)IDC_BTN_DRAW, hInstance, nullptr);
    y += 37;
    hBtnDrawCoord = CreateWindowW(L"BUTTON", L"Рисовать по координате",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, y, 180, 32, hWnd, (HMENU)IDC_BTN_DRAW_COORD, hInstance, nullptr);
    y += 40;

    // Счётчик шагов
    CreateWindowW(L"STATIC", L"Шагов:",
        WS_CHILD | WS_VISIBLE,
        10, y + 2, 60, 20, hWnd, nullptr, hInstance, nullptr);
    hLabelSteps = CreateWindowW(L"STATIC", L"0",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        75, y + 2, 200, 20, hWnd, (HMENU)IDC_LABEL_STEPS, hInstance, nullptr);
    y += 20;

    // Длина шага
    CreateWindowW(L"STATIC", L"Шаг:",
        WS_CHILD | WS_VISIBLE,
        10, y + 2, 60, 20, hWnd, nullptr, hInstance, nullptr);
    hLabelDeltaX = CreateWindowW(L"STATIC", L"0",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        75, y + 2, 200, 20, hWnd, (HMENU)IDC_LABEL_DELTAX, hInstance, nullptr);

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
    UpdateDeltaXLabel();
    return TRUE;
}

static void DoCalculation(HWND hWnd)
{
    g_snapshots.clear();   // иначе история копится между запусками
    g_snapshotTimes.clear();

    double a, b, f, g, temperatureStart, temperatureEnd, lambda, lenght, accuracy;
    int intervalCount = 0;

    if (!GetDbl(hEditA, a) || !GetDbl(hEditB, b) || !GetDbl(hEditF, f) ||
        !GetDbl(hEditG, g) || !GetDbl(hEditTStart, temperatureStart) ||
        !GetDbl(hEditTEnd, temperatureEnd) || !GetDbl(hEditLambda, lambda) ||
        !GetDbl(hEditLength, lenght) || !GetDbl(hEditEpsilon, accuracy) ||
        !GetInt(hEditN, intervalCount) || intervalCount <= 1 || lenght <= 0.0) {
        MessageBoxW(hWnd, L"Некорректные входные данные.", L"Ошибка",
            MB_OK | MB_ICONERROR);
        return;
    }

    double deltaX = lenght / intervalCount;
    double deltaT = 0.0;
    if (IsDlgButtonChecked(hWnd, IDC_RADIO_TRIPLE) == BST_CHECKED)      deltaT = 0.001;
    else if (IsDlgButtonChecked(hWnd, IDC_RADIO_QUAD) == BST_CHECKED) deltaT = 0.0001;
    else if (IsDlgButtonChecked(hWnd, IDC_RADIO_QUINT) == BST_CHECKED) deltaT = 0.00001;
    else {
        MessageBoxW(hWnd, L"Не выбрана точность по времени (deltaT).", L"Ошибка",
            MB_OK | MB_ICONERROR);
        return;
    }

    bool isExplicit = true;

    if (IsDlgButtonChecked(hWnd, IDC_CHK_JACOBI) != BST_CHECKED) {
        isExplicit = false;
    }

    double C_min = a * temperatureStart * temperatureStart * temperatureStart
        + b * temperatureStart * temperatureStart
        + f * temperatureStart + g;
    if (C_min <= 1e-10) C_min = 1e-10;

    std::vector<double> pieces(intervalCount + 1);

    pieces[0] = temperatureEnd;
    for (int i = 1; i <= intervalCount; ++i) pieces[i] = temperatureStart;

    int tMax = INT_MAX;
    if (IsDlgButtonChecked(hWnd, IDC_CHK_WATCHTMAX) == BST_CHECKED) {
        int tmp = 1000000;
        GetInt(hEditTMax, tmp);
        tMax = tmp;
    }

    int t = 0;
    if (isExplicit)
    {
        double deltaT_max = 0.5 * C_min * deltaX * deltaX / lambda;
        if (deltaT > deltaT_max) {
            wchar_t msg[512];
            swprintf_s(msg,
                L"Внимание! Выбранный deltaT = %g превышает максимально допустимый для устойчивости: %g.\nРешение может быть неустойчивым.",
                deltaT, deltaT_max);
            MessageBoxW(hWnd, msg, L"Предупреждение", MB_OK | MB_ICONWARNING);
        }

        std::vector<double> next(intervalCount + 1);
        while (fabs(pieces[intervalCount] - temperatureEnd) > accuracy && tMax >= t) {
            next[0] = pieces[0];

            for (int i = 1; i < intervalCount; ++i) {
                double C = FindC(a, b, f, g, pieces[i]);
                next[i] = pieces[i]
                    + lambda * deltaT
                    * (pieces[i + 1] - 2 * pieces[i] + pieces[i - 1])
                    / (deltaX * deltaX * C);
            }

            int last = intervalCount;
            double C = FindC(a, b, f, g, pieces[last]);
            next[last] = pieces[last]
                + deltaT * lambda * 2 * (pieces[last - 1] - pieces[last])
                / (deltaX * deltaX * C);

            pieces.swap(next);
            ++t;
        }
    }
    else
    {
        std::vector<double> next(intervalCount + 1);

        std::vector<double> lower(intervalCount + 1);
        std::vector<double> diag(intervalCount + 1);
        std::vector<double> upper(intervalCount + 1);
        std::vector<double> rhs(intervalCount + 1);

        std::vector<double> cPrime(intervalCount + 1);
        std::vector<double> dPrime(intervalCount + 1);

        const int N = intervalCount;
        int snapshotInterval = 1;
        int snapshotStep = 0;

        while (fabs(pieces[N] - temperatureEnd) > accuracy && t < tMax)
        {
            std::vector<double> prevIter = pieces;
            std::vector<double> curIter(intervalCount + 1);

            const int maxPicard = 50;
            double diff = 1e30;

            for (int picard = 0; picard < maxPicard && diff > 1e-10; ++picard)
            {

                for (int i = 1; i <= N; ++i) {
                    double C = FindC(a, b, f, g, prevIter[i]);
                    double alpha = deltaT * lambda / (deltaX * deltaX * C);
                    lower[i] = -alpha;
                    diag[i] = 1.0 + 2.0 * alpha;
                    upper[i] = -alpha;
                    rhs[i] = pieces[i];
                }

                {
                    double alpha1 = -lower[1];
                    lower[1] = 0.0;
                    rhs[1] += alpha1 * temperatureEnd;
                }
                {
                    double alphaN = -lower[N];
                    lower[N] = -2.0 * alphaN;
                    upper[N] = 0.0;
                    diag[N] = 1.0 + 2.0 * alphaN;
                }

                cPrime[1] = upper[1] / diag[1];
                dPrime[1] = rhs[1] / diag[1];
                for (int i = 2; i <= N; ++i) {
                    double denom = diag[i] - lower[i] * cPrime[i - 1];
                    cPrime[i] = upper[i] / denom;
                    dPrime[i] = (rhs[i] - lower[i] * dPrime[i - 1]) / denom;
                }
                curIter[N] = dPrime[N];
                for (int i = N - 1; i >= 1; --i)
                    curIter[i] = dPrime[i] - cPrime[i] * curIter[i + 1];
                curIter[0] = pieces[0];

                diff = 0.0;
                for (int i = 1; i <= N; ++i)
                    diff = (((diff) > (fabs(curIter[i] - prevIter[i]))) ? (diff) : (fabs(curIter[i] - prevIter[i])));

                prevIter.swap(curIter);
            }

            snapshotStep++;

            if (g_snapshots.size() >= g_maxSnapshots) {
                // Вот тут надо объяснение
                /*
                Суть алгоритма:
                Вначале сохраняем pieces на каждой итерации в g_snapshots
                Ждём когда общий счёт дойдёт до предела g_maxSnapshots
                Удаляем каждый второй элемент из g_snapshots
                Теперь записываем каждый второй pieces
                Ждём когда общий счёт дойдёт до предела g_maxSnapshots опять (В общем 200 итераций)
                Удаляем каждый второй шаг из уже накопившихся (остаётся каждый четвертый из первых 200)
                Теперь записываем каждый четвертый шаг
                Ждём когда общий счёт дойдёт до предела g_maxSnapshots в третий раз (В сумме - 400 итераций)
                ...
                Таким образом, у нас всегда будет от 50 до 100 снапшотов,
                которые находятся на одинаковом расстоянии друг от друга
                */
                std::vector<std::vector<double>> kept;
                std::vector<double> keptTimes;
                for (size_t i = 1; i < g_snapshots.size(); i += 2) {
                    kept.push_back(std::move(g_snapshots[i]));
                    keptTimes.push_back(g_snapshotTimes[i]);
                }
                g_snapshots = std::move(kept);
                g_snapshotTimes = std::move(keptTimes);
                snapshotInterval *= 2;
            }

            pieces.swap(prevIter);
            ++t;

            if (snapshotStep == snapshotInterval) {
                g_snapshots.push_back(pieces);
                g_snapshotTimes.push_back(t);
                snapshotStep = 0;
            }
        }
    }


    g_pieces = std::move(pieces);
    g_hasResult = true;
    g_lastSteps = t;

    wchar_t buf[64];
    swprintf_s(buf, L"%d", t);
    SetWindowTextW(hLabelSteps, buf);
}


static void TemperatureToRGB(double value, int& r, int& g, int& b)
{
    value = (value < 0.0) ? 0.0 : (value > 1.0 ? 1.0 : value);
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
    r = (r < 0) ? 0 : (r > 255 ? 255 : r);
    g = (g < 0) ? 0 : (g > 255 ? 255 : g);
    b = (b < 0) ? 0 : (b > 255 ? 255 : b);
}

static void PaintBars(HWND hWnd, HDC hdc)
{
    if (!g_hasResult) return;

    // Выбираем источник строк: снапшоты, иначе — финальный срез (одна строка)
    const std::vector<std::vector<double>>* rows = nullptr;
    std::vector<std::vector<double>> fallback;
    if (!g_snapshots.empty()) {
        rows = &g_snapshots;
    }
    else if (!g_pieces.empty()) {
        fallback.push_back(g_pieces);
        rows = &fallback;
    }
    else {
        return;
    }

    double temperatureStart = 0.0, temperatureEnd = 0.0;
    if (!GetDbl(hEditTStart, temperatureStart) || !GetDbl(hEditTEnd, temperatureEnd))
        return;
    double denom = temperatureEnd - temperatureStart;

    RECT area = { DRAW_X, DRAW_Y, DRAW_X + DRAW_W, DRAW_Y + DRAW_H };
    HBRUSH hWhite = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hdc, &area, hWhite);
    DeleteObject(hWhite);

    int numRows = (int)rows->size();       // число снапшотов (время)
    int numCols = (int)(*rows)[0].size();  // число узлов (пространство)
    if (numRows == 0 || numCols == 0) return;

    double cellW = (double)DRAW_W / numCols;
    double cellH = (double)DRAW_H / numRows;

    // Кэш кисти: пересоздаём только при смене цвета
    COLORREF lastColor = 0xFFFFFFFF;
    HBRUSH lastBrush = nullptr;

    for (int s = 0; s < numRows; ++s) {
        // t = 0 (snapshot 0) внизу, растёт вверх
        int top = DRAW_Y + DRAW_H - (int)(cellH * (s + 1));
        int bottom = DRAW_Y + DRAW_H - (int)(cellH * s);

        const auto& snap = (*rows)[s];
        for (int col = 0; col < numCols; ++col) {
            double v = (denom != 0.0) ? (snap[col] - temperatureStart) / denom : 0.0;
            int r, g, b;
            TemperatureToRGB(v, r, g, b);
            COLORREF color = RGB(r, g, b);

            RECT rc;
            rc.left = DRAW_X + (int)(cellW * col);
            rc.right = DRAW_X + (int)(cellW * (col + 1));
            rc.top = top;
            rc.bottom = bottom;

            if (color != lastColor) {
                if (lastBrush) DeleteObject(lastBrush);
                lastBrush = CreateSolidBrush(color);
                lastColor = color;
            }
            FillRect(hdc, &rc, lastBrush);
        }
    }
    if (lastBrush) DeleteObject(lastBrush);

    FrameRect(hdc, &area, (HBRUSH)GetStockObject(BLACK_BRUSH));
}

static void PaintGraph(HWND hWnd, HDC hdc, double x_coord)
{
    if (!g_hasResult || g_snapshots.empty()) return;
    if (g_snapshots.size() != g_snapshotTimes.size()) return;

    // ── Параметры задачи ─────────────────────────────────────────────
    double temperatureStart, temperatureEnd, length;
    int n;
    if (!GetDbl(hEditTStart, temperatureStart) ||
        !GetDbl(hEditTEnd, temperatureEnd) ||
        !GetDbl(hEditLength, length) ||
        !GetInt(hEditN, n) || n < 1 || length <= 0.0)
        return;

    const int numPoints = (int)g_snapshots[0].size();
    const int intervals = numPoints - 1;
    const double deltaX = length / intervals;

    // ── Индексы сетки для x_coord ─────────────────────────────────────
    double xi = x_coord / deltaX;
    if (xi < 0.0)        xi = 0.0;
    if (xi > intervals)  xi = intervals;

    int    i0 = (int)std::floor(xi);
    int    i1 = (std::min)(i0 + 1, intervals);
    double f = xi - i0;                  // f=0.5 → ровно середина
    if (i0 == i1) f = 0.0;

    // ── Извлекаем T(t) ───────────────────────────────────────────────
    const int numSnap = (int)g_snapshots.size();
    std::vector<double> temps(numSnap);
    for (int s = 0; s < numSnap; ++s) {
        const auto& snap = g_snapshots[s];
        temps[s] = (1.0 - f) * snap[i0] + f * snap[i1];   // среднее / интерполяция
    }

    // ── Диапазоны осей ───────────────────────────────────────────────
    double Tmin = (std::min)(temperatureStart, temperatureEnd);
    double Tmax = (std::max)(temperatureStart, temperatureEnd);
    if (Tmax - Tmin < 1e-12) Tmax = Tmin + 1.0;

    double tMin = (double)g_snapshotTimes.front();
    double tMax = (double)g_snapshotTimes.back();
    if (tMax - tMin < 1e-12) tMax = tMin + 1.0;

    // 5% отступа по краям, чтобы точки не «прилипали» к рамке
    const double TPad = (Tmax - Tmin) * 0.05;
    const double tPad = (tMax - tMin) * 0.05;
    Tmin -= TPad; Tmax += TPad;
    tMin -= tPad; tMax += tPad;

    // ── Очистка области ──────────────────────────────────────────────
    RECT area = { DRAW_X, DRAW_Y, DRAW_X + DRAW_W, DRAW_Y + DRAW_H };
    HBRUSH hWhite = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hdc, &area, hWhite);
    DeleteObject(hWhite);

    // ── Отображение координат → пиксели ──────────────────────────────
    auto toPx = [&](double t) {
        return DRAW_X + (int)((t - tMin) / (tMax - tMin) * DRAW_W);
        };
    auto toPy = [&](double T) {
        return DRAW_Y + DRAW_H - (int)((T - Tmin) / (Tmax - Tmin) * DRAW_H);
        };

    // ── Сетка ────────────────────────────────────────────────────────
    HPEN hGrid = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    HPEN hOld = (HPEN)SelectObject(hdc, hGrid);

    for (int k = 0; k <= 4; ++k) {                  // вертикальные линии = время
        int px = toPx(tMin + (tMax - tMin) * k / 4.0);
        MoveToEx(hdc, px, DRAW_Y, nullptr);
        LineTo(hdc, px, DRAW_Y + DRAW_H);
    }
    for (int k = 0; k <= 4; ++k) {                  // горизонтальные линии = температура
        int py = toPy(Tmin + (Tmax - Tmin) * k / 4.0);
        MoveToEx(hdc, DRAW_X, py, nullptr);
        LineTo(hdc, DRAW_X + DRAW_W, py);
    }

    // ── Кривая T(t): по X — время, по Y — температура ────────────────
    HPEN hCurve = CreatePen(PS_SOLID, 2, RGB(0, 0, 200));
    SelectObject(hdc, hCurve);
    for (int s = 0; s < numSnap; ++s) {
        int px = toPx((double)g_snapshotTimes[s]);   // было: toPx(temps[s])
        int py = toPy(temps[s]);                     // было: toPy(snapshotTimes[s])
        if (s == 0) MoveToEx(hdc, px, py, nullptr);
        else        LineTo(hdc, px, py);
    }

    // Маркеры
    for (int s = 0; s < numSnap; ++s) {
        int px = toPx((double)g_snapshotTimes[s]);
        int py = toPy(temps[s]);
        Rectangle(hdc, px - 2, py - 2, px + 3, py + 3);
    }

    SelectObject(hdc, hOld);
    DeleteObject(hCurve);
    DeleteObject(hGrid);

    // ── Подписи осей (тоже меняются местами) ─────────────────────────
    wchar_t buf[128];

    // Снизу — подписи ВРЕМЕНИ (ось X)
    swprintf_s(buf, L"%d", g_snapshotTimes.front());
    TextOutW(hdc, DRAW_X + 2, DRAW_Y + DRAW_H + 4, buf, (int)wcslen(buf));
    swprintf_s(buf, L"%d", g_snapshotTimes.back());
    int tw = (int)wcslen(buf) * 8;
    TextOutW(hdc, DRAW_X + DRAW_W - tw, DRAW_Y + DRAW_H + 4, buf, (int)wcslen(buf));

    // Слева — подписи ТЕМПЕРАТУРЫ (ось Y)
    swprintf_s(buf, L"%.1f", Tmin + TPad);
    TextOutW(hdc, DRAW_X - 45, DRAW_Y + DRAW_H - 8, buf, (int)wcslen(buf));
    swprintf_s(buf, L"%.1f", Tmax - TPad);
    TextOutW(hdc, DRAW_X - 45, DRAW_Y, buf, (int)wcslen(buf));

    // Заголовок
    swprintf_s(buf, L"T(t), x = %.3f м", x_coord);
    TextOutW(hdc, DRAW_X + 6, DRAW_Y + 6, buf, (int)wcslen(buf));

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

        if (wmId == IDC_EDIT_N && wmEvent == EN_CHANGE) {
            UpdateDeltaXLabel();
            return 0;
        }

        if (wmId == IDC_EDIT_LENGHT && wmEvent == EN_CHANGE) {
            UpdateDeltaXLabel();
            return 0;
        }

        if (wmId == IDC_BTN_DRAW && wmEvent == BN_CLICKED) {
            whichDraw = 1;
            InvalidateRect(hWnd, nullptr, TRUE);
            return 0;
        }

        if (wmId == IDC_BTN_DRAW_COORD && wmEvent == BN_CLICKED) {
            whichDraw = 2;
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
        double xCoord;
        GetDbl(hEditA, xCoord);

        switch (whichDraw)
        {
        case 1:
        {
            PaintBars(hWnd, hdc);
        }
        break;
        case 2:
        {
            double xCoord;
            GetDbl(hEditXCoord, xCoord);
            PaintGraph(hWnd, hdc, xCoord);
        }
        break;
        default:
        {}
        break;
        }
        
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

// Сравнить что происходит при c || lambda = const и c || lambda != const