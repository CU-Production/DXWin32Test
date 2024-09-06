#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#include "slang.h"
#include "slang-gfx.h"
#include "slang-com-ptr.h"
#include "slang-com-helper.h"
#include "gfx-util/shader-cursor.h"
#include <vector>
#include <iostream>
#include <string>

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

Slang::ComPtr<gfx::IPipelineState> gPipelineState;
Slang::ComPtr<gfx::IBufferResource> gVertexBuffer;

gfx::Result loadShaderProgram(
        gfx::IDevice*         device,
        gfx::IShaderProgram** outProgram);

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "GLFW + Slang-gfx", nullptr, nullptr);

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

    // triangle demo
    {
        gfx::InputElementDesc inputElements[] =
        {
            { "POSITION", 0, gfx::Format::R32G32B32_FLOAT, offsetof(Vertex, position) },
            { "COLOR",    0, gfx::Format::R32G32B32_FLOAT, offsetof(Vertex, color) },
        };
        auto inputLayout = gDevice->createInputLayout(sizeof(Vertex), &inputElements[0], 2);

        gfx::IBufferResource::Desc vertexBufferDesc = {};
        vertexBufferDesc.type         = gfx::IResource::Type::Buffer;
        vertexBufferDesc.sizeInBytes  = kVertexCount * sizeof(Vertex);
        vertexBufferDesc.defaultState = gfx::ResourceState::VertexBuffer;
        gVertexBuffer = gDevice->createBufferResource(vertexBufferDesc, &kVertexData[0]);

        Slang::ComPtr<gfx::IShaderProgram> shaderProgram;
        SLANG_RETURN_ON_FAIL(loadShaderProgram(gDevice, shaderProgram.writeRef()));

        gfx::GraphicsPipelineStateDesc desc = {};
        desc.inputLayout = inputLayout;
        desc.program = shaderProgram;
        desc.framebufferLayout = gFramebufferLayout;
        auto pipelineState = gDevice->createGraphicsPipelineState(desc);
        if (!pipelineState)
            return SLANG_FAIL;

        gPipelineState = pipelineState;
    }

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        // main loop
        {
            int frameBufferIndex = gSwapchain->acquireNextImage();
            if (frameBufferIndex == -1)
                return EXIT_FAILURE;

            gTransientHeaps[frameBufferIndex]->synchronizeAndReset();
            //renderFrame(frameBufferIndex);
            {
                Slang::ComPtr<gfx::ICommandBuffer> commandBuffer = gTransientHeaps[frameBufferIndex]->createCommandBuffer();
                auto renderEncoder = commandBuffer->encodeRenderCommands(gRenderPass, gFramebuffers[frameBufferIndex]);

                gfx::Viewport viewport = {};
                viewport.maxZ = 1.0f;
                viewport.extentX = (float)SCREEN_WIDTH;
                viewport.extentY = (float)SCREEN_HEIGHT;
                renderEncoder->setViewportAndScissor(viewport);

                auto rootObject = renderEncoder->bindPipeline(gPipelineState);
                auto deviceInfo = gDevice->getDeviceInfo();

                gfx::ShaderCursor rootCursor(rootObject);
                rootCursor["Uniforms"]["modelViewProjection"].setData(deviceInfo.identityProjectionMatrix, sizeof(float) * 16);

                renderEncoder->setVertexBuffer(0, gVertexBuffer);
                renderEncoder->setPrimitiveTopology(gfx::PrimitiveTopology::TriangleList);
                renderEncoder->draw(3);
                renderEncoder->endEncoding();
                commandBuffer->close();
                gQueue->executeCommandBuffer(commandBuffer);
                gSwapchain->present();
            }
            gTransientHeaps[frameBufferIndex]->finish();
        }
    }


    glfwTerminate();
    return EXIT_SUCCESS;
}

void diagnoseIfNeeded(slang::IBlob* diagnosticsBlob)
{
    if( diagnosticsBlob != nullptr )
    {
        printf("%s", (const char*) diagnosticsBlob->getBufferPointer());
    }
}

gfx::Result loadShaderProgram(
        gfx::IDevice*         device,
        gfx::IShaderProgram** outProgram)
{
    // We need to obatin a compilation session (`slang::ISession`) that will provide
    // a scope to all the compilation and loading of code we do.
    //
    // Our example application uses the `gfx` graphics API abstraction layer, which already
    // creates a Slang compilation session for us, so we just grab and use it here.
    Slang::ComPtr<slang::ISession> slangSession;
    slangSession = device->getSlangSession();

    // We can now start loading code into the slang session.
    //
    // The simplest way to load code is by calling `loadModule` with the name of a Slang
    // module. A call to `loadModule("MyStuff")` will behave more or less as if you
    // wrote:
    //
    //      import MyStuff;
    //
    // In a Slang shader file. The compiler will use its search paths to try to locate
    // `MyModule.slang`, then compile and load that file. If a matching module had
    // already been loaded previously, that would be used directly.
    //
    Slang::ComPtr<slang::IBlob> diagnosticsBlob;
    std::string path = "triangle.slang";
    slang::IModule* module = slangSession->loadModule(path.c_str(), diagnosticsBlob.writeRef());
    diagnoseIfNeeded(diagnosticsBlob);
    if(!module)
        return SLANG_FAIL;

    // Loading the `shaders` module will compile and check all the shader code in it,
    // including the shader entry points we want to use. Now that the module is loaded
    // we can look up those entry points by name.
    //
    // Note: If you are using this `loadModule` approach to load your shader code it is
    // important to tag your entry point functions with the `[shader("...")]` attribute
    // (e.g., `[shader("vertex")] void vertexMain(...)`). Without that information there
    // is no umambiguous way for the compiler to know which functions represent entry
    // points when it parses your code via `loadModule()`.
    //
    Slang::ComPtr<slang::IEntryPoint> vertexEntryPoint;
    SLANG_RETURN_ON_FAIL(module->findEntryPointByName("vertexMain", vertexEntryPoint.writeRef()));
    //
    Slang::ComPtr<slang::IEntryPoint> fragmentEntryPoint;
    SLANG_RETURN_ON_FAIL(module->findEntryPointByName("fragmentMain", fragmentEntryPoint.writeRef()));

    // At this point we have a few different Slang API objects that represent
    // pieces of our code: `module`, `vertexEntryPoint`, and `fragmentEntryPoint`.
    //
    // A single Slang module could contain many different entry points (e.g.,
    // four vertex entry points, three fragment entry points, and two compute
    // shaders), and before we try to generate output code for our target API
    // we need to identify which entry points we plan to use together.
    //
    // Modules and entry points are both examples of *component types* in the
    // Slang API. The API also provides a way to build a *composite* out of
    // other pieces, and that is what we are going to do with our module
    // and entry points.
    //
    std::vector<slang::IComponentType*> componentTypes;
    componentTypes.push_back(module);

    // Later on when we go to extract compiled kernel code for our vertex
    // and fragment shaders, we will need to make use of their order within
    // the composition, so we will record the relative ordering of the entry
    // points here as we add them.
    int entryPointCount = 0;
    int vertexEntryPointIndex = entryPointCount++;
    componentTypes.push_back(vertexEntryPoint);

    int fragmentEntryPointIndex = entryPointCount++;
    componentTypes.push_back(fragmentEntryPoint);

    // Actually creating the composite component type is a single operation
    // on the Slang session, but the operation could potentially fail if
    // something about the composite was invalid (e.g., you are trying to
    // combine multiple copies of the same module), so we need to deal
    // with the possibility of diagnostic output.
    //
    Slang::ComPtr<slang::IComponentType> linkedProgram;
    SlangResult result = slangSession->createCompositeComponentType(
            componentTypes.data(),
            componentTypes.size(),
            linkedProgram.writeRef(),
            diagnosticsBlob.writeRef());
    diagnoseIfNeeded(diagnosticsBlob);
    SLANG_RETURN_ON_FAIL(result);

    // Once we've described the particular composition of entry points
    // that we want to compile, we defer to the graphics API layer
    // to extract compiled kernel code and load it into the API-specific
    // program representation.
    //
    gfx::IShaderProgram::Desc programDesc = {};
    programDesc.slangGlobalScope = linkedProgram;
    SLANG_RETURN_ON_FAIL(device->createProgram(programDesc, outProgram));

    return SLANG_OK;
}
