/* Native Win32 / Windows CE host layer, the counterpart of ff_platform_sdl2.c.
   The device build of FFRace.exe presents through GAPI; here the back buffer is
   a top-down 16bpp DIB section and Platform_Present is a BitBlt.  Windows CE
   only ships the wide API, so every call here is the W form. */
#include "ff_platform.h"
#include "ff_consts.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FF_PATH_MAX 512
#define FF_RAWQ     16
#define FF_MSG_W    2048

static HWND        ff_hwnd;
static HDC         ff_memdc;
static HBITMAP     ff_dib;
static HGDIOBJ     ff_oldbmp;
static uint16_t   *ff_pixels;
static ff_surface  ff_back;
static int         ff_w;
static int         ff_h;
static int         ff_dst_x;
static int         ff_dst_y;
static int         ff_dst_w;
static int         ff_dst_h;
static int         ff_keys[FF_KEY_COUNT];
static int         ff_running;
static int         ff_tap_x;
static int         ff_tap_y;
static int         ff_tap_pending;
static int         ff_back_pending;

static const WCHAR ff_class[] = L"FFRaceWnd";

/* FFRace.exe WndProc 0x0004f6bc matches the WM_KEYDOWN code against the six
   binding slots based at 0x000833c4. */
static int ff_keymap[FF_KEY_COUNT] = {
    VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN, VK_SPACE, VK_RETURN
};

static int ff_rawq[FF_RAWQ];
static int ff_rawq_head;
static int ff_rawq_tail;

static int WLen(const WCHAR *s)
{
    int n = 0;

    while (s[n] != 0)
        n++;
    return n;
}

static void WCopy(WCHAR *dst, const WCHAR *src, int max)
{
    int i = 0;

    while (i < max - 1 && src[i] != 0) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

static void WCat(WCHAR *dst, const WCHAR *src, int max)
{
    int n = WLen(dst);
    int i = 0;

    while (n + i < max - 1 && src[i] != 0) {
        dst[n + i] = src[i];
        i++;
    }
    dst[n + i] = 0;
}

static void WidenAscii(const char *src, WCHAR *dst, int max)
{
    int i = 0;

    while (i < max - 1 && src[i] != '\0') {
        dst[i] = (WCHAR)(unsigned char)src[i];
        i++;
    }
    dst[i] = 0;
}

static void NarrowAscii(const WCHAR *src, char *dst, int max)
{
    int i = 0;

    while (i < max - 1 && src[i] != 0) {
        dst[i] = (src[i] < 0x80) ? (char)src[i] : '?';
        i++;
    }
    dst[i] = '\0';
}

static void ToWide(const char *src, WCHAR *dst, int max)
{
    if (MultiByteToWideChar(CP_ACP, 0, src, -1, dst, max) == 0)
        WidenAscii(src, dst, max);
}

/* Windows CE has no current directory and its file API rejects '/', so a path
   assembled by ff_assets.c has to be turned back into a backslash path. */
static void ToPath(const char *src, WCHAR *dst, int max)
{
    int i;

    ToWide(src, dst, max);
    for (i = 0; dst[i] != 0; i++) {
        if (dst[i] == L'/')
            dst[i] = L'\\';
    }
}

static void BlitToDC(HDC dc)
{
    if (ff_dst_w == ff_w && ff_dst_h == ff_h) {
        BitBlt(dc, ff_dst_x, ff_dst_y, ff_w, ff_h, ff_memdc, 0, 0, SRCCOPY);
        return;
    }
    StretchBlt(dc, ff_dst_x, ff_dst_y, ff_dst_w, ff_dst_h,
               ff_memdc, 0, 0, ff_w, ff_h, SRCCOPY);
}

static int MapKey(int code)
{
    int k;

    if (code == FF_KEY_UNBOUND)
        return -1;
    for (k = 0; k < FF_KEY_COUNT; k++) {
        if (ff_keymap[k] == code)
            return k;
    }
    return -1;
}

static void NoteTap(int px, int py)
{
    if (ff_dst_w < 1 || ff_dst_h < 1)
        return;
    if (px < ff_dst_x || px >= ff_dst_x + ff_dst_w ||
        py < ff_dst_y || py >= ff_dst_y + ff_dst_h)
        return;
    ff_tap_x       = (px - ff_dst_x) * ff_w / ff_dst_w;
    ff_tap_y       = (py - ff_dst_y) * ff_h / ff_dst_h;
    ff_tap_pending = 1;
}

/* FFRace.exe WndProc 0x0004f6bc keeps the stylus point in 0x000a77e0 and
   0x000a77e4 on the 0x201 arm, and takes the back branch at 0x0004ffd8 on the
   0x100 arm. */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    int k;

    switch (msg) {
    case WM_CLOSE:
        ff_running = 0;
        return 0;
    case WM_DESTROY:
        ff_running = 0;
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC         dc = BeginPaint(hwnd, &ps);

        if (dc != NULL && ff_memdc != NULL)
            BlitToDC(dc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN:
        NoteTap((int)(short)LOWORD(lp), (int)(short)HIWORD(lp));
        return 0;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (wp == VK_ESCAPE) {
            ff_back_pending = 1;
            return 0;
        }
        if ((lp & 0x40000000L) == 0) {
            int next = (ff_rawq_head + 1) % FF_RAWQ;

            if (next != ff_rawq_tail) {
                ff_rawq[ff_rawq_head] = (int)wp;
                ff_rawq_head          = next;
            }
        }
        k = MapKey((int)wp);
        if (k >= 0)
            ff_keys[k] = 1;
        return 0;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        k = MapKey((int)wp);
        if (k >= 0)
            ff_keys[k] = 0;
        return 0;
    case WM_ACTIVATE:
        Platform_AudioPause(LOWORD(wp) == WA_INACTIVE);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

#ifdef FF_WINCE
/* aygshell.dll SHFullScreen hides the taskbar and the SIP button.  Plain
   Windows CE images ship no aygshell, so the entry point is resolved at run
   time and its absence is not an error. */
typedef BOOL(WINAPI *ff_shfullscreen)(HWND, DWORD);

static void GoFullScreen(HWND hwnd)
{
    HMODULE         lib = LoadLibraryW(L"aygshell.dll");
    ff_shfullscreen fn;

    if (lib == NULL)
        return;
    fn = (ff_shfullscreen)GetProcAddress(lib, L"SHFullScreen");
    if (fn != NULL)
        fn(hwnd, 0x0002 | 0x0008 | 0x0010);
}
#endif

int Platform_Init(int w, int h, int scale, const char *title)
{
    struct {
        BITMAPINFOHEADER h;
        DWORD            mask[3];
    } bi;
    WNDCLASSW  wc;
    WCHAR      wtitle[128];
    HINSTANCE  hinst = GetModuleHandleW(NULL);
    HDC        dc;
    int        stride;

    ToWide(title, wtitle, 128);

    memset(&wc, 0, sizeof wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hinst;
#ifndef FF_WINCE
    /* IDC_ARROW is a MAKEINTRESOURCE ordinal typed for the narrow API unless
       UNICODE is defined, so the W entry point needs the cast.  Windows CE has
       no mouse cursor and no IDC_ARROW at all. */
    wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
#endif
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = ff_class;
    RegisterClassW(&wc);

#ifdef FF_WINCE
    {
        int sw = GetSystemMetrics(SM_CXSCREEN);
        int sh = GetSystemMetrics(SM_CYSCREEN);
        int s  = sw / w;
        int t  = sh / h;

        (void)scale;
        if (t < s)
            s = t;
        if (s < 1) {
            ff_dst_w = sw;
            ff_dst_h = sh;
        } else {
            ff_dst_w = w * s;
            ff_dst_h = h * s;
        }
        ff_dst_x = (sw - ff_dst_w) / 2;
        ff_dst_y = (sh - ff_dst_h) / 2;
        ff_hwnd  = CreateWindowExW(0, ff_class, wtitle, WS_VISIBLE | WS_POPUP,
                                   0, 0, sw, sh, NULL, NULL, hinst, NULL);
    }
#else
    {
        RECT  r;
        DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

        if (scale < 1)
            scale = 1;
        ff_dst_x = 0;
        ff_dst_y = 0;
        ff_dst_w = w * scale;
        ff_dst_h = h * scale;
        r.left   = 0;
        r.top    = 0;
        r.right  = ff_dst_w;
        r.bottom = ff_dst_h;
        AdjustWindowRect(&r, style, FALSE);
        ff_hwnd = CreateWindowExW(0, ff_class, wtitle, style,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  r.right - r.left, r.bottom - r.top,
                                  NULL, NULL, hinst, NULL);
    }
#endif
    if (ff_hwnd == NULL)
        return 0;

    dc = GetDC(ff_hwnd);
    if (dc == NULL)
        return 0;

    memset(&bi, 0, sizeof bi);
    bi.h.biSize = sizeof(BITMAPINFOHEADER);
    bi.h.biWidth = w;
    /* JumpyBall.exe Game_Init 0x000113bc passes 0 to Gfx_CreateBackBuffer
       0x00021698 when g_viewH 0x0002626c is not 0xf0, so g_clipHRow 0x000269e8
       keeps its .data value 0x140 and Blit_TileH admits y == h. */
    bi.h.biHeight      = -(h + 1);
    bi.h.biPlanes      = 1;
    bi.h.biBitCount    = 16;
    bi.h.biCompression = BI_BITFIELDS;
    bi.mask[0]         = 0xf800u;
    bi.mask[1]         = 0x07e0u;
    bi.mask[2]         = 0x001fu;

    ff_dib = CreateDIBSection(dc, (BITMAPINFO *)&bi, DIB_RGB_COLORS,
                              (void **)&ff_pixels, NULL, 0);
    ff_memdc = CreateCompatibleDC(dc);
    ReleaseDC(ff_hwnd, dc);
    if (ff_dib == NULL || ff_memdc == NULL || ff_pixels == NULL)
        return 0;
    ff_oldbmp = SelectObject(ff_memdc, ff_dib);

    stride = ((w * 16 + 31) / 32) * 2;

    ff_w = w;
    ff_h = h;
    ff_back.pixels  = ff_pixels;
    ff_back.bpp     = 0x10;
    ff_back.fmt     = FF_FMT_RGB565;
    ff_back.x_pitch = 1;
    ff_back.y_pitch = stride;
    ff_clip_w     = w;
    ff_clip_h     = h;
    ff_clip_h_row = h;

    ShowWindow(ff_hwnd, SW_SHOW);
    UpdateWindow(ff_hwnd);
    SetForegroundWindow(ff_hwnd);
#ifdef FF_WINCE
    GoFullScreen(ff_hwnd);
#endif
    ff_running = 1;
    return 1;
}

void Platform_Shutdown(void)
{
    Platform_AudioShutdown();
    if (ff_memdc != NULL) {
        SelectObject(ff_memdc, ff_oldbmp);
        DeleteDC(ff_memdc);
        ff_memdc = NULL;
    }
    if (ff_dib != NULL) {
        DeleteObject(ff_dib);
        ff_dib = NULL;
    }
    ff_pixels = NULL;
    if (ff_hwnd != NULL) {
        DestroyWindow(ff_hwnd);
        ff_hwnd = NULL;
    }
    UnregisterClassW(ff_class, GetModuleHandleW(NULL));
}

ff_surface *Platform_BackBuffer(void)
{
    return &ff_back;
}

void Platform_Present(void)
{
    HDC dc;

    if (ff_hwnd == NULL || ff_memdc == NULL)
        return;
    dc = GetDC(ff_hwnd);
    if (dc == NULL)
        return;
    BlitToDC(dc);
    ReleaseDC(ff_hwnd, dc);
}

int Platform_PollEvents(void)
{
    MSG msg;

    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            ff_running = 0;
            break;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return ff_running;
}

int Platform_KeyDown(int key)
{
    if (key < 0 || key >= FF_KEY_COUNT)
        return 0;
    return ff_keys[key];
}

int Platform_KeyBinding(int key)
{
    if (key < 0 || key >= FF_KEY_COUNT)
        return FF_KEY_UNBOUND;
    return ff_keymap[key];
}

void Platform_SetKeyBinding(int key, int code)
{
    if (key < 0 || key >= FF_KEY_COUNT)
        return;
    ff_keymap[key] = code;
    ff_keys[key]   = 0;
}

int Platform_NextRawKey(void)
{
    int code;

    if (ff_rawq_tail == ff_rawq_head)
        return FF_KEY_UNBOUND;
    code         = ff_rawq[ff_rawq_tail];
    ff_rawq_tail = (ff_rawq_tail + 1) % FF_RAWQ;
    return code;
}

void Platform_FlushRawKeys(void)
{
    ff_rawq_tail = ff_rawq_head;
}

int Platform_NextTap(int *x, int *y)
{
    if (!ff_tap_pending)
        return 0;
    ff_tap_pending = 0;
    *x = ff_tap_x;
    *y = ff_tap_y;
    return 1;
}

int Platform_TakeBack(void)
{
    int b = ff_back_pending;

    ff_back_pending = 0;
    return b;
}

/* The on-screen D-pad of ff_touch_sdl2.c needs an SDL renderer, so the native
   build ships the stylus only and the game keeps its keyboard controls. */
int Platform_TouchActive(void)
{
    return 0;
}

unsigned Platform_Ticks(void)
{
    return (unsigned)GetTickCount();
}

void Platform_Delay(unsigned ms)
{
    Sleep((DWORD)ms);
}

int Platform_FileExists(const char *path)
{
    WCHAR wpath[FF_PATH_MAX];
    DWORD attr;

    ToPath(path, wpath, FF_PATH_MAX);
    attr = GetFileAttributesW(wpath);
    if (attr == 0xffffffffu)
        return 0;
    return (attr & FILE_ATTRIBUTE_DIRECTORY) ? 0 : 1;
}

unsigned char *Platform_ReadFile(const char *path, long *out_len)
{
    WCHAR          wpath[FF_PATH_MAX];
    HANDLE         fh;
    DWORD          size, got = 0;
    unsigned char *buf;

    ToPath(path, wpath, FF_PATH_MAX);
    fh = CreateFileW(wpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL, NULL);
    if (fh == INVALID_HANDLE_VALUE)
        return NULL;
    size = GetFileSize(fh, NULL);
    if (size == 0xffffffffu || size == 0 || size > 0x2000000u) {
        CloseHandle(fh);
        return NULL;
    }
    buf = (unsigned char *)malloc((size_t)size);
    if (buf == NULL) {
        CloseHandle(fh);
        return NULL;
    }
    if (!ReadFile(fh, buf, size, &got, NULL) || got != size) {
        CloseHandle(fh);
        free(buf);
        return NULL;
    }
    CloseHandle(fh);
    *out_len = (long)size;
    return buf;
}

static char        ff_base[FF_PATH_MAX];
static const char *ff_base_origin = "";

static int HasAssets(const char *dir)
{
    char probe[FF_PATH_MAX];

    sprintf(probe, "%sBITMAP\\%d.bmp", dir, FF_RES_FONT);
    return Platform_FileExists(probe);
}

const char *Platform_BasePath(void)
{
    WCHAR wpath[FF_PATH_MAX];
    int   n;

    if (ff_base[0] != '\0')
        return ff_base;

    wpath[0] = 0;
    if (GetModuleFileNameW(NULL, wpath, FF_PATH_MAX) == 0)
        GetModuleFileNameW(GetModuleHandleW(NULL), wpath, FF_PATH_MAX);
    n = WLen(wpath);
    while (n > 0 && wpath[n - 1] != L'\\' && wpath[n - 1] != L'/')
        n--;
    if (n > 0) {
        wpath[n] = 0;
        NarrowAscii(wpath, ff_base, FF_PATH_MAX);
        ff_base_origin = "GetModuleFileName";
        if (HasAssets(ff_base))
            return ff_base;
    }

#ifdef FF_WINCE
    /* A Windows CE process has no current directory, so a relative path only
       resolves against \Windows.  These are the directories an installer or a
       manual copy actually puts the game in. */
    {
        static const char *const dirs[] = {
            "\\Storage Card\\FFRace\\", "\\SD Card\\FFRace\\",
            "\\Program Files\\FFRace\\", "\\My Documents\\FFRace\\",
            "\\Windows\\FFRace\\", "\\FFRace\\", "\\Storage Card\\", "\\"
        };
        size_t i;

        for (i = 0; i < sizeof dirs / sizeof dirs[0]; i++) {
            if (!HasAssets(dirs[i]))
                continue;
            strcpy(ff_base, dirs[i]);
            ff_base_origin = "asset probe";
            return ff_base;
        }
    }
#endif
    if (ff_base[0] == '\0') {
        strcpy(ff_base, ".\\");
        ff_base_origin = "working directory";
    }
    return ff_base;
}

const char *Platform_BaseOrigin(void)
{
    if (ff_base[0] == '\0')
        Platform_BasePath();
    return ff_base_origin;
}

const char *Platform_PrefPath(void)
{
    return Platform_BasePath();
}

void Platform_ShowError(const char *title, const char *text)
{
    static WCHAR wtitle[128];
    static WCHAR wtext[FF_MSG_W];

    ToWide(title, wtitle, 128);
    ToWide(text, wtext, FF_MSG_W);
    MessageBoxW(ff_hwnd, wtext, wtitle, MB_OK | MB_ICONERROR);
}

const char *Platform_LastError(void)
{
    static char msg[256];
    WCHAR       buf[256];
    DWORD       err = GetLastError();
    int         n;

    buf[0] = 0;
    if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, err, 0, buf, 256, NULL) == 0) {
        sprintf(msg, "error %lu", (unsigned long)err);
        return msg;
    }
    NarrowAscii(buf, msg, sizeof msg);
    n = (int)strlen(msg);
    while (n > 0 && (msg[n - 1] == '\r' || msg[n - 1] == '\n' ||
                     msg[n - 1] == ' ' || msg[n - 1] == '\t'))
        msg[--n] = '\0';
    return msg;
}
