#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

IDXGISwapChain* g_swapchain;
ID3D11Device* g_dev;
ID3D11DeviceContext* g_devcon;
ID3D11RenderTargetView* g_backbuffer;
ID3D11VertexShader* g_VS;
ID3D11PixelShader* g_PS;
ID3D11InputLayout* g_layout;
ID3D11Buffer* g_vertexbuffer;

struct VERTEX
{
    float X, Y, Z;
    float R, G, B, A;
};

void InitD3D(HWND hWnd);
void RenderFrame();
void CleanD3D();
void InitPipeline();
void InitGraphics();

LRESULT CALLBACK WindowProc(HWND hWnd,
                            UINT message,
                            WPARAM wParam,
                            LPARAM lParam);

static const char* g_shader = R"(
struct VSOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VSOut VSmain(float4 position : POSITION, float4 color : COLOR)
{
    VSOut output;

    output.position = position;
    output.color = color;

    return output;
}

float4 PSmain(float4 position : SV_POSITION, float4 color : COLOR) : SV_TARGET
{
    return color;
}
)";

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nShowCmd)
{
//    MessageBox(NULL, "Hello World!", "Just another Hello World program!", MB_ICONEXCLAMATION | MB_OK);

    HWND hWnd; // the handle for the window, filled by a function
    WNDCLASSEXW wc; // this struct holds information for the window class

    ZeroMemory(&wc, sizeof(WNDCLASSEXW));

    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = L"WindowClass1";

    RegisterClassExW(&wc);

    RECT wr = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    hWnd = CreateWindowExW(NULL,
                          L"WindowClass1",    // name of the window class
                          L"Our First Windowed Program",   // title of the window
                          WS_OVERLAPPEDWINDOW,    // window style
                          300,    // x-position of the window
                          300,    // y-position of the window
                          wr.right - wr.left,    // width of the window
                          wr.bottom - wr.top,    // height of the window
                          NULL,    // we have no parent window, NULL
                          NULL,    // we aren't using menus, NULL
                          hInstance,    // application handle
                          NULL);    // used with multiple windows, NULL

    ShowWindow(hWnd, nShowCmd);

    InitD3D(hWnd);

    MSG msg = {0};

    while (TRUE)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
                break;
        }

        RenderFrame();
    }

    CleanD3D();

    return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        } break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

void InitD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC scd;
    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

    scd.BufferCount = 1;                                    // one back buffer
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color
    scd.BufferDesc.Width = SCREEN_WIDTH;                    // set the back buffer width
    scd.BufferDesc.Height = SCREEN_HEIGHT;                  // set the back buffer height
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
    scd.OutputWindow = hWnd;                                // the window to be used
    scd.SampleDesc.Count = 4;                               // how many multisamples
    scd.Windowed = TRUE;                                    // windowed/full-screen mode
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;     // allow full-screen switching

    D3D11CreateDeviceAndSwapChain(NULL,
                                  D3D_DRIVER_TYPE_HARDWARE,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  D3D11_SDK_VERSION,
                                  &scd,
                                  &g_swapchain,
                                  &g_dev,
                                  NULL,
                                  &g_devcon);

    ID3D11Texture2D* pBackBuffer;
    g_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    g_dev->CreateRenderTargetView(pBackBuffer, NULL, &g_backbuffer);

    g_devcon->OMSetRenderTargets(1, &g_backbuffer, NULL);

    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = SCREEN_WIDTH;
    viewport.Height = SCREEN_HEIGHT;

    g_devcon->RSSetViewports(1, &viewport);

    InitPipeline();
    InitGraphics();
}

void CleanD3D()
{
    g_swapchain->SetFullscreenState(FALSE, NULL); // switch to windowed mode

    g_VS->Release();
    g_PS->Release();
    g_vertexbuffer->Release();
    g_layout->Release();
    g_swapchain->Release();
    g_dev->Release();
    g_devcon->Release();
    g_backbuffer->Release();
}

void RenderFrame()
{
    float color[4] = {0.0f, 0.2f, 0.4f, 1.0f};
    g_devcon->ClearRenderTargetView(g_backbuffer, color);

    {
        UINT stride = sizeof(VERTEX);
        UINT offset = 0;

        g_devcon->IASetVertexBuffers(0, 1, &g_vertexbuffer, &stride, &offset);
        g_devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_devcon->Draw(3, 0);
    }

    g_swapchain->Present(1, 0);
}

void InitPipeline()
{
    ID3DBlob *pVSblob, *pPSblob;
    ID3DBlob* pErrorBlob;

    D3DCompile(g_shader, strlen(g_shader), nullptr, nullptr, nullptr, "VSmain", "vs_5_0", 0, 0, &pVSblob, &pErrorBlob);
    D3DCompile(g_shader, strlen(g_shader), nullptr, nullptr, nullptr, "PSmain", "ps_5_0", 0, 0, &pPSblob, &pErrorBlob);

    g_dev->CreateVertexShader(pVSblob->GetBufferPointer(), pVSblob->GetBufferSize(), NULL, &g_VS);
    g_dev->CreatePixelShader(pPSblob->GetBufferPointer(), pPSblob->GetBufferSize(), NULL, &g_PS);

    g_devcon->VSSetShader(g_VS, NULL, 0);
    g_devcon->PSSetShader(g_PS, NULL, 0);

    D3D11_INPUT_ELEMENT_DESC ied[] =
    {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    g_dev->CreateInputLayout(ied, 2, pVSblob->GetBufferPointer(), pVSblob->GetBufferSize(), &g_layout);
    g_devcon->IASetInputLayout(g_layout);

    pVSblob->Release();
    pPSblob->Release();
}

void InitGraphics()
{
    VERTEX OurVertices[] =
    {
        {0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f},
        {0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f},
        {-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f},
    };

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));

    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(VERTEX) * 3;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    g_dev->CreateBuffer(&bd, NULL, &g_vertexbuffer);

    D3D11_MAPPED_SUBRESOURCE ms;
    g_devcon->Map(g_vertexbuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
    memcpy(ms.pData, OurVertices, sizeof(OurVertices));
    g_devcon->Unmap(g_vertexbuffer, NULL);
}
