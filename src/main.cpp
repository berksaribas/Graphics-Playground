#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <vector>
#include "d3d_utils.h"
#include "raytracer.h"

static ID3D11Device*            g_pd3dDevice = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

void render_imgui(RaytracerData& raytracer_data);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

struct WindowData {
    WNDCLASSEX wc;
    HWND hwnd;
    int width;
    int height;

};

int main(int, char**) {
    WindowData window_data;
    window_data.width = 1938;
    window_data.height = 1127;

    window_data.wc = {
        sizeof(WNDCLASSEX),
        CS_CLASSDC,
        WndProc,
        0L,
        0L,
        GetModuleHandle(NULL),
        NULL,
        NULL,
        NULL,
        NULL,
        _T("PlaygroundTracer"),
        NULL
    };

    RegisterClassEx(&window_data.wc);
    window_data.hwnd = CreateWindow(window_data.wc.lpszClassName, _T("PlaygroundTracer"), WS_OVERLAPPEDWINDOW, 100, 100, window_data.width, window_data.height, NULL, NULL, window_data.wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(window_data.hwnd))
    {
        CleanupDeviceD3D();
        UnregisterClass(window_data.wc.lpszClassName, window_data.wc.hInstance);
        return 1;
    }

    ShowWindow(window_data.hwnd, SW_SHOWDEFAULT);
    UpdateWindow(window_data.hwnd);

    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        ImGui::StyleColorsDark();

        ImGui_ImplWin32_Init(window_data.hwnd);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    }

    QuadRenderer quad_renderer = initialize_quad_renderer(g_pd3dDevice);
    ComputeShaderData compute_data = create_raytracer_shader(g_pd3dDevice, g_pSwapChain);

    // RAYTRACER DATA
    std::vector<Sphere> spheres;
    std::vector<Material> materials;
    {
        Material mat_light = {};
        mat_light.albedo = Vector3(10, 10, 10);
        mat_light.type = 0;

        Material mat_light_red = {};
        mat_light_red.albedo = Vector3(10, 0, 0);
        mat_light_red.type = 0;

        Material mat_light_green = {};
        mat_light_green.albedo = Vector3(0, 10, 0);
        mat_light_green.type = 0;

        Material mat_light_blue = {};
        mat_light_blue.albedo = Vector3(0, 0, 10);
        mat_light_blue.type = 0;

        Material mat_lambert_yellowish = {};
        mat_lambert_yellowish.albedo = Vector3(0.8f, 0.8f, 0.0f);
        mat_lambert_yellowish.type = 1;

        Material mat_lambert_reddish = {};
        mat_lambert_reddish.albedo = Vector3(0.7f, 0.3f, 0.3f);
        mat_lambert_reddish.type = 1;

        Material mat_lambert_greenish = {};
        mat_lambert_greenish.albedo = Vector3(0.3f, 0.7f, 0.3f);
        mat_lambert_greenish.type = 1;

        Material mat_lambert_bluish = {};
        mat_lambert_bluish.albedo = Vector3(0.3f, 0.3f, 0.7f);
        mat_lambert_bluish.type = 1;

        Material mat_mirror = {};
        mat_mirror.albedo = Vector3(0.8f, 0.8f, 0.8f);
        mat_mirror.type = 2;
        mat_mirror.fuzziness = 0.0f;

        Material mat_less_fuzzy_metal = {};
        mat_less_fuzzy_metal.albedo = Vector3(0.8f, 0.8f, 0.8f);
        mat_less_fuzzy_metal.type = 2;
        mat_less_fuzzy_metal.fuzziness = 0.3f;

        Material mat_less_fuzzy_pink_metal = {};
        mat_less_fuzzy_pink_metal.albedo = Vector3(0.6f, 0.0f, 0.3f);
        mat_less_fuzzy_pink_metal.type = 2;
        mat_less_fuzzy_pink_metal.fuzziness = 0.5f;

        Material mat_fuzzy_metal = {};
        mat_fuzzy_metal.albedo = Vector3(0.3f, 0.6f, 0.8f);
        mat_fuzzy_metal.type = 2;
        mat_fuzzy_metal.fuzziness = 0.7f;

        spheres.push_back(Sphere(Vector3(0, 40, 0), 10));
        materials.push_back(mat_light_red);
        
        spheres.push_back(Sphere(Vector3(0, 40, -40), 10));
        materials.push_back(mat_light_green);
        
        spheres.push_back(Sphere(Vector3(0, 40, 40), 10));
        materials.push_back(mat_light_blue);
        
        spheres.push_back(Sphere(Vector3(0, -100.5f, -1), 100));
        materials.push_back(mat_fuzzy_metal);
        
        spheres.push_back(Sphere(Vector3(0, 0, -1), 0.5f));
        materials.push_back(mat_mirror);
        
        spheres.push_back(Sphere(Vector3(0, 1, -1), 0.5f));
        materials.push_back(mat_mirror);
        
        spheres.push_back(Sphere(Vector3(-1.2f, 0, -1.5f), 0.5f));
        materials.push_back(mat_lambert_yellowish);
        
        spheres.push_back(Sphere(Vector3(1.2f, 0, -1), 0.5f));
        materials.push_back(mat_lambert_reddish);
        
        spheres.push_back(Sphere(Vector3(0.5f, -0.2, 0), 0.3f));
        materials.push_back(mat_lambert_greenish);
        
        spheres.push_back(Sphere(Vector3(1.4, -0.2, 0), 0.3));
        materials.push_back(mat_fuzzy_metal);
        
        spheres.push_back(Sphere(Vector3(-0.8f, -0.2, -0.2), 0.3f));
        materials.push_back(mat_lambert_bluish);
        
        spheres.push_back(Sphere(Vector3(-0.2f, -0.4, 0), 0.1f));
        materials.push_back(mat_less_fuzzy_pink_metal);
    }


    const auto aspect_ratio = (float)1920 / 1080;
    Camera camera(Vector3(0, 1, 1), Vector3(0, 0, -1), Vector3(0, 1, 0), aspect_ratio, 90, 0.0f, 1.5f);
    RaytracerProperties properties = { 1920, 1080, 0, spheres.size(), camera};
    RaytracerData raytracer_data = { properties, spheres.data(), materials.data() };
    
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
            {
                done = true;
            }
        }
        if (done)
        {
            break;
        }

        const float clear_color_with_alpha[4] = { 1.0, 1.0, 1.0, 1.0 };
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        
        RECT winRect;
        GetClientRect(window_data.hwnd, &winRect);
        D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (FLOAT)(winRect.right - winRect.left), (FLOAT)(winRect.bottom - winRect.top), 0.0f, 1.0f };
        g_pd3dDeviceContext->RSSetViewports(1, &viewport);

        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);

        raytracer_render(g_pd3dDeviceContext, compute_data, raytracer_data, quad_renderer);

        render_imgui(raytracer_data);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(0, 0); 
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(window_data.hwnd);
    UnregisterClass(window_data.wc.lpszClassName, window_data.wc.hInstance);

    return 0;
}


void render_imgui(RaytracerData& raytracer_data)
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("Playground");
        char buffer[50];
        const char* elements[] = { "Light", "Lambertian", "Metal" };
        
        for (int i = 0; i < raytracer_data.properties.sphere_count; i++) {
            sprintf_s(buffer, "Sphere #%d", i);
            if (ImGui::DragFloat3(buffer, &raytracer_data.spheres[i].center.x, 0.01f)) {
                raytracer_data.properties.frame_count = 0;
            }
            sprintf_s(buffer, "Material #%d", i);
            if (ImGui::ColorEdit3(buffer, &raytracer_data.materials[i].albedo.x)) {
                raytracer_data.properties.frame_count = 0;
            }
            if (raytracer_data.materials[i].type == 2) {
                sprintf_s(buffer, "Fuzziness #%d", i);
                if (ImGui::DragFloat(buffer, &raytracer_data.materials[i].fuzziness, 0.01f, 0, 1)) {
                    raytracer_data.properties.frame_count = 0;
                }
            }

            sprintf_s(buffer, "Material type #%d", i);
            if (ImGui::ListBox(buffer, &raytracer_data.materials[i].type, elements, 3)) {
                raytracer_data.properties.frame_count = 0;
            }
            ImGui::NewLine();
        }

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    ImGui::Render();
}

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();

    if (g_pSwapChain)
    {
        g_pSwapChain->Release(); g_pSwapChain = NULL;
    }
    if (g_pd3dDeviceContext)
    {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = NULL;
    }
    if (g_pd3dDevice) {
        g_pd3dDevice->Release();
        g_pd3dDevice = NULL;
    }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView)
    {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = NULL;
    }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
