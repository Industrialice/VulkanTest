#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <volk/volk.h>
//#include <vulkan/vulkan.hpp>
#include "WinAPI.hpp"
#include <StdPlatformAbstractionLib.hpp>
#include <stdio.h>

using namespace std;
using namespace StdLib;

//#define LOG(...) fprintf(stderr, __VA_ARGS__)
#define LOG(...) do { char buf[1024]; snprintf(buf, sizeof(buf), __VA_ARGS__); OutputDebugStringA(buf); } while(0)

static optional<HWND> CreateSystemWindow(bool isFullscreen, const string &title, HINSTANCE instance, bool hideBorders, RECT &dimensions);
static LRESULT WINAPI MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

class Application
{
    HWND _windowHandle{};
    VkInstance _vkInstance{};

public:
    ~Application()
    {
    }

    Application(HWND windowHandle) : _windowHandle(windowHandle)
    {
    }

    bool Initialize()
    {
        if (volkInitialize() != VK_SUCCESS)
        {
            LOG("volkInitialize failed\n");
            return false;
        }

        LOG("volkInitialize succeeded\n");

        uint32_t supportedExtsCount;
        if (vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtsCount, nullptr) != VK_SUCCESS)
        {
            LOG("vkEnumerateInstanceExtensionProperties failed\n");
            return false;
        }

        vector<VkExtensionProperties> supportedExts(supportedExtsCount);
        if (vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtsCount, supportedExts.data()) != VK_SUCCESS)
        {
            LOG("vkEnumerateInstanceExtensionProperties failed\n");
            return false;
        }

        for (const auto &ext : supportedExts)
        {
            LOG("Supporting ext %s\n", ext.extensionName);
        }

        const char *const extensions[] =
        {
            VK_KHR_SURFACE_EXTENSION_NAME
        };

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.enabledExtensionCount = Funcs::CountOf(extensions);
        createInfo.ppEnabledExtensionNames = extensions;

        if (vkCreateInstance(&createInfo, nullptr, &_vkInstance) != VK_SUCCESS)
        {
            LOG("vkCreateInstance failed\n");
            return false;
        }

        volkLoadInstance(_vkInstance);

        return true;
    }

    void MessageLoop()
    {
        MSG o_msg;
        auto lastUpdate = TimeMoment::Now();
        auto firstUpdate = TimeMoment::Now();

        do
        {
            if (PeekMessageA(&o_msg, 0, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&o_msg);
                DispatchMessageA(&o_msg);
            }
            else
            {
            }
        } while (o_msg.message != WM_QUIT);
    }

private:
};

int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    //freopen("CONIN$", "r", stdin);
    //freopen("CONOUT$", "w", stdout);
    //freopen("CONOUT$", "w", stderr);

    StdLib::Initialization::PlatformAbstractionInitialize({});

    i32 x = 1400, y = 800;
    i32 width = 500, height = 500;
    RECT dimensions{x, y, x + width, y + height};
    auto windowHandle = CreateSystemWindow(false, "VK", GetModuleHandleW(NULL), false, dimensions);
    if (!windowHandle)
    {
        LOG("CreateSystemWindow failed\n");
        return 1;
    }

    Application app(*windowHandle);
    if (!app.Initialize())
    {
        LOG("Failed to initialize the app\n");
        return 1;
    }

    app.MessageLoop();

    return 0;
}

optional<HWND> CreateSystemWindow(bool isFullscreen, const string &title, HINSTANCE instance, bool hideBorders, RECT &dimensions)
{
    auto className = "window_" + title;

    WNDCLASSA wc;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // means that we need to redraw the whole window if its size changes, not just a new portion, CS_OWNDC is required by OpenGL
    wc.lpfnWndProc = MsgProc; // note that the message procedure runs in the same thread that created the window (it's a requirement)
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instance;
    wc.hIcon = LoadIconA(0, IDI_APPLICATION);
    wc.hCursor = LoadCursorA(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = className.c_str();

    if (!RegisterClassA(&wc))
    {
        LOG("failed to register class\n");
        return nullopt;
    }

    ui32 width = dimensions.right - dimensions.left;
    ui32 height = dimensions.bottom - dimensions.top;

    if (isFullscreen)
    {
        DEVMODE dmScreenSettings = {};
        dmScreenSettings.dmSize = sizeof(dmScreenSettings);
        dmScreenSettings.dmPelsWidth = width;
        dmScreenSettings.dmPelsHeight = height;
        dmScreenSettings.dmBitsPerPel = 32;
        dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

        if (ChangeDisplaySettingsA(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
        {
            LOG("Failed to set fullscreen mode\n");
        }
        else
        {
            LOG("Display mode has been changed to fullscreen\n");
            ShowCursor(FALSE);
        }
    }

    DWORD style = isFullscreen || hideBorders ? WS_POPUP : WS_SYSMENU;

    AdjustWindowRect(&dimensions, style, false);

    HWND hwnd = CreateWindowA(className.c_str(), title.c_str(), style, dimensions.left, dimensions.top, width, height, 0, 0, instance, 0);
    if (!hwnd)
    {
        LOG("failed to create requested window\n");
        return nullopt;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    return hwnd;
}

LRESULT WINAPI MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INPUT:
        break;
    case WM_SETCURSOR:
    case WM_MOUSEMOVE:
    case WM_KEYDOWN:
    case WM_KEYUP:
        break;
    case WM_ACTIVATE:
        //  switching window's active state
        break;
    case WM_SIZE:
    {
        //window->width = LOWORD( lParam );
        //window->height = HIWORD( lParam );
    } break;
    // WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
    case WM_ENTERSIZEMOVE:
        break;
        // WM_EXITSIZEMOVE is sent when the user releases the resize bars.
    case WM_EXITSIZEMOVE:
        break;
        // WM_DESTROY is sent when the window is being destroyed.
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
        // somebody has asked our window to close
    case WM_CLOSE:
        break;
        // Catch this message so to prevent the window from becoming too small.
    case WM_GETMINMAXINFO:
        //((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
        //((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200; 
        break;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}