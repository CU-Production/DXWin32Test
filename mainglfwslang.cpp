#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#include "slang.h"
#include "slang-gfx.h"
#include "slang-com-ptr.h"
#include "slang-com-helper.h"
#include <vector>

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

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
