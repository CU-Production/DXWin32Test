#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

#include <filament/Camera.h>
#include <filament/Engine.h>
#include <filament/IndexBuffer.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <filament/RenderableManager.h>
#include <filament/Scene.h>
#include <filament/Skybox.h>
#include <filament/TransformManager.h>
#include <filament/VertexBuffer.h>
#include <filament/View.h>
#include <filament/Viewport.h>
#include <filament/Renderer.h>

#include <utils/EntityManager.h>


/**
 * matc -o bakedColor.filament -a all bakedColor.mat
 * bin2header.exe -o bakedColor.filament.h bakedColor.filament
 */
#include "bakedColor.filament.h"

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

#pragma optimize("", off)

struct Vertex {
    filament::math::float2 position;
    uint32_t color;
};

static const Vertex TRIANGLE_VERTICES[3] = {
        {{   0,  0.5}, 0xffff0000u},
        {{-0.5, -0.5}, 0xff00ff00u},
        {{ 0.5, -0.5}, 0xff0000ffu},
};

static constexpr uint16_t TRIANGLE_INDICES[3] = { 0, 1, 2 };

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "GLFW + Filament", nullptr, nullptr);

//    filament::Engine* engine = filament::Engine::create(filament::Engine::Backend::OPENGL);
    filament::Engine* engine = filament::Engine::create(filament::Engine::Backend::VULKAN);
    filament::SwapChain* swapChain = engine->createSwapChain(glfwGetWin32Window(window));
    filament::Renderer* renderer = engine->createRenderer();

    filament::Camera* camera = engine->createCamera(utils::EntityManager::get().create());
//    constexpr float ZOOM = 1.5f;
//    const uint32_t w = SCREEN_WIDTH;
//    const uint32_t h = SCREEN_HEIGHT;
//    const float aspect = (float) w / h;
//    camera->setProjection(filament::Camera::Projection::ORTHO, -aspect * ZOOM, aspect * ZOOM, -ZOOM, ZOOM, 0, 1);

    filament::View* view = engine->createView();
    view->setName("view0");
    view->setViewport({ 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT });
    view->setPostProcessingEnabled(false);

    filament::Scene* scene = engine->createScene();

    filament::Skybox* skybox = filament::Skybox::Builder().color({0.1, 0.125, 0.25, 1.0}).build(*engine);
    scene->setSkybox(skybox);

    filament::Material* material = filament::Material::Builder()
            .package((void*) bakedColor_filament, sizeof(bakedColor_filament))
            .build(*engine);
    filament::MaterialInstance* materialInstance = material->createInstance();

    filament::VertexBuffer* vertexBuffer = filament::VertexBuffer::Builder()
            .vertexCount(3)
            .bufferCount(1)
            .attribute(filament::VertexAttribute::POSITION, 0, filament::VertexBuffer::AttributeType::FLOAT2, 0, 12)
            .attribute(filament::VertexAttribute::COLOR, 0, filament::VertexBuffer::AttributeType::UBYTE4, 8, 12)
            .normalized(filament::VertexAttribute::COLOR)
            .build(*engine);
    vertexBuffer->setBufferAt(*engine, 0, filament::VertexBuffer::BufferDescriptor(TRIANGLE_VERTICES, 36, nullptr));

    filament::IndexBuffer* indexBuffer = filament::IndexBuffer::Builder()
            .indexCount(3)
            .bufferType(filament::IndexBuffer::IndexType::USHORT)
            .build(*engine);
    indexBuffer->setBuffer(*engine, filament::IndexBuffer::BufferDescriptor(TRIANGLE_INDICES, 6, nullptr));

    utils::Entity renderable = utils::EntityManager::get().create();
    // build a triangle
    filament::RenderableManager::Builder(1)
            .boundingBox({{ -1, -1, -1 }, { 1, 1, 1 }})
            .material(0, materialInstance)
            .geometry(0, filament::RenderableManager::PrimitiveType::TRIANGLES, vertexBuffer, indexBuffer, 0, 3)
            .culling(false)
            .receiveShadows(false)
            .castShadows(false)
            .build(*engine, renderable);
    scene->addEntity(renderable);
//    auto& tcm = engine->getTransformManager();
//    tcm.setTransform(tcm.getInstance(renderable), filament::math::mat4f::rotation(0.0, filament::math::float3{ 0, 0, 1 }));

    view->setCamera(camera);
    view->setScene(scene);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        // beginFrame() returns false if we need to skip a frame
        if (renderer->beginFrame(swapChain)) {
            // for each View
            renderer->render(view);
            renderer->endFrame();
        }

    }

    engine->destroy(skybox);
    engine->destroy(renderable);
    engine->destroy(materialInstance);
    engine->destroy(material);
    engine->destroy(vertexBuffer);
    engine->destroy(indexBuffer);
    engine->destroy(swapChain);
    engine->destroy(renderer);
//    engine->destroyCameraComponent(camera);
//    utils::EntityManager::get().destroy(camera);
    filament::Engine::destroy(engine);

    glfwTerminate();
    return EXIT_SUCCESS;
}
