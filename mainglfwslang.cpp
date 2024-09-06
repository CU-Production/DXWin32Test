#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#include "slang.h"
#include "slang-gfx.h"
#include "slang-com-ptr.h"
#include "slang-com-helper.h"
#include <vector>
#include <iostream>

static const int SCREEN_WIDTH = 800;
static const int SCREEN_HEIGHT = 600;
static const int kSwapchainImageCount = 2;

struct Vertex
{
    float position[3];
    float color[3];
};

static const int kVertexCount = 3;
static const Vertex kVertexData[kVertexCount] =
        {
                { { 0,  0, 0.5 }, { 1, 0, 0 } },
                { { 0,  1, 0.5 }, { 0, 0, 1 } },
                { { 1,  0, 0.5 }, { 0, 1, 0 } },
        };

struct MyDebugCallback : public gfx::IDebugCallback
{
    virtual SLANG_NO_THROW void SLANG_MCALL handleMessage(
            gfx::DebugMessageType type,
            gfx::DebugMessageSource source,
            const char* message) override
    {
        const char* typeStr = "";
        switch (type)
        {
            case gfx::DebugMessageType::Info:
                typeStr = "INFO: ";
                break;
            case gfx::DebugMessageType::Warning:
                typeStr = "WARNING: ";
                break;
            case gfx::DebugMessageType::Error:
                typeStr = "ERROR: ";
                break;
            default:
                break;
        }
        const char* sourceStr = "";
        switch (source)
        {
            case gfx::DebugMessageSource::Slang:
                sourceStr = "[Slang]: ";
                break;
            case gfx::DebugMessageSource::Driver:
                sourceStr = "[Driver]: ";
                break;
            case gfx::DebugMessageSource::Layer:
                sourceStr = "[GraphicsLayer]: ";
                break;
            default:
                break;
        }
        printf("%s%s%s\n", sourceStr, typeStr, message);
    }
};
MyDebugCallback gCallback;

Slang::ComPtr<gfx::IDevice> gDevice;

Slang::ComPtr<gfx::ISwapchain> gSwapchain;
Slang::ComPtr<gfx::IFramebufferLayout> gFramebufferLayout;
std::vector<Slang::ComPtr<gfx::IFramebuffer>> gFramebuffers;
std::vector<Slang::ComPtr<gfx::ITransientResourceHeap>> gTransientHeaps;
Slang::ComPtr<gfx::IRenderPassLayout> gRenderPass;
Slang::ComPtr<gfx::ICommandQueue> gQueue;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "GLFW + DX11", nullptr, nullptr);

    // void initGfx()
    {
        gfx::gfxEnableDebugLayer();
        gfx::gfxSetDebugCallback(&gCallback);

        gfx::IDevice::Desc deviceDesc = {};
        deviceDesc.deviceType = gfx::DeviceType::Default;
//        deviceDesc.deviceType = gfx::DeviceType::DirectX12;
//        deviceDesc.deviceType = gfx::DeviceType::Vulkan;
        gfx::gfxCreateDevice(&deviceDesc, gDevice.writeRef());

        auto deviceInfo = gDevice->getDeviceInfo();
        std::cout << "Using Device (" << deviceInfo.apiName << ": " << deviceInfo.adapterName << ")";

        gfx::ICommandQueue::Desc queueDesc = {};
        queueDesc.type = gfx::ICommandQueue::QueueType::Graphics;
        gQueue = gDevice->createCommandQueue(queueDesc);

        gfx::ISwapchain::Desc swapchainDesc = {};
        swapchainDesc.format     = gfx::Format::R8G8B8A8_UNORM;
        swapchainDesc.width      = SCREEN_WIDTH;
        swapchainDesc.height     = SCREEN_HEIGHT;
        swapchainDesc.imageCount = kSwapchainImageCount;
        swapchainDesc.queue      = gQueue;
        gfx::WindowHandle windowHandle = gfx::WindowHandle::FromHwnd(glfwGetWin32Window(window)); // glfw or Win32API
        gSwapchain = gDevice->createSwapchain(swapchainDesc, windowHandle);

        gfx::IFramebufferLayout::TargetLayout renderTargetLayout = {gSwapchain->getDesc().format, 1};
        gfx::IFramebufferLayout::TargetLayout depthLayout = {gfx::Format::D32_FLOAT, 1};
        gfx::IFramebufferLayout::Desc framebufferLayoutDesc;
        framebufferLayoutDesc.renderTargetCount = 1;
        framebufferLayoutDesc.renderTargets     = &renderTargetLayout;
        framebufferLayoutDesc.depthStencil      = &depthLayout;
        SLANG_RETURN_ON_FAIL(
                gDevice->createFramebufferLayout(framebufferLayoutDesc, gFramebufferLayout.writeRef()));

        // createSwapchainFramebuffers
        gFramebuffers.clear();
        for (uint32_t i = 0; i < kSwapchainImageCount; i++)
        {
            gfx::ITextureResource::Desc depthBufferDesc = {};
            depthBufferDesc.type          = gfx::IResource::Type::Texture2D;
            depthBufferDesc.size.width    = gSwapchain->getDesc().width;
            depthBufferDesc.size.height   = gSwapchain->getDesc().height;
            depthBufferDesc.size.depth    = 1;
            depthBufferDesc.format        = gfx::Format::D32_FLOAT;
            depthBufferDesc.defaultState  = gfx::ResourceState::DepthWrite;
            depthBufferDesc.allowedStates = gfx::ResourceStateSet(gfx::ResourceState::DepthWrite);
            gfx::ClearValue depthClearValue = {};
            depthBufferDesc.optimalClearValue = &depthClearValue;
            Slang::ComPtr<gfx::ITextureResource> depthBufferResource =
                    gDevice->createTextureResource(depthBufferDesc, nullptr);
            Slang::ComPtr<gfx::ITextureResource> colorBuffer;
            gSwapchain->getImage(i, colorBuffer.writeRef());

            gfx::IResourceView::Desc colorBufferViewDesc = {};
            memset(&colorBufferViewDesc, 0, sizeof(colorBufferViewDesc));
            colorBufferViewDesc.format             = gSwapchain->getDesc().format;
            colorBufferViewDesc.renderTarget.shape = gfx::IResource::Type::Texture2D;
            colorBufferViewDesc.type               = gfx::IResourceView::Type::RenderTarget;
            Slang::ComPtr<gfx::IResourceView> rtv =
                    gDevice->createTextureView(colorBuffer.get(), colorBufferViewDesc);

            gfx::IResourceView::Desc depthBufferViewDesc = {};
            memset(&depthBufferViewDesc, 0, sizeof(depthBufferViewDesc));
            depthBufferViewDesc.format             = gfx::Format::D32_FLOAT;
            depthBufferViewDesc.renderTarget.shape = gfx::IResource::Type::Texture2D;
            depthBufferViewDesc.type               = gfx::IResourceView::Type::DepthStencil;
            Slang::ComPtr<gfx::IResourceView> dsv =
                    gDevice->createTextureView(depthBufferResource.get(), depthBufferViewDesc);

            gfx::IFramebuffer::Desc framebufferDesc = {};
            framebufferDesc.renderTargetCount = 1;
            framebufferDesc.depthStencilView  = dsv.get();
            framebufferDesc.renderTargetViews = rtv.readRef();
            framebufferDesc.layout = gFramebufferLayout;
            Slang::ComPtr<gfx::IFramebuffer> frameBuffer = gDevice->createFramebuffer(framebufferDesc);
            gFramebuffers.push_back(frameBuffer);
        }

        for (uint32_t i = 0; i < kSwapchainImageCount; i++)
        {
            gfx::ITransientResourceHeap::Desc transientHeapDesc = {};
            transientHeapDesc.constantBufferSize = 4096 * 1024;
            auto transientHeap = gDevice->createTransientResourceHeap(transientHeapDesc);
            gTransientHeaps.push_back(transientHeap);
        }

        gfx::IRenderPassLayout::Desc renderPassDesc = {};
        renderPassDesc.framebufferLayout = gFramebufferLayout;
        renderPassDesc.renderTargetCount = 1;
        gfx::IRenderPassLayout::TargetAccessDesc renderTargetAccess = {};
        gfx::IRenderPassLayout::TargetAccessDesc depthStencilAccess = {};
        renderTargetAccess.loadOp       = gfx::IRenderPassLayout::TargetLoadOp::Clear;
        renderTargetAccess.storeOp      = gfx::IRenderPassLayout::TargetStoreOp::Store;
        renderTargetAccess.initialState = gfx::ResourceState::Undefined;
        renderTargetAccess.finalState   = gfx::ResourceState::Present;
        depthStencilAccess.loadOp       = gfx::IRenderPassLayout::TargetLoadOp::Clear;
        depthStencilAccess.storeOp      = gfx::IRenderPassLayout::TargetStoreOp::Store;
        depthStencilAccess.initialState = gfx::ResourceState::DepthWrite;
        depthStencilAccess.finalState   = gfx::ResourceState::DepthWrite;
        renderPassDesc.renderTargetAccess = &renderTargetAccess;
        renderPassDesc.depthStencilAccess = &depthStencilAccess;
        gRenderPass = gDevice->createRenderPassLayout(renderPassDesc);
    }

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

    }


    glfwTerminate();
    return EXIT_SUCCESS;
}
