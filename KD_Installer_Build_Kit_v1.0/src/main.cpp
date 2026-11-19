#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <knownfolders.h>
#include <gdiplus.h>
#include <winreg.h>
#include <winioctl.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <dxgi.h>
#include <powrprof.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <wbemidl.h>
#include <oleauto.h>
#include <psapi.h>
#include <cstdio>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <mutex>
#include <algorithm>
#include <atomic>
#include <thread>
#include <chrono>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <memory>
#include <cwchar>
#include <cwctype>
#include <cstdint>
#include <cmath>
#include <limits>
#include "modules.hpp"
#include "resource.h"

#undef DrawText
#undef DrawTextEx

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "powrprof.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "comctl32.lib")

using namespace Gdiplus;

namespace {

constexpr wchar_t kClassName[] = L"KDCoreWindow";
constexpr wchar_t kAppVersion[] = L"2.0.0";
constexpr wchar_t kProductName[] = L"KD Core";
constexpr UINT WM_APP_STATS = WM_APP + 10;
constexpr UINT WM_APP_MODULE_DONE = WM_APP + 11;
constexpr UINT_PTR kAnimTimer = 1;
constexpr int kMinWidth = 1180;
constexpr int kMinHeight = 700;

// KD Core palette — restrained graphite / light blue, no neon or heavy effects.
const Color C_GRAPHITE(255, 38, 50, 56);       // #263238
const Color C_GRAPHITE_LIGHT(255, 55, 71, 79); // #37474F
const Color C_LIGHT_BLUE(255, 129, 212, 250);  // #81D4FA
const Color C_ICE_WHITE(255, 234, 246, 252);   // #EAF6FC

const Color C_INK(255, 234, 246, 252);
const Color C_TEXT(255, 214, 229, 237);
const Color C_MUTED(255, 168, 189, 201);
const Color C_DIM(255, 126, 150, 163);
const Color C_SAGE(255, 129, 212, 250);        // accent alias kept for call sites
const Color C_MINT(255, 129, 212, 250);
const Color C_CYAN(255, 129, 212, 250);
const Color C_GREEN(255, 126, 206, 168);
const Color C_AMBER(255, 232, 184, 106);
const Color C_RED(255, 240, 128, 138);
const Color C_LINE(56, 129, 212, 250);
const Color C_LINE_SOFT(42, 234, 246, 252);
const Color C_BUTTON_TEXT(255, 26, 50, 56);
const Color C_OUTPUT_BG(255, 24, 34, 39);
const Color C_CARD_TOP(255, 55, 71, 79);
const Color C_CARD_BOTTOM(255, 38, 50, 56);

constexpr float kChartSeconds = 90.0f;

enum NavIndex {
    NAV_DASHBOARD = 0,
    NAV_DIAGNOSTICS = 1,
    NAV_MODULES = 2,
    NAV_SYSTEM = 3,
    NAV_HARDWARE = 4,
    NAV_DISKS = 5,
    NAV_NETWORK = 6,
    NAV_PERFORMANCE = 7,
    NAV_SECURITY = 8,
    NAV_DEVICES = 9,
    NAV_MAINTENANCE = 10,
    NAV_REPORTS = 11,
    NAV_SETTINGS = 12,
    NAV_ABOUT = 13,
    NAV_COUNT = 14
};

enum class Icon {
    None, Dashboard, Modules, Cpu, Disk, Network, Gauge, Shield, Devices, Wrench,
    Report, Settings, System, Memory, Gpu, Search, Play, Download, Folder, Check,
    Warning, Admin, Refresh, Close, Info, Clock, Trash, Lock, Sparkle, Chevron
};

enum class RunState { Idle, Running, Ok, Error, Cancelled, Timeout };

enum HoverButton {
    HB_NONE = 0, HB_RUN, HB_EXPORT_MODULE, HB_EXPORT_REPORT, HB_OPEN_REPORTS,
    HB_CLEAR_HISTORY, HB_ADMIN, HB_ABOUT, HB_ANIM_TOGGLE, HB_AUTOSAVE_TOGGLE,
    HB_REFRESH_1, HB_REFRESH_2, HB_REFRESH_5,
    HB_QUICK_DIAGNOSTICS, HB_QUICK_MODULES, HB_QUICK_REPORTS,
    HB_HISTORY_0, HB_HISTORY_1, HB_HISTORY_2, HB_HISTORY_3,
    HB_ABOUT_ADMIN, HB_ABOUT_REPORTS
};

struct Settings {
    int refreshSeconds = 1;
    bool animations = true;
    bool autoSaveReports = true;
};

struct DiskVolume {
    wchar_t letter = L'C';
    std::wstring label;
    std::wstring fs;
    std::wstring model;
    std::wstring bus;
    uint64_t total = 0;
    uint64_t free = 0;
    bool ssd = false;
    bool ssdKnown = false;
    bool removable = false;
};

struct GpuStats {
    std::wstring name = L"Wykrywanie karty graficznej…";
    std::wstring vendor;
    std::wstring driver = L"n/d";
    std::wstring temp = L"n/d";
    uint64_t vram = 0;
    int usage = -1;
};

struct Snapshot {
    bool valid = false;
    bool cpuReady = false;
    int cpuPercent = 0;
    std::wstring cpuName = L"Procesor";
    int cores = 0;
    int threads = 0;
    int cpuMhz = 0;
    bool cpuMhzCurrent = false;
    std::wstring cpuTemp = L"n/d";
    uint64_t ramTotal = 0;
    uint64_t ramAvail = 0;
    uint64_t ramUsed = 0;
    int ramPercent = 0;
    uint64_t commitUsed = 0;
    uint64_t commitLimit = 0;
    std::vector<DiskVolume> disks;
    double diskRead = 0;
    double diskWrite = 0;
    bool diskIo = false;
    GpuStats gpu;
    std::wstring osName = L"Windows";
    std::wstring arch = L"n/d";
    std::wstring computer = L"n/d";
    std::wstring uptime = L"n/d";
    std::wstring bios = L"n/d";
    std::wstring board = L"n/d";
    std::wstring model = L"n/d";
    std::wstring sampleTime;
};

struct RunInfo {
    RunState state = RunState::Idle;
    double seconds = 0;
    int exitCode = 0;
    std::wstring finishedAt;
};

struct Report {
    int moduleId = 0;
    std::wstring moduleName;
    std::wstring category;
    std::wstring output;
    std::wstring startedAt;
    double seconds = 0;
    std::wstring durationText; // raw duration for reports loaded from disk
    int exitCode = 0;
    bool timedOut = false;
    bool cancelled = false;
    bool autoSaved = false;
    std::wstring file;
};

struct ExecResult {
    std::wstring output;
    DWORD exitCode = 0;
    double seconds = 0;
    bool timedOut = false;
    bool cancelled = false;
    bool spawnFailed = false;
};

struct ModuleOutcome {
    int id = 0;
    std::wstring output;
    DWORD exitCode = 0;
    double seconds = 0;
    bool timedOut = false;
    bool cancelled = false;
    bool spawnFailed = false;
};

struct Layout {
    RECT sidebar{};
    RECT nav[NAV_COUNT]{};
    RECT headerTitle{};
    RECT headerSub{};
    RECT headerStatus{};
    RECT headerSearch{};
    RECT searchRect{};
    RECT adminPill{};
    RECT sidebarLogo{};
    RECT sidebarFooter{};
    RECT chips[9]{};
    int chipCount = 0;

    RECT dashboard{};
    RECT dashWelcome{};
    RECT dashWelcomeLogo{};
    RECT dashWelcomeStatus{};
    RECT dashQuick{};
    RECT dashQuickItems[3]{};
    RECT dashHistory{};
    RECT dashHistoryRows[4]{};
    RECT dashCpu{};
    RECT dashRam{};
    RECT dashDisk{};
    RECT dashGpu{};
    RECT dashSystem{};
    RECT cpuRing{};
    RECT ramRing{};
    RECT gpuRing{};

    RECT listPanel{};
    RECT listHeader{};
    RECT listArea{};
    RECT detailPanel{};
    RECT detailTitle{};
    RECT detailStatus{};
    RECT detailSub{};
    RECT detailDesc{};
    RECT detailMeta{};
    RECT detailNote{};
    RECT runButton{};
    RECT detailInfo{};
    RECT outputLabel{};
    RECT outputPanel{};
    RECT outputEdit{};
    RECT exportButton{};

    RECT reportListPanel{};
    RECT reportListHeader{};
    RECT reportListArea{};
    RECT previewPanel{};
    RECT previewTitle{};
    RECT previewMeta{};
    RECT previewStatus{};
    RECT previewOutputLabel{};
    RECT previewOutputPanel{};
    RECT previewOutputEdit{};
    RECT previewExport{};
    RECT previewFolder{};

    RECT settings1{};
    RECT settings2{};
    RECT settings3{};
    RECT animToggle{};
    RECT refreshSeg[3]{};
    RECT autoSaveToggle{};
    RECT settingsPath{};
    RECT openReportsButton{};
    RECT clearHistoryButton{};
    RECT checkAdminButton{};
    RECT aboutButton{};

    RECT aboutCard{};
    RECT aboutLogo{};
    RECT aboutAdmin{};
    RECT aboutReports{};

    int width = 0;
    int height = 0;
};

struct AppState {
    HWND hwnd = nullptr;
    HWND searchEdit = nullptr;
    HWND outputEdit = nullptr;
    HWND previewEdit = nullptr;
    HBRUSH searchBrush = nullptr;
    HBRUSH outputBrush = nullptr;
    HFONT searchFont = nullptr;
    HFONT outputFont = nullptr;

    Layout layout;
    bool admin = false;
    Settings settings;
    int nav = NAV_DASHBOARD;
    int selectedModule = 1;
    int selectedReport = -1;
    int listScroll = 0;
    int reportScroll = 0;
    int hoverNav = -1;
    int hoverModule = -1;
    int hoverReport = -1;
    int hoverChip = -1;
    int hoverButton = HB_NONE;
    int hoverRun = -1;
    bool trackingMouse = false;

    bool busy = false;
    int runningModuleId = 0;
    std::wstring runningModuleName;
    std::wstring runningStarted;
    std::wstring status = L"GOTOWY";
    bool lastRunError = false;

    Snapshot stats;
    std::deque<float> cpuHistory;
    std::deque<float> ramHistory;
    int spinnerAngle = 0;
    bool animTimerActive = false;

    std::vector<const Module*> filtered;
    std::map<int, RunInfo> runs;
    std::map<int, std::wstring> lastOutput;
    std::vector<Report> reports;
    std::wstring searchValue;
    std::wstring reportsDir;

    std::mutex statsMutex;
    Snapshot pendingStats;
    bool statsDirty = false;

    std::atomic<bool> quit{false};
    std::atomic<int> refreshSeconds{1};
    std::atomic<bool> cancelRequested{false};
    std::thread statsThread;
    std::thread moduleThread;
};

AppState g;
ULONG_PTR g_gdiplusToken = 0;
std::unique_ptr<FontFamily> g_fontFamily;
std::map<int, std::unique_ptr<Font>> g_fontCache;
std::map<std::pair<std::wstring, int>, float> g_textMeasureCache;

// ── Cached surfaces ─────────────────────────────────────────────────────────
// The window background and the glass chrome are static between layout changes,
// so they are rendered once into DIB sections and composed with GDI blits.
struct Surface {
    HDC dc = nullptr;
    HBITMAP bitmap = nullptr;
    HGDIOBJ previous = nullptr;
    int width = 0;
    int height = 0;

    ~Surface() { Reset(); }

    void Reset() {
        if (!dc) return;
        if (previous) SelectObject(dc, previous);
        if (bitmap) DeleteObject(bitmap);
        DeleteDC(dc);
        dc = nullptr;
        bitmap = nullptr;
        previous = nullptr;
        width = 0;
        height = 0;
    }

    bool Create(int w, int h) {
        if (w <= 0 || h <= 0) return false;
        Reset();
        HDC screen = GetDC(nullptr);
        dc = CreateCompatibleDC(screen);
        if (!dc) {
            ReleaseDC(nullptr, screen);
            return false;
        }
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = w;
        info.bmiHeader.biHeight = -h;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        void* bits = nullptr;
        bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (!bitmap || !bits) {
            ReleaseDC(nullptr, screen);
            Reset();
            return false;
        }
        previous = SelectObject(dc, bitmap);
        ReleaseDC(nullptr, screen);
        width = w;
        height = h;
        return true;
    }
};

// Chrome kinds that are baked into cached tiles.
enum class ChromeKind {
    Glass = 0,
    OutputPanel = 1,
};

struct PanelTile {
    Surface surface;
    RECT device{};
    RECT padded{};
    float radius = 0.0f;
    int group = -1;
    int kind = 0;
    ULONGLONG lastUse = 0;
};

Surface g_backgroundSurface;
int g_backgroundW = 0;
int g_backgroundH = 0;
std::vector<std::unique_ptr<PanelTile>> g_panelTiles;
size_t g_panelTileBytes = 0;
ULONGLONG g_panelUseTick = 0;
RECT g_paintDirty{};
bool g_fullPaint = false;
bool g_painting = false;

float g_scale = 1.0f;
HDC g_memDC = nullptr;
HBITMAP g_memBitmap = nullptr;
HGDIOBJ g_memOld = nullptr;
int g_memWidth = 0;
int g_memHeight = 0;
HANDLE g_wakeEvent = nullptr;

// ── Diagnostics (opt-in via --perf, no UI impact) ────────────────────────────
struct PerfState {
    bool enabled = false;
    FILE* file = nullptr;
    LARGE_INTEGER frequency{};
    LONGLONG start = 0;
    LONGLONG lastFlush = 0;

    LONGLONG frames = 0;
    double frameSum = 0.0;
    double frameMax = 0.0;
    double windowFrameSum = 0.0;
    double windowFrameMax = 0.0;
    LONGLONG windowFrames = 0;
    LONGLONG windowStart = 0;

    LONGLONG events = 0;
    double eventSum = 0.0;
    double eventMax = 0.0;
    UINT eventMaxMsg = 0;
    double eventMaxNonPaint = 0.0;
    UINT eventMaxNonPaintMsg = 0;
    double windowEventMax = 0.0;
    UINT windowEventMaxMsg = 0;
    double windowEventSum = 0.0;
    LONGLONG windowEvents = 0;

    ULONGLONG cpuKernel = 0;
    ULONGLONG cpuUser = 0;
    LONGLONG cpuWall = 0;
    double windowCpu = 0.0;

    double sectionBackground = 0.0;
    double sectionSidebar = 0.0;
    double sectionHeader = 0.0;
    double sectionView = 0.0;
    double sectionBlit = 0.0;
    double sectionText = 0.0;
    double sectionGlassShadow = 0.0;
    double sectionGlassBody = 0.0;
    double sectionGlassSheen = 0.0;
    double sectionGlassBorder = 0.0;
    double sectionList = 0.0;
    double sectionDetail = 0.0;
    double sectionMenu = 0.0;
    double sectionCharts = 0.0;
    double sectionDetailHeader = 0.0;
    double sectionDetailChips = 0.0;
    double sectionDetailRun = 0.0;
    double sectionDetailOutput = 0.0;
    double sectionDetailOutFill = 0.0;
    double sectionDetailOutStroke = 0.0;
    double sectionDetailOutText = 0.0;
    int sectionFrames = 0;
    LONGLONG labelCalls = 0;
    LONGLONG glassCalls = 0;
    LONGLONG pathCalls = 0;
    LONGLONG panelBlits = 0;
    LONGLONG panelBuilds = 0;
    LONGLONG fullPaints = 0;
    LONGLONG partialPaints = 0;

    static double NowMs(LARGE_INTEGER freq, LONGLONG origin) {
        LARGE_INTEGER counter{};
        QueryPerformanceCounter(&counter);
        return static_cast<double>(counter.QuadPart - origin) * 1000.0 / static_cast<double>(freq.QuadPart);
    }

    void Init() {
        enabled = true;
        QueryPerformanceFrequency(&frequency);
        LARGE_INTEGER counter{};
        QueryPerformanceCounter(&counter);
        start = counter.QuadPart;
        lastFlush = start;
        windowStart = start;
        wchar_t temp[MAX_PATH]{};
        if (GetTempPathW(MAX_PATH, temp) > 0) {
            std::wstring path = std::wstring(temp) + L"kd_perf.log";
            file = _wfopen(path.c_str(), L"w");
        }
        if (!file) enabled = false;
        FILETIME creation{}, exit{}, kernel{}, user{};
        if (GetThreadTimes(GetCurrentThread(), &creation, &exit, &kernel, &user)) {
            cpuKernel = (static_cast<ULONGLONG>(kernel.dwHighDateTime) << 32) | kernel.dwLowDateTime;
            cpuUser = (static_cast<ULONGLONG>(user.dwHighDateTime) << 32) | user.dwLowDateTime;
            cpuWall = counter.QuadPart;
        }
        if (file) {
            std::fprintf(file, "KD_PERF_START freq=%lld\n", static_cast<long long>(frequency.QuadPart));
            std::fflush(file);
        }
    }

    void Frame(double ms) {
        if (!enabled) return;
        ++frames;
        frameSum += ms;
        if (ms > frameMax) frameMax = ms;
        ++windowFrames;
        windowFrameSum += ms;
        if (ms > windowFrameMax) windowFrameMax = ms;
    }

    void Event(double ms, UINT message) {
        if (!enabled) return;
        ++events;
        eventSum += ms;
        ++windowEvents;
        windowEventSum += ms;
        if (ms > eventMax) { eventMax = ms; eventMaxMsg = message; }
        if (ms > windowEventMax) { windowEventMax = ms; windowEventMaxMsg = message; }
        if (message != WM_PAINT && ms > eventMaxNonPaint) { eventMaxNonPaint = ms; eventMaxNonPaintMsg = message; }
        MaybeFlush();
    }

    void MaybeFlush() {
        if (!enabled || !file) return;
        LARGE_INTEGER counter{};
        QueryPerformanceCounter(&counter);
        const double elapsed = static_cast<double>(counter.QuadPart - lastFlush) * 1000.0 / static_cast<double>(frequency.QuadPart);
        if (elapsed < 5000.0) return;
        Flush(false);
    }

    void Flush(bool final) {
        if (!enabled || !file) return;
        LARGE_INTEGER counter{};
        QueryPerformanceCounter(&counter);
        const double wallMs = static_cast<double>(counter.QuadPart - (final ? windowStart : lastFlush)) * 1000.0 /
                              static_cast<double>(frequency.QuadPart);
        double cpuPercent = 0.0;
        FILETIME creation{}, exit{}, kernel{}, user{};
        if (GetThreadTimes(GetCurrentThread(), &creation, &exit, &kernel, &user)) {
            const ULONGLONG k = (static_cast<ULONGLONG>(kernel.dwHighDateTime) << 32) | kernel.dwLowDateTime;
            const ULONGLONG u = (static_cast<ULONGLONG>(user.dwHighDateTime) << 32) | user.dwLowDateTime;
            if (cpuWall != 0) {
                const double wall100ns = static_cast<double>(counter.QuadPart - cpuWall) * 10000000.0 /
                                         static_cast<double>(frequency.QuadPart);
                if (wall100ns > 0.0) cpuPercent = 100.0 * static_cast<double>((k - cpuKernel) + (u - cpuUser)) / wall100ns;
            }
            cpuKernel = k;
            cpuUser = u;
            cpuWall = counter.QuadPart;
        }
        PROCESS_MEMORY_COUNTERS_EX memory{};
        memory.cb = sizeof(memory);
        double ramMb = 0.0;
        double privateMb = 0.0;
        if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory))) {
            ramMb = static_cast<double>(memory.WorkingSetSize) / (1024.0 * 1024.0);
            privateMb = static_cast<double>(memory.PrivateUsage) / (1024.0 * 1024.0);
        }
        const double windowSeconds = wallMs / 1000.0;
        const double windowFps = windowSeconds > 0.0 ? static_cast<double>(windowFrames) / windowSeconds : 0.0;
        const double framesForSections = sectionFrames > 0 ? static_cast<double>(sectionFrames) : 1.0;
        std::fprintf(file,
                     "PERF t=%.1fs frames=%lld fps=%.1f avg=%.2fms max=%.2fms | events=%lld avg=%.2fms max=%.2fms(msg=0x%X) "
                     "max_nonpaint=%.2fms(msg=0x%X) | cpu=%.1f%% ram=%.1fMB private=%.1fMB\n",
                     static_cast<double>(counter.QuadPart - start) / static_cast<double>(frequency.QuadPart),
                     frames,
                     windowFps,
                     windowFrames > 0 ? windowFrameSum / static_cast<double>(windowFrames) : 0.0,
                     windowFrameMax,
                     events,
                     windowEvents > 0 ? windowEventSum / static_cast<double>(windowEvents) : 0.0,
                     windowEventMax,
                     windowEventMaxMsg,
                     eventMaxNonPaint,
                     eventMaxNonPaintMsg,
                     cpuPercent,
                     ramMb,
                     privateMb);
        std::fprintf(file,
                     "PERF_PARTS frames=%d bg=%.2f sidebar=%.2f header=%.2f view=%.2f blit=%.2f | labels=%lld glass=%lld "
                     "| text=%.2f glass_shadow=%.2f glass_body=%.2f glass_sheen=%.2f glass_border=%.2f "
                     "| list=%.2f detail=%.2f menu=%.2f charts=%.2f "
                     "| d_head=%.2f d_chips=%.2f d_run=%.2f d_out=%.2f d_out_text=%.2f d_out_fill=%.2f d_out_stroke=%.2f "
                     "| tile_blits=%lld tile_builds=%lld full=%lld partial=%lld cache=%.1fMB\n",
                     sectionFrames,
                     sectionBackground / framesForSections,
                     sectionSidebar / framesForSections,
                     sectionHeader / framesForSections,
                     sectionView / framesForSections,
                     sectionBlit / framesForSections,
                     labelCalls,
                     glassCalls,
                     sectionText / framesForSections,
                     sectionGlassShadow / framesForSections,
                     sectionGlassBody / framesForSections,
                     sectionGlassSheen / framesForSections,
                     sectionGlassBorder / framesForSections,
                     sectionList / framesForSections,
                     sectionDetail / framesForSections,
                     sectionMenu / framesForSections,
                     sectionCharts / framesForSections,
                     sectionDetailHeader / framesForSections,
                     sectionDetailChips / framesForSections,
                     sectionDetailRun / framesForSections,
                     sectionDetailOutput / framesForSections,
                     sectionDetailOutText / framesForSections,
                     sectionDetailOutFill / framesForSections,
                     sectionDetailOutStroke / framesForSections,
                     panelBlits,
                     panelBuilds,
                     fullPaints,
                     partialPaints,
                     static_cast<double>(g_panelTileBytes) / (1024.0 * 1024.0));
        std::fflush(file);
        sectionBackground = 0.0;
        sectionSidebar = 0.0;
        sectionHeader = 0.0;
        sectionView = 0.0;
        sectionBlit = 0.0;
        sectionText = 0.0;
        sectionGlassShadow = 0.0;
        sectionGlassBody = 0.0;
        sectionGlassSheen = 0.0;
        sectionGlassBorder = 0.0;
        sectionList = 0.0;
        sectionDetail = 0.0;
        sectionMenu = 0.0;
        sectionCharts = 0.0;
        sectionDetailHeader = 0.0;
        sectionDetailChips = 0.0;
        sectionDetailRun = 0.0;
        sectionDetailOutput = 0.0;
        sectionDetailOutText = 0.0;
        sectionDetailOutFill = 0.0;
        sectionDetailOutStroke = 0.0;
        sectionFrames = 0;
        labelCalls = 0;
        glassCalls = 0;
        panelBlits = 0;
        panelBuilds = 0;
        fullPaints = 0;
        partialPaints = 0;
        std::fflush(file);
        lastFlush = counter.QuadPart;
        windowStart = counter.QuadPart;
        windowFrames = 0;
        windowFrameSum = 0.0;
        windowFrameMax = 0.0;
        windowEvents = 0;
        windowEventSum = 0.0;
        windowEventMax = 0.0;
        windowEventMaxMsg = 0;
        if (final) {
            std::fprintf(file, "PERF_END frames=%lld avg_frame=%.2fms max_frame=%.2fms events=%lld max_event=%.2fms max_event_msg=0x%X\n",
                         frames, frames > 0 ? frameSum / static_cast<double>(frames) : 0.0, frameMax,
                         events, eventMax, eventMaxMsg);
            std::fflush(file);
            std::fclose(file);
            file = nullptr;
            enabled = false;
        }
    }
};

PerfState g_perf;

struct PerfFrameGuard {
    LARGE_INTEGER begin{};
    bool active = false;
    PerfFrameGuard() {
        if (g_perf.enabled) {
            QueryPerformanceCounter(&begin);
            active = true;
        }
    }
    ~PerfFrameGuard() {
        if (!active) return;
        LARGE_INTEGER end{};
        QueryPerformanceCounter(&end);
        const double ms = static_cast<double>(end.QuadPart - begin.QuadPart) * 1000.0 /
                          static_cast<double>(g_perf.frequency.QuadPart);
        g_perf.Frame(ms);
    }
};

struct PerfEventGuard {
    LARGE_INTEGER begin{};
    UINT message = 0;
    bool active = false;
    explicit PerfEventGuard(UINT msg) : message(msg) {
        if (g_perf.enabled) {
            QueryPerformanceCounter(&begin);
            active = true;
        }
    }
    ~PerfEventGuard() {
        if (!active) return;
        LARGE_INTEGER end{};
        QueryPerformanceCounter(&end);
        const double ms = static_cast<double>(end.QuadPart - begin.QuadPart) * 1000.0 /
                          static_cast<double>(g_perf.frequency.QuadPart);
        g_perf.Event(ms, message);
    }
};

struct PerfSpan {
    double* target = nullptr;
    LARGE_INTEGER begin{};
    bool active = false;
    explicit PerfSpan(double* value) : target(value) {
        if (g_perf.enabled) {
            QueryPerformanceCounter(&begin);
            active = true;
        }
    }
    ~PerfSpan() {
        if (!active) return;
        LARGE_INTEGER end{};
        QueryPerformanceCounter(&end);
        *target += static_cast<double>(end.QuadPart - begin.QuadPart) * 1000.0 /
                   static_cast<double>(g_perf.frequency.QuadPart);
    }
};


const wchar_t* kNavLabels[NAV_COUNT] = {
    L"Dashboard", L"Diagnostyka", L"Moduły", L"Monitor systemu", L"Sprzęt", L"Dyski", L"Sieć",
    L"Wydajność", L"Bezpieczeństwo", L"Urządzenia", L"Konserwacja", L"Raporty", L"Ustawienia",
    L"Informacje o KD Core"
};

const wchar_t* kNavTitles[NAV_COUNT] = {
    L"Pulpit KD Core", L"Diagnostyka", L"Moduły", L"Monitor systemu", L"Sprzęt", L"Dyski", L"Sieć",
    L"Wydajność", L"Bezpieczeństwo", L"Urządzenia", L"Konserwacja", L"Raporty", L"Ustawienia",
    L"Informacje o KD Core"
};

RECT MakeRect(int x, int y, int w, int h) { return RECT{x, y, x + w, y + h}; }
int Width(const RECT& r) { return r.right - r.left; }
int Height(const RECT& r) { return r.bottom - r.top; }
bool Contains(const RECT& r, POINT p) { return p.x >= r.left && p.x < r.right && p.y >= r.top && p.y < r.bottom; }
bool Intersects(const RECT& a, const RECT& b) {
    return a.left < b.right && b.left < a.right && a.top < b.bottom && b.top < a.bottom;
}
bool ContainsRect(const RECT& outer, const RECT& inner) {
    return inner.left >= outer.left && inner.top >= outer.top && inner.right <= outer.right &&
           inner.bottom <= outer.bottom;
}
RECT DeviceRect(const RECT& r) {
    RECT d{};
    d.left = static_cast<LONG>(std::floor(static_cast<double>(r.left) * g_scale));
    d.top = static_cast<LONG>(std::floor(static_cast<double>(r.top) * g_scale));
    d.right = static_cast<LONG>(std::ceil(static_cast<double>(r.right) * g_scale));
    d.bottom = static_cast<LONG>(std::ceil(static_cast<double>(r.bottom) * g_scale));
    return d;
}
bool Visible(const RECT& rect) {
    if (!g_painting) return true;
    return Intersects(DeviceRect(rect), g_paintDirty);
}
Color WithAlpha(const Color& c, BYTE alpha) { return Color(alpha, c.GetR(), c.GetG(), c.GetB()); }

int ScalePx(int value) { return static_cast<int>(std::lround(static_cast<double>(value) * g_scale)); }
POINT PointToLogical(POINT point) {
    point.x = static_cast<LONG>(std::lround(static_cast<double>(point.x) / g_scale));
    point.y = static_cast<LONG>(std::lround(static_cast<double>(point.y) / g_scale));
    return point;
}
void InvalidateAll() { if (g.hwnd) InvalidateRect(g.hwnd, nullptr, FALSE); }
void InvalidateLogical(const RECT& rect) {
    if (!g.hwnd) return;
    RECT physical{ScalePx(rect.left - 2), ScalePx(rect.top - 2), ScalePx(rect.right + 2), ScalePx(rect.bottom + 2)};
    InvalidateRect(g.hwnd, &physical, FALSE);
}
void UpdateAnimTimer();
void RecalculateLayout();
void ClampScroll();
void UpdateControlPositions();
void UpdateOutputText();
void UpdateReportPreviewText();
void RebuildFiltered();
void EnsureSelection();
void StartStatsThread();
void StopStatsThread();
void ActivateNav(int index);
RECT ModuleRowRect(int index);
RECT ReportRowRect(int index);
int FindButtonAt(POINT point);
bool ButtonDisabled(int button);
void OnButtonClicked(int button);
void PaintWindow(HWND hwnd);
void DrawMetaPill(Graphics& graphics, const RECT& rect, const std::wstring& text, const Color& color, float fontSize);
void DrawKdMark(Graphics& graphics, RECT rect);
void DrawPanelHeader(Graphics& graphics, const RECT& panel, Icon icon, const std::wstring& title,
                     const std::wstring& right, const Color& accent = C_SAGE, bool rightPill = false,
                     const Color& rightColor = C_MUTED);
void EnsureBackground();
void DrawSidebar(Graphics& graphics);
void DrawHeader(Graphics& graphics);
void DrawDashboard(Graphics& graphics);
void DrawModulesView(Graphics& graphics);
void DrawReportsView(Graphics& graphics);
void DrawSettingsView(Graphics& graphics);
void DrawAboutView(Graphics& graphics);
void LoadReportsFromDisk();

std::wstring Trim(const std::wstring& value) {
    const size_t first = value.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) return L"";
    const size_t last = value.find_last_not_of(L" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::wstring Lower(std::wstring value) {
    for (wchar_t& c : value) {
        switch (c) {
        case L'Ą': c = L'ą'; break;
        case L'Ć': c = L'ć'; break;
        case L'Ę': c = L'ę'; break;
        case L'Ł': c = L'ł'; break;
        case L'Ń': c = L'ń'; break;
        case L'Ó': c = L'ó'; break;
        case L'Ś': c = L'ś'; break;
        case L'Ź': c = L'ź'; break;
        case L'Ż': c = L'ż'; break;
        default:
            if (c >= L'A' && c <= L'Z') c = static_cast<wchar_t>(c - L'A' + L'a');
            else c = static_cast<wchar_t>(std::towlower(c));
            break;
        }
    }
    return value;
}

bool StartsWithI(const std::wstring& text, const std::wstring& prefix) {
    if (text.size() < prefix.size()) return false;
    return _wcsnicmp(text.c_str(), prefix.c_str(), prefix.size()) == 0;
}

std::wstring Comma(std::wstring value) {
    std::replace(value.begin(), value.end(), L'.', L',');
    return value;
}

std::wstring FormatBytes(uint64_t bytes) {
    wchar_t buffer[64]{};
    if (bytes >= (1ull << 40)) swprintf(buffer, 64, L"%.2f TB", static_cast<double>(bytes) / 1099511627776.0);
    else if (bytes >= (1ull << 30)) swprintf(buffer, 64, L"%.1f GB", static_cast<double>(bytes) / 1073741824.0);
    else if (bytes >= (1ull << 20)) swprintf(buffer, 64, L"%.0f MB", static_cast<double>(bytes) / 1048576.0);
    else swprintf(buffer, 64, L"%.0f KB", static_cast<double>(bytes) / 1024.0);
    return Comma(buffer);
}

std::wstring FormatSpeed(double bytesPerSecond) {
    if (bytesPerSecond < 0) bytesPerSecond = 0;
    wchar_t buffer[64]{};
    if (bytesPerSecond >= 1048576.0) swprintf(buffer, 64, L"%.1f MB/s", bytesPerSecond / 1048576.0);
    else if (bytesPerSecond >= 1024.0) swprintf(buffer, 64, L"%.0f KB/s", bytesPerSecond / 1024.0);
    else swprintf(buffer, 64, L"%.0f B/s", bytesPerSecond);
    return Comma(buffer);
}

std::wstring FormatDuration(double seconds) {
    if (seconds < 0) seconds = 0;
    wchar_t buffer[64]{};
    if (seconds < 60.0) swprintf(buffer, 64, L"%.2f s", seconds);
    else if (seconds < 3600.0) swprintf(buffer, 64, L"%d min %d s", static_cast<int>(seconds / 60.0), static_cast<int>(std::fmod(seconds, 60.0)));
    else swprintf(buffer, 64, L"%d g %d min", static_cast<int>(seconds / 3600.0), static_cast<int>(std::fmod(seconds, 3600.0) / 60.0));
    return Comma(buffer);
}

std::wstring FormatUptime(ULONGLONG milliseconds) {
    const ULONGLONG totalMinutes = milliseconds / 60000ull;
    const ULONGLONG days = totalMinutes / 1440ull;
    const ULONGLONG hours = (totalMinutes % 1440ull) / 60ull;
    const ULONGLONG minutes = totalMinutes % 60ull;
    wchar_t buffer[96]{};
    if (days > 0) swprintf(buffer, 96, L"%llu d %llu g %llu min", days, hours, minutes);
    else if (hours > 0) swprintf(buffer, 96, L"%llu g %llu min", hours, minutes);
    else swprintf(buffer, 96, L"%llu min", minutes);
    return buffer;
}

std::wstring FormatMhz(int mhz) {
    if (mhz <= 0) return L"n/d";
    wchar_t buffer[64]{};
    if (mhz >= 1000) swprintf(buffer, 64, L"%.2f GHz", mhz / 1000.0);
    else swprintf(buffer, 64, L"%d MHz", mhz);
    return Comma(buffer);
}

std::wstring CurrentTimeString() {
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t buffer[32]{};
    swprintf(buffer, 32, L"%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
    return buffer;
}

std::wstring FullTimestamp() {
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t buffer[64]{};
    swprintf(buffer, 64, L"%02d.%02d.%04d, %02d:%02d:%02d", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
    return buffer;
}

std::wstring FileStamp() {
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t buffer[64]{};
    swprintf(buffer, 64, L"%04d%02d%02d-%02d%02d%02d-%03d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    return buffer;
}

std::wstring NormalizeNewlines(const std::wstring& value) {
    std::wstring out;
    out.reserve(value.size() + 32);
    for (size_t i = 0; i < value.size(); ++i) {
        const wchar_t c = value[i];
        if (c == L'\r') {
            out += L"\r\n";
            if (i + 1 < value.size() && value[i + 1] == L'\n') ++i;
        } else if (c == L'\n') {
            out += L"\r\n";
        } else if (c != L'\0') {
            out += c;
        }
    }
    return out;
}

std::string WideToUtf8String(const std::wstring& value) {
    if (value.empty()) return std::string();
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) return std::string();
    std::string result(static_cast<size_t>(bytes), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), bytes, nullptr, nullptr);
    return result;
}

std::wstring Utf8ToWide(const std::string& value) {
    if (value.empty()) return L"";
    const int chars = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (chars <= 0) return L"";
    std::wstring result(static_cast<size_t>(chars), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), chars);
    return result;
}

bool WriteTextFileUtf8(const std::filesystem::path& path, const std::wstring& text) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) return false;
    const unsigned char bom[3] = {0xEF, 0xBB, 0xBF};
    file.write(reinterpret_cast<const char*>(bom), 3);
    const std::string utf8 = WideToUtf8String(text);
    if (!utf8.empty()) file.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
    return file.good();
}

std::wstring RegReadString(HKEY root, const wchar_t* subkey, const wchar_t* name) {
    DWORD size = 0;
    if (RegGetValueW(root, subkey, name, RRF_RT_REG_SZ, nullptr, nullptr, &size) != ERROR_SUCCESS || size < sizeof(wchar_t)) return L"";
    std::wstring value(size / sizeof(wchar_t), L'\0');
    if (RegGetValueW(root, subkey, name, RRF_RT_REG_SZ, nullptr, value.data(), &size) != ERROR_SUCCESS) return L"";
    while (!value.empty() && value.back() == L'\0') value.pop_back();
    return Trim(value);
}

DWORD RegReadDword(HKEY root, const wchar_t* subkey, const wchar_t* name, DWORD fallback) {
    DWORD value = fallback;
    DWORD size = sizeof(value);
    DWORD type = 0;
    if (RegGetValueW(root, subkey, name, RRF_RT_REG_DWORD, &type, &value, &size) != ERROR_SUCCESS) return fallback;
    return value;
}

void RegWriteDword(const wchar_t* subkey, const wchar_t* name, DWORD value) {
    HKEY key = nullptr;
    DWORD disposition = 0;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, subkey, 0, nullptr, 0, KEY_WRITE, nullptr, &key, &disposition) == ERROR_SUCCESS) {
        RegSetValueExW(key, name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
        RegCloseKey(key);
    }
}

std::wstring GetExePath() {
    wchar_t path[MAX_PATH * 2]{};
    GetModuleFileNameW(nullptr, path, static_cast<DWORD>(std::size(path)));
    return path;
}

std::wstring SanitizeFileName(std::wstring name) {
    if (name.empty()) name = L"Modul";
    for (wchar_t& c : name) {
        if (c < 32 || wcschr(L"<>:\"/\\|?*", c) || c == L' ') c = L'_';
    }
    if (name.size() > 48) name.resize(48);
    return name;
}

void LoadSettings() {
    const wchar_t* key = L"Software\\KD Core";
    const wchar_t* legacy = L"Software\\Kombajn Diagnostyczny";
    DWORD probe = 0;
    const bool hasNew = RegGetValueW(HKEY_CURRENT_USER, key, L"RefreshSeconds", RRF_RT_REG_DWORD,
                                     nullptr, nullptr, &probe) == ERROR_SUCCESS;
    const wchar_t* source = hasNew ? key : legacy;
    int refresh = static_cast<int>(RegReadDword(HKEY_CURRENT_USER, source, L"RefreshSeconds", 1));
    if (refresh != 1 && refresh != 2 && refresh != 5) refresh = 1;
    g.settings.refreshSeconds = refresh;
    g.settings.animations = RegReadDword(HKEY_CURRENT_USER, source, L"Animations", 1) != 0;
    g.settings.autoSaveReports = RegReadDword(HKEY_CURRENT_USER, source, L"AutoSaveReports", 1) != 0;
    g.refreshSeconds.store(refresh);
    if (!hasNew) {
        // One-time migration of user preferences from the previous product name.
        RegWriteDword(key, L"RefreshSeconds", static_cast<DWORD>(refresh));
        RegWriteDword(key, L"Animations", g.settings.animations ? 1u : 0u);
        RegWriteDword(key, L"AutoSaveReports", g.settings.autoSaveReports ? 1u : 0u);
    }
}

void SaveSettings() {
    const wchar_t* key = L"Software\\KD Core";
    RegWriteDword(key, L"RefreshSeconds", static_cast<DWORD>(g.settings.refreshSeconds));
    RegWriteDword(key, L"Animations", g.settings.animations ? 1u : 0u);
    RegWriteDword(key, L"AutoSaveReports", g.settings.autoSaveReports ? 1u : 0u);
}

void ResolveReportsDir() {
    std::filesystem::path documents;
    PWSTR known = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_CREATE, nullptr, &known)) && known) {
        documents = known;
        CoTaskMemFree(known);
    } else {
        wchar_t profile[MAX_PATH]{};
        if (GetEnvironmentVariableW(L"USERPROFILE", profile, MAX_PATH)) documents = profile;
        documents /= L"Documents";
    }
    std::error_code ec;
    const std::filesystem::path documentsRoot = documents;
    documents /= L"KD Core";
    documents /= L"Raporty";
    g.reportsDir = documents.wstring();
    std::filesystem::create_directories(documents, ec);

    // Bring reports written by the previous product name into the new folder.
    const std::filesystem::path legacy = documentsRoot / L"Kombajn Diagnostyczny" / L"Raporty";
    if (std::filesystem::is_directory(legacy, ec)) {
        for (std::filesystem::directory_iterator it(legacy, ec), end; it != end; it.increment(ec)) {
            if (ec) break;
            if (!it->is_regular_file(ec) || it->path().extension() != L".txt") continue;
            const std::filesystem::path target = documents / it->path().filename();
            if (std::filesystem::exists(target, ec)) continue;
            std::filesystem::copy_file(it->path(), target, ec);
            ec.clear();
        }
    }
}

std::wstring ReportFileSortKey(const std::filesystem::path& path) {
    // Generated names start with YYYYMMDD-HHMMSS; migrated reports keep that prefix.
    // Prefer it over the filesystem timestamp, which is identical for all migrated files.
    std::wstring digits;
    for (wchar_t c : path.stem().wstring()) {
        if (c >= L'0' && c <= L'9') {
            digits.push_back(c);
            if (digits.size() == 14) return digits;
        }
    }
    return L"";
}

void LoadReportsFromDisk() {
    namespace fs = std::filesystem;
    struct Candidate {
        fs::path path;
        std::wstring key;
        fs::file_time_type written;
    };

    std::error_code ec;
    std::vector<Candidate> files;
    for (fs::directory_iterator it(g.reportsDir, ec), end; it != end; it.increment(ec)) {
        if (ec) break;
        if (!it->is_regular_file(ec) || it->path().extension() != L".txt") continue;
        std::error_code timeEc;
        files.push_back({it->path(), ReportFileSortKey(it->path()), fs::last_write_time(it->path(), timeEc)});
    }
    std::sort(files.begin(), files.end(), [](const Candidate& a, const Candidate& b) {
        if (!a.key.empty() || !b.key.empty()) {
            if (a.key.empty()) return false;
            if (b.key.empty()) return true;
            if (a.key != b.key) return a.key > b.key;
        }
        if (a.written != b.written) return a.written > b.written;
        return a.path.filename().wstring() > b.path.filename().wstring();
    });
    if (files.size() > 8) files.resize(8);

    auto field = [](const std::wstring& text, const wchar_t* label) {
        const std::wstring prefix = label;
        size_t pos = 0;
        while (pos < text.size()) {
            const size_t end = text.find(L'\n', pos);
            const size_t len = (end == std::wstring::npos) ? text.size() - pos : end - pos;
            std::wstring line = text.substr(pos, len);
            if (!line.empty() && line.back() == L'\r') line.pop_back();
            if (line.compare(0, prefix.size(), prefix) == 0) return Trim(line.substr(prefix.size()));
            if (end == std::wstring::npos) break;
            pos = end + 1;
        }
        return std::wstring();
    };

    for (const Candidate& candidate : files) {
        const fs::path& path = candidate.path;
        std::ifstream file(path, std::ios::binary);
        if (!file) continue;
        std::string raw((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (raw.size() > 192u * 1024u) raw.resize(192u * 1024u);
        const std::wstring text = Utf8ToWide(raw);
        if (text.empty()) continue;
        Report report;
        report.file = path.wstring();
        report.moduleName = field(text, L"Moduł:");
        report.category = field(text, L"Kategoria:");
        report.startedAt = field(text, L"Start:");
        report.durationText = field(text, L"Czas:");
        report.moduleId = _wtoi(field(text, L"ID modułu:").c_str());
        report.exitCode = _wtoi(field(text, L"Kod wyjścia:").c_str());
        const std::wstring status = field(text, L"Status:");
        report.timedOut = status.find(L"Limit czasu") != std::wstring::npos;
        report.cancelled = status.find(L"Anulowano") != std::wstring::npos;
        report.autoSaved = true;
        report.output = text;
        if (report.moduleName.empty()) report.moduleName = path.stem().wstring();
        g.reports.push_back(report);
    }
}

std::wstring ReportDurationText(const Report& report) {
    if (report.seconds <= 0.0 && !report.durationText.empty()) return report.durationText;
    return FormatDuration(report.seconds);
}

const wchar_t* CurrentCategory() {
    if (g.nav >= NAV_SYSTEM && g.nav <= NAV_MAINTENANCE) return kCategories[g.nav - NAV_SYSTEM];
    return nullptr;
}

bool ModulesView() { return g.nav >= NAV_DIAGNOSTICS && g.nav <= NAV_MAINTENANCE; }

const Module* FindModuleById(int id) {
    for (const auto& module : kModules) if (module.id == id) return &module;
    return nullptr;
}

void RebuildFiltered() {
    g.filtered.clear();
    const std::wstring query = Lower(Trim(g.searchValue));
    const wchar_t* selectedCategory = CurrentCategory();
    for (const wchar_t* category : kCategories) {
        if (selectedCategory && wcscmp(category, selectedCategory) != 0) continue;
        for (const auto& module : kModules) {
            if (wcscmp(module.category, category) != 0) continue;
            if (!query.empty()) {
                const std::wstring haystack = Lower(std::wstring(module.name) + L" " + module.description + L" " + module.category + L" " + std::to_wstring(module.id));
                if (haystack.find(query) == std::wstring::npos) continue;
            }
            g.filtered.push_back(&module);
        }
    }
}

void EnsureSelection() {
    if (!ModulesView()) return;
    if (g.filtered.empty()) {
        g.selectedModule = 0;
        return;
    }
    bool found = false;
    for (const Module* module : g.filtered) {
        if (module->id == g.selectedModule) { found = true; break; }
    }
    if (!found) g.selectedModule = g.filtered.front()->id;
}

int CategoryCount(const wchar_t* category) {
    int count = 0;
    for (const auto& module : kModules) if (wcscmp(module.category, category) == 0) ++count;
    return count;
}

Icon IconForCategory(const wchar_t* category) {
    if (wcscmp(category, L"SYSTEM") == 0) return Icon::System;
    if (wcscmp(category, L"SPRZĘT") == 0) return Icon::Cpu;
    if (wcscmp(category, L"DYSKI") == 0) return Icon::Disk;
    if (wcscmp(category, L"SIEĆ") == 0) return Icon::Network;
    if (wcscmp(category, L"WYDAJNOŚĆ") == 0) return Icon::Gauge;
    if (wcscmp(category, L"BEZPIECZEŃSTWO") == 0) return Icon::Shield;
    if (wcscmp(category, L"URZĄDZENIA") == 0) return Icon::Devices;
    if (wcscmp(category, L"KONSERWACJA") == 0) return Icon::Wrench;
    return Icon::Modules;
}

Icon IconForNav(int nav) {
    switch (nav) {
    case NAV_DASHBOARD: return Icon::Dashboard;
    case NAV_DIAGNOSTICS: return Icon::Play;
    case NAV_MODULES: return Icon::Modules;
    case NAV_SYSTEM: return Icon::System;
    case NAV_HARDWARE: return Icon::Cpu;
    case NAV_DISKS: return Icon::Disk;
    case NAV_NETWORK: return Icon::Network;
    case NAV_PERFORMANCE: return Icon::Gauge;
    case NAV_SECURITY: return Icon::Shield;
    case NAV_DEVICES: return Icon::Devices;
    case NAV_MAINTENANCE: return Icon::Wrench;
    case NAV_REPORTS: return Icon::Report;
    case NAV_SETTINGS: return Icon::Settings;
    case NAV_ABOUT: return Icon::Info;
    default: return Icon::None;
    }
}

Font* GetFont(float size, bool bold) {
    if (!g_fontFamily) return nullptr;
    const int key = static_cast<int>(std::lround(size * 10.0f)) * 2 + (bold ? 1 : 0);
    auto it = g_fontCache.find(key);
    if (it != g_fontCache.end()) return it->second.get();
    auto font = std::make_unique<Font>(g_fontFamily.get(), size, bold ? FontStyleBold : FontStyleRegular, UnitPixel);
    Font* pointer = font.get();
    g_fontCache.emplace(key, std::move(font));
    return pointer;
}

// GDI+ StringFormat construction is surprisingly expensive. The UI uses only a
// handful of layouts, so formats are created once and reused for every label.
StringFormat* GetStringFormat(StringAlignment align, bool wrap) {
    static StringFormat* cache[2][3] = {};
    int index = 0;
    if (align == StringAlignmentCenter) index = 1;
    else if (align == StringAlignmentFar) index = 2;
    StringFormat*& slot = cache[wrap ? 1 : 0][index];
    if (slot) return slot;
    slot = new StringFormat();
    slot->SetAlignment(align);
    slot->SetLineAlignment(wrap ? StringAlignmentNear : StringAlignmentCenter);
    slot->SetTrimming(wrap ? StringTrimmingEllipsisWord : StringTrimmingEllipsisCharacter);
    if (!wrap) slot->SetFormatFlags(StringFormatFlagsNoWrap);
    return slot;
}

void DrawLabel(Graphics& graphics, const std::wstring& text, const RECT& rect, float size, const Color& color,
               bool bold = false, StringAlignment align = StringAlignmentNear) {
    if (text.empty() || Width(rect) <= 0 || Height(rect) <= 0 || !Visible(rect)) return;
    ++g_perf.labelCalls;
    Font* font = GetFont(size, bold);
    if (!font) return;
    SolidBrush brush(color);
    const RectF box(static_cast<REAL>(rect.left), static_cast<REAL>(rect.top), static_cast<REAL>(Width(rect)), static_cast<REAL>(Height(rect)));
    PerfSpan span(&g_perf.sectionText);
    graphics.DrawString(text.c_str(), static_cast<INT>(text.size()), font, box, GetStringFormat(align, false), &brush);
}

void DrawLabelWrap(Graphics& graphics, const std::wstring& text, const RECT& rect, float size, const Color& color) {
    if (text.empty() || Width(rect) <= 0 || Height(rect) <= 0 || !Visible(rect)) return;
    ++g_perf.labelCalls;
    Font* font = GetFont(size, false);
    if (!font) return;
    SolidBrush brush(color);
    const RectF box(static_cast<REAL>(rect.left), static_cast<REAL>(rect.top), static_cast<REAL>(Width(rect)), static_cast<REAL>(Height(rect)));
    GraphicsState state = graphics.Save();
    graphics.SetClip(Rect(rect.left, rect.top, Width(rect), Height(rect)), CombineModeIntersect);
    PerfSpan span(&g_perf.sectionText);
    graphics.DrawString(text.c_str(), static_cast<INT>(text.size()), font, box, GetStringFormat(StringAlignmentNear, true), &brush);
    graphics.Restore(state);
}

float MeasureText(Graphics& graphics, const std::wstring& text, float size, bool bold) {
    Font* font = GetFont(size, bold);
    if (!font) return 0;
    const int fontKey = static_cast<int>(std::lround(size * 10.0f)) * 2 + (bold ? 1 : 0);
    const int scaleKey = static_cast<int>(std::lround(g_scale * 100.0f));
    auto cacheKey = std::make_pair(text, fontKey * 1000 + scaleKey);
    auto it = g_textMeasureCache.find(cacheKey);
    if (it != g_textMeasureCache.end()) return it->second;
    RectF bounds;
    graphics.MeasureString(text.c_str(), static_cast<INT>(text.size()), font, PointF(0, 0), &bounds);
    g_textMeasureCache.emplace(cacheKey, bounds.Width);
    return bounds.Width;
}

void AppendRoundedPath(GraphicsPath& path, const RectF& rect, float radius) {
    const float w = rect.Width;
    const float h = rect.Height;
    float r = radius;
    if (r > w * 0.5f) r = w * 0.5f;
    if (r > h * 0.5f) r = h * 0.5f;
    if (r < 0.5f) { path.AddRectangle(rect); return; }
    const float d = r * 2.0f;
    path.AddArc(rect.X, rect.Y, d, d, 180.0f, 90.0f);
    path.AddArc(rect.X + w - d, rect.Y, d, d, 270.0f, 90.0f);
    path.AddArc(rect.X + w - d, rect.Y + h - d, d, d, 0.0f, 90.0f);
    path.AddArc(rect.X, rect.Y + h - d, d, d, 90.0f, 90.0f);
    path.CloseFigure();
}

void RectRoundedPath(GraphicsPath& path, const RECT& rect, float radius) {
    AppendRoundedPath(path, RectF(static_cast<REAL>(rect.left), static_cast<REAL>(rect.top),
                                  static_cast<REAL>(Width(rect)), static_cast<REAL>(Height(rect))), radius);
}

void FillRounded(Graphics& graphics, const RECT& rect, float radius, const Color& color) {
    GraphicsPath path;
    RectRoundedPath(path, rect, radius);
    SolidBrush brush(color);
    graphics.FillPath(&brush, &path);
}

void StrokeRounded(Graphics& graphics, const RECT& rect, float radius, const Color& color, float width = 1.0f) {
    GraphicsPath path;
    RectRoundedPath(path, rect, radius);
    Pen pen(color, width);
    graphics.DrawPath(&pen, &path);
}

void DrawGlow(Graphics& graphics, const RectF& rect, const Color& centerColor) {
    GraphicsPath path;
    path.AddEllipse(rect);
    PathGradientBrush brush(&path);
    brush.SetCenterColor(centerColor);
    Color surround[1] = {Color(0, centerColor.GetR(), centerColor.GetG(), centerColor.GetB())};
    int count = 1;
    brush.SetSurroundColors(surround, &count);
    graphics.FillPath(&brush, &path);
}

void RenderGlassPanel(Graphics& graphics, const RECT& rect, float radius, bool hovered, bool selected) {
    const RectF box(static_cast<REAL>(rect.left), static_cast<REAL>(rect.top), static_cast<REAL>(Width(rect)), static_cast<REAL>(Height(rect)));
    GraphicsPath path;
    AppendRoundedPath(path, box, radius);

    // One restrained offset edge is enough to lift the card off the surface.
    {
        PerfSpan span(&g_perf.sectionGlassShadow);
        GraphicsPath edge;
        RectF offsetBox(box.X, box.Y + 2.0f, box.Width, box.Height);
        AppendRoundedPath(edge, offsetBox, radius);
        SolidBrush shade(Color(26, 10, 18, 22));
        graphics.FillPath(&shade, &edge);
    }
    {
        PerfSpan span(&g_perf.sectionGlassBody);
        LinearGradientBrush body(box, C_CARD_TOP, C_CARD_BOTTOM, LinearGradientModeVertical);
        graphics.FillPath(&body, &path);
    }
    if (hovered || selected) {
        PerfSpan span(&g_perf.sectionGlassSheen);
        SolidBrush tint(selected ? Color(28, 129, 212, 250) : Color(16, 129, 212, 250));
        graphics.FillPath(&tint, &path);
    }
    Color border = selected ? WithAlpha(C_LIGHT_BLUE, 150) : (hovered ? WithAlpha(C_LIGHT_BLUE, 96) : C_LINE);
    {
        PerfSpan span(&g_perf.sectionGlassBorder);
        Pen pen(border, selected ? 1.4f : 1.0f);
        graphics.DrawPath(&pen, &path);
        Pen highlight(WithAlpha(C_ICE_WHITE, 24), 1.0f);
        graphics.DrawLine(&highlight, static_cast<REAL>(rect.left + static_cast<int>(radius)), static_cast<REAL>(rect.top + 1),
                          static_cast<REAL>(rect.right - static_cast<int>(radius)), static_cast<REAL>(rect.top + 1));
    }
}

// All eight module category screens share one layout, so their chrome tiles are
// interchangeable. Grouping keeps the cache small and avoids re-rendering the
// same panels when the user browses categories.
int PanelGroup() {
    if (g.nav == NAV_DASHBOARD) return 0;
    if (g.nav == NAV_REPORTS) return 2;
    if (g.nav == NAV_SETTINGS) return 3;
    if (g.nav == NAV_ABOUT) return 4;
    return 1;
}

void EvictPanelTiles(size_t incomingBytes) {
    constexpr size_t kMaxTileBytes = 24u * 1024u * 1024u;
    constexpr size_t kMaxTiles = 24;
    while (!g_panelTiles.empty() &&
           (g_panelTiles.size() >= kMaxTiles || g_panelTileBytes + incomingBytes > kMaxTileBytes)) {
        size_t victim = 0;
        for (size_t i = 1; i < g_panelTiles.size(); ++i) {
            if (g_panelTiles[i]->lastUse < g_panelTiles[victim]->lastUse) victim = i;
        }
        const size_t bytes = static_cast<size_t>(g_panelTiles[victim]->surface.width) *
                             static_cast<size_t>(g_panelTiles[victim]->surface.height) * 4u;
        g_panelTileBytes = (g_panelTileBytes > bytes) ? (g_panelTileBytes - bytes) : 0u;
        g_panelTiles.erase(g_panelTiles.begin() + static_cast<std::ptrdiff_t>(victim));
    }
}

// Glass panels and output wells never change shape between layout changes.
// Render the whole chrome once into an opaque tile and blit it back, which is
// an order of magnitude cheaper than compositing large alpha gradients again on
// every repaint.
void RenderChrome(Graphics& graphics, ChromeKind kind, const RECT& rect, float radius) {
    switch (kind) {
    case ChromeKind::Glass:
        RenderGlassPanel(graphics, rect, radius, false, false);
        break;
    case ChromeKind::OutputPanel: {
        {
            PerfSpan span(&g_perf.sectionDetailOutFill);
            FillRounded(graphics, rect, radius, WithAlpha(C_OUTPUT_BG, 244));
        }
        {
            PerfSpan span(&g_perf.sectionDetailOutStroke);
            StrokeRounded(graphics, rect, radius, WithAlpha(C_LIGHT_BLUE, 42), 1.0f);
        }
        break;
    }
    }
}

bool CompositeCachedChrome(Graphics& graphics, const RECT& rect, float radius, ChromeKind kind, float padLogical) {
    (void)graphics;
    if (!g_memDC || !g_backgroundSurface.dc || !g_painting) return false;
    const RECT device = DeviceRect(rect);
    if (Width(device) <= 2 || Height(device) <= 2) return false;
    const int pad = static_cast<int>(std::ceil(padLogical * g_scale)) + 2;
    RECT padded{device.left - pad, device.top - pad, device.right + pad, device.bottom + pad};
    padded.left = std::max<LONG>(padded.left, 0);
    padded.top = std::max<LONG>(padded.top, 0);
    padded.right = std::min<LONG>(padded.right, g_memWidth);
    padded.bottom = std::min<LONG>(padded.bottom, g_memHeight);
    if (Width(padded) <= 0 || Height(padded) <= 0) return false;

    const int group = PanelGroup();
    const int kindId = static_cast<int>(kind);
    for (auto& tile : g_panelTiles) {
        if (tile->kind != kindId || tile->group != group || tile->radius != radius) continue;
        if (!EqualRect(&tile->device, &device) || !EqualRect(&tile->padded, &padded)) continue;
        tile->lastUse = ++g_panelUseTick;
        ++g_perf.panelBlits;
        if (Intersects(tile->padded, g_paintDirty)) {
            BitBlt(g_memDC, tile->padded.left, tile->padded.top, Width(tile->padded), Height(tile->padded),
                   tile->surface.dc, 0, 0, SRCCOPY);
        }
        return true;
    }

    // The tile bakes the pixels underneath it, so only build it when this paint
    // has just refreshed the background for the whole padded area.
    if (!g_fullPaint && !ContainsRect(g_paintDirty, padded)) return false;

    const size_t bytes = static_cast<size_t>(Width(padded)) * static_cast<size_t>(Height(padded)) * 4u;
    EvictPanelTiles(bytes);
    auto tile = std::make_unique<PanelTile>();
    if (!tile->surface.Create(Width(padded), Height(padded))) return false;
    BitBlt(tile->surface.dc, 0, 0, Width(padded), Height(padded), g_memDC, padded.left, padded.top, SRCCOPY);
    {
        Graphics tileGraphics(tile->surface.dc);
        tileGraphics.SetSmoothingMode(SmoothingModeAntiAlias);
        tileGraphics.SetPixelOffsetMode(PixelOffsetModeHalf);
        tileGraphics.SetCompositingQuality(CompositingQualityHighQuality);
        tileGraphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
        Matrix matrix(g_scale, 0.0f, 0.0f, g_scale,
                      static_cast<REAL>(-padded.left), static_cast<REAL>(-padded.top));
        tileGraphics.SetTransform(&matrix);
        RenderChrome(tileGraphics, kind, rect, radius);
    }
    BitBlt(g_memDC, padded.left, padded.top, Width(padded), Height(padded), tile->surface.dc, 0, 0, SRCCOPY);
    tile->device = device;
    tile->padded = padded;
    tile->radius = radius;
    tile->group = group;
    tile->kind = kindId;
    tile->lastUse = ++g_panelUseTick;
    ++g_perf.panelBuilds;
    g_panelTileBytes += bytes;
    g_panelTiles.push_back(std::move(tile));
    return true;
}

bool DrawCachedGlassPanel(Graphics& graphics, const RECT& rect, float radius) {
    return CompositeCachedChrome(graphics, rect, radius, ChromeKind::Glass, 9.0f);
}

bool DrawCachedOutputPanel(Graphics& graphics, const RECT& rect) {
    return CompositeCachedChrome(graphics, rect, 12.0f, ChromeKind::OutputPanel, 3.0f);
}

void DrawGlassPanel(Graphics& graphics, const RECT& rect, float radius, bool hovered = false, bool selected = false) {
    if (Width(rect) <= 2 || Height(rect) <= 2) return;
    ++g_perf.glassCalls;
    if (!hovered && !selected && DrawCachedGlassPanel(graphics, rect, radius)) return;
    RenderGlassPanel(graphics, rect, radius, hovered, selected);
}

void DrawVectorIcon(Graphics& graphics, Icon icon, const RectF& rect, const Color& color, float stroke = 1.65f) {
    if (icon == Icon::None) return;
    if (g_painting) {
        const RECT bounds{static_cast<LONG>(std::floor(rect.X)), static_cast<LONG>(std::floor(rect.Y)),
                          static_cast<LONG>(std::ceil(rect.X + rect.Width)), static_cast<LONG>(std::ceil(rect.Y + rect.Height))};
        if (!Visible(bounds)) return;
    }
    Pen pen(color, stroke);
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);
    pen.SetLineJoin(LineJoinRound);
    SolidBrush brush(color);
    const float x = rect.X, y = rect.Y, w = rect.Width, h = rect.Height;
    const float cx = x + w * 0.5f, cy = y + h * 0.5f;

    switch (icon) {
    case Icon::Dashboard: {
        const float gap = w * 0.13f;
        const float qw = (w - gap) * 0.5f;
        const float qh = (h - gap) * 0.5f;
        for (int i = 0; i < 4; ++i) {
            RECT q = MakeRect(static_cast<int>(x + (i % 2) * (qw + gap)), static_cast<int>(y + (i / 2) * (qh + gap)),
                              static_cast<int>(qw), static_cast<int>(qh));
            GraphicsPath path;
            RectRoundedPath(path, q, 1.6f);
            graphics.DrawPath(&pen, &path);
        }
        break;
    }
    case Icon::Modules: {
        for (int i = 0; i < 3; ++i) {
            const float yy = y + i * (h * 0.34f);
            GraphicsPath path;
            AppendRoundedPath(path, RectF(x + w * 0.08f, yy, w * 0.84f, h * 0.22f), 1.4f);
            graphics.DrawPath(&pen, &path);
            graphics.FillEllipse(&brush, x + w * 0.16f, yy + h * 0.07f, w * 0.08f, w * 0.08f);
        }
        break;
    }
    case Icon::Cpu: {
        const RectF body(x + w * 0.23f, y + h * 0.23f, w * 0.54f, h * 0.54f);
        GraphicsPath path;
        AppendRoundedPath(path, body, 2.0f);
        graphics.DrawPath(&pen, &path);
        GraphicsPath inner;
        AppendRoundedPath(inner, RectF(x + w * 0.38f, y + h * 0.38f, w * 0.24f, h * 0.24f), 1.0f);
        graphics.DrawPath(&pen, &inner);
        for (int i = 0; i < 3; ++i) {
            const float px = x + w * (0.34f + i * 0.16f);
            const float py = y + h * (0.34f + i * 0.16f);
            graphics.DrawLine(&pen, px, y + h * 0.04f, px, y + h * 0.22f);
            graphics.DrawLine(&pen, px, y + h * 0.78f, px, y + h * 0.96f);
            graphics.DrawLine(&pen, x + w * 0.04f, py, x + w * 0.22f, py);
            graphics.DrawLine(&pen, x + w * 0.78f, py, x + w * 0.96f, py);
        }
        break;
    }
    case Icon::Disk: {
        graphics.DrawEllipse(&pen, x + w * 0.12f, y + h * 0.1f, w * 0.76f, h * 0.25f);
        graphics.DrawLine(&pen, x + w * 0.12f, y + h * 0.22f, x + w * 0.12f, y + h * 0.78f);
        graphics.DrawLine(&pen, x + w * 0.88f, y + h * 0.22f, x + w * 0.88f, y + h * 0.78f);
        graphics.DrawArc(&pen, x + w * 0.12f, y + h * 0.66f, w * 0.76f, h * 0.25f, 0.0f, 180.0f);
        graphics.DrawEllipse(&pen, x + w * 0.12f, y + h * 0.4f, w * 0.76f, h * 0.25f);
        break;
    }
    case Icon::Network: {
        const PointF top(cx, y + h * 0.18f);
        const PointF left(x + w * 0.18f, y + h * 0.82f);
        const PointF right(x + w * 0.82f, y + h * 0.82f);
        graphics.DrawLine(&pen, top.X, top.Y, left.X, left.Y);
        graphics.DrawLine(&pen, top.X, top.Y, right.X, right.Y);
        graphics.DrawLine(&pen, left.X, left.Y, right.X, right.Y);
        graphics.DrawEllipse(&pen, top.X - w * 0.12f, top.Y - w * 0.12f, w * 0.24f, w * 0.24f);
        graphics.DrawEllipse(&pen, left.X - w * 0.11f, left.Y - w * 0.11f, w * 0.22f, w * 0.22f);
        graphics.DrawEllipse(&pen, right.X - w * 0.11f, right.Y - w * 0.11f, w * 0.22f, w * 0.22f);
        break;
    }
    case Icon::Gauge: {
        graphics.DrawArc(&pen, x + w * 0.12f, y + h * 0.12f, w * 0.76f, h * 0.76f, 150.0f, 240.0f);
        graphics.DrawLine(&pen, cx, cy, x + w * 0.7f, y + h * 0.3f);
        graphics.FillEllipse(&brush, cx - 2.6f, cy - 2.6f, 5.2f, 5.2f);
        break;
    }
    case Icon::Shield:
    case Icon::Admin: {
        GraphicsPath path;
        path.AddLine(cx, y + h * 0.08f, x + w * 0.86f, y + h * 0.24f);
        path.AddLine(x + w * 0.86f, y + h * 0.24f, x + w * 0.78f, y + h * 0.68f);
        path.AddLine(x + w * 0.78f, y + h * 0.68f, cx, y + h * 0.94f);
        path.AddLine(cx, y + h * 0.94f, x + w * 0.22f, y + h * 0.68f);
        path.AddLine(x + w * 0.22f, y + h * 0.68f, x + w * 0.14f, y + h * 0.24f);
        path.CloseFigure();
        graphics.DrawPath(&pen, &path);
        PointF check[3] = {PointF(x + w * 0.32f, cy), PointF(x + w * 0.46f, cy + h * 0.15f), PointF(x + w * 0.7f, y + h * 0.3f)};
        graphics.DrawLines(&pen, check, 3);
        break;
    }
    case Icon::Devices: {
        GraphicsPath monitor;
        AppendRoundedPath(monitor, RectF(x + w * 0.06f, y + h * 0.16f, w * 0.62f, h * 0.48f), 2.0f);
        graphics.DrawPath(&pen, &monitor);
        graphics.DrawLine(&pen, x + w * 0.25f, y + h * 0.72f, x + w * 0.5f, y + h * 0.72f);
        graphics.DrawLine(&pen, x + w * 0.37f, y + h * 0.64f, x + w * 0.37f, y + h * 0.72f);
        GraphicsPath phone;
        AppendRoundedPath(phone, RectF(x + w * 0.67f, y + h * 0.3f, w * 0.28f, h * 0.58f), 2.0f);
        graphics.DrawPath(&pen, &phone);
        graphics.DrawLine(&pen, x + w * 0.74f, y + h * 0.76f, x + w * 0.88f, y + h * 0.76f);
        break;
    }
    case Icon::Wrench: {
        graphics.DrawLine(&pen, x + w * 0.18f, y + h * 0.84f, x + w * 0.64f, y + h * 0.38f);
        graphics.DrawArc(&pen, x + w * 0.46f, y + h * 0.04f, w * 0.5f, h * 0.5f, 40.0f, 270.0f);
        graphics.FillEllipse(&brush, x + w * 0.1f, y + h * 0.76f, w * 0.16f, h * 0.16f);
        break;
    }
    case Icon::Report: {
        GraphicsPath page;
        page.AddLine(x + w * 0.2f, y + h * 0.08f, x + w * 0.62f, y + h * 0.08f);
        page.AddLine(x + w * 0.62f, y + h * 0.08f, x + w * 0.82f, y + h * 0.28f);
        page.AddLine(x + w * 0.82f, y + h * 0.28f, x + w * 0.82f, y + h * 0.92f);
        page.AddLine(x + w * 0.82f, y + h * 0.92f, x + w * 0.2f, y + h * 0.92f);
        page.CloseFigure();
        graphics.DrawPath(&pen, &page);
        graphics.DrawLine(&pen, x + w * 0.32f, y + h * 0.44f, x + w * 0.7f, y + h * 0.44f);
        graphics.DrawLine(&pen, x + w * 0.32f, y + h * 0.58f, x + w * 0.7f, y + h * 0.58f);
        graphics.DrawLine(&pen, x + w * 0.32f, y + h * 0.72f, x + w * 0.58f, y + h * 0.72f);
        break;
    }
    case Icon::Settings: {
        for (int i = 0; i < 3; ++i) {
            const float yy = y + h * (0.22f + i * 0.28f);
            graphics.DrawLine(&pen, x + w * 0.1f, yy, x + w * 0.9f, yy);
            const float kx = x + w * (i == 0 ? 0.65f : (i == 1 ? 0.36f : 0.54f));
            graphics.FillEllipse(&brush, kx - 2.7f, yy - 2.7f, 5.4f, 5.4f);
        }
        break;
    }
    case Icon::System: {
        GraphicsPath monitor;
        AppendRoundedPath(monitor, RectF(x + w * 0.08f, y + h * 0.12f, w * 0.84f, h * 0.58f), 2.0f);
        graphics.DrawPath(&pen, &monitor);
        graphics.DrawLine(&pen, x + w * 0.34f, y + h * 0.86f, x + w * 0.66f, y + h * 0.86f);
        graphics.DrawLine(&pen, x + w * 0.5f, y + h * 0.7f, x + w * 0.5f, y + h * 0.86f);
        break;
    }
    case Icon::Memory: {
        GraphicsPath body;
        AppendRoundedPath(body, RectF(x + w * 0.14f, y + h * 0.27f, w * 0.72f, h * 0.46f), 1.5f);
        graphics.DrawPath(&pen, &body);
        for (int i = 0; i < 4; ++i) {
            const float px = x + w * (0.25f + i * 0.16f);
            graphics.DrawLine(&pen, px, y + h * 0.15f, px, y + h * 0.27f);
            graphics.DrawLine(&pen, px, y + h * 0.73f, px, y + h * 0.85f);
        }
        break;
    }
    case Icon::Gpu: {
        GraphicsPath body;
        AppendRoundedPath(body, RectF(x + w * 0.1f, y + h * 0.22f, w * 0.8f, h * 0.46f), 2.0f);
        graphics.DrawPath(&pen, &body);
        graphics.DrawEllipse(&pen, x + w * 0.32f, y + h * 0.3f, w * 0.34f, h * 0.3f);
        graphics.FillEllipse(&brush, cx - 2.2f, cy - 2.2f, 4.4f, 4.4f);
        graphics.DrawLine(&pen, x + w * 0.2f, y + h * 0.76f, x + w * 0.36f, y + h * 0.76f);
        graphics.DrawLine(&pen, x + w * 0.64f, y + h * 0.76f, x + w * 0.8f, y + h * 0.76f);
        break;
    }
    case Icon::Search: {
        graphics.DrawEllipse(&pen, x + w * 0.12f, y + h * 0.12f, w * 0.5f, h * 0.5f);
        graphics.DrawLine(&pen, x + w * 0.55f, y + h * 0.55f, x + w * 0.88f, y + h * 0.88f);
        break;
    }
    case Icon::Play: {
        GraphicsPath triangle;
        triangle.AddLine(x + w * 0.3f, y + h * 0.16f, x + w * 0.82f, cy);
        triangle.AddLine(x + w * 0.82f, cy, x + w * 0.3f, y + h * 0.84f);
        triangle.CloseFigure();
        graphics.FillPath(&brush, &triangle);
        break;
    }
    case Icon::Download: {
        graphics.DrawLine(&pen, cx, y + h * 0.1f, cx, y + h * 0.62f);
        graphics.DrawLine(&pen, cx, y + h * 0.62f, x + w * 0.3f, y + h * 0.4f);
        graphics.DrawLine(&pen, cx, y + h * 0.62f, x + w * 0.7f, y + h * 0.4f);
        graphics.DrawLine(&pen, x + w * 0.16f, y + h * 0.8f, x + w * 0.84f, y + h * 0.8f);
        break;
    }
    case Icon::Folder: {
        GraphicsPath folder;
        folder.AddLine(x + w * 0.1f, y + h * 0.28f, x + w * 0.42f, y + h * 0.28f);
        folder.AddLine(x + w * 0.42f, y + h * 0.28f, x + w * 0.5f, y + h * 0.4f);
        folder.AddLine(x + w * 0.5f, y + h * 0.4f, x + w * 0.9f, y + h * 0.4f);
        folder.AddLine(x + w * 0.9f, y + h * 0.4f, x + w * 0.9f, y + h * 0.84f);
        folder.AddLine(x + w * 0.9f, y + h * 0.84f, x + w * 0.1f, y + h * 0.84f);
        folder.CloseFigure();
        graphics.DrawPath(&pen, &folder);
        break;
    }
    case Icon::Check: {
        PointF points[3] = {PointF(x + w * 0.16f, cy), PointF(x + w * 0.42f, y + h * 0.72f), PointF(x + w * 0.84f, y + h * 0.26f)};
        graphics.DrawLines(&pen, points, 3);
        break;
    }
    case Icon::Warning: {
        GraphicsPath triangle;
        triangle.AddLine(cx, y + h * 0.1f, x + w * 0.9f, y + h * 0.88f);
        triangle.AddLine(x + w * 0.9f, y + h * 0.88f, x + w * 0.1f, y + h * 0.88f);
        triangle.CloseFigure();
        graphics.DrawPath(&pen, &triangle);
        graphics.DrawLine(&pen, cx, y + h * 0.4f, cx, y + h * 0.62f);
        graphics.FillEllipse(&brush, cx - 1.4f, y + h * 0.71f, 2.8f, 2.8f);
        break;
    }
    case Icon::Refresh: {
        graphics.DrawArc(&pen, x + w * 0.14f, y + h * 0.14f, w * 0.72f, h * 0.72f, 40.0f, 280.0f);
        graphics.DrawLine(&pen, x + w * 0.72f, y + h * 0.15f, x + w * 0.86f, y + h * 0.15f);
        graphics.DrawLine(&pen, x + w * 0.86f, y + h * 0.15f, x + w * 0.86f, y + h * 0.31f);
        break;
    }
    case Icon::Close: {
        graphics.DrawLine(&pen, x + w * 0.22f, y + h * 0.22f, x + w * 0.78f, y + h * 0.78f);
        graphics.DrawLine(&pen, x + w * 0.78f, y + h * 0.22f, x + w * 0.22f, y + h * 0.78f);
        break;
    }
    case Icon::Info: {
        graphics.DrawEllipse(&pen, x + w * 0.12f, y + h * 0.12f, w * 0.76f, h * 0.76f);
        graphics.DrawLine(&pen, cx, y + h * 0.46f, cx, y + h * 0.7f);
        graphics.FillEllipse(&brush, cx - 1.3f, y + h * 0.31f, 2.6f, 2.6f);
        break;
    }
    case Icon::Clock: {
        graphics.DrawEllipse(&pen, x + w * 0.12f, y + h * 0.12f, w * 0.76f, h * 0.76f);
        graphics.DrawLine(&pen, cx, cy, cx, y + h * 0.28f);
        graphics.DrawLine(&pen, cx, cy, x + w * 0.68f, cy + h * 0.12f);
        break;
    }
    case Icon::Trash: {
        graphics.DrawLine(&pen, x + w * 0.2f, y + h * 0.24f, x + w * 0.8f, y + h * 0.24f);
        graphics.DrawLine(&pen, x + w * 0.36f, y + h * 0.15f, x + w * 0.64f, y + h * 0.15f);
        GraphicsPath can;
        AppendRoundedPath(can, RectF(x + w * 0.25f, y + h * 0.32f, w * 0.5f, h * 0.52f), 2.0f);
        graphics.DrawPath(&pen, &can);
        graphics.DrawLine(&pen, x + w * 0.42f, y + h * 0.44f, x + w * 0.42f, y + h * 0.72f);
        graphics.DrawLine(&pen, x + w * 0.58f, y + h * 0.44f, x + w * 0.58f, y + h * 0.72f);
        break;
    }
    case Icon::Lock: {
        GraphicsPath body;
        AppendRoundedPath(body, RectF(x + w * 0.2f, y + h * 0.45f, w * 0.6f, h * 0.43f), 2.0f);
        graphics.DrawPath(&pen, &body);
        graphics.DrawArc(&pen, x + w * 0.3f, y + h * 0.12f, w * 0.4f, h * 0.46f, 180.0f, 180.0f);
        break;
    }
    case Icon::Sparkle: {
        GraphicsPath star;
        star.AddLine(cx, y + h * 0.06f, cx + w * 0.08f, y + h * 0.42f);
        star.AddLine(cx + w * 0.08f, y + h * 0.42f, x + w * 0.94f, cy);
        star.AddLine(x + w * 0.94f, cy, cx + w * 0.08f, y + h * 0.58f);
        star.AddLine(cx + w * 0.08f, y + h * 0.58f, cx, y + h * 0.94f);
        star.AddLine(cx, y + h * 0.94f, cx - w * 0.08f, y + h * 0.58f);
        star.AddLine(cx - w * 0.08f, y + h * 0.58f, x + w * 0.06f, cy);
        star.AddLine(x + w * 0.06f, cy, cx - w * 0.08f, y + h * 0.42f);
        star.CloseFigure();
        graphics.FillPath(&brush, &star);
        break;
    }
    case Icon::Chevron: {
        graphics.DrawLine(&pen, x + w * 0.32f, y + h * 0.22f, x + w * 0.68f, cy);
        graphics.DrawLine(&pen, x + w * 0.68f, cy, x + w * 0.32f, y + h * 0.78f);
        break;
    }
    default: break;
    }
}

void DrawIconBadge(Graphics& graphics, const RECT& rect, Icon icon, const Color& color, float iconPadding = 9.0f) {
    FillRounded(graphics, rect, static_cast<float>(Width(rect)) * 0.32f, WithAlpha(color, 30));
    StrokeRounded(graphics, rect, static_cast<float>(Width(rect)) * 0.32f, WithAlpha(color, 56), 1.0f);
    DrawVectorIcon(graphics, icon,
        RectF(static_cast<REAL>(rect.left) + iconPadding, static_cast<REAL>(rect.top) + iconPadding,
              static_cast<REAL>(Width(rect)) - iconPadding * 2, static_cast<REAL>(Height(rect)) - iconPadding * 2),
        color, 1.55f);
}

void DrawButton(Graphics& graphics, const RECT& rect, const std::wstring& label, Icon icon, bool primary,
                bool hovered, bool disabled, bool danger = false, float fontSize = 10.5f) {
    if (Width(rect) <= 2 || Height(rect) <= 2) return;
    Color top, bottom, textColor;
    if (disabled) {
        top = Color(30, 92, 108, 118);
        bottom = Color(20, 72, 86, 94);
        textColor = WithAlpha(C_MUTED, 110);
    } else if (danger) {
        top = Color(72, 200, 80, 92);
        bottom = Color(46, 150, 58, 68);
        textColor = Color(255, 245, 216, 220);
        if (hovered) { top = WithAlpha(top, 96); bottom = WithAlpha(bottom, 72); }
    } else if (primary) {
        top = hovered ? Color(255, 170, 226, 252) : C_LIGHT_BLUE;
        bottom = hovered ? C_LIGHT_BLUE : Color(255, 106, 180, 222);
        textColor = C_BUTTON_TEXT;
    } else {
        top = hovered ? Color(64, 55, 71, 79) : Color(52, 48, 60, 66);
        bottom = hovered ? Color(48, 44, 56, 62) : Color(38, 40, 51, 57);
        textColor = C_TEXT;
    }
    const RectF box(static_cast<REAL>(rect.left), static_cast<REAL>(rect.top), static_cast<REAL>(Width(rect)), static_cast<REAL>(Height(rect)));
    GraphicsPath path;
    AppendRoundedPath(path, box, static_cast<float>(Height(rect)) * 0.34f);
    LinearGradientBrush fill(box, top, bottom, LinearGradientModeVertical);
    graphics.FillPath(&fill, &path);
    if (!primary && !danger) {
        Pen border(hovered ? WithAlpha(C_LIGHT_BLUE, 112) : C_LINE, 1.0f);
        graphics.DrawPath(&border, &path);
    } else if (primary) {
        Pen border(WithAlpha(C_ICE_WHITE, 104), 1.0f);
        graphics.DrawPath(&border, &path);
    } else {
        Pen border(WithAlpha(C_RED, 96), 1.0f);
        graphics.DrawPath(&border, &path);
    }

    Font* font = GetFont(fontSize, true);
    if (!font) return;
    const float textWidth = MeasureText(graphics, label, fontSize, true);
    const float iconSize = (icon == Icon::None) ? 0.0f : fontSize + 5.0f;
    const float iconGap = (icon == Icon::None) ? 0.0f : 7.0f;
    const float total = textWidth + iconSize + iconGap;
    float startX = static_cast<float>(rect.left) + (static_cast<float>(Width(rect)) - total) * 0.5f;
    if (icon != Icon::None) {
        DrawVectorIcon(graphics, icon,
            RectF(startX, static_cast<float>(rect.top) + (static_cast<float>(Height(rect)) - iconSize) * 0.5f, iconSize, iconSize),
            textColor, 1.5f);
        startX += iconSize + iconGap;
    }
    SolidBrush brush(textColor);
    RectF textBox(startX, static_cast<REAL>(rect.top), textWidth + 12.0f, static_cast<REAL>(Height(rect)));
    graphics.DrawString(label.c_str(), static_cast<INT>(label.size()), font, textBox, GetStringFormat(StringAlignmentNear, false), &brush);
}

void DrawChip(Graphics& graphics, const RECT& rect, const std::wstring& text, bool active, bool hovered, float fontSize = 9.0f) {
    const Color fill = active ? C_LIGHT_BLUE : (hovered ? Color(40, 55, 71, 79) : Color(28, 38, 50, 56));
    FillRounded(graphics, rect, static_cast<float>(Height(rect)) * 0.5f, fill);
    StrokeRounded(graphics, rect, static_cast<float>(Height(rect)) * 0.5f, active ? WithAlpha(C_ICE_WHITE, 140) : C_LINE, 1.0f);
    DrawLabel(graphics, text, rect, fontSize, active ? C_BUTTON_TEXT : (hovered ? C_INK : C_TEXT), true, StringAlignmentCenter);
}

void DrawStatusPill(Graphics& graphics, const RECT& rect, const std::wstring& text, const Color& color, bool spinner) {
    FillRounded(graphics, rect, static_cast<float>(Height(rect)) * 0.5f, WithAlpha(color, 26));
    StrokeRounded(graphics, rect, static_cast<float>(Height(rect)) * 0.5f, WithAlpha(color, 72), 1.0f);
    if (spinner) {
        RectF arc(static_cast<REAL>(rect.left + 10), static_cast<REAL>(rect.top) + (static_cast<REAL>(Height(rect)) - 12.0f) * 0.5f, 12.0f, 12.0f);
        Pen pen(color, 2.0f);
        pen.SetStartCap(LineCapRound);
        pen.SetEndCap(LineCapRound);
        graphics.DrawArc(&pen, arc, static_cast<REAL>(g.spinnerAngle), 245.0f);
    } else {
        SolidBrush dot(color);
        graphics.FillEllipse(&dot, static_cast<REAL>(rect.left + 13), static_cast<REAL>(rect.top) + static_cast<REAL>(Height(rect)) * 0.5f - 3.0f, 6.0f, 6.0f);
    }
    DrawLabel(graphics, text, MakeRect(rect.left + 26, rect.top, Width(rect) - 34, Height(rect)), 9.0f, color, true);
}

void DrawToggleSwitch(Graphics& graphics, const RECT& rect, bool on, bool hovered) {
    FillRounded(graphics, rect, static_cast<float>(Height(rect)) * 0.5f, on ? C_LIGHT_BLUE : Color(42, 55, 63, 70));
    StrokeRounded(graphics, rect, static_cast<float>(Height(rect)) * 0.5f, on ? WithAlpha(C_MINT, 100) : WithAlpha(C_MUTED, 52), 1.0f);
    const int knob = Height(rect) - 8;
    const int x = on ? rect.right - knob - 4 : rect.left + 4;
    SolidBrush shadow(Color(60, 0, 0, 0));
    graphics.FillEllipse(&shadow, static_cast<REAL>(x + 1), static_cast<REAL>(rect.top + 5), static_cast<REAL>(knob), static_cast<REAL>(knob));
    SolidBrush knobBrush(on ? C_BUTTON_TEXT : Color(255, 225, 238, 230));
    graphics.FillEllipse(&knobBrush, static_cast<REAL>(x), static_cast<REAL>(rect.top + 4), static_cast<REAL>(knob), static_cast<REAL>(knob));
    if (hovered) {
        Pen glow(WithAlpha(on ? C_MINT : C_TEXT, 60), 1.0f);
        graphics.DrawEllipse(&glow, static_cast<REAL>(x), static_cast<REAL>(rect.top + 4), static_cast<REAL>(knob), static_cast<REAL>(knob));
    }
}

void DrawBar(Graphics& graphics, const RECT& rect, float percent, const Color& color) {
    if (Width(rect) <= 2) return;
    PerfSpan chartSpan(&g_perf.sectionCharts);
    FillRounded(graphics, rect, static_cast<float>(Height(rect)) * 0.5f, Color(42, 58, 66, 72));
    const float clamped = std::clamp(percent, 0.0f, 100.0f);
    if (clamped <= 0.1f) return;
    const int fillWidth = std::max(4, static_cast<int>(Width(rect) * clamped / 100.0f));
    RECT fillRect{rect.left, rect.top, rect.left + fillWidth, rect.bottom};
    FillRounded(graphics, fillRect, static_cast<float>(Height(rect)) * 0.5f, color);
}

Color UsageColor(float percent) {
    if (percent >= 90.0f) return C_RED;
    if (percent >= 72.0f) return C_AMBER;
    return C_MINT;
}

void DrawRing(Graphics& graphics, const RECT& rect, float percent, const Color& color, const std::wstring& center, const std::wstring& sub) {
    PerfSpan chartSpan(&g_perf.sectionCharts);
    const float thickness = std::max(5.0f, static_cast<float>(Width(rect)) * 0.08f);
    RectF arc(static_cast<REAL>(rect.left) + thickness * 0.5f, static_cast<REAL>(rect.top) + thickness * 0.5f,
              static_cast<REAL>(Width(rect)) - thickness, static_cast<REAL>(Height(rect)) - thickness);
    Pen track(Color(46, 62, 70, 76), thickness);
    track.SetStartCap(LineCapRound);
    track.SetEndCap(LineCapRound);
    graphics.DrawArc(&track, arc, -90.0f, 359.0f);
    const float clamped = std::clamp(percent, 0.0f, 100.0f);
    if (clamped > 0.2f) {
        Pen value(UsageColor(clamped), thickness);
        value.SetStartCap(LineCapRound);
        value.SetEndCap(LineCapRound);
        graphics.DrawArc(&value, arc, -90.0f, 359.0f * clamped / 100.0f);
    }
    const std::wstring valueText = center;
    DrawLabel(graphics, valueText, MakeRect(rect.left, rect.top + 7, Width(rect), Height(rect) / 2), 18.0f, C_INK, true, StringAlignmentCenter);
    if (!sub.empty()) DrawLabel(graphics, sub, MakeRect(rect.left, rect.top + Height(rect) / 2 + 7, Width(rect), 16), 8.0f, C_MUTED, true, StringAlignmentCenter);
}

void DrawSparkline(Graphics& graphics, const RECT& rect, const std::deque<float>& values, const Color& color) {
    if (Width(rect) < 20 || Height(rect) < 20) return;
    PerfSpan chartSpan(&g_perf.sectionCharts);
    Pen grid(Color(26, 234, 246, 252), 1.0f);
    for (int i = 1; i <= 3; ++i) {
        const int y = rect.top + Height(rect) * i / 4;
        graphics.DrawLine(&grid, rect.left, y, rect.right, y);
    }
    if (values.size() < 2) {
        DrawLabel(graphics, L"Zbieranie danych…", rect, 9.5f, C_DIM, false, StringAlignmentCenter);
        return;
    }
    const int count = static_cast<int>(values.size());
    GraphicsPath line;
    GraphicsPath area;
    const bool areaStarted = true;
    area.AddLine(static_cast<REAL>(rect.left), static_cast<REAL>(rect.bottom), static_cast<REAL>(rect.left), static_cast<REAL>(rect.bottom));
    (void)areaStarted;
    for (int i = 0; i < count; ++i) {
        const float px = static_cast<float>(rect.left) + (static_cast<float>(rect.right - rect.left - 2) * static_cast<float>(i) / static_cast<float>(std::max(1, count - 1))) + 1.0f;
        const float value = std::clamp(values[static_cast<size_t>(i)], 0.0f, 100.0f);
        const float py = static_cast<float>(rect.bottom - 3) - (value / 100.0f) * static_cast<float>(rect.bottom - rect.top - 8);
        if (i == 0) { line.AddLine(px, py, px, py); area.AddLine(px, py, px, py); }
        else { line.AddLine(px, py, px, py); area.AddLine(px, py, px, py); }
    }
    GraphicsPath fillPath;
    fillPath.AddPath(&area, TRUE);
    fillPath.AddLine(static_cast<REAL>(rect.right - 1), static_cast<REAL>(rect.bottom), static_cast<REAL>(rect.left), static_cast<REAL>(rect.bottom));
    fillPath.CloseFigure();
    LinearGradientBrush fill(RectF(static_cast<REAL>(rect.left), static_cast<REAL>(rect.top), static_cast<REAL>(Width(rect)), static_cast<REAL>(Height(rect))),
                             WithAlpha(color, 76), WithAlpha(color, 2), LinearGradientModeVertical);
    graphics.FillPath(&fill, &fillPath);
    Pen pen(color, 1.8f);
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);
    pen.SetLineJoin(LineJoinRound);
    graphics.DrawPath(&pen, &line);
    const float lastValue = std::clamp(values.back(), 0.0f, 100.0f);
    const float lastX = static_cast<float>(rect.right - 1);
    const float lastY = static_cast<float>(rect.bottom - 3) - (lastValue / 100.0f) * static_cast<float>(rect.bottom - rect.top - 8);
    SolidBrush glow(WithAlpha(color, 55));
    graphics.FillEllipse(&glow, lastX - 5.0f, lastY - 5.0f, 10.0f, 10.0f);
    SolidBrush dot(color);
    graphics.FillEllipse(&dot, lastX - 2.2f, lastY - 2.2f, 4.4f, 4.4f);
}

void DrawScrollbar(Graphics& graphics, const RECT& area, int firstVisible, int total, int visibleCount) {
    if (total <= visibleCount || visibleCount <= 0) return;
    RECT track{area.right - 4, area.top + 4, area.right, area.bottom - 4};
    FillRounded(graphics, track, 2.0f, Color(30, 40, 48, 54));
    const float ratio = static_cast<float>(visibleCount) / static_cast<float>(total);
    const int thumbHeight = std::max(24, static_cast<int>(Height(track) * ratio));
    const int maxFirst = std::max(1, total - visibleCount);
    const int offset = static_cast<int>((Height(track) - thumbHeight) * (static_cast<float>(firstVisible) / static_cast<float>(maxFirst)));
    RECT thumb{track.left, track.top + offset, track.right, track.top + offset + thumbHeight};
    FillRounded(graphics, thumb, 2.0f, WithAlpha(C_LIGHT_BLUE, 112));
}

std::wstring RunStateText(RunState state) {
    switch (state) {
    case RunState::Running: return L"W toku";
    case RunState::Ok: return L"Zakończono";
    case RunState::Error: return L"Błąd";
    case RunState::Cancelled: return L"Anulowano";
    case RunState::Timeout: return L"Limit czasu";
    default: return L"Gotowy";
    }
}

Color RunStateColor(RunState state) {
    switch (state) {
    case RunState::Running: return C_AMBER;
    case RunState::Ok: return C_GREEN;
    case RunState::Error: return C_RED;
    case RunState::Cancelled: return C_AMBER;
    case RunState::Timeout: return C_AMBER;
    default: return C_MUTED;
    }
}

RunState StateFromOutcome(const ModuleOutcome& outcome) {
    if (outcome.cancelled) return RunState::Cancelled;
    if (outcome.timedOut) return RunState::Timeout;
    if (outcome.spawnFailed || outcome.exitCode != 0) return RunState::Error;
    return RunState::Ok;
}

std::wstring ReportStatus(const Report& report) {
    if (report.cancelled) return L"Anulowano";
    if (report.timedOut) return L"Limit czasu";
    if (report.exitCode != 0) return L"Błąd";
    return L"Zakończono";
}

Color ReportStatusColor(const Report& report) {
    if (report.cancelled) return C_AMBER;
    if (report.timedOut) return C_AMBER;
    if (report.exitCode != 0) return C_RED;
    return C_GREEN;
}

std::wstring BuildReportText(const Report& report) {
    const Module* module = FindModuleById(report.moduleId);
    std::wostringstream out;
    out << L"KD CORE " << kAppVersion << L" — RAPORT MODUŁU\r\n";
    out << L"================================================================\r\n\r\n";
    out << L"Moduł:      " << report.moduleName << L"\r\n";
    out << L"Kategoria:  " << report.category << L"\r\n";
    out << L"ID modułu:  " << report.moduleId << L"\r\n";
    if (module) {
        out << L"Opis:       " << module->description << L"\r\n";
        out << L"Polecenie:  " << (module->command && *module->command ? module->command : L"(moduł natywny)") << L"\r\n";
        out << L"Uprawnienia: " << (module->requiresAdmin ? L"wymagany administrator" : L"standardowe") << L"\r\n";
    }
    out << L"Start:      " << report.startedAt << L"\r\n";
    out << L"Czas:       " << ReportDurationText(report) << L"\r\n";
    out << L"Kod wyjścia: " << report.exitCode << L"\r\n";
    out << L"Status:     " << ReportStatus(report) << L"\r\n";
    if (!report.file.empty()) out << L"Plik:       " << report.file << L"\r\n";
    out << L"\r\n----------------------------------------------------------------\r\n";
    out << L"WYNIK\r\n";
    out << L"----------------------------------------------------------------\r\n\r\n";
    out << (report.output.empty() ? L"Brak danych wyjściowych.\r\n" : report.output);
    std::wstring text = out.str();
    if (text.size() < 2 || text.compare(text.size() - 2, 2, L"\r\n") != 0) text += L"\r\n";
    return text;
}

bool EnsureReportsFolder() {
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(g.reportsDir), ec);
    return std::filesystem::exists(std::filesystem::path(g.reportsDir));
}

std::wstring SaveReportFile(const Report& report) {
    if (!EnsureReportsFolder()) return L"";
    const std::wstring fileName = FileStamp() + L"_" + SanitizeFileName(report.moduleName) + L".txt";
    const std::filesystem::path path = std::filesystem::path(g.reportsDir) / fileName;
    if (!WriteTextFileUtf8(path, BuildReportText(report))) return L"";
    return path.wstring();
}

void ExportReportDialog(const Report& report) {
    wchar_t path[MAX_PATH * 2]{};
    const std::wstring suggested = L"Raport-KDCore-" + FileStamp() + L".txt";
    lstrcpynW(path, suggested.c_str(), static_cast<int>(std::size(path)));
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = g.hwnd;
    dialog.lpstrFilter = L"Raport tekstowy (*.txt)\0*.txt\0Wszystkie pliki (*.*)\0*.*\0\0";
    dialog.lpstrFile = path;
    dialog.nMaxFile = static_cast<DWORD>(std::size(path));
    dialog.lpstrDefExt = L"txt";
    dialog.lpstrInitialDir = g.reportsDir.c_str();
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (!GetSaveFileNameW(&dialog)) return;
    if (WriteTextFileUtf8(path, BuildReportText(report))) {
        g.status = L"RAPORT ZAPISANY";
        MessageBoxW(g.hwnd, (L"Raport został zapisany:\r\n\r\n" + std::wstring(path)).c_str(), L"Eksport raportu", MB_OK | MB_ICONINFORMATION);
        InvalidateAll();
    } else {
        MessageBoxW(g.hwnd, L"Nie udało się zapisać pliku raportu.", L"Błąd eksportu", MB_OK | MB_ICONERROR);
    }
}

void OpenReportsFolder() {
    EnsureReportsFolder();
    HINSTANCE result = ShellExecuteW(g.hwnd, L"open", g.reportsDir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        MessageBoxW(g.hwnd, L"Nie udało się otworzyć folderu raportów.", L"Folder raportów", MB_OK | MB_ICONERROR);
    }
}

std::wstring DecodeConsoleBytes(const std::string& bytes) {
    if (bytes.empty()) return L"";
    std::string data = bytes;
    if (data.size() >= 3 && static_cast<unsigned char>(data[0]) == 0xEF && static_cast<unsigned char>(data[1]) == 0xBB && static_cast<unsigned char>(data[2]) == 0xBF) data.erase(0, 3);
    int chars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, data.data(), static_cast<int>(data.size()), nullptr, 0);
    UINT codePage = CP_UTF8;
    if (chars <= 0) {
        codePage = CP_OEMCP;
        chars = MultiByteToWideChar(codePage, 0, data.data(), static_cast<int>(data.size()), nullptr, 0);
    }
    if (chars <= 0) return L"Nie udało się zdekodować tekstu wyjściowego.\r\n";
    std::wstring result(static_cast<size_t>(chars), L'\0');
    MultiByteToWideChar(codePage, 0, data.data(), static_cast<int>(data.size()), result.data(), chars);
    return NormalizeNewlines(result);
}

ExecResult ExecuteCommand(const std::wstring& command) {
    ExecResult result;
    const auto started = std::chrono::steady_clock::now();
    auto finish = [&]() {
        result.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    };
    const std::wstring trimmed = Trim(command);
    if (trimmed.empty()) {
        result.output = L"Moduł nie ma przypisanego polecenia systemowego.\r\n";
        result.spawnFailed = true;
        finish();
        return result;
    }

    if (StartsWithI(trimmed, L"start ") || StartsWithI(trimmed, L"explorer ")) {
        std::wstring target = Trim(trimmed.substr(trimmed.find(L' ') + 1));
        if (target.size() >= 2 && target.front() == L'"' && target.back() == L'"') target = target.substr(1, target.size() - 2);
        HINSTANCE launched = ShellExecuteW(nullptr, L"open", target.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        if (reinterpret_cast<INT_PTR>(launched) > 32) {
            result.output = L"Uruchomiono narzędzie systemowe w osobnym oknie.\r\n\r\nElement: " + target + L"\r\n";
            result.exitCode = 0;
        } else {
            result.output = L"Nie udało się uruchomić narzędzia systemowego.\r\nKod ShellExecute: " + std::to_wstring(reinterpret_cast<INT_PTR>(launched)) + L"\r\n";
            result.exitCode = 1;
            result.spawnFailed = true;
        }
        finish();
        return result;
    }

    SECURITY_ATTRIBUTES security{};
    security.nLength = sizeof(security);
    security.bInheritHandle = TRUE;
    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &security, 0)) {
        result.output = L"Błąd tworzenia potoku wyjściowego: " + std::to_wstring(GetLastError()) + L"\r\n";
        result.spawnFailed = true;
        result.exitCode = 1;
        finish();
        return result;
    }
    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);
    HANDLE nullInput = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &security, OPEN_EXISTING, 0, nullptr);
    if (nullInput == INVALID_HANDLE_VALUE) nullInput = GetStdHandle(STD_INPUT_HANDLE);

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    startup.hStdInput = nullInput;
    startup.hStdOutput = writePipe;
    startup.hStdError = writePipe;

    wchar_t comspec[MAX_PATH]{};
    if (!GetEnvironmentVariableW(L"ComSpec", comspec, MAX_PATH)) lstrcpynW(comspec, L"C:\\Windows\\System32\\cmd.exe", MAX_PATH);
    std::wstring commandLine = L"\"" + std::wstring(comspec) + L"\" /D /S /C chcp 65001 >nul & " + command;
    std::vector<wchar_t> mutableCommand(commandLine.begin(), commandLine.end());
    mutableCommand.push_back(L'\0');

    PROCESS_INFORMATION process{};
    const BOOL created = CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                                        nullptr, nullptr, &startup, &process);
    CloseHandle(writePipe);
    if (nullInput && nullInput != INVALID_HANDLE_VALUE && nullInput != GetStdHandle(STD_INPUT_HANDLE)) CloseHandle(nullInput);
    if (!created) {
        CloseHandle(readPipe);
        result.output = L"Błąd uruchamiania polecenia: " + std::to_wstring(GetLastError()) + L"\r\n";
        result.exitCode = GetLastError();
        result.spawnFailed = true;
        finish();
        return result;
    }

    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    if (job) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo{};
        jobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(job, JobObjectExtendedLimitInformation, &jobInfo, sizeof(jobInfo));
        AssignProcessToJobObject(job, process.hProcess);
    }

    int timeoutSeconds = 600;
    const std::wstring lowerCommand = Lower(command);
    if (lowerCommand.find(L"sfc ") != std::wstring::npos || lowerCommand.find(L"dism ") != std::wstring::npos ||
        lowerCommand.find(L"chkdsk") != std::wstring::npos || lowerCommand.find(L"defrag") != std::wstring::npos ||
        lowerCommand.find(L"get-childitem") != std::wstring::npos || lowerCommand.find(L"get-winevent") != std::wstring::npos ||
        lowerCommand.find(L"get-storagereliabilitycounter") != std::wstring::npos) timeoutSeconds = 1800;

    std::string captured;
    captured.reserve(16384);
    bool outputTruncated = false;
    const ULONGLONG startTick = GetTickCount64();
    for (;;) {
        if (g.cancelRequested.load()) {
            result.cancelled = true;
            if (job) TerminateJobObject(job, 1);
            else TerminateProcess(process.hProcess, 1);
            WaitForSingleObject(process.hProcess, 3000);
            break;
        }
        DWORD available = 0;
        if (!PeekNamedPipe(readPipe, nullptr, 0, nullptr, &available, nullptr)) break;
        if (available > 0) {
            char buffer[8192];
            DWORD readBytes = 0;
            const DWORD toRead = std::min<DWORD>(available, sizeof(buffer));
            if (ReadFile(readPipe, buffer, toRead, &readBytes, nullptr) && readBytes > 0) {
                if (captured.size() < 4u * 1024u * 1024u) {
                    const size_t remaining = 4u * 1024u * 1024u - captured.size();
                    captured.append(buffer, std::min<size_t>(readBytes, remaining));
                } else {
                    outputTruncated = true;
                }
            }
        }
        const DWORD wait = WaitForSingleObject(process.hProcess, 50);
        if (wait == WAIT_OBJECT_0) {
            DWORD remaining = 0;
            if (!PeekNamedPipe(readPipe, nullptr, 0, nullptr, &remaining, nullptr) || remaining == 0) break;
        }
        if (GetTickCount64() - startTick > static_cast<ULONGLONG>(timeoutSeconds) * 1000ull) {
            result.timedOut = true;
            if (job) TerminateJobObject(job, 1);
            else TerminateProcess(process.hProcess, 1);
            WaitForSingleObject(process.hProcess, 3000);
            break;
        }
    }

    DWORD exitCode = 0;
    GetExitCodeProcess(process.hProcess, &exitCode);
    result.exitCode = exitCode;
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    CloseHandle(readPipe);
    if (job) CloseHandle(job);

    result.output = DecodeConsoleBytes(captured);
    if (outputTruncated) result.output += L"\r\n[Wyjście zostało obcięte do 4 MB.]\r\n";
    if (result.cancelled) result.output += L"\r\nOperacja została anulowana przez użytkownika.\r\n";
    else if (result.timedOut) result.output += L"\r\nOperacja przekroczyła limit czasu (" + std::to_wstring(timeoutSeconds) + L" s) i została przerwana.\r\n";
    else if (Trim(result.output).empty()) result.output = L"Polecenie zakończyło się bez danych wyjściowych.\r\nKod wyjścia: " + std::to_wstring(exitCode) + L"\r\n";
    finish();
    return result;
}

std::wstring NativeModuleOutput(int id) {
    if (id == 111) {
        return L"KD CORE " + std::wstring(kAppVersion) + L"\r\n"
               L"================================================================\r\n\r\n"
               L"Natywna aplikacja diagnostyczna dla Windows 10/11.\r\n"
               L"Interfejs: Win32 / GDI+ (KD Core UI)\r\n"
               L"Silniki: WinAPI, WMI, PDH, DXGI, Storage API\r\n\r\n"
               L"Baza modułów: 215 rzeczywistych narzędzi w 8 kategoriach.\r\n"
               L"Monitorowanie: CPU, RAM, dyski, GPU, system, urządzenia.\r\n"
               L"Raporty: automatyczny zapis i eksport ręczny.\r\n"
               L"Tryb pracy: lokalny, bez wysyłania danych do internetu.\r\n\r\n"
               L"Wykonanie: " + CurrentTimeString() + L"\r\n";
    }
    if (id == 112) {
        std::wstring out = L"Sprawdzenie uprawnień sesji\r\n----------------------------------------\r\n";
        out += std::wstring(L"Administrator: ") + (IsUserAnAdmin() ? L"TAK" : L"NIE") + L"\r\n";
        wchar_t user[256]{};
        DWORD userSize = static_cast<DWORD>(std::size(user));
        GetUserNameW(user, &userSize);
        out += L"Użytkownik: " + std::wstring(user) + L"\r\n";
        out += L"Proces: " + GetExePath() + L"\r\n\r\n";
        out += IsUserAnAdmin()
            ? L"Ta sesja ma podniesione uprawnienia. Moduły administracyjne mogą działać poprawnie.\r\n"
            : L"Ta sesja działa jako standardowy użytkownik. Moduły oznaczone ADMIN mogą wymagać podniesienia uprawnień.\r\n";
        return out;
    }
    if (id == 170) {
        LASTINPUTINFO info{};
        info.cbSize = sizeof(info);
        const DWORD now = GetTickCount();
        DWORD idleMs = 0;
        if (GetLastInputInfo(&info)) idleMs = now - info.dwTime;
        const ULONGLONG idleSeconds = idleMs / 1000ull;
        std::wstring out = L"Czas bezczynności użytkownika\r\n----------------------------------------\r\n";
        out += L"Bezczynność: " + std::to_wstring(idleSeconds) + L" s";
        if (idleSeconds >= 60) out += L" (" + FormatDuration(static_cast<double>(idleSeconds)) + L")";
        out += L"\r\nOstatnia aktywność: " + std::wstring(idleMs >= 1000 ? L"ponad sekundę temu" : L"przed chwilą") + L"\r\n";
        out += L"\r\nWartość pochodzi z GetLastInputInfo (klawiatura i mysz).\r\n";
        return out;
    }
    return L"Moduł natywny nie zwrócił danych.\r\n";
}

void StartModule(int id);
void CancelRunningModule();

void FinishModule(std::unique_ptr<ModuleOutcome> outcome) {
    if (!outcome) return;
    if (g.moduleThread.joinable()) g.moduleThread.join();
    g.busy = false;
    g.runningModuleId = 0;
    g.cancelRequested.store(false);

    const Module* module = FindModuleById(outcome->id);
    const bool wasSelected = (g.selectedModule == outcome->id);
    Report report;
    report.moduleId = outcome->id;
    report.moduleName = module ? module->name : L"Moduł";
    report.category = module ? module->category : L"";
    report.output = outcome->output;
    report.startedAt = g.runningStarted;
    report.seconds = outcome->seconds;
    report.exitCode = static_cast<int>(outcome->exitCode);
    report.timedOut = outcome->timedOut;
    report.cancelled = outcome->cancelled;
    report.autoSaved = g.settings.autoSaveReports;
    if (report.autoSaved) {
        report.file = SaveReportFile(report);
        if (report.file.empty()) {
            report.output += L"\r\n[Uwaga: nie udało się automatycznie zapisać raportu w folderze dokumentów.]\r\n";
        }
    }

    RunInfo info;
    info.state = StateFromOutcome(*outcome);
    info.seconds = outcome->seconds;
    info.exitCode = report.exitCode;
    info.finishedAt = CurrentTimeString();
    g.runs[outcome->id] = info;
    g.lastOutput[outcome->id] = report.output;
    g.reports.insert(g.reports.begin(), report);
    if (g.reports.size() > 100) g.reports.resize(100);
    g.selectedReport = g.reports.empty() ? -1 : 0;
    g.lastRunError = info.state == RunState::Error || info.state == RunState::Timeout;
    g.status = g.lastRunError ? L"BŁĄD MODUŁU" : L"GOTOWY";

    if (wasSelected) UpdateOutputText();
    UpdateAnimTimer();
    InvalidateAll();
}

void StartModule(int id) {
    if (g.busy) return;
    const Module* module = FindModuleById(id);
    if (!module) {
        MessageBoxW(g.hwnd, L"Najpierw wybierz moduł z listy.", L"KD Core", MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (id == 120) {
        if (MessageBoxW(g.hwnd, L"Zamknąć aplikację KD Core?", L"Wyjście", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
            PostMessageW(g.hwnd, WM_CLOSE, 0, 0);
        }
        return;
    }

    std::wstring warning;
    if (module->requiresAdmin && !g.admin) warning += L"Ten moduł może wymagać uprawnień administratora. Uruchomienie bez podniesienia uprawnień może zakończyć się błędem.\r\n\r\n";
    if (module->destructive) warning += L"Operacja może zmienić konfigurację systemu, usunąć dane lub zrestartować komputer.\r\n\r\n";
    if (!warning.empty()) {
        const std::wstring prompt = std::wstring(module->name) + L"\r\n\r\n" + warning + L"Kontynuować?";
        if (MessageBoxW(g.hwnd, prompt.c_str(), L"Potwierdź operację", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) return;
    }

    if (g.moduleThread.joinable()) g.moduleThread.join();
    g.busy = true;
    g.runningModuleId = id;
    g.runningModuleName = module->name;
    g.runningStarted = FullTimestamp();
    g.cancelRequested.store(false);
    g.status = L"DIAGNOSTYKA";
    UpdateOutputText();
    UpdateAnimTimer();
    InvalidateAll();

    const int moduleId = id;
    const std::wstring command = module->command ? module->command : L"";
    HWND hwnd = g.hwnd;
    try {
        g.moduleThread = std::thread([hwnd, moduleId, command]() {
            auto outcome = std::make_unique<ModuleOutcome>();
            outcome->id = moduleId;
            const auto started = std::chrono::steady_clock::now();
            if (moduleId == 111 || moduleId == 112 || moduleId == 170) {
                outcome->output = NativeModuleOutput(moduleId);
                outcome->exitCode = 0;
            } else {
                ExecResult executed = ExecuteCommand(command);
                outcome->output = executed.output;
                outcome->exitCode = executed.exitCode;
                outcome->timedOut = executed.timedOut;
                outcome->cancelled = executed.cancelled;
                outcome->spawnFailed = executed.spawnFailed;
            }
            outcome->seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
            ModuleOutcome* raw = outcome.release();
            if (!PostMessageW(hwnd, WM_APP_MODULE_DONE, static_cast<WPARAM>(moduleId), reinterpret_cast<LPARAM>(raw))) delete raw;
        });
    } catch (...) {
        g.busy = false;
        g.runningModuleId = 0;
        g.status = L"BŁĄD URUCHAMIANIA";
        MessageBoxW(g.hwnd, L"Nie udało się utworzyć wątku diagnostycznego.", L"Błąd", MB_OK | MB_ICONERROR);
        InvalidateAll();
    }
}

void CancelRunningModule() {
    if (!g.busy) return;
    g.cancelRequested.store(true);
    g.status = L"ANULOWANIE…";
    InvalidateAll();
}

std::wstring FindGpuDriverVersion(const std::wstring& adapterName) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}",
                      0, KEY_READ, &key) != ERROR_SUCCESS) return L"";
    const std::wstring wanted = Lower(adapterName);
    std::wstring fallback;
    for (DWORD index = 0;; ++index) {
        wchar_t subKey[64]{};
        DWORD subLength = static_cast<DWORD>(std::size(subKey));
        const LSTATUS status = RegEnumKeyExW(key, index, subKey, &subLength, nullptr, nullptr, nullptr, nullptr);
        if (status == ERROR_NO_MORE_ITEMS) break;
        if (status != ERROR_SUCCESS) break;
        if (subKey[0] < L'0' || subKey[0] > L'9') continue;
        wchar_t driverDesc[256]{};
        DWORD descSize = sizeof(driverDesc);
        wchar_t driverVersion[128]{};
        DWORD versionSize = sizeof(driverVersion);
        if (RegGetValueW(key, subKey, L"DriverDesc", RRF_RT_REG_SZ, nullptr, driverDesc, &descSize) != ERROR_SUCCESS) continue;
        if (RegGetValueW(key, subKey, L"DriverVersion", RRF_RT_REG_SZ, nullptr, driverVersion, &versionSize) != ERROR_SUCCESS) continue;
        const std::wstring desc = Lower(driverDesc);
        if (fallback.empty()) fallback = driverVersion;
        if (!wanted.empty() && (wanted.find(desc) != std::wstring::npos || desc.find(wanted) != std::wstring::npos)) {
            RegCloseKey(key);
            return driverVersion;
        }
    }
    RegCloseKey(key);
    return fallback;
}

struct DiskIdentity {
    std::wstring model;
    int ssd = -1; // -1: n/d, 0: HDD, 1: SSD
};

struct Sampler {
    PDH_HQUERY diskQuery = nullptr;
    PDH_HCOUNTER diskRead = nullptr;
    PDH_HCOUNTER diskWrite = nullptr;
    PDH_HQUERY gpuQuery = nullptr;
    PDH_HCOUNTER gpuUsage = nullptr;
    PDH_HQUERY thermalQuery = nullptr;
    PDH_HCOUNTER thermal = nullptr;
    bool initialized = false;

    ULONGLONG prevIdle = 0;
    ULONGLONG prevKernel = 0;
    ULONGLONG prevUser = 0;
    bool cpuPrev = false;

    std::wstring cpuName;
    int cpuMaxMhz = 0;
    int cpuCores = 0;
    int cpuThreads = 0;
    std::wstring osName;
    std::wstring architecture;
    std::wstring computer;
    std::wstring bios;
    std::wstring board;
    std::wstring model;
    std::wstring gpuName;
    std::wstring gpuVendor;
    std::wstring gpuDriver;
    uint64_t gpuVram = 0;
    std::map<int, DiskIdentity> diskCache;

    void Initialize();
    void Fill(Snapshot& snapshot);
};

void Sampler::Initialize() {
    initialized = true;

    cpuName = RegReadString(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"ProcessorNameString");
    if (cpuName.empty()) cpuName = L"Procesor";
    cpuMaxMhz = static_cast<int>(RegReadDword(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"~MHz", 0));

    SYSTEM_INFO systemInfo{};
    GetNativeSystemInfo(&systemInfo);
    cpuThreads = static_cast<int>(systemInfo.dwNumberOfProcessors);
    DWORD bufferSize = 0;
    if (!GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &bufferSize) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER && bufferSize > 0) {
        std::vector<BYTE> buffer(bufferSize);
        if (GetLogicalProcessorInformationEx(RelationProcessorCore, reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()), &bufferSize)) {
            int cores = 0;
            DWORD offset = 0;
            while (offset < bufferSize) {
                auto* info = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data() + offset);
                if (info->Relationship == RelationProcessorCore) ++cores;
                if (info->Size == 0) break;
                offset += info->Size;
            }
            cpuCores = cores;
        }
    }
    if (cpuCores <= 0) cpuCores = cpuThreads;

    const std::wstring product = RegReadString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"ProductName");
    const std::wstring display = RegReadString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"DisplayVersion");
    const std::wstring build = RegReadString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuild");
    const DWORD ubr = RegReadDword(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"UBR", 0);
    std::wstring os = product.empty() ? L"Windows" : product;
    if (!build.empty()) {
        const int buildNumber = _wtoi(build.c_str());
        if (buildNumber >= 22000 && os.find(L"Windows 10") != std::wstring::npos) {
            const size_t pos = os.find(L"Windows 10");
            os.replace(pos, 10, L"Windows 11");
        }
        os += L" " + display;
        os += L" (build " + build;
        if (ubr > 0) os += L"." + std::to_wstring(ubr);
        os += L")";
    }
    osName = Trim(os);
    if (osName.empty()) osName = L"Windows";

    switch (systemInfo.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64: architecture = L"x64"; break;
    case PROCESSOR_ARCHITECTURE_ARM64: architecture = L"ARM64"; break;
    case PROCESSOR_ARCHITECTURE_ARM: architecture = L"ARM"; break;
    case PROCESSOR_ARCHITECTURE_INTEL: architecture = L"x86"; break;
    default: architecture = L"n/d"; break;
    }

    wchar_t nameBuffer[MAX_COMPUTERNAME_LENGTH + 1]{};
    DWORD nameSize = static_cast<DWORD>(std::size(nameBuffer));
    if (GetComputerNameW(nameBuffer, &nameSize)) computer = nameBuffer;
    if (computer.empty()) computer = L"n/d";

    const std::wstring biosVendor = RegReadString(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BIOSVendor");
    const std::wstring biosVersion = RegReadString(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BIOSVersion");
    bios = Trim(biosVendor + L" " + biosVersion);
    HKEY secureBoot = nullptr;
    bool uefi = false;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\SecureBoot\\State", 0, KEY_READ, &secureBoot) == ERROR_SUCCESS) {
        uefi = true;
        RegCloseKey(secureBoot);
    }
    if (bios.empty()) bios = uefi ? L"UEFI" : L"n/d";
    else bios = (uefi ? L"UEFI · " : L"BIOS · ") + bios;

    const std::wstring boardVendor = RegReadString(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BaseBoardManufacturer");
    const std::wstring boardProduct = RegReadString(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BaseBoardProduct");
    board = Trim(boardVendor + L" " + boardProduct);
    if (board.empty()) board = L"n/d";

    const std::wstring systemVendor = RegReadString(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemManufacturer");
    const std::wstring systemProduct = RegReadString(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemProductName");
    model = Trim(systemVendor + L" " + systemProduct);
    if (model.empty()) model = L"n/d";

    IDXGIFactory1* factory = nullptr;
    if (SUCCEEDED(CreateDXGIFactory1(IID_IDXGIFactory1, reinterpret_cast<void**>(&factory))) && factory) {
        for (UINT index = 0;; ++index) {
            IDXGIAdapter1* adapter = nullptr;
            if (factory->EnumAdapters1(index, &adapter) == DXGI_ERROR_NOT_FOUND) break;
            if (!adapter) break;
            DXGI_ADAPTER_DESC1 desc{};
            const HRESULT hr = adapter->GetDesc1(&desc);
            const bool software = SUCCEEDED(hr) && (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE);
            if (SUCCEEDED(hr) && !software) {
                gpuName = desc.Description;
                gpuVram = static_cast<uint64_t>(desc.DedicatedVideoMemory);
                switch (desc.VendorId) {
                case 0x10DE: gpuVendor = L"NVIDIA"; break;
                case 0x1002:
                case 0x1022: gpuVendor = L"AMD"; break;
                case 0x8086: gpuVendor = L"Intel"; break;
                case 0x1414: gpuVendor = L"Microsoft"; break;
                case 0x1AF4: gpuVendor = L"Red Hat"; break;
                default: {
                    wchar_t vendorBuffer[32]{};
                    swprintf(vendorBuffer, 32, L"0x%04X", desc.VendorId);
                    gpuVendor = vendorBuffer;
                    break;
                }
                }
                adapter->Release();
                break;
            }
            adapter->Release();
        }
        factory->Release();
    }
    if (gpuName.empty()) gpuName = L"Brak karty graficznej";
    gpuDriver = FindGpuDriverVersion(gpuName);
    if (gpuDriver.empty()) gpuDriver = L"n/d";

    if (PdhOpenQueryW(nullptr, 0, &diskQuery) == ERROR_SUCCESS) {
        if (PdhAddEnglishCounterW(diskQuery, L"\\PhysicalDisk(_Total)\\Disk Read Bytes/sec", 0, &diskRead) != ERROR_SUCCESS) diskRead = nullptr;
        if (PdhAddEnglishCounterW(diskQuery, L"\\PhysicalDisk(_Total)\\Disk Write Bytes/sec", 0, &diskWrite) != ERROR_SUCCESS) diskWrite = nullptr;
        PdhCollectQueryData(diskQuery);
    }
    if (PdhOpenQueryW(nullptr, 0, &gpuQuery) == ERROR_SUCCESS) {
        if (PdhAddEnglishCounterW(gpuQuery, L"\\GPU Engine(*engtype_3D)\\Utilization Percentage", 0, &gpuUsage) != ERROR_SUCCESS) gpuUsage = nullptr;
        if (gpuUsage) PdhCollectQueryData(gpuQuery);
    }
    if (PdhOpenQueryW(nullptr, 0, &thermalQuery) == ERROR_SUCCESS) {
        if (PdhAddEnglishCounterW(thermalQuery, L"\\Thermal Zone Information(*)\\Temperature", 0, &thermal) != ERROR_SUCCESS) thermal = nullptr;
        if (thermal) PdhCollectQueryData(thermalQuery);
    }
}

DiskIdentity QueryDiskIdentity(int diskNumber) {
    DiskIdentity identity;
    const std::wstring path = L"\\\\.\\PhysicalDrive" + std::to_wstring(diskNumber);
    HANDLE handle = CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return identity;

    STORAGE_PROPERTY_QUERY query{};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;
    BYTE buffer[1024]{};
    DWORD returned = 0;
    if (DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), buffer, sizeof(buffer), &returned, nullptr)) {
        auto* descriptor = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(buffer);
        if (descriptor->ProductIdOffset > 0 && descriptor->ProductIdOffset < returned) {
            identity.model = Trim(reinterpret_cast<const wchar_t*>(buffer + descriptor->ProductIdOffset));
        }
    }

    STORAGE_PROPERTY_QUERY seekQuery{};
    seekQuery.PropertyId = StorageDeviceSeekPenaltyProperty;
    seekQuery.QueryType = PropertyStandardQuery;
    DEVICE_SEEK_PENALTY_DESCRIPTOR penalty{};
    if (DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY, &seekQuery, sizeof(seekQuery), &penalty, sizeof(penalty), &returned, nullptr)) {
        identity.ssd = penalty.IncursSeekPenalty ? 0 : 1;
    }
    CloseHandle(handle);

    if (identity.ssd < 0) {
        const std::wstring lowerModel = Lower(identity.model);
        if (lowerModel.find(L"nvme") != std::wstring::npos || lowerModel.find(L"ssd") != std::wstring::npos) identity.ssd = 1;
    }
    return identity;
}

void Sampler::Fill(Snapshot& snapshot) {
    snapshot.valid = true;

    FILETIME idleTime{}, kernelTime{}, userTime{};
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        auto toU64 = [](const FILETIME& value) -> ULONGLONG {
            return (static_cast<ULONGLONG>(value.dwHighDateTime) << 32) | value.dwLowDateTime;
        };
        const ULONGLONG idle = toU64(idleTime);
        const ULONGLONG kernel = toU64(kernelTime);
        const ULONGLONG user = toU64(userTime);
        if (cpuPrev) {
            const ULONGLONG idleDelta = idle - prevIdle;
            const ULONGLONG totalDelta = (kernel - prevKernel) + (user - prevUser);
            if (totalDelta > 0) {
                const double usage = 100.0 * (1.0 - static_cast<double>(idleDelta) / static_cast<double>(totalDelta));
                snapshot.cpuPercent = static_cast<int>(std::clamp(usage, 0.0, 100.0) + 0.5);
                snapshot.cpuReady = true;
            }
        }
        prevIdle = idle;
        prevKernel = kernel;
        prevUser = user;
        cpuPrev = true;
    }
    snapshot.cpuName = cpuName;
    snapshot.cores = cpuCores;
    snapshot.threads = cpuThreads;

    int currentMhz = 0;
    if (cpuThreads > 0 && cpuThreads < 256) {
        struct KdProcessorPowerInformation {
            ULONG Number;
            ULONG MaxMhz;
            ULONG CurrentMhz;
            ULONG MhzLimit;
            ULONG MaxIdleState;
            ULONG CurrentIdleState;
        };
        std::vector<KdProcessorPowerInformation> power(static_cast<size_t>(cpuThreads));
        if (CallNtPowerInformation(ProcessorInformation, nullptr, 0, power.data(),
                                   static_cast<ULONG>(power.size() * sizeof(KdProcessorPowerInformation))) == 0) {
            currentMhz = static_cast<int>(power[0].CurrentMhz);
        }
    }
    if (currentMhz > 0) {
        snapshot.cpuMhz = currentMhz;
        snapshot.cpuMhzCurrent = true;
    } else {
        snapshot.cpuMhz = cpuMaxMhz;
    }

    if (thermalQuery && thermal) {
        if (PdhCollectQueryData(thermalQuery) == ERROR_SUCCESS) {
            DWORD size = 0;
            DWORD count = 0;
            PdhGetFormattedCounterArrayW(thermal, PDH_FMT_DOUBLE, &size, &count, nullptr);
            if (size > 0 && count > 0) {
                std::vector<BYTE> buffer(size);
                auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
                if (PdhGetFormattedCounterArrayW(thermal, PDH_FMT_DOUBLE, &size, &count, items) == ERROR_SUCCESS) {
                    double kelvin = 0.0;
                    for (DWORD i = 0; i < count; ++i) {
                        if (items[i].FmtValue.CStatus != PDH_CSTATUS_VALID_DATA) continue;
                        double value = items[i].FmtValue.doubleValue;
                        if (value > 1000.0) value /= 10.0;
                        if (value > 200.0 && value < 450.0) {
                            if (kelvin == 0.0 || value < kelvin) kelvin = value;
                        }
                    }
                    if (kelvin > 0.0) {
                        const double celsius = kelvin - 273.15;
                        if (celsius > 10.0 && celsius < 120.0) {
                            wchar_t bufferText[32]{};
                            swprintf(bufferText, 32, L"%.1f °C", celsius);
                            snapshot.cpuTemp = Comma(bufferText);
                        }
                    }
                }
            }
        }
    }

    MEMORYSTATUSEX memory{};
    memory.dwLength = sizeof(memory);
    if (GlobalMemoryStatusEx(&memory)) {
        snapshot.ramTotal = memory.ullTotalPhys;
        snapshot.ramAvail = memory.ullAvailPhys;
        snapshot.ramUsed = memory.ullTotalPhys - memory.ullAvailPhys;
        snapshot.ramPercent = static_cast<int>(memory.dwMemoryLoad);
        snapshot.commitLimit = memory.ullTotalPageFile;
        snapshot.commitUsed = memory.ullTotalPageFile - memory.ullAvailPageFile;
    }

    if (diskQuery && (diskRead || diskWrite)) {
        if (PdhCollectQueryData(diskQuery) == ERROR_SUCCESS) {
            PDH_FMT_COUNTERVALUE value{};
            if (diskRead && PdhGetFormattedCounterValue(diskRead, PDH_FMT_DOUBLE, nullptr, &value) == ERROR_SUCCESS) {
                snapshot.diskRead = value.doubleValue;
            }
            if (diskWrite && PdhGetFormattedCounterValue(diskWrite, PDH_FMT_DOUBLE, nullptr, &value) == ERROR_SUCCESS) {
                snapshot.diskWrite = value.doubleValue;
            }
            snapshot.diskIo = (diskRead != nullptr && diskWrite != nullptr);
        }
    }

    snapshot.gpu.name = gpuName;
    snapshot.gpu.vendor = gpuVendor;
    snapshot.gpu.driver = gpuDriver;
    snapshot.gpu.vram = gpuVram;
    if (gpuQuery && gpuUsage) {
        if (PdhCollectQueryData(gpuQuery) == ERROR_SUCCESS) {
            DWORD size = 0;
            DWORD count = 0;
            PdhGetFormattedCounterArrayW(gpuUsage, PDH_FMT_DOUBLE, &size, &count, nullptr);
            if (size > 0 && count > 0) {
                std::vector<BYTE> buffer(size);
                auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
                if (PdhGetFormattedCounterArrayW(gpuUsage, PDH_FMT_DOUBLE, &size, &count, items) == ERROR_SUCCESS) {
                    double sum = 0.0;
                    for (DWORD i = 0; i < count; ++i) {
                        if (items[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA ||
                            items[i].FmtValue.CStatus == PDH_CSTATUS_NEW_DATA) {
                            sum += std::max(0.0, items[i].FmtValue.doubleValue);
                        }
                    }
                    if (count > 0) snapshot.gpu.usage = static_cast<int>(std::clamp(sum, 0.0, 100.0) + 0.5);
                }
            }
        }
    }

    wchar_t drives[512]{};
    const DWORD driveLength = GetLogicalDriveStringsW(static_cast<DWORD>(std::size(drives)), drives);
    if (driveLength > 0) {
        std::map<int, DiskIdentity> updatedCache;
        for (const wchar_t* drive = drives; *drive; drive += wcslen(drive) + 1) {
            const UINT type = GetDriveTypeW(drive);
            if (type != DRIVE_FIXED && type != DRIVE_REMOVABLE) continue;
            ULARGE_INTEGER freeBytes{}, totalBytes{};
            if (!GetDiskFreeSpaceExW(drive, &freeBytes, &totalBytes, nullptr)) continue;
            if (totalBytes.QuadPart == 0) continue;

            DiskVolume volume;
            volume.letter = drive[0];
            volume.total = totalBytes.QuadPart;
            volume.free = freeBytes.QuadPart;
            volume.removable = type == DRIVE_REMOVABLE;
            wchar_t label[128]{};
            wchar_t fs[32]{};
            if (GetVolumeInformationW(drive, label, static_cast<DWORD>(std::size(label)), nullptr, nullptr, nullptr,
                                      fs, static_cast<DWORD>(std::size(fs)))) {
                volume.label = label;
                volume.fs = fs;
            }
            if (volume.fs.empty()) volume.fs = volume.removable ? L"USB" : L"n/d";

            int diskNumber = -1;
            std::wstring volumePath = L"\\\\.\\";
            volumePath += drive[0];
            volumePath += L":";
            HANDLE volumeHandle = CreateFileW(volumePath.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                              OPEN_EXISTING, 0, nullptr);
            if (volumeHandle != INVALID_HANDLE_VALUE) {
                BYTE extentBuffer[sizeof(VOLUME_DISK_EXTENTS) + sizeof(DISK_EXTENT) * 8]{};
                DWORD returned = 0;
                if (DeviceIoControl(volumeHandle, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0,
                                    extentBuffer, sizeof(extentBuffer), &returned, nullptr)) {
                    auto* extents = reinterpret_cast<VOLUME_DISK_EXTENTS*>(extentBuffer);
                    if (extents->NumberOfDiskExtents > 0) diskNumber = static_cast<int>(extents->Extents[0].DiskNumber);
                }
                CloseHandle(volumeHandle);
            }

            if (diskNumber >= 0) {
                auto cached = diskCache.find(diskNumber);
                if (cached != diskCache.end()) {
                    volume.model = cached->second.model;
                    volume.ssdKnown = cached->second.ssd >= 0;
                    volume.ssd = cached->second.ssd == 1;
                    updatedCache[diskNumber] = cached->second;
                } else {
                    const DiskIdentity identity = QueryDiskIdentity(diskNumber);
                    diskCache[diskNumber] = identity;
                    updatedCache[diskNumber] = identity;
                    volume.model = identity.model;
                    volume.ssdKnown = identity.ssd >= 0;
                    volume.ssd = identity.ssd == 1;
                }
            }
            if (volume.model.empty()) volume.model = volume.removable ? L"Nośnik wymienny" : L"Dysk fizyczny";
            snapshot.disks.push_back(volume);
        }
        diskCache.swap(updatedCache);
    }
    std::stable_sort(snapshot.disks.begin(), snapshot.disks.end(), [](const DiskVolume& a, const DiskVolume& b) {
        if (a.letter == L'C') return true;
        if (b.letter == L'C') return false;
        return a.letter < b.letter;
    });

    snapshot.osName = osName;
    snapshot.arch = architecture;
    snapshot.computer = computer;
    snapshot.uptime = FormatUptime(GetTickCount64());
    snapshot.bios = bios;
    snapshot.board = board;
    snapshot.model = model;
    snapshot.sampleTime = CurrentTimeString();
}

void StatsThreadProc(HWND hwnd) {
    Sampler sampler;
    if (!sampler.initialized) sampler.Initialize();
    while (!g.quit.load()) {
        Snapshot snapshot;
        sampler.Fill(snapshot);
        {
            std::lock_guard<std::mutex> lock(g.statsMutex);
            g.pendingStats = std::move(snapshot);
            g.statsDirty = true;
        }
        if (!PostMessageW(hwnd, WM_APP_STATS, 0, 0)) break;
        const DWORD interval = static_cast<DWORD>(std::max(1, g.refreshSeconds.load())) * 1000u;
        WaitForSingleObject(g_wakeEvent, interval);
    }
}

void StartStatsThread() {
    if (g.statsThread.joinable()) return;
    g.quit.store(false);
    if (!g_wakeEvent) g_wakeEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    try {
        g.statsThread = std::thread(StatsThreadProc, g.hwnd);
    } catch (...) {
        g.status = L"BŁĄD MONITOROWANIA";
    }
}

void StopStatsThread() {
    g.quit.store(true);
    if (g_wakeEvent) SetEvent(g_wakeEvent);
    if (g.statsThread.joinable()) g.statsThread.join();
    if (g_wakeEvent) {
        CloseHandle(g_wakeEvent);
        g_wakeEvent = nullptr;
    }
}

constexpr int kSidebarWidth = 248;
constexpr int kHeaderHeight = 96;
constexpr int kPageMargin = 24;
constexpr int kPanelGap = 16;
constexpr int kChipLabelsCount = 9;

const wchar_t* kChipLabels[kChipLabelsCount] = {
    L"Wszystkie", L"System", L"Sprzęt", L"Dyski", L"Sieć",
    L"Wydajność", L"Bezpieczeństwo", L"Urządzenia", L"Konserwacja"
};

void RecalculateLayout() {
    Layout& layout = g.layout;
    RECT client{};
    GetClientRect(g.hwnd, &client);
    layout.width = static_cast<int>(std::lround(static_cast<double>(client.right) / g_scale));
    layout.height = static_cast<int>(std::lround(static_cast<double>(client.bottom) / g_scale));
    if (layout.width <= 0 || layout.height <= 0) return;

    layout.sidebar = MakeRect(0, 0, kSidebarWidth, layout.height);
    layout.sidebarLogo = MakeRect(16, 18, kSidebarWidth - 32, 50);
    layout.sidebarFooter = MakeRect(16, layout.height - 92, kSidebarWidth - 32, 74);

    const int navStart = 106;
    const int navHeight = 42;
    const int navStep = 46;
    for (int i = 0; i < NAV_COUNT; ++i) {
        layout.nav[i] = MakeRect(16, navStart + i * navStep, kSidebarWidth - 32, navHeight);
    }

    const int searchWidth = std::clamp(layout.width / 5, 264, 360);
    layout.headerSearch = MakeRect(layout.width - kPageMargin - searchWidth, 27, searchWidth, 38);
    layout.searchRect = MakeRect(layout.headerSearch.left + 38, layout.headerSearch.top + 7,
                                 searchWidth - 52, 24);
    layout.headerStatus = MakeRect(layout.headerSearch.left - 196, 30, 178, 32);
    layout.headerTitle = MakeRect(kSidebarWidth + kPageMargin, 16, layout.headerStatus.left - kSidebarWidth - kPageMargin - 18, 36);
    layout.headerSub = MakeRect(kSidebarWidth + kPageMargin, 53, layout.headerTitle.right - layout.headerTitle.left, 22);
    layout.adminPill = MakeRect(16, layout.height - 92, 100, 28);

    const int left = kSidebarWidth + kPageMargin;
    const int right = layout.width - kPageMargin;
    const int top = kHeaderHeight + 14;
    const int bottom = layout.height - 20;
    layout.dashboard = MakeRect(left, top, right - left, bottom - top);

    if (g.nav == NAV_DASHBOARD) {
        const int totalHeight = bottom - top;
        const int gap = 14;
        const int welcomeH = std::clamp(static_cast<int>(totalHeight * 0.18), 96, 140);
        const int metricsH = std::clamp(static_cast<int>(totalHeight * 0.26), 142, 210);
        const int disksH = std::clamp(static_cast<int>(totalHeight * 0.24), 132, 200);
        const int bottomH = std::max(126, totalHeight - welcomeH - metricsH - disksH - 3 * gap);

        const int welcomeW = static_cast<int>((right - left) * 0.40);
        const int quickW = static_cast<int>((right - left) * 0.26);
        layout.dashWelcome = MakeRect(left, top, welcomeW, welcomeH);
        layout.dashQuick = MakeRect(left + welcomeW + gap, top, quickW, welcomeH);
        const int historyLeft = left + welcomeW + quickW + 2 * gap;
        layout.dashHistory = MakeRect(historyLeft, top, right - historyLeft, welcomeH);

        const int markSize = std::clamp(welcomeH - 48, 34, 44);
        layout.dashWelcomeLogo = MakeRect(layout.dashWelcome.left + 16, layout.dashWelcome.top + 14, markSize, markSize);
        layout.dashWelcomeStatus = MakeRect(layout.dashWelcome.left + 16, layout.dashWelcome.bottom - 27,
                                            std::min(156, welcomeW - 32), 20);

        const int quickPad = 12;
        const int quickButtonH = std::clamp((welcomeH - 42) / 3 - 5, 18, 30);
        for (int i = 0; i < 3; ++i) {
            layout.dashQuickItems[i] = MakeRect(layout.dashQuick.left + quickPad,
                layout.dashQuick.top + 34 + i * (quickButtonH + 5),
                Width(layout.dashQuick) - 2 * quickPad, quickButtonH);
        }
        const int historyRowH = std::clamp((welcomeH - 34) / 4, 16, 24);
        for (int i = 0; i < 4; ++i) {
            layout.dashHistoryRows[i] = MakeRect(layout.dashHistory.left + 12,
                layout.dashHistory.top + 30 + i * historyRowH,
                Width(layout.dashHistory) - 24, historyRowH - 2);
        }

        const int metricsTop = top + welcomeH + gap;
        const int cpuWidth = static_cast<int>((right - left - gap) * 0.61);
        layout.dashCpu = MakeRect(left, metricsTop, cpuWidth, metricsH);
        layout.dashRam = MakeRect(left + cpuWidth + gap, metricsTop,
                                  right - (left + cpuWidth + gap), metricsH);
        const int disksTop = metricsTop + metricsH + gap;
        layout.dashDisk = MakeRect(left, disksTop, right - left, disksH);
        const int bottomTop = disksTop + disksH + gap;
        const int gpuWidth = static_cast<int>((right - left - gap) * 0.50);
        layout.dashGpu = MakeRect(left, bottomTop, gpuWidth, bottomH);
        layout.dashSystem = MakeRect(left + gpuWidth + gap, bottomTop,
                                     right - (left + gpuWidth + gap), bottomH);

        const int ringSize = std::clamp(Height(layout.dashCpu) - 64, 70, 104);
        layout.cpuRing = MakeRect(layout.dashCpu.right - 18 - ringSize,
                                  layout.dashCpu.top + 40, ringSize, ringSize);
        const int ramRingSize = std::clamp(Height(layout.dashRam) - 64, 70, 104);
        layout.ramRing = MakeRect(layout.dashRam.right - 18 - ramRingSize,
                                  layout.dashRam.top + 40, ramRingSize, ramRingSize);
        const int gpuRingSize = std::clamp(Height(layout.dashGpu) - 62, 62, 90);
        layout.gpuRing = MakeRect(layout.dashGpu.right - 18 - gpuRingSize,
                                  layout.dashGpu.top + 38, gpuRingSize, gpuRingSize);
        return;
    }

    if (g.nav == NAV_ABOUT) {
        layout.aboutCard = MakeRect(left, top, right - left, bottom - top);
        layout.aboutLogo = MakeRect(left + 36, top + 38, 64, 64);
        const int buttonY = bottom - 68;
        layout.aboutAdmin = MakeRect(left + 36, buttonY, 224, 38);
        layout.aboutReports = MakeRect(left + 272, buttonY, 224, 38);
        return;
    }

    if (ModulesView()) {
        int chipX = left;
        const int chipTop = top;
        g.layout.chipCount = kChipLabelsCount;
        HDC dc = GetDC(g.hwnd);
        Graphics measure(dc);
        for (int i = 0; i < kChipLabelsCount; ++i) {
            const float textWidth = MeasureText(measure, kChipLabels[i], 8.2f, true);
            int width = static_cast<int>(textWidth) + 28;
            if (chipX + width > right) width = std::max(48, right - chipX);
            layout.chips[i] = MakeRect(chipX, chipTop, width, 28);
            chipX += width + 7;
        }
        ReleaseDC(g.hwnd, dc);

        const int listTop = chipTop + 42;
        const int listWidth = std::clamp(static_cast<int>((right - left) * 0.44), 350, 600);
        layout.listPanel = MakeRect(left, listTop, listWidth, bottom - listTop);
        layout.listHeader = MakeRect(layout.listPanel.left, layout.listPanel.top, Width(layout.listPanel), 56);
        layout.listArea = MakeRect(layout.listPanel.left + 10, layout.listHeader.bottom + 2,
                                   Width(layout.listPanel) - 20, layout.listPanel.bottom - layout.listHeader.bottom - 14);
        layout.detailPanel = MakeRect(left + listWidth + kPanelGap, listTop,
                                      right - (left + listWidth + kPanelGap), bottom - listTop);

        const RECT& panel = layout.detailPanel;
        const int pad = 20;
        layout.detailTitle = MakeRect(panel.left + pad, panel.top + 18, Width(panel) - 2 * pad - 128, 32);
        layout.detailStatus = MakeRect(panel.right - pad - 118, panel.top + 22, 118, 24);
        layout.detailSub = MakeRect(panel.left + pad, panel.top + 52, Width(panel) - 2 * pad, 18);
        layout.detailDesc = MakeRect(panel.left + pad, panel.top + 74, Width(panel) - 2 * pad, 38);
        layout.detailMeta = MakeRect(panel.left + pad, panel.top + 116, Width(panel) - 2 * pad, 24);
        layout.detailNote = MakeRect(panel.left + pad, panel.top + 144, Width(panel) - 2 * pad, 20);
        layout.runButton = MakeRect(panel.left + pad, panel.top + 172, 196, 40);
        layout.detailInfo = MakeRect(panel.left + pad + 212, panel.top + 174, Width(panel) - 2 * pad - 212, 38);
        layout.outputLabel = MakeRect(panel.left + pad, panel.top + 228, Width(panel) - 2 * pad, 18);
        layout.outputPanel = MakeRect(panel.left + pad, panel.top + 250, Width(panel) - 2 * pad,
                                      panel.bottom - 60 - (panel.top + 250));
        layout.outputEdit = MakeRect(layout.outputPanel.left + 10, layout.outputPanel.top + 10,
                                     Width(layout.outputPanel) - 20, Height(layout.outputPanel) - 20);
        layout.exportButton = MakeRect(panel.right - pad - 158, panel.bottom - 46, 158, 32);
        return;
    }

    if (g.nav == NAV_REPORTS) {
        const int listWidth = std::clamp(static_cast<int>((right - left) * 0.35), 320, 480);
        layout.reportListPanel = MakeRect(left, top, listWidth, bottom - top);
        layout.reportListHeader = MakeRect(layout.reportListPanel.left, layout.reportListPanel.top,
                                           listWidth, 56);
        layout.reportListArea = MakeRect(layout.reportListPanel.left + 10, layout.reportListHeader.bottom + 2,
                                         listWidth - 20, bottom - layout.reportListHeader.bottom - 14);
        layout.previewPanel = MakeRect(left + listWidth + kPanelGap, top,
                                       right - (left + listWidth + kPanelGap), bottom - top);
        const RECT& panel = layout.previewPanel;
        const int pad = 20;
        layout.previewTitle = MakeRect(panel.left + pad, panel.top + 18, Width(panel) - 2 * pad - 120, 32);
        layout.previewStatus = MakeRect(panel.right - pad - 110, panel.top + 22, 110, 24);
        layout.previewMeta = MakeRect(panel.left + pad, panel.top + 52, Width(panel) - 2 * pad, 20);
        layout.previewOutputLabel = MakeRect(panel.left + pad, panel.top + 84, Width(panel) - 2 * pad, 18);
        layout.previewOutputPanel = MakeRect(panel.left + pad, panel.top + 106, Width(panel) - 2 * pad,
                                             panel.bottom - 70 - (panel.top + 106));
        layout.previewOutputEdit = MakeRect(layout.previewOutputPanel.left + 10, layout.previewOutputPanel.top + 10,
                                            Width(layout.previewOutputPanel) - 20, Height(layout.previewOutputPanel) - 20);
        layout.previewFolder = MakeRect(panel.left + pad, panel.bottom - 48, 168, 32);
        layout.previewExport = MakeRect(panel.right - pad - 158, panel.bottom - 48, 158, 32);
        return;
    }

    if (g.nav == NAV_SETTINGS) {
        const int columnGap = 16;
        const int leftWidth = static_cast<int>((right - left - columnGap) * 0.52);
        layout.settings1 = MakeRect(left, top, leftWidth, 240);
        layout.settings2 = MakeRect(left, top + 256, leftWidth, bottom - (top + 256));
        layout.settings3 = MakeRect(left + leftWidth + columnGap, top, right - (left + leftWidth + columnGap),
                                    bottom - top);

        const RECT& card1 = layout.settings1;
        layout.refreshSeg[0] = MakeRect(card1.right - 22 - 3 * 58 - 2 * 6, card1.top + 76, 58, 30);
        layout.refreshSeg[1] = MakeRect(layout.refreshSeg[0].right + 6, card1.top + 76, 58, 30);
        layout.refreshSeg[2] = MakeRect(layout.refreshSeg[1].right + 6, card1.top + 76, 58, 30);
        layout.animToggle = MakeRect(card1.right - 22 - 48, card1.top + 148, 48, 26);

        const RECT& card2 = layout.settings2;
        layout.autoSaveToggle = MakeRect(card2.right - 22 - 48, card2.top + 58, 48, 26);
        layout.settingsPath = MakeRect(card2.left + 22, card2.top + 124, Width(card2) - 44, 46);
        layout.openReportsButton = MakeRect(card2.left + 22, card2.top + 186, 172, 34);
        layout.clearHistoryButton = MakeRect(card2.right - 22 - 172, card2.top + 186, 172, 34);

        const RECT& card3 = layout.settings3;
        layout.checkAdminButton = MakeRect(card3.left + 22, card3.top + 178, 184, 36);
        layout.aboutButton = MakeRect(card3.right - 22 - 184, card3.top + 178, 184, 36);
    }
}

void ClampScroll() {
    if (ModulesView()) {
        const int rowStep = 66;
        const int visible = std::max(1, Height(g.layout.listArea) / rowStep);
        const int maxScroll = std::max(0, static_cast<int>(g.filtered.size()) - visible);
        g.listScroll = std::clamp(g.listScroll, 0, maxScroll);
    }
    if (g.nav == NAV_REPORTS) {
        const int rowStep = 62;
        const int visible = std::max(1, Height(g.layout.reportListArea) / rowStep);
        const int maxScroll = std::max(0, static_cast<int>(g.reports.size()) - visible);
        g.reportScroll = std::clamp(g.reportScroll, 0, maxScroll);
    }
}

RECT ModuleRowRect(int index) {
    const int first = g.listScroll;
    const int offset = (index - first) * 66;
    return MakeRect(g.layout.listArea.left + 4, g.layout.listArea.top + 2 + offset,
                    Width(g.layout.listArea) - 8, 60);
}

RECT ReportRowRect(int index) {
    const int first = g.reportScroll;
    const int offset = (index - first) * 62;
    return MakeRect(g.layout.reportListArea.left + 4, g.layout.reportListArea.top + 2 + offset,
                    Width(g.layout.reportListArea) - 8, 56);
}

const Report* ReportForModule(int moduleId) {
    for (const auto& report : g.reports) {
        if (report.moduleId == moduleId) return &report;
    }
    return nullptr;
}

void UpdateControlPositions() {
    if (!g.hwnd) return;
    auto place = [](HWND control, const RECT& rect, bool visible) {
        if (!control) return;
        if (!visible) {
            ShowWindow(control, SW_HIDE);
            return;
        }
        SetWindowPos(control, nullptr, ScalePx(rect.left), ScalePx(rect.top),
                     std::max(10, ScalePx(Width(rect))), std::max(10, ScalePx(Height(rect))),
                     SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    };

    const bool modules = ModulesView();
    place(g.searchEdit, g.layout.searchRect, modules);
    place(g.outputEdit, g.layout.outputEdit, modules);
    place(g.previewEdit, g.layout.previewOutputEdit, g.nav == NAV_REPORTS);
}

void UpdateOutputText() {
    if (!g.outputEdit) return;
    std::wstring text;
    const Module* module = FindModuleById(g.selectedModule);
    if (g.busy && g.runningModuleId == g.selectedModule && module) {
        text = L"Trwa wykonywanie modułu diagnostycznego…\r\n";
        text += L"Moduł:    " + std::wstring(module->name) + L"\r\n";
        text += L"Start:    " + g.runningStarted + L"\r\n";
        text += L"Polecenie: " + (module->command && *module->command ? std::wstring(module->command) : L"(moduł natywny)") + L"\r\n";
        text += L"\r\nWynik pojawi się automatycznie po zakończeniu. Możesz anulować operację przyciskiem obok.\r\n";
    } else {
        auto it = g.lastOutput.find(g.selectedModule);
        if (it != g.lastOutput.end() && !it->second.empty()) {
            text = it->second;
        } else if (module) {
            text = L"Moduł jest gotowy do uruchomienia.\r\n\r\n";
            text += L"Opis: " + std::wstring(module->description ? module->description : L"") + L"\r\n";
            text += L"Kategoria: " + std::wstring(module->category) + L"  ·  ID: " + std::to_wstring(module->id) + L"\r\n";
            if (module->requiresAdmin) text += L"Uprawnienia: może wymagać administratora\r\n";
            if (module->destructive) text += L"Uwaga: operacja zmienia konfigurację systemu\r\n";
            text += L"\r\nKliknij „Uruchom moduł”, aby wykonać diagnostykę.\r\n";
        } else {
            text = L"Wybierz moduł z listy po lewej stronie.\r\n";
        }
    }
    if (text.size() > 600000) {
        text.resize(600000);
        text += L"\r\n\r\n[Podgląd skrócony — pełny wynik znajduje się w raporcie.]\r\n";
    }
    SetWindowTextW(g.outputEdit, text.c_str());
    SendMessageW(g.outputEdit, EM_SETSEL, 0, 0);
    SendMessageW(g.outputEdit, EM_SCROLLCARET, 0, 0);
}

void UpdateReportPreviewText() {
    if (!g.previewEdit) return;
    std::wstring text;
    if (g.selectedReport >= 0 && g.selectedReport < static_cast<int>(g.reports.size())) {
        const Report& report = g.reports[static_cast<size_t>(g.selectedReport)];
        text = BuildReportText(report);
        if (text.size() > 600000) {
            text.resize(600000);
            text += L"\r\n\r\n[Podgląd skrócony — pełny raport znajduje się w pliku.]\r\n";
        }
    } else {
        text = L"Brak wybranego raportu.\r\n\r\nUruchom dowolny moduł diagnostyczny, aby utworzyć raport.\r\n";
    }
    SetWindowTextW(g.previewEdit, text.c_str());
    SendMessageW(g.previewEdit, EM_SETSEL, 0, 0);
    SendMessageW(g.previewEdit, EM_SCROLLCARET, 0, 0);
}

void ActivateNav(int index) {
    if (index < 0 || index >= NAV_COUNT) return;
    g.nav = index;
    g.hoverNav = -1;
    g.hoverChip = -1;
    g.hoverModule = -1;
    g.hoverReport = -1;
    g.hoverButton = HB_NONE;
    g.searchValue.clear();
    if (g.searchEdit) SetWindowTextW(g.searchEdit, L"");
    if (ModulesView()) {
        RebuildFiltered();
        EnsureSelection();
        g.listScroll = 0;
        UpdateOutputText();
    }
    if (index == NAV_REPORTS) {
        ClampScroll();
        UpdateReportPreviewText();
    }
    RecalculateLayout();
    UpdateControlPositions();
    UpdateAnimTimer();
    InvalidateAll();
}

std::vector<RECT> ButtonRects() {
    std::vector<RECT> rects;
    if (g.nav == NAV_DASHBOARD) {
        for (const RECT& rect : g.layout.dashQuickItems) rects.push_back(rect);
        for (const RECT& rect : g.layout.dashHistoryRows) rects.push_back(rect);
    } else if (g.nav == NAV_ABOUT) {
        rects.push_back(g.layout.aboutAdmin);
        rects.push_back(g.layout.aboutReports);
    } else if (ModulesView()) {
        rects.push_back(g.layout.runButton);
        rects.push_back(g.layout.exportButton);
    } else if (g.nav == NAV_REPORTS) {
        rects.push_back(g.layout.previewExport);
        rects.push_back(g.layout.previewFolder);
    } else if (g.nav == NAV_SETTINGS) {
        rects.push_back(g.layout.refreshSeg[0]);
        rects.push_back(g.layout.refreshSeg[1]);
        rects.push_back(g.layout.refreshSeg[2]);
        rects.push_back(g.layout.animToggle);
        rects.push_back(g.layout.autoSaveToggle);
        rects.push_back(g.layout.openReportsButton);
        rects.push_back(g.layout.clearHistoryButton);
        rects.push_back(g.layout.checkAdminButton);
        rects.push_back(g.layout.aboutButton);
    }
    return rects;
}

int FindButtonAt(POINT point) {
    if (g.nav == NAV_DASHBOARD) {
        for (int i = 0; i < 3; ++i) {
            if (Contains(g.layout.dashQuickItems[i], point)) return HB_QUICK_DIAGNOSTICS + i;
        }
        for (int i = 0; i < 4; ++i) {
            if (Contains(g.layout.dashHistoryRows[i], point)) return HB_HISTORY_0 + i;
        }
    } else if (g.nav == NAV_ABOUT) {
        if (Contains(g.layout.aboutAdmin, point)) return HB_ABOUT_ADMIN;
        if (Contains(g.layout.aboutReports, point)) return HB_ABOUT_REPORTS;
    } else if (ModulesView()) {
        if (Contains(g.layout.runButton, point)) return HB_RUN;
        if (Contains(g.layout.exportButton, point)) return HB_EXPORT_MODULE;
    } else if (g.nav == NAV_REPORTS) {
        if (Contains(g.layout.previewExport, point)) return HB_EXPORT_REPORT;
        if (Contains(g.layout.previewFolder, point)) return HB_OPEN_REPORTS;
    } else if (g.nav == NAV_SETTINGS) {
        for (int i = 0; i < 3; ++i) if (Contains(g.layout.refreshSeg[i], point)) return HB_REFRESH_1 + i;
        if (Contains(g.layout.animToggle, point)) return HB_ANIM_TOGGLE;
        if (Contains(g.layout.autoSaveToggle, point)) return HB_AUTOSAVE_TOGGLE;
        if (Contains(g.layout.openReportsButton, point)) return HB_OPEN_REPORTS;
        if (Contains(g.layout.clearHistoryButton, point)) return HB_CLEAR_HISTORY;
        if (Contains(g.layout.checkAdminButton, point)) return HB_ADMIN;
        if (Contains(g.layout.aboutButton, point)) return HB_ABOUT;
    }
    return HB_NONE;
}

bool ButtonDisabled(int button) {
    switch (button) {
    case HB_RUN:
        if (g.selectedModule == 0) return true;
        if (g.busy && g.runningModuleId != g.selectedModule) return true;
        return false;
    case HB_EXPORT_MODULE:
        return ReportForModule(g.selectedModule) == nullptr && g.lastOutput.find(g.selectedModule) == g.lastOutput.end();
    case HB_EXPORT_REPORT:
        return g.selectedReport < 0 || g.selectedReport >= static_cast<int>(g.reports.size());
    case HB_CLEAR_HISTORY:
        return g.reports.empty();
    case HB_QUICK_DIAGNOSTICS:
        return g.busy;
    case HB_HISTORY_0:
    case HB_HISTORY_1:
    case HB_HISTORY_2:
    case HB_HISTORY_3:
        return (button - HB_HISTORY_0) >= static_cast<int>(g.reports.size());
    default:
        return false;
    }
}

void OnButtonClicked(int button) {
    switch (button) {
    case HB_RUN:
        if (g.busy) {
            if (g.runningModuleId == g.selectedModule) CancelRunningModule();
        } else {
            StartModule(g.selectedModule);
        }
        break;
    case HB_EXPORT_MODULE: {
        const Report* report = ReportForModule(g.selectedModule);
        if (report) {
            ExportReportDialog(*report);
        } else {
            auto it = g.lastOutput.find(g.selectedModule);
            if (it != g.lastOutput.end()) {
                const Module* module = FindModuleById(g.selectedModule);
                Report manual;
                manual.moduleId = g.selectedModule;
                manual.moduleName = module ? module->name : L"Moduł";
                manual.category = module ? module->category : L"";
                manual.output = it->second;
                manual.startedAt = FullTimestamp();
                manual.seconds = g.runs[g.selectedModule].seconds;
                manual.exitCode = g.runs[g.selectedModule].exitCode;
                ExportReportDialog(manual);
            } else {
                MessageBoxW(g.hwnd, L"Najpierw uruchom wybrany moduł, aby wyeksportować wynik.", L"Brak wyniku", MB_OK | MB_ICONINFORMATION);
            }
        }
        break;
    }
    case HB_EXPORT_REPORT:
        if (g.selectedReport >= 0 && g.selectedReport < static_cast<int>(g.reports.size())) {
            ExportReportDialog(g.reports[static_cast<size_t>(g.selectedReport)]);
        }
        break;
    case HB_OPEN_REPORTS:
        OpenReportsFolder();
        break;
    case HB_CLEAR_HISTORY:
        if (g.reports.empty()) break;
        if (MessageBoxW(g.hwnd, L"Usunąć całą historię raportów z tej sesji?\r\nPliki zapisane na dysku pozostaną bez zmian.",
                        L"Wyczyść historię", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
            g.reports.clear();
            g.selectedReport = -1;
            g.reportScroll = 0;
            UpdateReportPreviewText();
            ClampScroll();
            InvalidateAll();
        }
        break;
    case HB_ANIM_TOGGLE:
        g.settings.animations = !g.settings.animations;
        SaveSettings();
        UpdateAnimTimer();
        InvalidateLogical(g.layout.settings1);
        break;
    case HB_AUTOSAVE_TOGGLE:
        g.settings.autoSaveReports = !g.settings.autoSaveReports;
        SaveSettings();
        InvalidateLogical(g.layout.settings2);
        break;
    case HB_REFRESH_1:
    case HB_REFRESH_2:
    case HB_REFRESH_5: {
        const int seconds = (button == HB_REFRESH_1) ? 1 : (button == HB_REFRESH_2 ? 2 : 5);
        g.settings.refreshSeconds = seconds;
        g.refreshSeconds.store(seconds);
        SaveSettings();
        if (g_wakeEvent) SetEvent(g_wakeEvent);
        InvalidateLogical(g.layout.settings1);
        break;
    }
    case HB_ADMIN:
        StartModule(112);
        break;
    case HB_ABOUT:
        ActivateNav(NAV_ABOUT);
        break;
    case HB_QUICK_DIAGNOSTICS:
        ActivateNav(NAV_DIAGNOSTICS);
        break;
    case HB_QUICK_MODULES:
        ActivateNav(NAV_MODULES);
        break;
    case HB_QUICK_REPORTS:
        ActivateNav(NAV_REPORTS);
        break;
    case HB_HISTORY_0:
    case HB_HISTORY_1:
    case HB_HISTORY_2:
    case HB_HISTORY_3: {
        const int index = button - HB_HISTORY_0;
        if (index >= 0 && index < static_cast<int>(g.reports.size())) {
            ActivateNav(NAV_REPORTS);
            g.selectedReport = index;
            g.reportScroll = std::max(0, index - 2);
            ClampScroll();
            UpdateReportPreviewText();
            InvalidateAll();
        }
        break;
    }
    case HB_ABOUT_ADMIN:
        StartModule(112);
        break;
    case HB_ABOUT_REPORTS:
        OpenReportsFolder();
        break;
    default:
        break;
    }
}

void DrawModulesView(Graphics& graphics) {
    {
    PerfSpan listSpan(&g_perf.sectionList);
    for (int i = 0; i < g.layout.chipCount && i < 9; ++i) {
        const bool active = (i == 0) ? (g.nav == NAV_DIAGNOSTICS || g.nav == NAV_MODULES)
                                     : (g.nav == NAV_SYSTEM + i - 1);
        DrawChip(graphics, g.layout.chips[i], kChipLabels[i], active, g.hoverChip == i, 9.0f);
    }

    const RECT& listPanel = g.layout.listPanel;
    DrawGlassPanel(graphics, listPanel, 18.0f);
    DrawLabel(graphics, L"MODUŁY", MakeRect(listPanel.left + 18, listPanel.top + 17, 120, 22), 9.8f, C_MUTED, true);
    const std::wstring countText = std::to_wstring(g.filtered.size()) + L" / " + std::to_wstring(kModuleCount);
    DrawLabel(graphics, countText, MakeRect(listPanel.right - 88, listPanel.top + 17, 70, 22), 8.4f, C_TEXT, true, StringAlignmentFar);

    const RunState currentState = g.busy ? RunState::Running : RunState::Idle;
    (void)currentState;

    if (g.filtered.empty()) {
        RECT empty = MakeRect(listPanel.left + 30, listPanel.top + 120, Width(listPanel) - 60, 120);
        DrawVectorIcon(graphics, Icon::Search, RectF(static_cast<REAL>(empty.left + Width(empty) / 2 - 16), static_cast<REAL>(empty.top), 32.0f, 32.0f), C_DIM, 1.8f);
        DrawLabel(graphics, L"Brak pasujących modułów", MakeRect(empty.left, empty.top + 44, Width(empty), 20), 11.0f, C_TEXT, true, StringAlignmentCenter);
        DrawLabel(graphics, L"Zmień frazę wyszukiwania lub wybierz inną kategorię.", MakeRect(empty.left, empty.top + 66, Width(empty), 18), 8.4f, C_DIM, false, StringAlignmentCenter);
    }

    const int visibleRows = std::max(1, Height(g.layout.listArea) / 66 + 1);
    for (int i = g.listScroll; i < static_cast<int>(g.filtered.size()) && i < g.listScroll + visibleRows; ++i) {
        const Module* module = g.filtered[static_cast<size_t>(i)];
        RECT row = ModuleRowRect(i);
        if (row.bottom > g.layout.listArea.bottom + 2) break;
        const bool selected = module->id == g.selectedModule;
        const bool hovered = g.hoverModule == i;
        if (selected) {
            FillRounded(graphics, row, 12.0f, Color(44, 129, 212, 250));
            StrokeRounded(graphics, row, 12.0f, WithAlpha(C_LIGHT_BLUE, 104), 1.0f);
        } else if (hovered) {
            FillRounded(graphics, row, 12.0f, Color(22, 55, 71, 79));
        }

        auto runIt = g.runs.find(module->id);
        const RunState state = (g.busy && g.runningModuleId == module->id) ? RunState::Running
                              : (runIt != g.runs.end() ? runIt->second.state : RunState::Idle);
        const Color stateColor = RunStateColor(state);
        SolidBrush dot(stateColor);
        graphics.FillEllipse(&dot, static_cast<REAL>(row.left + 14), static_cast<REAL>(row.top + 17), 7.0f, 7.0f);
        if (state == RunState::Running) {
            Pen ringPen(WithAlpha(stateColor, 80), 1.4f);
            graphics.DrawEllipse(&ringPen, static_cast<REAL>(row.left + 12), static_cast<REAL>(row.top + 15), 11.0f, 11.0f);
        }

        const int nameWidth = Width(row) - 28 - 100;
        DrawLabel(graphics, module->name, MakeRect(row.left + 28, row.top + 7, nameWidth, 18), 9.8f,
                  selected ? C_INK : C_TEXT, true);
        DrawLabel(graphics, module->description ? module->description : L"",
                  MakeRect(row.left + 28, row.top + 26, Width(row) - 40, 14), 8.0f, selected ? C_TEXT : C_MUTED);
        std::wstring meta = L"ID " + std::to_wstring(module->id) + L"  ·  " + module->category;
        if (module->requiresAdmin) meta += L"  ·  ADMIN";
        if (module->destructive) meta += L"  ·  OPERACJA";
        DrawLabel(graphics, meta, MakeRect(row.left + 28, row.top + 43, Width(row) - 150, 12), 7.2f,
                  module->destructive ? WithAlpha(C_RED, 190) : C_DIM);

        if (runIt != g.runs.end() && state != RunState::Running) {
            DrawLabel(graphics, RunStateText(state), MakeRect(row.right - 92, row.top + 7, 78, 16), 7.4f, stateColor, true, StringAlignmentFar);
            if (runIt->second.seconds > 0.0) {
                DrawLabel(graphics, FormatDuration(runIt->second.seconds), MakeRect(row.right - 92, row.top + 42, 78, 12), 7.2f,
                          C_DIM, false, StringAlignmentFar);
            }
        } else if (state == RunState::Running) {
            DrawLabel(graphics, L"W TOKU", MakeRect(row.right - 92, row.top + 7, 78, 16), 7.4f, C_AMBER, true, StringAlignmentFar);
        }
    }
    DrawScrollbar(graphics, g.layout.listArea, g.listScroll, static_cast<int>(g.filtered.size()), visibleRows);
    }

    // ── Detail ─────────────────────────────────────────────────────────────
    PerfSpan detailSpan(&g_perf.sectionDetail);
    const RECT& panel = g.layout.detailPanel;
    DrawGlassPanel(graphics, panel, 18.0f);
    const Module* module = FindModuleById(g.selectedModule);
    if (!module) {
        DrawVectorIcon(graphics, Icon::Modules, RectF(static_cast<REAL>(panel.left + Width(panel) / 2 - 20), static_cast<REAL>(panel.top + 90), 40.0f, 40.0f), C_DIM, 1.8f);
        DrawLabel(graphics, L"Wybierz moduł diagnostyczny", MakeRect(panel.left, panel.top + 144, Width(panel), 22), 13.0f, C_TEXT, true, StringAlignmentCenter);
        DrawLabel(graphics, L"Użyj listy po lewej stronie lub wyszukiwarki w nagłówku.", MakeRect(panel.left, panel.top + 170, Width(panel), 18), 8.6f, C_DIM, false, StringAlignmentCenter);
        return;
    }

    auto runIt = g.runs.find(module->id);
    RunState state = RunState::Idle;
    if (g.busy && g.runningModuleId == module->id) state = RunState::Running;
    else if (runIt != g.runs.end()) state = runIt->second.state;
    {
    PerfSpan headerSpan(&g_perf.sectionDetailHeader);
    DrawLabel(graphics, module->name, g.layout.detailTitle, 14.5f, C_INK, true);
    DrawStatusPill(graphics, g.layout.detailStatus, RunStateText(state), RunStateColor(state), state == RunState::Running && g.settings.animations);

    DrawLabel(graphics, L"MODUŁ #" + std::to_wstring(module->id) + L"   ·   " + module->category,
              g.layout.detailSub, 8.2f, C_MUTED, true);
    DrawLabelWrap(graphics, module->description ? module->description : L"",
                  g.layout.detailDesc, 9.2f, C_TEXT);
    }

    {
    PerfSpan chipsSpan(&g_perf.sectionDetailChips);
    int chipX = g.layout.detailMeta.left;
    auto metaChip = [&](const std::wstring& text, const Color& color) {
        const int width = static_cast<int>(MeasureText(graphics, text, 7.6f, true)) + 22;
        RECT chip = MakeRect(chipX, g.layout.detailMeta.top + 2, width, 20);
        DrawMetaPill(graphics, chip, text, color, 7.6f);
        chipX += width + 6;
    };
    if (module->requiresAdmin) metaChip(L"WYMAGA ADMINA", C_AMBER);
    if (module->destructive) metaChip(L"ZMIENIA SYSTEM", C_RED);
    if (!module->requiresAdmin && !module->destructive) metaChip(L"TYLKO ODCZYT", C_GREEN);
    if (!module->command || !*module->command) metaChip(L"NATYWNY", C_CYAN);

    std::wstring note;
    Color noteColor = C_DIM;
    if (module->requiresAdmin && !g.admin) {
        note = L"Ten moduł może wymagać uprawnień administratora — uruchom aplikację jako administrator.";
        noteColor = C_AMBER;
    } else if (module->destructive) {
        note = L"Operacja zmienia konfigurację systemu — przed uruchomieniem wymagane jest potwierdzenie.";
        noteColor = C_AMBER;
    } else {
        note = L"Moduł nie wymaga podniesionych uprawnień i nie zmienia konfiguracji systemu.";
        noteColor = C_GREEN;
    }
    DrawLabel(graphics, note, g.layout.detailNote, 8.2f, noteColor);
    }

    {
    PerfSpan runSpan(&g_perf.sectionDetailRun);
    const bool runningThis = g.busy && g.runningModuleId == module->id;
    const bool runDisabled = g.busy && !runningThis;
    const std::wstring runLabel = runningThis ? L"Anuluj diagnostykę" : (runDisabled ? L"Diagnostyka w toku" : L"Uruchom moduł");
    DrawButton(graphics, g.layout.runButton, runLabel, runningThis ? Icon::Close : Icon::Play,
               !runningThis && !runDisabled, g.hoverButton == HB_RUN && !runDisabled, runDisabled, runningThis, 10.0f);
    }

    {
    PerfSpan outputSpan(&g_perf.sectionDetailOutput);
    {
    PerfSpan outTextSpan(&g_perf.sectionDetailOutText);
    if (runIt != g.runs.end()) {
        const RunInfo& info = runIt->second;
        DrawLabel(graphics, L"OSTATNIE URUCHOMIENIE", MakeRect(g.layout.detailInfo.left, g.layout.detailInfo.top, Width(g.layout.detailInfo), 13), 7.0f, C_DIM, true);
        std::wstring line = info.finishedAt + L"  ·  " + FormatDuration(info.seconds) + L"  ·  kod " + std::to_wstring(info.exitCode);
        DrawLabel(graphics, line, MakeRect(g.layout.detailInfo.left, g.layout.detailInfo.top + 15, Width(g.layout.detailInfo), 18), 8.4f, C_TEXT);
    } else {
        DrawLabel(graphics, L"Moduł nie był jeszcze uruchamiany w tej sesji.", g.layout.detailInfo, 8.4f, C_DIM);
    }

    DrawLabel(graphics, L"WYNIK DIAGNOSTYKI", MakeRect(g.layout.outputLabel.left, g.layout.outputLabel.top, 200, 18), 8.6f, C_MUTED, true);
    if (runIt != g.runs.end()) {
        const std::wstring right = L"Czas " + FormatDuration(runIt->second.seconds) + L"  ·  kod " + std::to_wstring(runIt->second.exitCode);
        DrawLabel(graphics, right, MakeRect(g.layout.outputLabel.right - 280, g.layout.outputLabel.top, 280, 18), 7.8f,
                  RunStateColor(runIt->second.state), false, StringAlignmentFar);
    }

    }
    if (!DrawCachedOutputPanel(graphics, g.layout.outputPanel)) {
        PerfSpan outFillSpan(&g_perf.sectionDetailOutFill);
        FillRounded(graphics, g.layout.outputPanel, 12.0f, WithAlpha(C_OUTPUT_BG, 244));
        PerfSpan outStrokeSpan(&g_perf.sectionDetailOutStroke);
        StrokeRounded(graphics, g.layout.outputPanel, 12.0f, WithAlpha(C_LIGHT_BLUE, 42), 1.0f);
    }

    const bool hasOutput = g.lastOutput.find(module->id) != g.lastOutput.end();
    DrawButton(graphics, g.layout.exportButton, L"Eksportuj raport", Icon::Download, false,
               g.hoverButton == HB_EXPORT_MODULE, !hasOutput && ReportForModule(module->id) == nullptr, false, 9.6f);
    }
}

void DrawReportsView(Graphics& graphics) {
    const RECT& listPanel = g.layout.reportListPanel;
    DrawGlassPanel(graphics, listPanel, 18.0f);
    DrawLabel(graphics, L"RAPORTY", MakeRect(listPanel.left + 18, listPanel.top + 17, 140, 22), 9.8f, C_MUTED, true);
    DrawLabel(graphics, std::to_wstring(g.reports.size()), MakeRect(listPanel.right - 60, listPanel.top + 17, 42, 22), 8.6f, C_TEXT, true, StringAlignmentFar);

    const int visibleRows = std::max(1, Height(g.layout.reportListArea) / 62 + 1);
    if (g.reports.empty()) {
        DrawVectorIcon(graphics, Icon::Report, RectF(static_cast<REAL>(listPanel.left + Width(listPanel) / 2 - 18), static_cast<REAL>(listPanel.top + 120), 36.0f, 36.0f), C_DIM, 1.7f);
        DrawLabel(graphics, L"Brak raportów", MakeRect(listPanel.left + 20, listPanel.top + 170, Width(listPanel) - 40, 20), 11.0f, C_TEXT, true, StringAlignmentCenter);
        DrawLabel(graphics, L"Uruchom dowolny moduł, aby utworzyć pierwszy raport.", MakeRect(listPanel.left + 20, listPanel.top + 192, Width(listPanel) - 40, 18), 8.3f, C_DIM, false, StringAlignmentCenter);
    }
    for (int i = g.reportScroll; i < static_cast<int>(g.reports.size()) && i < g.reportScroll + visibleRows; ++i) {
        const Report& report = g.reports[static_cast<size_t>(i)];
        RECT row = ReportRowRect(i);
        if (row.bottom > g.layout.reportListArea.bottom + 2) break;
        const bool selected = g.selectedReport == i;
        if (selected) {
            FillRounded(graphics, row, 12.0f, Color(44, 129, 212, 250));
            StrokeRounded(graphics, row, 12.0f, WithAlpha(C_LIGHT_BLUE, 104), 1.0f);
        } else if (g.hoverReport == i) {
            FillRounded(graphics, row, 12.0f, Color(22, 55, 71, 79));
        }
        const Color stateColor = ReportStatusColor(report);
        SolidBrush dot(stateColor);
        graphics.FillEllipse(&dot, static_cast<REAL>(row.left + 14), static_cast<REAL>(row.top + 16), 7.0f, 7.0f);
        DrawLabel(graphics, report.moduleName, MakeRect(row.left + 28, row.top + 6, Width(row) - 118, 17), 9.4f,
                  selected ? C_INK : C_TEXT, true);
        DrawLabel(graphics, report.category + L"  ·  " + report.startedAt, MakeRect(row.left + 28, row.top + 24, Width(row) - 40, 13), 7.3f, C_DIM);
        DrawLabel(graphics, ReportStatus(report) + L"  ·  " + ReportDurationText(report),
                  MakeRect(row.left + 28, row.top + 39, Width(row) - 40, 12), 7.2f, stateColor);
    }
    DrawScrollbar(graphics, g.layout.reportListArea, g.reportScroll, static_cast<int>(g.reports.size()), visibleRows);

    const RECT& panel = g.layout.previewPanel;
    DrawGlassPanel(graphics, panel, 18.0f);
    if (g.selectedReport < 0 || g.selectedReport >= static_cast<int>(g.reports.size())) {
        DrawVectorIcon(graphics, Icon::Report, RectF(static_cast<REAL>(panel.left + Width(panel) / 2 - 22), static_cast<REAL>(panel.top + 100), 44.0f, 44.0f), C_DIM, 1.8f);
        DrawLabel(graphics, L"Podgląd raportu", MakeRect(panel.left, panel.top + 156, Width(panel), 22), 13.0f, C_TEXT, true, StringAlignmentCenter);
        DrawLabel(graphics, L"Wybierz raport z listy lub uruchom nowy moduł diagnostyczny.", MakeRect(panel.left, panel.top + 182, Width(panel), 18), 8.6f, C_DIM, false, StringAlignmentCenter);
        DrawButton(graphics, g.layout.previewFolder, L"Otwórz folder", Icon::Folder, false,
                   g.hoverButton == HB_OPEN_REPORTS, false, false, 9.6f);
        return;
    }

    const Report& report = g.reports[static_cast<size_t>(g.selectedReport)];
    DrawLabel(graphics, report.moduleName, g.layout.previewTitle, 14.0f, C_INK, true);
    const Color stateColor = ReportStatusColor(report);
    DrawStatusPill(graphics, g.layout.previewStatus, ReportStatus(report), stateColor, g.busy && report.moduleId == g.runningModuleId);
    std::wstring meta = L"ID " + std::to_wstring(report.moduleId) + L"  ·  " + report.category +
                        L"  ·  " + report.startedAt + L"  ·  " + ReportDurationText(report) +
                        L"  ·  kod " + std::to_wstring(report.exitCode);
    DrawLabel(graphics, meta, g.layout.previewMeta, 8.2f, C_MUTED);
    DrawLabel(graphics, L"PEŁNY RAPORT", MakeRect(g.layout.previewOutputLabel.left, g.layout.previewOutputLabel.top, 200, 18), 8.6f, C_MUTED, true);
    DrawLabel(graphics, report.file.empty() ? L"tylko w pamięci" : L"zapisano w Dokumentach",
              MakeRect(g.layout.previewOutputLabel.right - 220, g.layout.previewOutputLabel.top, 220, 18), 7.6f,
              report.file.empty() ? C_DIM : C_GREEN, false, StringAlignmentFar);
    if (!DrawCachedOutputPanel(graphics, g.layout.previewOutputPanel)) {
        FillRounded(graphics, g.layout.previewOutputPanel, 12.0f, WithAlpha(C_OUTPUT_BG, 244));
        StrokeRounded(graphics, g.layout.previewOutputPanel, 12.0f, WithAlpha(C_LIGHT_BLUE, 42), 1.0f);
    }
    DrawButton(graphics, g.layout.previewFolder, L"Otwórz folder", Icon::Folder, false,
               g.hoverButton == HB_OPEN_REPORTS, false, false, 9.6f);
    DrawButton(graphics, g.layout.previewExport, L"Eksportuj raport", Icon::Download, true,
               g.hoverButton == HB_EXPORT_REPORT, false, false, 9.6f);
}

void DrawSettingsView(Graphics& graphics) {
    const RECT& card1 = g.layout.settings1;
    DrawGlassPanel(graphics, card1, 18.0f);
    DrawPanelHeader(graphics, card1, Icon::Gauge, L"MONITOROWANIE", L"", C_MINT);
    DrawLabel(graphics, L"Interwał odświeżania danych pulpitu oraz animacje interfejsu.",
              MakeRect(card1.left + 22, card1.top + 50, Width(card1) - 44, 18), 8.2f, C_MUTED);
    DrawLabel(graphics, L"Częstotliwość próbkowania", MakeRect(card1.left + 22, card1.top + 82, 220, 18), 9.2f, C_TEXT, true);
    for (int i = 0; i < 3; ++i) {
        const int seconds = i == 0 ? 1 : (i == 1 ? 2 : 5);
        const bool active = g.settings.refreshSeconds == seconds;
        DrawChip(graphics, g.layout.refreshSeg[i], std::to_wstring(seconds) + L" s", active, g.hoverButton == HB_REFRESH_1 + i, 9.0f);
    }
    Pen divider(C_LINE_SOFT, 1.0f);
    graphics.DrawLine(&divider, static_cast<REAL>(card1.left + 22), static_cast<REAL>(card1.top + 126),
                      static_cast<REAL>(card1.right - 22), static_cast<REAL>(card1.top + 126));
    DrawLabel(graphics, L"Animacje interfejsu", MakeRect(card1.left + 22, card1.top + 140, 220, 18), 9.2f, C_TEXT, true);
    DrawLabel(graphics, L"Spinner diagnostyki i płynne wskaźniki", MakeRect(card1.left + 22, card1.top + 157, Width(card1) - 110, 16), 8.0f, C_DIM);
    DrawToggleSwitch(graphics, g.layout.animToggle, g.settings.animations, g.hoverButton == HB_ANIM_TOGGLE);

    const RECT& card2 = g.layout.settings2;
    DrawGlassPanel(graphics, card2, 18.0f);
    DrawPanelHeader(graphics, card2, Icon::Report, L"RAPORTY", L"", C_CYAN);
    DrawLabel(graphics, L"Automatyczny zapis", MakeRect(card2.left + 22, card2.top + 52, 220, 18), 9.2f, C_TEXT, true);
    DrawLabel(graphics, L"Każdy wynik zapisywany jest w folderze Dokumentów", MakeRect(card2.left + 22, card2.top + 69, Width(card2) - 110, 16), 8.0f, C_DIM);
    DrawToggleSwitch(graphics, g.layout.autoSaveToggle, g.settings.autoSaveReports, g.hoverButton == HB_AUTOSAVE_TOGGLE);
    graphics.DrawLine(&divider, static_cast<REAL>(card2.left + 22), static_cast<REAL>(card2.top + 96),
                      static_cast<REAL>(card2.right - 22), static_cast<REAL>(card2.top + 96));
    DrawLabel(graphics, L"Folder raportów", MakeRect(card2.left + 22, card2.top + 106, 220, 16), 8.6f, C_MUTED, true);
    FillRounded(graphics, g.layout.settingsPath, 10.0f, Color(28, 20, 32, 37));
    StrokeRounded(graphics, g.layout.settingsPath, 10.0f, C_LINE_SOFT, 1.0f);
    DrawLabel(graphics, g.reportsDir, MakeRect(g.layout.settingsPath.left + 12, g.layout.settingsPath.top,
              Width(g.layout.settingsPath) - 60, Height(g.layout.settingsPath)), 8.0f, C_TEXT);
    DrawVectorIcon(graphics, Icon::Folder, RectF(static_cast<REAL>(g.layout.settingsPath.right - 34),
                   static_cast<REAL>(g.layout.settingsPath.top) + (Height(g.layout.settingsPath) - 16.0f) * 0.5f, 16.0f, 16.0f), C_MUTED, 1.5f);

    DrawButton(graphics, g.layout.openReportsButton, L"Otwórz folder", Icon::Folder, false,
               g.hoverButton == HB_OPEN_REPORTS, false, false, 9.4f);
    DrawButton(graphics, g.layout.clearHistoryButton, L"Wyczyść historię", Icon::Trash, false,
               g.hoverButton == HB_CLEAR_HISTORY, g.reports.empty(), true, 9.4f);
    DrawLabel(graphics, L"Raporty w tej sesji: " + std::to_wstring(g.reports.size()) +
              L"  ·  format TXT / UTF-8, pełna treść wyników.",
              MakeRect(card2.left + 22, card2.bottom - 54, Width(card2) - 44, 16), 7.8f, C_DIM);
    DrawLabel(graphics, L"Pliki pozostają na dysku po wyczyszczeniu historii.",
              MakeRect(card2.left + 22, card2.bottom - 38, Width(card2) - 44, 16), 7.6f, C_DIM);

    const RECT& card3 = g.layout.settings3;
    DrawGlassPanel(graphics, card3, 18.0f);
    DrawPanelHeader(graphics, card3, Icon::Info, L"INFORMACJE", L"", C_SAGE);

    RECT mark = MakeRect(card3.left + 22, card3.top + 56, 56, 56);
    DrawKdMark(graphics, mark);
    DrawLabel(graphics, L"KD Core", MakeRect(mark.right + 16, card3.top + 58, Width(card3) - 120, 24), 13.0f, C_INK, true);
    DrawLabel(graphics, L"Wersja " + std::wstring(kAppVersion), MakeRect(mark.right + 16, card3.top + 82, 220, 16), 8.6f, C_MUTED);
    DrawLabel(graphics, std::to_wstring(kModuleCount) + L" modułów  ·  8 kategorii  ·  " + std::to_wstring(NAV_COUNT) + L" widoków",
              MakeRect(mark.right + 16, card3.top + 98, Width(card3) - 120, 16), 8.2f, C_DIM);

    graphics.DrawLine(&divider, static_cast<REAL>(card3.left + 22), static_cast<REAL>(card3.top + 130),
                      static_cast<REAL>(card3.right - 22), static_cast<REAL>(card3.top + 130));
    DrawLabel(graphics, L"Uprawnienia sesji", MakeRect(card3.left + 22, card3.top + 146, 220, 20), 9.2f, C_TEXT, true);
    RECT adminPill = MakeRect(card3.right - 22 - 150, card3.top + 145, 150, 24);
    DrawStatusPill(graphics, adminPill, g.admin ? L"ADMINISTRATOR" : L"STANDARD", g.admin ? C_GREEN : C_AMBER, false);
    DrawButton(graphics, g.layout.checkAdminButton, L"Sprawdź uprawnienia", Icon::Admin, false,
               g.hoverButton == HB_ADMIN, false, false, 9.2f);
    DrawButton(graphics, g.layout.aboutButton, L"Informacje o KD Core", Icon::Info, false,
               g.hoverButton == HB_ABOUT, false, false, 8.6f);

    int infoTop = card3.top + 232;
    const int lineHeight = 34;
    const std::wstring labels[5] = {L"Silniki danych", L"Interfejs", L"Tryb pracy", L"Baza modułów", L"Folder raportów"};
    const std::wstring values[5] = {
        L"WinAPI · WMI · PDH · DXGI",
        L"Win32 · GDI+ · KD Core UI",
        L"Lokalny, bez połączeń z internetem",
        std::to_wstring(kModuleCount) + L" rzeczywistych modułów",
        g.reportsDir,
    };
    for (int i = 0; i < 5; ++i) {
        const int y = infoTop + i * lineHeight;
        if (y + lineHeight > card3.bottom - 24) break;
        DrawLabel(graphics, labels[i], MakeRect(card3.left + 22, y, 120, 14), 7.4f, C_DIM, true);
        DrawLabel(graphics, values[i], MakeRect(card3.left + 150, y, Width(card3) - 172, 18), 8.4f, C_TEXT);
        if (i < 4) {
            graphics.DrawLine(&divider, static_cast<REAL>(card3.left + 22), static_cast<REAL>(y + lineHeight - 8),
                              static_cast<REAL>(card3.right - 22), static_cast<REAL>(y + lineHeight - 8));
        }
    }
    DrawLabel(graphics, L"Aplikacja nie wysyła danych diagnostycznych do internetu.",
              MakeRect(card3.left + 22, card3.bottom - 32, Width(card3) - 44, 16), 7.6f, C_DIM);
}

void EnsureBuffers(int width, int height) {
    if (width <= 0 || height <= 0) return;
    if (g_memDC && g_memWidth == width && g_memHeight == height) return;
    if (g_memDC) {
        if (g_memOld) SelectObject(g_memDC, g_memOld);
        if (g_memBitmap) DeleteObject(g_memBitmap);
        DeleteDC(g_memDC);
        g_memDC = nullptr;
        g_memBitmap = nullptr;
        g_memOld = nullptr;
    }
    HDC screen = GetDC(g.hwnd);
    g_memDC = CreateCompatibleDC(screen);
    g_memBitmap = CreateCompatibleBitmap(screen, width, height);
    ReleaseDC(g.hwnd, screen);
    if (!g_memDC || !g_memBitmap) {
        if (g_memDC) { DeleteDC(g_memDC); g_memDC = nullptr; }
        if (g_memBitmap) { DeleteObject(g_memBitmap); g_memBitmap = nullptr; }
        return;
    }
    g_memOld = SelectObject(g_memDC, g_memBitmap);
    g_memWidth = width;
    g_memHeight = height;
    g_backgroundSurface.Reset();
    g_backgroundW = 0;
    g_backgroundH = 0;
    g_panelTiles.clear();
    g_panelTileBytes = 0;
}

void PaintWindow(HWND hwnd) {
    PerfFrameGuard perfFrame;
    ++g_perf.sectionFrames;
    PAINTSTRUCT paint{};
    HDC hdc = BeginPaint(hwnd, &paint);
    RECT client{};
    GetClientRect(hwnd, &client);
    EnsureBuffers(client.right, client.bottom);
    if (!g_memDC) {
        FillRect(hdc, &paint.rcPaint, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        EndPaint(hwnd, &paint);
        return;
    }

    RECT update = paint.rcPaint;
    if (update.right <= update.left || update.bottom <= update.top) update = client;
    update.left = std::max<LONG>(update.left, 0);
    update.top = std::max<LONG>(update.top, 0);
    update.right = std::min<LONG>(update.right, client.right);
    update.bottom = std::min<LONG>(update.bottom, client.bottom);
    if (update.right <= update.left || update.bottom <= update.top) update = client;
    g_paintDirty = update;
    g_fullPaint = update.left <= 0 && update.top <= 0 && update.right >= client.right && update.bottom >= client.bottom;
    if (g_fullPaint) ++g_perf.fullPaints; else ++g_perf.partialPaints;
    g_painting = true;

    Graphics graphics(g_memDC);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetPixelOffsetMode(PixelOffsetModeHalf);
    graphics.SetCompositingQuality(CompositingQualityHighQuality);
    graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    EnsureBackground();
    {
        PerfSpan span(&g_perf.sectionBackground);
        if (g_backgroundSurface.dc) {
            BitBlt(g_memDC, update.left, update.top, Width(update), Height(update),
                   g_backgroundSurface.dc, update.left, update.top, SRCCOPY);
        } else {
            HGDIOBJ previousBrush = SelectObject(g_memDC, GetStockObject(DC_BRUSH));
            SetDCBrushColor(g_memDC, RGB(38, 50, 56));
            FillRect(g_memDC, &update, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
            SelectObject(g_memDC, previousBrush);
        }
    }

    GraphicsState state = graphics.Save();
    graphics.ScaleTransform(g_scale, g_scale);
    graphics.SetClip(RectF((static_cast<REAL>(update.left) - 2.0f) / g_scale,
                           (static_cast<REAL>(update.top) - 2.0f) / g_scale,
                           (static_cast<REAL>(Width(update)) + 4.0f) / g_scale,
                           (static_cast<REAL>(Height(update)) + 4.0f) / g_scale),
                     CombineModeReplace);
    {
        PerfSpan span(&g_perf.sectionSidebar);
        if (Intersects(update, DeviceRect(g.layout.sidebar))) DrawSidebar(graphics);
    }
    {
        PerfSpan span(&g_perf.sectionHeader);
        DrawHeader(graphics);
    }

    {
        PerfSpan span(&g_perf.sectionView);
        switch (g.nav) {
        case NAV_DASHBOARD: DrawDashboard(graphics); break;
        case NAV_DIAGNOSTICS:
        case NAV_MODULES:
        case NAV_SYSTEM:
        case NAV_HARDWARE:
        case NAV_DISKS:
        case NAV_NETWORK:
        case NAV_PERFORMANCE:
        case NAV_SECURITY:
        case NAV_DEVICES:
        case NAV_MAINTENANCE: DrawModulesView(graphics); break;
        case NAV_REPORTS: DrawReportsView(graphics); break;
        case NAV_SETTINGS: DrawSettingsView(graphics); break;
        case NAV_ABOUT: DrawAboutView(graphics); break;
        default: break;
        }
    }
    graphics.Restore(state);

    {
        PerfSpan span(&g_perf.sectionBlit);
        BitBlt(hdc, update.left, update.top, Width(update), Height(update),
               g_memDC, update.left, update.top, SRCCOPY);
    }
    g_painting = false;
    EndPaint(hwnd, &paint);
}

void DrawMetaPill(Graphics& graphics, const RECT& rect, const std::wstring& text, const Color& color, float fontSize = 8.2f) {
    if (Width(rect) <= 4) return;
    FillRounded(graphics, rect, static_cast<float>(Height(rect)) * 0.5f, WithAlpha(color, 24));
    StrokeRounded(graphics, rect, static_cast<float>(Height(rect)) * 0.5f, WithAlpha(color, 68), 1.0f);
    DrawLabel(graphics, text, rect, fontSize, color, true, StringAlignmentCenter);
}

void DrawPanelHeader(Graphics& graphics, const RECT& panel, Icon icon, const std::wstring& title,
                     const std::wstring& right, const Color& accent, bool rightPill,
                     const Color& rightColor) {
    RECT badge = MakeRect(panel.left + 18, panel.top + 16, 30, 30);
    DrawIconBadge(graphics, badge, icon, accent, 7.0f);
    DrawLabel(graphics, title, MakeRect(badge.right + 10, panel.top + 16, Width(panel) - 130, 30), 9.6f, C_MUTED, true);
    if (right.empty()) return;
    const float textWidth = MeasureText(graphics, right, 8.2f, true) + 24.0f;
    int width = static_cast<int>(textWidth);
    width = std::min(width, std::max(70, Width(panel) / 2));
    RECT rightRect = MakeRect(panel.right - 18 - width, panel.top + 20, width, 24);
    if (rightPill) DrawMetaPill(graphics, rightRect, right, rightColor);
    else DrawLabel(graphics, right, rightRect, 8.4f, rightColor, true, StringAlignmentFar);
}

// Shared KD Core mark: graphite tile, restrained facet, white KD, blue pulse.
// Rendered once per size into a tiny ARGB bitmap, then blitted like the cached
// chrome tiles. This keeps the richer mark effectively free on repaint.
namespace {
std::map<std::pair<int, int>, std::unique_ptr<Bitmap>>& KdMarkCache() {
    static std::map<std::pair<int, int>, std::unique_ptr<Bitmap>> cache;
    return cache;
}

void PaintKdMark(Graphics& graphics, const RectF& r) {
    const REAL w = r.Width;
    const REAL h = r.Height;
    const REAL radius = std::max<REAL>(3.0f, w * 0.235f);

    GraphicsPath tile;
    AppendRoundedPath(tile, RectF(r.X, r.Y, w, h), radius);
    SolidBrush tileBrush(C_GRAPHITE);
    graphics.FillPath(&tileBrush, &tile);

    GraphicsState state = graphics.Save();
    graphics.SetClip(&tile, CombineModeIntersect);
    SolidBrush facetBrush(C_GRAPHITE_LIGHT);
    const PointF facet[] = {
        PointF(r.X, r.Y),
        PointF(r.X + w * 0.62f, r.Y),
        PointF(r.X, r.Y + h * 0.62f),
    };
    graphics.FillPolygon(&facetBrush, facet, 3);
    graphics.Restore(state);

    Pen outline(WithAlpha(C_LIGHT_BLUE, 96), std::max<REAL>(1.0f, w * 0.006f));
    graphics.DrawPath(&outline, &tile);

    const int labelHeight = std::max(10, static_cast<int>(h * 0.60f));
    RECT label = MakeRect(static_cast<int>(r.X),
                          static_cast<int>(r.Y + h * 0.06f),
                          static_cast<int>(w), labelHeight);
    DrawLabel(graphics, L"KD", label, std::max<float>(8.0f, w * 0.40f),
              C_ICE_WHITE, true, StringAlignmentCenter);

    const PointF pulse[] = {
        PointF(r.X + w * 0.14f, r.Y + h * 0.78f),
        PointF(r.X + w * 0.30f, r.Y + h * 0.78f),
        PointF(r.X + w * 0.39f, r.Y + h * 0.60f),
        PointF(r.X + w * 0.48f, r.Y + h * 0.88f),
        PointF(r.X + w * 0.58f, r.Y + h * 0.52f),
        PointF(r.X + w * 0.68f, r.Y + h * 0.78f),
        PointF(r.X + w * 0.86f, r.Y + h * 0.78f),
    };
    Pen pulsePen(C_LIGHT_BLUE, std::max<REAL>(1.0f, w * 0.035f));
    pulsePen.SetLineJoin(LineJoinRound);
    graphics.DrawLines(&pulsePen, pulse, 7);
}
}  // namespace

void DrawKdMark(Graphics& graphics, RECT rect) {
    if (Width(rect) <= 0 || Height(rect) <= 0) return;

    const int w = Width(rect);
    const int h = Height(rect);
    const auto key = std::make_pair(w, h);
    auto& cache = KdMarkCache();
    auto it = cache.find(key);
    if (it == cache.end()) {
        constexpr int kSupersample = 3;
        auto hi = std::make_unique<Bitmap>(w * kSupersample, h * kSupersample,
                                           PixelFormat32bppARGB);
        {
            Graphics buffer(hi.get());
            buffer.Clear(Color(0, 0, 0, 0));
            buffer.SetSmoothingMode(SmoothingModeHighQuality);
            buffer.SetCompositingQuality(CompositingQualityHighQuality);
            buffer.SetInterpolationMode(InterpolationModeHighQualityBicubic);
            PaintKdMark(buffer, RectF(0.0f, 0.0f,
                                      static_cast<REAL>(hi->GetWidth()),
                                      static_cast<REAL>(hi->GetHeight())));
        }
        auto bitmap = std::make_unique<Bitmap>(w, h, PixelFormat32bppARGB);
        Graphics destination(bitmap.get());
        destination.Clear(Color(0, 0, 0, 0));
        destination.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        destination.DrawImage(hi.get(), 0, 0, w, h);
        it = cache.emplace(key, std::move(bitmap)).first;
    }

    graphics.DrawImage(it->second.get(), rect.left, rect.top, w, h);
}

void EnsureBackground() {
    if (g_memWidth <= 0 || g_memHeight <= 0) return;
    if (g_backgroundSurface.dc && g_backgroundW == g_memWidth && g_backgroundH == g_memHeight) return;
    if (!g_backgroundSurface.Create(g_memWidth, g_memHeight)) return;
    g_backgroundW = g_memWidth;
    g_backgroundH = g_memHeight;
    g_panelTiles.clear();
    g_panelTileBytes = 0;
    Graphics graphics(g_backgroundSurface.dc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetCompositingQuality(CompositingQualityHighQuality);
    graphics.ScaleTransform(g_scale, g_scale);
    const float width = static_cast<float>(g_memWidth) / g_scale;
    const float height = static_cast<float>(g_memHeight) / g_scale;

    // KD Core: one clean graphite gradient, one restrained light-blue tint.
    LinearGradientBrush base(RectF(0, 0, width, height), C_GRAPHITE, C_GRAPHITE_LIGHT,
                             LinearGradientModeForwardDiagonal);
    graphics.FillRectangle(&base, RectF(0, 0, width, height));

    LinearGradientBrush tint(RectF(0, height * 0.45f, width, height * 0.55f),
                             Color(0, 129, 212, 250), Color(12, 129, 212, 250),
                             LinearGradientModeVertical);
    graphics.FillRectangle(&tint, RectF(0, height * 0.45f, width, height * 0.55f));

    Pen topLine(WithAlpha(C_LIGHT_BLUE, 56), 1.0f);
    graphics.DrawLine(&topLine, 0.0f, 0.5f, width, 0.5f);
}

void DrawSidebar(Graphics& graphics) {
    const RECT& area = g.layout.sidebar;
    RECT panel = MakeRect(area.left + 12, 12, Width(area) - 24, Height(area) - 24);
    DrawGlassPanel(graphics, panel, 22.0f);

    RECT mark = MakeRect(panel.left + 18, panel.top + 18, 38, 38);
    DrawKdMark(graphics, mark);

    DrawLabel(graphics, L"KD CORE", MakeRect(mark.right + 11, panel.top + 17, 96, 18), 11.5f, C_INK, true);
    DrawLabel(graphics, L"DIAGNOSTYKA SYSTEMU", MakeRect(mark.right + 11, panel.top + 35, 112, 16), 6.9f, C_MUTED, true);
    RECT versionPill = MakeRect(panel.right - 18 - 40, panel.top + 25, 40, 20);
    DrawMetaPill(graphics, versionPill, L"2.0", C_LIGHT_BLUE, 7.6f);

    Pen divider(C_LINE_SOFT, 1.0f);
    graphics.DrawLine(&divider, static_cast<REAL>(panel.left + 18), static_cast<REAL>(panel.top + 74),
                      static_cast<REAL>(panel.right - 18), static_cast<REAL>(panel.top + 74));

    {
    PerfSpan menuSpan(&g_perf.sectionMenu);
    for (int i = 0; i < NAV_COUNT; ++i) {
        const RECT& item = g.layout.nav[i];
        const bool active = (g.nav == i);
        const bool hovered = (g.hoverNav == i);
        if (active) {
            FillRounded(graphics, item, 12.0f, Color(44, 129, 212, 250));
            StrokeRounded(graphics, item, 12.0f, WithAlpha(C_LIGHT_BLUE, 80), 1.0f);
            RECT accent = MakeRect(item.left + 5, item.top + 13, 3, Height(item) - 26);
            FillRounded(graphics, accent, 1.5f, C_LIGHT_BLUE);
        } else if (hovered) {
            FillRounded(graphics, item, 12.0f, Color(22, 55, 71, 79));
        }

        const Color iconColor = active ? C_LIGHT_BLUE : WithAlpha(C_TEXT, 205);
        DrawVectorIcon(graphics, IconForNav(i),
                       RectF(static_cast<REAL>(item.left + 15), static_cast<REAL>(item.top) + 11.0f, 19.0f, 19.0f),
                       iconColor, 1.55f);
        const std::wstring label = kNavLabels[i];
        RECT labelRect = MakeRect(item.left + 45, item.top, Width(item) - 86, Height(item));
        DrawLabel(graphics, label, labelRect, 10.2f, active ? C_INK : (hovered ? C_INK : C_TEXT), active);

        if (i >= NAV_SYSTEM && i <= NAV_MAINTENANCE) {
            const int count = CategoryCount(kCategories[i - NAV_SYSTEM]);
            RECT countRect = MakeRect(item.right - 38, item.top, 26, Height(item));
            DrawLabel(graphics, std::to_wstring(count), countRect, 8.4f,
                      active ? WithAlpha(C_BUTTON_TEXT, 190) : C_DIM, true, StringAlignmentFar);
        }
    }
    }

    const RECT& footer = g.layout.sidebarFooter;
    RECT admin = MakeRect(footer.left + 2, footer.top, 118, 26);
    const Color adminColor = g.admin ? C_GREEN : C_AMBER;
    FillRounded(graphics, admin, 13.0f, WithAlpha(adminColor, 24));
    StrokeRounded(graphics, admin, 13.0f, WithAlpha(adminColor, 62), 1.0f);
    DrawVectorIcon(graphics, g.admin ? Icon::Check : Icon::Admin,
                   RectF(static_cast<REAL>(admin.left + 9), static_cast<REAL>(admin.top + 7), 12.0f, 12.0f), adminColor, 1.6f);
    DrawLabel(graphics, g.admin ? L"ADMINISTRATOR" : L"STANDARD", MakeRect(admin.left + 25, admin.top, Width(admin) - 30, Height(admin)),
              7.4f, adminColor, true);

    DrawLabel(graphics, std::wstring(L"v") + kAppVersion + L"  ·  " + std::to_wstring(kModuleCount) + L" modułów",
              MakeRect(footer.left + 2, footer.top + 34, Width(footer) - 4, 16), 7.8f, C_TEXT, true);
    DrawLabel(graphics, L"8 kategorii · 4 silniki danych",
              MakeRect(footer.left + 2, footer.top + 48, Width(footer) - 4, 14), 7.3f, C_DIM);
    DrawLabel(graphics, L"WinAPI · WMI · PDH · DXGI",
              MakeRect(footer.left + 2, footer.top + 61, Width(footer) - 4, 13), 7.1f, C_DIM);
}

void DrawHeader(Graphics& graphics) {
    DrawLabel(graphics, kNavTitles[g.nav], g.layout.headerTitle, 22.0f, C_INK, true);
    std::wstring subtitle;
    if (g.nav == NAV_DASHBOARD) {
        subtitle = L"Monitorowanie systemu w czasie rzeczywistym";
        if (!g.stats.sampleTime.empty()) subtitle += L"  ·  próbka " + g.stats.sampleTime;
    } else if (ModulesView()) {
        subtitle = std::to_wstring(kModuleCount) + L" modułów  ·  8 kategorii  ·  wyszukiwanie w czasie rzeczywistym";
    } else if (g.nav == NAV_REPORTS) {
        subtitle = std::to_wstring(g.reports.size()) + L" raportów w tej sesji  ·  automatyczny zapis do Dokumentów";
    } else {
        subtitle = L"Personalizacja monitorowania, raportów i uprawnień";
    }
    DrawLabel(graphics, subtitle, g.layout.headerSub, 9.0f, C_MUTED);

    const Color statusColor = g.busy ? C_AMBER : (g.lastRunError ? C_RED : C_GREEN);
    std::wstring statusText = g.busy ? L"DIAGNOSTYKA" : (g.lastRunError ? L"OSTATNI BŁĄD" : L"SYSTEM GOTOWY");
    DrawStatusPill(graphics, g.layout.headerStatus, statusText, statusColor, g.busy && g.settings.animations);

    if (ModulesView()) {
        const RECT& box = g.layout.headerSearch;
        FillRounded(graphics, box, 12.0f, Color(28, 20, 32, 37));
        StrokeRounded(graphics, box, 12.0f, C_LINE, 1.0f);
        DrawVectorIcon(graphics, Icon::Search,
                       RectF(static_cast<REAL>(box.left + 14), static_cast<REAL>(box.top) + 11.0f, 16.0f, 16.0f),
                       C_MUTED, 1.6f);
    }
}

void DrawDashboard(Graphics& graphics) {
    const Snapshot& snapshot = g.stats;
    const std::wstring unknown = L"n/d";
    const Color statusColor = g.busy ? C_AMBER : (g.lastRunError ? C_RED : C_GREEN);
    const std::wstring statusText = g.busy ? L"DIAGNOSTYKA" : (g.lastRunError ? L"OSTATNI BŁĄD" : L"SYSTEM GOTOWY");

    // ── Welcome ────────────────────────────────────────────────────────────
    const RECT& welcome = g.layout.dashWelcome;
    DrawGlassPanel(graphics, welcome, 18.0f);
    const RECT& mark = g.layout.dashWelcomeLogo;
    DrawKdMark(graphics, mark);

    const int textX = mark.right + 12;
    const int textW = std::max(40, static_cast<int>(welcome.right - 16 - textX));
    DrawLabel(graphics, L"KD Core", MakeRect(textX, welcome.top + 14, textW, 22), 13.5f, C_INK, true);
    DrawLabel(graphics, L"DIAGNOSTYKA SYSTEMU WINDOWS", MakeRect(textX, welcome.top + 37, textW, 16),
              7.2f, C_MUTED, true);
    DrawLabel(graphics, std::to_wstring(kModuleCount) + L" modułów  ·  8 kategorii  ·  wersja " + kAppVersion,
              MakeRect(welcome.left + 16, welcome.bottom - 50, Width(welcome) - 32, 16), 8.0f, C_TEXT);
    DrawStatusPill(graphics, g.layout.dashWelcomeStatus, statusText, statusColor,
                   g.busy && g.settings.animations);

    // ── Quick access ───────────────────────────────────────────────────────
    const RECT& quick = g.layout.dashQuick;
    DrawGlassPanel(graphics, quick, 18.0f);
    DrawLabel(graphics, L"SZYBKI DOSTĘP", MakeRect(quick.left + 12, quick.top + 11,
              Width(quick) - 24, 16), 8.8f, C_MUTED, true);
    DrawButton(graphics, g.layout.dashQuickItems[0], L"Uruchom diagnostykę", Icon::Play, true,
               g.hoverButton == HB_QUICK_DIAGNOSTICS, false, false, 8.4f);
    DrawButton(graphics, g.layout.dashQuickItems[1], L"Moduły", Icon::Modules, false,
               g.hoverButton == HB_QUICK_MODULES, false, false, 8.4f);
    DrawButton(graphics, g.layout.dashQuickItems[2], L"Raporty", Icon::Report, false,
               g.hoverButton == HB_QUICK_REPORTS, false, false, 8.4f);

    // ── Recent scans from real report files ────────────────────────────────
    const RECT& history = g.layout.dashHistory;
    DrawGlassPanel(graphics, history, 18.0f);
    DrawLabel(graphics, L"OSTATNIE SKANY", MakeRect(history.left + 12, history.top + 11,
              Width(history) - 24, 16), 8.8f, C_MUTED, true);
    if (g.reports.empty()) {
        DrawLabel(graphics, L"Brak zapisanych raportów", MakeRect(history.left + 12, history.top + 36,
                  Width(history) - 24, Height(history) - 46), 8.2f, C_DIM, false, StringAlignmentCenter);
    } else {
        for (int i = 0; i < 4 && i < static_cast<int>(g.reports.size()); ++i) {
            const Report& report = g.reports[static_cast<size_t>(i)];
            RECT row = g.layout.dashHistoryRows[i];
            if (g.hoverButton == HB_HISTORY_0 + i) {
                FillRounded(graphics, row, 8.0f, Color(22, 55, 71, 79));
            }
            const Color stateColor = ReportStatusColor(report);
            SolidBrush dotBrush(stateColor);
            graphics.FillEllipse(&dotBrush, static_cast<REAL>(row.left + 4),
                                 static_cast<REAL>(row.top + (Height(row) - 6) * 0.5f), 6.0f, 6.0f);
            DrawLabel(graphics, report.moduleName.empty() ? unknown : report.moduleName,
                      MakeRect(row.left + 15, row.top, std::max(30, Width(row) - 104), Height(row)),
                      7.8f, C_TEXT);
            DrawLabel(graphics, ReportDurationText(report),
                      MakeRect(row.right - 84, row.top, 80, Height(row)), 7.2f, C_DIM,
                      false, StringAlignmentFar);
        }
    }

    // ── CPU ────────────────────────────────────────────────────────────────
    const RECT& cpu = g.layout.dashCpu;
    DrawGlassPanel(graphics, cpu, 18.0f);
    const bool hasTemp = !snapshot.cpuTemp.empty() && snapshot.cpuTemp != L"n/d";
    DrawPanelHeader(graphics, cpu, Icon::Cpu, L"PROCESOR", hasTemp ? L"TEMP " + snapshot.cpuTemp : L"TEMP n/d",
                    C_LIGHT_BLUE, true, hasTemp ? C_AMBER : C_DIM);

    const int cpuPad = 16;
    const int ringLeft = g.layout.cpuRing.left;
    const int cpuTextWidth = std::max(80, static_cast<int>(ringLeft - cpu.left - cpuPad - 12));
    DrawLabel(graphics, snapshot.cpuName.empty() ? L"Wykrywanie procesora…" : snapshot.cpuName,
              MakeRect(cpu.left + cpuPad, cpu.top + 50, cpuTextWidth, 20), 11.5f, C_INK, true);

    std::wstring cpuSub;
    if (snapshot.cores > 0) cpuSub = std::to_wstring(snapshot.cores) + L" rdzeni · " + std::to_wstring(snapshot.threads) + L" wątków";
    if (snapshot.cpuMhz > 0) {
        if (!cpuSub.empty()) cpuSub += L" · ";
        cpuSub += FormatMhz(snapshot.cpuMhz);
        if (!snapshot.cpuMhzCurrent) cpuSub += L" (max)";
    }
    DrawLabel(graphics, cpuSub.empty() ? L"Odczyt specyfikacji procesora…" : cpuSub,
              MakeRect(cpu.left + cpuPad, cpu.top + 70, cpuTextWidth, 15), 8.0f, C_MUTED);

    RECT cpuChart = MakeRect(cpu.left + cpuPad, cpu.top + 88, cpuTextWidth,
                             cpu.bottom - (cpu.top + 88) - 12);
    DrawSparkline(graphics, cpuChart, g.cpuHistory, C_LIGHT_BLUE);

    if (snapshot.cpuReady) {
        DrawRing(graphics, g.layout.cpuRing, static_cast<float>(snapshot.cpuPercent),
                 UsageColor(static_cast<float>(snapshot.cpuPercent)),
                 std::to_wstring(snapshot.cpuPercent) + L"%", L"OBCIĄŻENIE");
    } else {
        DrawRing(graphics, g.layout.cpuRing, 0.0f, C_MUTED, L"—", L"OBCIĄŻENIE");
    }

    // ── RAM ────────────────────────────────────────────────────────────────
    const RECT& ram = g.layout.dashRam;
    DrawGlassPanel(graphics, ram, 18.0f);
    DrawPanelHeader(graphics, ram, Icon::Memory, L"PAMIĘĆ RAM",
                    snapshot.ramTotal ? std::to_wstring(snapshot.ramPercent) + L"%" : L"n/d",
                    C_LIGHT_BLUE, true, snapshot.ramTotal ? UsageColor(static_cast<float>(snapshot.ramPercent)) : C_DIM);

    const int ramRingLeft = g.layout.ramRing.left;
    const int ramTextWidth = std::max(80, static_cast<int>(ramRingLeft - ram.left - cpuPad - 12));
    const std::wstring ramText = snapshot.ramTotal
        ? FormatBytes(snapshot.ramUsed) + L" / " + FormatBytes(snapshot.ramTotal)
        : L"Odczyt pamięci…";
    DrawLabel(graphics, ramText, MakeRect(ram.left + cpuPad, ram.top + 50, ramTextWidth, 22),
              12.0f, C_INK, true);
    std::wstring ramSub;
    if (snapshot.ramTotal) {
        ramSub = L"Dostępne " + FormatBytes(snapshot.ramAvail);
        if (snapshot.commitLimit) ramSub += L"  ·  Wirtualna " + FormatBytes(snapshot.commitUsed) + L" / " + FormatBytes(snapshot.commitLimit);
    }
    DrawLabel(graphics, ramSub, MakeRect(ram.left + cpuPad, ram.top + 73, ramTextWidth, 15), 8.0f, C_MUTED);

    RECT ramChart = MakeRect(ram.left + cpuPad, ram.top + 90, ramTextWidth,
                             ram.bottom - (ram.top + 90) - 12);
    DrawSparkline(graphics, ramChart, g.ramHistory, C_LIGHT_BLUE);
    DrawRing(graphics, g.layout.ramRing, static_cast<float>(snapshot.ramPercent),
             UsageColor(static_cast<float>(snapshot.ramPercent)),
             snapshot.ramTotal ? std::to_wstring(snapshot.ramPercent) + L"%" : L"n/d", L"UŻYCIE");

    // ── Disks ──────────────────────────────────────────────────────────────
    const RECT& disks = g.layout.dashDisk;
    DrawGlassPanel(graphics, disks, 18.0f);
    std::wstring diskHeaderRight = snapshot.disks.empty() ? L"BRAK WOLUMINÓW"
        : std::to_wstring(snapshot.disks.size()) + (snapshot.disks.size() == 1 ? L" WOLUMIN" : L" WOLUMINY");
    DrawPanelHeader(graphics, disks, Icon::Disk, L"DYSKI", diskHeaderRight, C_LIGHT_BLUE, true, C_TEXT);

    const int diskPad = 16;
    const int rowHeight = 28;
    const int rowsTop = disks.top + 46;
    const int maxRows = std::max(1, (Height(disks) - 70) / rowHeight);
    const int drawCount = std::min(static_cast<int>(snapshot.disks.size()), maxRows);
    for (int i = 0; i < drawCount; ++i) {
        const DiskVolume& volume = snapshot.disks[static_cast<size_t>(i)];
        const int y = rowsTop + i * rowHeight;
        RECT badge = MakeRect(disks.left + diskPad, y + 3, 22, 19);
        FillRounded(graphics, badge, 6.0f, Color(36, 55, 71, 79));
        StrokeRounded(graphics, badge, 6.0f, C_LINE, 1.0f);
        DrawLabel(graphics, std::wstring(1, volume.letter), badge, 8.4f, C_INK, true, StringAlignmentCenter);

        const uint64_t used = volume.total > volume.free ? volume.total - volume.free : 0;
        const int usedPercent = volume.total ? static_cast<int>((used * 100) / volume.total) : 0;
        const int infoX = badge.right + 8;
        const int rightWidth = 176;
        const int metaWidth = std::max(40, static_cast<int>(disks.right - diskPad - rightWidth - infoX - 10));

        std::wstring label = volume.label.empty() ? L"Wolumin " + std::wstring(1, volume.letter) : volume.label;
        DrawLabel(graphics, label, MakeRect(infoX, y, metaWidth, 13), 8.8f, C_INK, true);
        std::wstring meta = volume.model;
        if (!volume.fs.empty()) meta += L"  ·  " + volume.fs;
        meta += volume.ssdKnown ? (volume.ssd ? L"  ·  SSD" : L"  ·  HDD") : L"  ·  typ n/d";
        DrawLabel(graphics, meta, MakeRect(infoX, y + 13, metaWidth, 11), 7.2f, C_DIM);

        DrawLabel(graphics, FormatBytes(used) + L" / " + FormatBytes(volume.total),
                  MakeRect(disks.right - diskPad - rightWidth, y, rightWidth, 13), 8.2f, C_TEXT, true, StringAlignmentFar);
        DrawLabel(graphics, FormatBytes(volume.free) + L" wolne",
                  MakeRect(disks.right - diskPad - rightWidth, y + 13, rightWidth, 11), 7.2f, C_MUTED, false, StringAlignmentFar);

        RECT bar = MakeRect(infoX, y + 24, std::max(40, static_cast<int>(disks.right - diskPad - infoX)), 3);
        DrawBar(graphics, bar, static_cast<float>(usedPercent), UsageColor(static_cast<float>(usedPercent)));
    }
    if (snapshot.disks.empty()) {
        DrawLabel(graphics, L"Nie wykryto woluminów lokalnych.",
                  MakeRect(disks.left + diskPad, rowsTop, Width(disks) - 2 * diskPad, 24), 8.8f, C_DIM);
    }
    const std::wstring ioText = snapshot.diskIo
        ? L"Odczyt " + FormatSpeed(snapshot.diskRead) + L"   ·   Zapis " + FormatSpeed(snapshot.diskWrite)
        : L"Liczniki wejścia/wyjścia niedostępne";
    DrawLabel(graphics, ioText, MakeRect(disks.left + diskPad, disks.bottom - 20,
              Width(disks) - 2 * diskPad - 160, 13), 7.6f, C_DIM);
    if (static_cast<int>(snapshot.disks.size()) > drawCount) {
        DrawLabel(graphics, L"+" + std::to_wstring(snapshot.disks.size() - drawCount) + L" kolejnych woluminów",
                  MakeRect(disks.right - diskPad - 200, disks.bottom - 20, 200, 13), 7.6f, C_MUTED,
                  false, StringAlignmentFar);
    }

    // ── GPU ────────────────────────────────────────────────────────────────
    const RECT& gpu = g.layout.dashGpu;
    DrawGlassPanel(graphics, gpu, 18.0f);
    DrawPanelHeader(graphics, gpu, Icon::Gpu, L"KARTA GRAFICZNA",
                    snapshot.gpu.vendor.empty() ? L"n/d" : snapshot.gpu.vendor, C_LIGHT_BLUE, false, C_TEXT);

    const int gpuTextWidth = std::max(80, static_cast<int>(g.layout.gpuRing.left - gpu.left - 30));
    DrawLabelWrap(graphics, snapshot.gpu.name.empty() ? L"Wykrywanie karty graficznej…" : snapshot.gpu.name,
                  MakeRect(gpu.left + 16, gpu.top + 44, gpuTextWidth, 28), 10.6f, C_INK);

    auto gpuRow = [&](int row, const std::wstring& label, const std::wstring& value) {
        const int y = gpu.top + 76 + row * 16;
        DrawLabel(graphics, label, MakeRect(gpu.left + 16, y, 70, 14), 7.4f, C_DIM, true);
        DrawLabel(graphics, value, MakeRect(gpu.left + 88, y, std::max(30, gpuTextWidth - 72), 14), 8.2f, C_TEXT);
    };
    gpuRow(0, L"VRAM", snapshot.gpu.vram ? FormatBytes(snapshot.gpu.vram) : L"n/d");
    gpuRow(1, L"Sterownik", snapshot.gpu.driver.empty() ? L"n/d" : snapshot.gpu.driver);
    gpuRow(2, L"Temperatura", snapshot.gpu.temp.empty() ? L"n/d" : snapshot.gpu.temp);

    if (snapshot.gpu.usage >= 0) {
        DrawRing(graphics, g.layout.gpuRing, static_cast<float>(snapshot.gpu.usage),
                 UsageColor(static_cast<float>(snapshot.gpu.usage)),
                 std::to_wstring(snapshot.gpu.usage) + L"%", L"UŻYCIE GPU");
    } else {
        DrawRing(graphics, g.layout.gpuRing, 0.0f, C_MUTED, L"n/d", L"UŻYCIE GPU");
    }
    DrawLabel(graphics, snapshot.gpu.usage >= 0 ? L"Źródło: licznik PDH GPU Engine"
                                                 : L"Sterownik nie udostępnia licznika użycia GPU",
              MakeRect(gpu.left + 16, gpu.bottom - 16, Width(gpu) - 32, 12), 7.2f, C_DIM);

    // ── System status ──────────────────────────────────────────────────────
    const RECT& system = g.layout.dashSystem;
    DrawGlassPanel(graphics, system, 18.0f);
    DrawPanelHeader(graphics, system, Icon::System, L"SYSTEM",
                    snapshot.sampleTime.empty() ? L"" : L"PRÓBKA " + snapshot.sampleTime, C_LIGHT_BLUE, false, C_DIM);

    struct SystemCell { std::wstring label; std::wstring value; };
    const std::wstring moduleSummary = std::to_wstring(kModuleCount) + L" w 8 kategoriach";
    const SystemCell cells[8] = {
        {L"SYSTEM OPERACYJNY", snapshot.osName},
        {L"ARCHITEKTURA", snapshot.arch},
        {L"KOMPUTER", snapshot.computer},
        {L"CZAS PRACY", snapshot.uptime},
        {L"BIOS / UEFI", snapshot.bios},
        {L"PŁYTA GŁÓWNA", snapshot.board},
        {L"MODEL", snapshot.model},
        {L"MODUŁY", moduleSummary},
    };
    const int gridLeft = system.left + 16;
    const int gridTop = system.top + 44;
    const int gridWidth = Width(system) - 32;
    const int gridHeight = Height(system) - 44 - 10;
    const int rowHeightGrid = std::max(34, gridHeight / 2);
    const int cellGap = 10;
    const int cellWidth = std::max(40, (gridWidth - 3 * cellGap) / 4);
    for (int i = 0; i < 8; ++i) {
        const int column = i % 4;
        const int row = i / 4;
        const int x = gridLeft + column * (cellWidth + cellGap);
        const int y = gridTop + row * rowHeightGrid;
        DrawLabel(graphics, cells[i].label, MakeRect(x, y, cellWidth, 13), 6.8f, C_DIM, true);
        DrawLabelWrap(graphics, cells[i].value.empty() ? unknown : cells[i].value,
                      MakeRect(x, y + 14, cellWidth, rowHeightGrid - 17), 8.2f, C_TEXT);
    }
}

void DrawAboutView(Graphics& graphics) {
    const RECT& card = g.layout.aboutCard;
    DrawGlassPanel(graphics, card, 20.0f);

    const RECT& mark = g.layout.aboutLogo;
    DrawKdMark(graphics, mark);

    const int titleX = mark.right + 20;
    const int titleW = std::max(120, static_cast<int>(card.right - 36 - titleX));
    DrawLabel(graphics, L"KD Core", MakeRect(titleX, card.top + 40, titleW, 30), 20.0f, C_INK, true);
    DrawLabel(graphics, L"Wersja " + std::wstring(kAppVersion) +
              L"  ·  natywna aplikacja diagnostyczna dla Windows 10/11",
              MakeRect(titleX, card.top + 74, titleW, 20), 9.0f, C_MUTED);

    Pen divider(C_LINE, 1.0f);
    graphics.DrawLine(&divider, static_cast<REAL>(card.left + 36), static_cast<REAL>(card.top + 126),
                      static_cast<REAL>(card.right - 36), static_cast<REAL>(card.top + 126));

    struct AboutItem { std::wstring label; std::wstring value; };
    const AboutItem items[6] = {
        {L"BAZA MODUŁÓW", std::to_wstring(kModuleCount) + L" rzeczywistych modułów w 8 kategoriach"},
        {L"SILNIKI DANYCH", L"WinAPI · WMI · PDH · DXGI · Storage API"},
        {L"INTERFEJS", L"Win32 / GDI+ · KD Core UI"},
        {L"TRYB PRACY", L"Lokalny, bez wysyłania danych do internetu"},
        {L"UPRAWNIENIA", g.admin ? L"Sesja z uprawnieniami administratora"
                                 : L"Sesja standardowa; część modułów może wymagać administratora"},
        {L"FOLDER RAPORTÓW", g.reportsDir},
    };
    const int columnGap = 28;
    const int columnWidth = std::max(160, (Width(card) - 72 - columnGap) / 2);
    const int rowHeight = 58;
    for (int i = 0; i < 6; ++i) {
        const int column = i % 2;
        const int row = i / 2;
        const int x = card.left + 36 + column * (columnWidth + columnGap);
        const int y = card.top + 150 + row * rowHeight;
        DrawLabel(graphics, items[i].label, MakeRect(x, y, columnWidth, 14), 7.4f, C_DIM, true);
        DrawLabelWrap(graphics, items[i].value, MakeRect(x, y + 18, columnWidth, 34), 9.0f, C_TEXT);
    }

    DrawButton(graphics, g.layout.aboutAdmin, L"Sprawdź uprawnienia", Icon::Admin, false,
               g.hoverButton == HB_ABOUT_ADMIN, false, false, 9.2f);
    DrawButton(graphics, g.layout.aboutReports, L"Otwórz folder raportów", Icon::Folder, false,
               g.hoverButton == HB_ABOUT_REPORTS, false, false, 9.2f);
    DrawLabel(graphics, L"KD Core nie wysyła wyników diagnostyki do internetu.",
              MakeRect(card.left + 36, card.bottom - 26, Width(card) - 72, 14), 7.6f, C_DIM);
}

constexpr int kSearchEditId = 9001;

void UpdateAnimTimer() {
    if (!g.hwnd) return;
    const bool needed = g.busy && g.settings.animations;
    if (needed && !g.animTimerActive) {
        SetTimer(g.hwnd, kAnimTimer, 90, nullptr);
        g.animTimerActive = true;
    } else if (!needed && g.animTimerActive) {
        KillTimer(g.hwnd, kAnimTimer);
        g.animTimerActive = false;
    }
}

void UpdateScale(HWND hwnd) {
    UINT dpi = 96;
    using GetDpiForWindowFn = UINT(WINAPI*)(HWND);
    auto getDpi = reinterpret_cast<GetDpiForWindowFn>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    if (getDpi) dpi = getDpi(hwnd);
    if (dpi < 96) dpi = 96;
    g_scale = static_cast<float>(dpi) / 96.0f;
    g_textMeasureCache.clear();
}

void CreateUiResources(HWND hwnd, HINSTANCE instance) {
    g.searchBrush = CreateSolidBrush(RGB(20, 32, 37));
    g.outputBrush = CreateSolidBrush(RGB(24, 34, 39));
    g.searchFont = CreateFontW(-ScalePx(15), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                               DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g.outputFont = CreateFontW(-ScalePx(14), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                               FIXED_PITCH | FF_MODERN, L"Consolas");

    g.searchEdit = CreateWindowExW(0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT,
        0, 0, 10, 10, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kSearchEditId)), instance, nullptr);
    g.outputEdit = CreateWindowExW(0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_NOHIDESEL,
        0, 0, 10, 10, hwnd, nullptr, instance, nullptr);
    g.previewEdit = CreateWindowExW(0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_NOHIDESEL,
        0, 0, 10, 10, hwnd, nullptr, instance, nullptr);

    if (g.searchEdit) {
        SendMessageW(g.searchEdit, WM_SETFONT, reinterpret_cast<WPARAM>(g.searchFont), TRUE);
        SendMessageW(g.searchEdit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Szukaj modułu (np. SMART, DNS, GPU)…"));
        SendMessageW(g.searchEdit, EM_SETLIMITTEXT, 120, 0);
        SendMessageW(g.searchEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(0, 0));
        SetWindowTheme(g.searchEdit, L"DarkMode_Explorer", nullptr);
    }
    for (HWND edit : {g.outputEdit, g.previewEdit}) {
        if (!edit) continue;
        SendMessageW(edit, WM_SETFONT, reinterpret_cast<WPARAM>(g.outputFont), TRUE);
        SendMessageW(edit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(8, 8));
        SetWindowTheme(edit, L"DarkMode_Explorer", nullptr);
    }
}

void ApplyDarkFrame(HWND hwnd) {
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));
    int corner = 2;
    DwmSetWindowAttribute(hwnd, 33, &corner, sizeof(corner));
    COLORREF caption = RGB(38, 50, 56);
    DwmSetWindowAttribute(hwnd, 35, &caption, sizeof(caption));
    COLORREF border = RGB(55, 71, 79);
    DwmSetWindowAttribute(hwnd, 34, &border, sizeof(border));
}

void InvalidateNav(int index) {
    if (index >= 0 && index < NAV_COUNT) InvalidateLogical(g.layout.nav[index]);
}

void InvalidateChip(int index) {
    if (index >= 0 && index < g.layout.chipCount) InvalidateLogical(g.layout.chips[index]);
}

void InvalidateButtons() {
    for (const RECT& rect : ButtonRects()) InvalidateLogical(rect);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    PerfEventGuard perfEvent(message);
    switch (message) {
    case WM_CREATE: {
        g.hwnd = hwnd;
        g.admin = IsUserAnAdmin() != FALSE;
        UpdateScale(hwnd);
        LoadSettings();
        ResolveReportsDir();
        LoadReportsFromDisk();
        g_fontFamily = std::make_unique<FontFamily>(L"Segoe UI");
        if (!g_fontFamily || g_fontFamily->GetLastStatus() != Ok) {
            g_fontFamily = std::make_unique<FontFamily>(L"Arial");
        }
        CreateUiResources(hwnd, reinterpret_cast<LPCREATESTRUCT>(lParam)->hInstance);
        RebuildFiltered();
        EnsureSelection();
        RecalculateLayout();
        UpdateControlPositions();
        UpdateOutputText();
        UpdateReportPreviewText();
        ApplyDarkFrame(hwnd);
        StartStatsThread();
        return 0;
    }

    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            RecalculateLayout();
            ClampScroll();
            UpdateControlPositions();
            InvalidateAll();
        }
        return 0;

    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
        info->ptMinTrackSize.x = ScalePx(kMinWidth);
        info->ptMinTrackSize.y = ScalePx(kMinHeight);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
        PaintWindow(hwnd);
        return 0;

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC: {
        HDC context = reinterpret_cast<HDC>(wParam);
        SetTextColor(context, RGB(214, 229, 237));
        SetBkColor(context, RGB(24, 34, 39));
        SetBkMode(context, OPAQUE);
        return reinterpret_cast<LRESULT>(g.outputBrush);
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == kSearchEditId && HIWORD(wParam) == EN_CHANGE && g.searchEdit) {
            const int length = GetWindowTextLengthW(g.searchEdit);
            std::wstring value(static_cast<size_t>(length) + 1, L'\0');
            GetWindowTextW(g.searchEdit, value.data(), length + 1);
            value.resize(static_cast<size_t>(length));
            g.searchValue = value;
            RebuildFiltered();
            EnsureSelection();
            g.listScroll = 0;
            ClampScroll();
            UpdateOutputText();
            InvalidateLogical(g.layout.listPanel);
            InvalidateLogical(g.layout.detailPanel);
        }
        return 0;

    case WM_MOUSEMOVE: {
        POINT point = PointToLogical(POINT{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
        if (!g.trackingMouse) {
            TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, hwnd, 0};
            TrackMouseEvent(&track);
            g.trackingMouse = true;
        }
        const int oldNav = g.hoverNav;
        const int oldChip = g.hoverChip;
        const int oldModule = g.hoverModule;
        const int oldReport = g.hoverReport;
        const int oldButton = g.hoverButton;

        g.hoverNav = -1;
        for (int i = 0; i < NAV_COUNT; ++i) {
            if (Contains(g.layout.nav[i], point)) { g.hoverNav = i; break; }
        }
        g.hoverChip = -1;
        if (ModulesView()) {
            for (int i = 0; i < g.layout.chipCount; ++i) {
                if (Contains(g.layout.chips[i], point)) { g.hoverChip = i; break; }
            }
        }
        g.hoverModule = -1;
        if (ModulesView() && Contains(g.layout.listArea, point)) {
            const int index = g.listScroll + (point.y - (g.layout.listArea.top + 2)) / 66;
            if (index >= 0 && index < static_cast<int>(g.filtered.size())) {
                if (Contains(ModuleRowRect(index), point)) g.hoverModule = index;
            }
        }
        g.hoverReport = -1;
        if (g.nav == NAV_REPORTS && Contains(g.layout.reportListArea, point)) {
            const int index = g.reportScroll + (point.y - (g.layout.reportListArea.top + 2)) / 62;
            if (index >= 0 && index < static_cast<int>(g.reports.size())) {
                if (Contains(ReportRowRect(index), point)) g.hoverReport = index;
            }
        }
        int button = FindButtonAt(point);
        if (button != HB_NONE && ButtonDisabled(button)) button = HB_NONE;
        g.hoverButton = button;

        const bool interactive = g.hoverNav >= 0 || g.hoverChip >= 0 || g.hoverModule >= 0 ||
                                 g.hoverReport >= 0 || g.hoverButton != HB_NONE;
        SetCursor(LoadCursorW(nullptr, interactive ? IDC_HAND : IDC_ARROW));

        if (oldNav != g.hoverNav) { InvalidateNav(oldNav); InvalidateNav(g.hoverNav); }
        if (oldChip != g.hoverChip) { InvalidateChip(oldChip); InvalidateChip(g.hoverChip); }
        if (oldButton != g.hoverButton) InvalidateButtons();
        if (oldModule != g.hoverModule) {
            if (oldModule >= 0 && oldModule < static_cast<int>(g.filtered.size())) InvalidateLogical(ModuleRowRect(oldModule));
            if (g.hoverModule >= 0) InvalidateLogical(ModuleRowRect(g.hoverModule));
        }
        if (oldReport != g.hoverReport) {
            if (oldReport >= 0 && oldReport < static_cast<int>(g.reports.size())) InvalidateLogical(ReportRowRect(oldReport));
            if (g.hoverReport >= 0) InvalidateLogical(ReportRowRect(g.hoverReport));
        }
        return 0;
    }

    case WM_MOUSELEAVE:
        g.trackingMouse = false;
        g.hoverNav = -1;
        g.hoverChip = -1;
        g.hoverModule = -1;
        g.hoverReport = -1;
        g.hoverButton = HB_NONE;
        SetCursor(LoadCursorW(nullptr, IDC_ARROW));
        InvalidateAll();
        return 0;

    case WM_LBUTTONUP: {
        POINT point = PointToLogical(POINT{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
        for (int i = 0; i < NAV_COUNT; ++i) {
            if (Contains(g.layout.nav[i], point)) { ActivateNav(i); return 0; }
        }
        if (ModulesView()) {
            for (int i = 0; i < g.layout.chipCount; ++i) {
                if (Contains(g.layout.chips[i], point)) {
                    ActivateNav(i == 0 ? NAV_MODULES : NAV_SYSTEM + i - 1);
                    return 0;
                }
            }
            if (Contains(g.layout.listArea, point)) {
                const int index = g.listScroll + (point.y - (g.layout.listArea.top + 2)) / 66;
                if (index >= 0 && index < static_cast<int>(g.filtered.size()) && Contains(ModuleRowRect(index), point)) {
                    g.selectedModule = g.filtered[static_cast<size_t>(index)]->id;
                    UpdateOutputText();
                    InvalidateLogical(g.layout.listPanel);
                    InvalidateLogical(g.layout.detailPanel);
                    return 0;
                }
            }
        }
        if (g.nav == NAV_REPORTS && Contains(g.layout.reportListArea, point)) {
            const int index = g.reportScroll + (point.y - (g.layout.reportListArea.top + 2)) / 62;
            if (index >= 0 && index < static_cast<int>(g.reports.size()) && Contains(ReportRowRect(index), point)) {
                g.selectedReport = index;
                UpdateReportPreviewText();
                InvalidateLogical(g.layout.reportListPanel);
                InvalidateLogical(g.layout.previewPanel);
                return 0;
            }
        }
        const int button = FindButtonAt(point);
        if (button != HB_NONE && !ButtonDisabled(button)) OnButtonClicked(button);
        return 0;
    }

    case WM_MOUSEWHEEL: {
        POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        ScreenToClient(hwnd, &point);
        point = PointToLogical(point);
        const int delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
        if (ModulesView() && Contains(g.layout.listArea, point)) {
            g.listScroll -= delta * 3;
            ClampScroll();
            InvalidateLogical(g.layout.listArea);
            return 0;
        }
        if (g.nav == NAV_REPORTS && Contains(g.layout.reportListArea, point)) {
            g.reportScroll -= delta * 3;
            ClampScroll();
            InvalidateLogical(g.layout.reportListArea);
            return 0;
        }
        return 0;
    }

    case WM_KEYDOWN: {
        if ((GetKeyState(VK_CONTROL) & 0x8000) && (wParam == 'F')) {
            if (ModulesView() && g.searchEdit) {
                SetFocus(g.searchEdit);
                SendMessageW(g.searchEdit, EM_SETSEL, 0, -1);
            }
            return 0;
        }
        if (wParam == VK_F5) {
            if (g_wakeEvent) SetEvent(g_wakeEvent);
            return 0;
        }
        if (wParam == VK_ESCAPE && g.busy) {
            CancelRunningModule();
            return 0;
        }
        if (ModulesView()) {
            HWND focus = GetFocus();
            if (focus == hwnd || focus == nullptr) {
                if (wParam == VK_UP || wParam == VK_DOWN) {
                    int current = -1;
                    for (int i = 0; i < static_cast<int>(g.filtered.size()); ++i) {
                        if (g.filtered[static_cast<size_t>(i)]->id == g.selectedModule) { current = i; break; }
                    }
                    if (current >= 0) {
                        const int target = std::clamp(current + (wParam == VK_DOWN ? 1 : -1), 0,
                                                      static_cast<int>(g.filtered.size()) - 1);
                        g.selectedModule = g.filtered[static_cast<size_t>(target)]->id;
                        const int visible = std::max(1, Height(g.layout.listArea) / 66);
                        if (target < g.listScroll) g.listScroll = target;
                        if (target >= g.listScroll + visible) g.listScroll = target - visible + 1;
                        ClampScroll();
                        UpdateOutputText();
                        InvalidateAll();
                    }
                    return 0;
                }
                if (wParam == VK_RETURN && !g.busy && g.selectedModule) {
                    StartModule(g.selectedModule);
                    return 0;
                }
            }
        }
        break;
    }

    case WM_TIMER:
        if (wParam == kAnimTimer) {
            g.spinnerAngle = (g.spinnerAngle + 24) % 360;
            InvalidateLogical(g.layout.headerStatus);
            if (ModulesView()) InvalidateLogical(g.layout.detailStatus);
            if (g.nav == NAV_REPORTS) InvalidateLogical(g.layout.previewStatus);
            return 0;
        }
        break;

    case WM_APP_STATS: {
        bool updated = false;
        {
            std::lock_guard<std::mutex> lock(g.statsMutex);
            if (g.statsDirty) {
                g.stats = g.pendingStats;
                g.statsDirty = false;
                updated = true;
            }
        }
        if (updated) {
            if (g.stats.cpuReady) {
                g.cpuHistory.push_back(static_cast<float>(g.stats.cpuPercent));
                while (g.cpuHistory.size() > static_cast<size_t>(kChartSeconds)) g.cpuHistory.pop_front();
            }
            if (g.stats.ramTotal) {
                g.ramHistory.push_back(static_cast<float>(g.stats.ramPercent));
                while (g.ramHistory.size() > static_cast<size_t>(kChartSeconds)) g.ramHistory.pop_front();
            }
            if (g.nav == NAV_DASHBOARD) {
                InvalidateLogical(g.layout.dashboard);
                InvalidateLogical(g.layout.headerSub);
            }
        }
        return 0;
    }

    case WM_APP_MODULE_DONE: {
        std::unique_ptr<ModuleOutcome> outcome(reinterpret_cast<ModuleOutcome*>(lParam));
        FinishModule(std::move(outcome));
        ClampScroll();
        if (g.nav == NAV_REPORTS) UpdateReportPreviewText();
        UpdateControlPositions();
        InvalidateAll();
        return 0;
    }

    case WM_CLOSE:
        if (g.busy) {
            if (MessageBoxW(hwnd, L"Trwa diagnostyka. Przerwać operację i zamknąć aplikację?",
                            L"Zamykanie", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) return 0;
            g.cancelRequested.store(true);
            if (g.moduleThread.joinable()) g.moduleThread.join();
            g.busy = false;
        }
        if (g.statsThread.joinable()) StopStatsThread();
        SaveSettings();
        {
            MSG pending{};
            while (PeekMessageW(&pending, hwnd, WM_APP_MODULE_DONE, WM_APP_MODULE_DONE, PM_REMOVE)) {
                delete reinterpret_cast<ModuleOutcome*>(pending.lParam);
            }
        }
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, kAnimTimer);
        if (g.statsThread.joinable()) StopStatsThread();
        if (g.moduleThread.joinable()) g.moduleThread.join();
        if (g_memDC) {
            if (g_memOld) SelectObject(g_memDC, g_memOld);
            if (g_memBitmap) DeleteObject(g_memBitmap);
            DeleteDC(g_memDC);
            g_memDC = nullptr;
            g_memBitmap = nullptr;
            g_memOld = nullptr;
            g_memWidth = 0;
            g_memHeight = 0;
        }
        g_backgroundSurface.Reset();
        g_panelTiles.clear();
        g_panelTileBytes = 0;
        g_textMeasureCache.clear();
        g_fontCache.clear();
        g_fontFamily.reset();
        if (g.searchFont) { DeleteObject(g.searchFont); g.searchFont = nullptr; }
        if (g.outputFont) { DeleteObject(g.outputFont); g.outputFont = nullptr; }
        if (g.searchBrush) { DeleteObject(g.searchBrush); g.searchBrush = nullptr; }
        if (g.outputBrush) { DeleteObject(g.outputBrush); g.outputBrush = nullptr; }
        g.hwnd = nullptr;
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR commandLine, int showCommand) {
    GdiplusStartupInput gdiplusInput;
    if (GdiplusStartup(&g_gdiplusToken, &gdiplusInput, nullptr) != Ok) return 1;
    if (commandLine && wcsstr(commandLine, L"--perf")) g_perf.Init();

    HDC screen = GetDC(nullptr);
    const int dpi = screen ? GetDeviceCaps(screen, LOGPIXELSX) : 96;
    if (screen) ReleaseDC(nullptr, screen);
    g_scale = std::max(1.0f, static_cast<float>(dpi) / 96.0f);

    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&controls);

    WNDCLASSEXW windowClass{sizeof(windowClass)};
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WndProc;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP));
    windowClass.hIconSm = reinterpret_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));
    if (!windowClass.hIconSm) windowClass.hIconSm = windowClass.hIcon;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = nullptr;
    windowClass.lpszClassName = kClassName;
    if (!RegisterClassExW(&windowClass)) {
        GdiplusShutdown(g_gdiplusToken);
        return 1;
    }

    RECT workArea{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    const int availableWidth = workArea.right - workArea.left;
    const int availableHeight = workArea.bottom - workArea.top;
    int width = std::min(ScalePx(1400), availableWidth - 40);
    int height = std::min(ScalePx(880), availableHeight - 40);
    width = std::max(width, ScalePx(kMinWidth));
    height = std::max(height, ScalePx(kMinHeight));
    const int x = workArea.left + std::max(0, (availableWidth - width) / 2);
    const int y = workArea.top + std::max(0, (availableHeight - height) / 2);

    HWND hwnd = CreateWindowExW(0, kClassName, kProductName,
                                WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                x, y, width, height, nullptr, nullptr, instance, nullptr);
    if (!hwnd) {
        GdiplusShutdown(g_gdiplusToken);
        return 1;
    }
    ShowWindow(hwnd, showCommand == SW_SHOWMINIMIZED ? SW_SHOWMINIMIZED : SW_SHOW);
    UpdateWindow(hwnd);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    g_perf.Flush(true);
    GdiplusShutdown(g_gdiplusToken);
    g_gdiplusToken = 0;
    return static_cast<int>(message.wParam);
}
