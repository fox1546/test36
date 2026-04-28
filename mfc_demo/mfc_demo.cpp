// mfc_demo.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "mfc_demo.h"

#define MAX_LOADSTRING 100
#define TIMER_ANIMATION 1001

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

static WCHAR g_szCurrentFile[MAX_PATH] = {0};
static MCIDEVICEID g_wDeviceID = 0;
static BOOL g_bPlaying = FALSE;
static BOOL g_bPaused = FALSE;
static int g_nAnimationFrame = 0;
static double g_dBeatIntensity = 0.0;
static double g_dBeatPhase = 0.0;
static HWND g_hWndMain = NULL;
static int g_nWindowWidth = 800;
static int g_nWindowHeight = 600;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

BOOL OpenMP3File(HWND hWnd);
BOOL PlayMP3(HWND hWnd);
BOOL PauseMP3(HWND hWnd);
BOOL StopMP3(HWND hWnd);
BOOL CloseMP3(HWND hWnd);
void DrawDancingFigure(HDC hdc, RECT* prcClient);
void UpdateBeatIntensity();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MFCDEMO, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MFCDEMO));

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MFCDEMO));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MFCDEMO);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance;

   g_hWndMain = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, g_nWindowWidth, g_nWindowHeight, nullptr, nullptr, hInstance, nullptr);

   if (!g_hWndMain)
   {
      return FALSE;
   }

   ShowWindow(g_hWndMain, nCmdShow);
   UpdateWindow(g_hWndMain);

   SetTimer(g_hWndMain, TIMER_ANIMATION, 50, NULL);

   return TRUE;
}

BOOL OpenMP3File(HWND hWnd)
{
    OPENFILENAMEW ofn;
    WCHAR szFileName[MAX_PATH] = {0};
    WCHAR szFilter[] = L"MP3 Files\0*.mp3\0All Files\0*.*\0";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrTitle = L"选择MP3文件";

    if (GetOpenFileNameW(&ofn))
    {
        if (g_wDeviceID != 0)
        {
            CloseMP3(hWnd);
        }

        wcscpy_s(g_szCurrentFile, MAX_PATH, szFileName);

        MCI_OPEN_PARMS mciOpenParms;
        ZeroMemory(&mciOpenParms, sizeof(mciOpenParms));
        mciOpenParms.lpstrDeviceType = L"mpegvideo";
        mciOpenParms.lpstrElementName = g_szCurrentFile;

        MCIERROR mciError = mciSendCommand(0, MCI_OPEN, 
            MCI_OPEN_TYPE | MCI_OPEN_ELEMENT, 
            (DWORD_PTR)&mciOpenParms);

        if (mciError == 0)
        {
            g_wDeviceID = mciOpenParms.wDeviceID;
            g_bPlaying = FALSE;
            g_bPaused = FALSE;
            g_nAnimationFrame = 0;
            g_dBeatIntensity = 0.0;
            return TRUE;
        }
        else
        {
            WCHAR szError[256];
            mciGetErrorStringW(mciError, szError, 256);
            MessageBoxW(hWnd, szError, L"打开MP3失败", MB_ICONERROR);
            return FALSE;
        }
    }
    return FALSE;
}

BOOL PlayMP3(HWND hWnd)
{
    if (g_wDeviceID == 0)
    {
        MessageBoxW(hWnd, L"请先打开MP3文件", L"提示", MB_ICONINFORMATION);
        return FALSE;
    }

    if (g_bPaused)
    {
        MCI_PLAY_PARMS mciPlayParms;
        ZeroMemory(&mciPlayParms, sizeof(mciPlayParms));
        
        MCIERROR mciError = mciSendCommand(g_wDeviceID, MCI_RESUME, 0, (DWORD_PTR)&mciPlayParms);
        if (mciError == 0)
        {
            g_bPlaying = TRUE;
            g_bPaused = FALSE;
            return TRUE;
        }
    }

    MCI_PLAY_PARMS mciPlayParms;
    ZeroMemory(&mciPlayParms, sizeof(mciPlayParms));
    
    MCIERROR mciError = mciSendCommand(g_wDeviceID, MCI_PLAY, MCI_FROM, (DWORD_PTR)&mciPlayParms);
    if (mciError == 0)
    {
        g_bPlaying = TRUE;
        g_bPaused = FALSE;
        return TRUE;
    }
    else
    {
        WCHAR szError[256];
        mciGetErrorStringW(mciError, szError, 256);
        MessageBoxW(hWnd, szError, L"播放失败", MB_ICONERROR);
        return FALSE;
    }
}

BOOL PauseMP3(HWND hWnd)
{
    if (g_wDeviceID == 0 || !g_bPlaying)
    {
        return FALSE;
    }

    MCI_GENERIC_PARMS mciParms;
    ZeroMemory(&mciParms, sizeof(mciParms));
    
    MCIERROR mciError = mciSendCommand(g_wDeviceID, MCI_PAUSE, 0, (DWORD_PTR)&mciParms);
    if (mciError == 0)
    {
        g_bPaused = TRUE;
        g_bPlaying = FALSE;
        return TRUE;
    }
    return FALSE;
}

BOOL StopMP3(HWND hWnd)
{
    if (g_wDeviceID == 0)
    {
        return FALSE;
    }

    MCI_GENERIC_PARMS mciParms;
    ZeroMemory(&mciParms, sizeof(mciParms));
    
    MCIERROR mciError = mciSendCommand(g_wDeviceID, MCI_STOP, 0, (DWORD_PTR)&mciParms);
    if (mciError == 0)
    {
        g_bPlaying = FALSE;
        g_bPaused = FALSE;
        
        MCI_SEEK_PARMS mciSeekParms;
        ZeroMemory(&mciSeekParms, sizeof(mciSeekParms));
        mciSeekParms.dwTo = 0;
        mciSendCommand(g_wDeviceID, MCI_SEEK, MCI_TO, (DWORD_PTR)&mciSeekParms);
        
        return TRUE;
    }
    return FALSE;
}

BOOL CloseMP3(HWND hWnd)
{
    if (g_wDeviceID == 0)
    {
        return TRUE;
    }

    StopMP3(hWnd);

    MCI_GENERIC_PARMS mciParms;
    ZeroMemory(&mciParms, sizeof(mciParms));
    
    mciSendCommand(g_wDeviceID, MCI_CLOSE, 0, (DWORD_PTR)&mciParms);
    
    g_wDeviceID = 0;
    g_bPlaying = FALSE;
    g_bPaused = FALSE;
    ZeroMemory(g_szCurrentFile, sizeof(g_szCurrentFile));
    
    return TRUE;
}

void UpdateBeatIntensity()
{
    g_nAnimationFrame++;
    g_dBeatPhase += 0.15;
    
    if (g_bPlaying && !g_bPaused)
    {
        double baseIntensity = 0.3 + 0.4 * sin(g_dBeatPhase);
        double highFreq = 0.3 * sin(g_dBeatPhase * 4.0);
        double noise = 0.1 * ((double)rand() / RAND_MAX - 0.5);
        
        g_dBeatIntensity = baseIntensity + highFreq + noise;
        g_dBeatIntensity = max(0.0, min(1.0, g_dBeatIntensity));
    }
    else
    {
        g_dBeatIntensity *= 0.95;
        if (g_dBeatIntensity < 0.01)
        {
            g_dBeatIntensity = 0.0;
        }
    }
}

void DrawDancingFigure(HDC hdc, RECT* prcClient)
{
    int centerX = (prcClient->right - prcClient->left) / 2;
    int centerY = (prcClient->bottom - prcClient->top) / 2 + 50;
    
    int scale = min(centerX, centerY) / 3;
    
    double beatEffect = g_dBeatIntensity;
    double swayAngle = sin(g_dBeatPhase * 2.0) * 0.15 * beatEffect;
    
    HPEN hPen = CreatePen(PS_SOLID, 3, RGB(0, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    
    HBRUSH hBrush = CreateSolidBrush(RGB(255, 200, 150));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
    
    int headRadius = (int)(scale * 0.25);
    int headY = centerY - (int)(scale * 1.8);
    
    Ellipse(hdc, 
        centerX - headRadius, 
        headY - headRadius, 
        centerX + headRadius, 
        headY + headRadius);
    
    HPEN hEyePen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
    HPEN hOldEyePen = (HPEN)SelectObject(hdc, hEyePen);
    
    Ellipse(hdc, centerX - 15, headY - 5, centerX - 5, headY + 5);
    Ellipse(hdc, centerX + 5, headY - 5, centerX + 15, headY + 5);
    
    if (beatEffect > 0.3)
    {
        Arc(hdc, centerX - 10, headY + 5, centerX + 10, headY + 20,
            centerX - 10, headY + 15, centerX + 10, headY + 15);
    }
    
    SelectObject(hdc, hOldEyePen);
    DeleteObject(hEyePen);
    
    int bodyTop = headY + headRadius;
    int bodyBottom = centerY - (int)(scale * 0.3);
    
    HBRUSH hBodyBrush = CreateSolidBrush(RGB(100, 150, 255));
    HBRUSH hOldBodyBrush = (HBRUSH)SelectObject(hdc, hBodyBrush);
    
    int bodyWidth = (int)(scale * 0.4);
    int bodyHeight = bodyBottom - bodyTop;
    
    Rectangle(hdc, 
        centerX - bodyWidth, 
        bodyTop, 
        centerX + bodyWidth, 
        bodyBottom);
    
    int armLength = (int)(scale * 0.6);
    int armY = bodyTop + (int)(bodyHeight * 0.3);
    
    double armSwing = sin(g_dBeatPhase * 3.0) * beatEffect * 30.0;
    
    int armEndX1 = centerX - bodyWidth - armLength + (int)armSwing;
    int armEndY1 = armY - (int)(fabs(armSwing) * 0.5);
    MoveToEx(hdc, centerX - bodyWidth, armY, NULL);
    LineTo(hdc, armEndX1, armEndY1);
    
    int armEndX2 = centerX + bodyWidth + armLength - (int)armSwing;
    int armEndY2 = armY - (int)(fabs(armSwing) * 0.5);
    MoveToEx(hdc, centerX + bodyWidth, armY, NULL);
    LineTo(hdc, armEndX2, armEndY2);
    
    int legLength = (int)(scale * 0.7);
    int legY = bodyBottom;
    double legSwing = sin(g_dBeatPhase * 2.5) * beatEffect * 25.0;
    
    int legEndX1 = centerX - bodyWidth / 2 + (int)legSwing;
    MoveToEx(hdc, centerX - bodyWidth / 2, legY, NULL);
    LineTo(hdc, legEndX1, legY + legLength);
    
    int legEndX2 = centerX + bodyWidth / 2 - (int)legSwing;
    MoveToEx(hdc, centerX + bodyWidth / 2, legY, NULL);
    LineTo(hdc, legEndX2, legY + legLength);
    
    SelectObject(hdc, hOldBodyBrush);
    DeleteObject(hBodyBrush);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
            case IDM_DIRECTORY_OPEN:
                OpenMP3File(hWnd);
                break;
            case IDM_PLAY:
                PlayMP3(hWnd);
                break;
            case IDM_PAUSE:
                PauseMP3(hWnd);
                break;
            case IDM_STOP:
                StopMP3(hWnd);
                break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                CloseMP3(hWnd);
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
            
            RECT rcClient;
            GetClientRect(hWnd, &rcClient);
            
            HBRUSH hBgBrush = CreateSolidBrush(RGB(240, 240, 255));
            FillRect(hdc, &rcClient, hBgBrush);
            DeleteObject(hBgBrush);
            
            DrawDancingFigure(hdc, &rcClient);
            
            if (g_szCurrentFile[0] != 0)
            {
                WCHAR szStatus[512];
                if (g_bPlaying)
                {
                    swprintf_s(szStatus, L"正在播放: %s", g_szCurrentFile);
                }
                else if (g_bPaused)
                {
                    swprintf_s(szStatus, L"已暂停: %s", g_szCurrentFile);
                }
                else
                {
                    swprintf_s(szStatus, L"已加载: %s", g_szCurrentFile);
                }
                
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(0, 0, 128));
                TextOutW(hdc, 10, 10, szStatus, (int)wcslen(szStatus));
            }
            else
            {
                WCHAR szHint[] = L"请通过 目录->打开 选择一个MP3文件";
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(128, 128, 128));
                TextOutW(hdc, 10, 10, szHint, (int)wcslen(szHint));
            }
            
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_TIMER:
        if (wParam == TIMER_ANIMATION)
        {
            UpdateBeatIntensity();
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
    case WM_DESTROY:
        KillTimer(hWnd, TIMER_ANIMATION);
        CloseMP3(hWnd);
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
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
