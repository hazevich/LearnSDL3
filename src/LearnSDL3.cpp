// LearnSDL3.cpp : Defines the entry point for the application.
//

#define SDL_MAIN_USE_CALLBACKS

#include "LearnSDL3.h"
#include "SDL3/SDL_main.h"
#include "SDL3/SDL.h"

using namespace std;

struct Vertex
{
    float x, y, z;
    float r, g, b, a;
};

static Vertex _vertices[]
{
    { 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f },
    { -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f },
    { 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f },
};

SDL_Window* _window;
SDL_GPUDevice* _graphics;
SDL_GPUBuffer* _vertexBuffer;
SDL_GPUTransferBuffer* _transferBuffer;
SDL_GPUShader* _vertexShader;
SDL_GPUShader* _fragmentShader;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
    _window = SDL_CreateWindow("LearnSDL3", 1920, 1080, SDL_WINDOW_RESIZABLE);
    _graphics = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    SDL_ClaimWindowForGPUDevice(_graphics, _window);

    SDL_GPUBufferCreateInfo bufferInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = sizeof(_vertices),
    };

    _vertexBuffer = SDL_CreateGPUBuffer(_graphics, &bufferInfo);

    SDL_GPUTransferBufferCreateInfo transferInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = sizeof(_vertices),
    };

    _transferBuffer = SDL_CreateGPUTransferBuffer(_graphics, &transferInfo);

    Vertex* transferData = (Vertex*) SDL_MapGPUTransferBuffer(_graphics, _transferBuffer, false);

    // data[i] = vertices[i]
    SDL_memcpy(transferData, _vertices, sizeof(_vertices));

    SDL_UnmapGPUTransferBuffer(_graphics, _transferBuffer);

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(_graphics);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

    SDL_GPUTransferBufferLocation transferLocation = {
        .transfer_buffer = _transferBuffer,
        .offset = 0,
    };

    SDL_GPUBufferRegion region = {
        .buffer = _vertexBuffer,
        .offset = 0,
        .size = sizeof(_vertices),
    };

    SDL_UploadToGPUBuffer(copyPass, &transferLocation, &region, true);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);

    size_t vertexCodeSize;
    void* vertexCode = SDL_LoadFile("shaders/vertex.spv", &vertexCodeSize);

    SDL_GPUShaderCreateInfo vertexShaderInfo = {
        .code_size = vertexCodeSize,
        .code = (Uint8*)vertexCode,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = SDL_GPU_SHADERSTAGE_VERTEX,
        .num_samplers = 0,
        .num_storage_textures = 0,
        .num_storage_buffers = 0,
        .num_uniform_buffers = 0,
    };

    _vertexShader = SDL_CreateGPUShader(_graphics, &vertexShaderInfo);

    SDL_free(vertexCode);

    size_t fragmentCodeSize;
    void* fragmentCode = SDL_LoadFile("shaders/fragment.spv", &fragmentCodeSize);

    SDL_GPUShaderCreateInfo fragmentShaderInfo = {
        .code_size = fragmentCodeSize,
        .code = (Uint8*)fragmentCode,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = SDL_GPU_SHADERSTAGE_VERTEX,
        .num_samplers = 0,
        .num_storage_textures = 0,
        .num_storage_buffers = 0,
        .num_uniform_buffers = 0,
    };

    _fragmentShader = SDL_CreateGPUShader(_graphics, &fragmentShaderInfo);

    SDL_free(fragmentCode);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(_graphics);


    SDL_GPUTexture* swapchainTexture;
    Uint32 width, height;
    SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, _window, &swapchainTexture, &width, &height);

    SDL_GPUColorTargetInfo colotTargetInfo = {
        .texture = swapchainTexture,
        .clear_color = { 58.f/255.f, 58.f/255.f, 58.f/255.f,},
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
    };

    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colotTargetInfo, 1, NULL);


    SDL_EndGPURenderPass(renderPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
    {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    SDL_ReleaseGPUBuffer(_graphics, _vertexBuffer);
    SDL_ReleaseGPUTransferBuffer(_graphics, _transferBuffer);

    SDL_ReleaseGPUShader(_graphics, _vertexShader);
    SDL_ReleaseGPUShader(_graphics, _fragmentShader);

    SDL_DestroyWindow(_window);
    SDL_DestroyGPUDevice(_graphics);
}
