#include <print>
#include <algorithm>
#include "SDL3/SDL.h"
#include "Common/misc.h"

float _vertices[] = {
    -0.5f, -0.5f, 0.0f,     1.0f, 0.0f, 0.0f,
     0.5f, -0.5f, 0.0f,     0.0f, 1.0f, 0.0f,
     0.0f,  0.5f, 0.0f,     0.0f, 0.0f, 1.0f,
};

bool _shouldQuit;

void PollEvents();

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return -1;
    }

    SDL_GPUDevice* graphicsDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);

    if (graphicsDevice == NULL)
    {
        SDL_Log("Failed to create GPU device: %s", SDL_GetError());
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("LearnSDL3", 800, 600, SDL_WINDOW_RESIZABLE);

    if (window == NULL)
    {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return -1;
    }

    if (!SDL_ClaimWindowForGPUDevice(graphicsDevice, window))
    {
        SDL_Log("Failed to clain window for GPU device: %s", SDL_GetError());
        return -1;
    }

    std::optional<Shader> vertexShader = 
        Shader::FromSPV(graphicsDevice, "shaders/vertex_shader.spv", { .ShaderStage = SDL_GPU_SHADERSTAGE_VERTEX });

    if (!vertexShader.has_value())
    {
        return -1;
    }

    std::optional<Shader> fragmentShader = 
        Shader::FromSPV(graphicsDevice, "shaders/fragment_shader.spv", { .ShaderStage = SDL_GPU_SHADERSTAGE_FRAGMENT });

    if (!fragmentShader.has_value())
    {
        return -1;
    }

    PipelineCreateInfo pipelineInfo = PipelineCreateInfo{
        .VertexShader = &vertexShader.value(),
        .FragmentShader = &fragmentShader.value(),
        .VertexBufferDescription = {.Slot = 0, .Pitch = 6 * sizeof(float) },
        .VertexAttributes = {
            SDL_GPUVertexAttribute{ .location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = 0 },
            SDL_GPUVertexAttribute{ .location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = 3 * sizeof(float) }
        },
    };

    std::optional<Pipeline> pipeline = Pipeline::Create(graphicsDevice, window, pipelineInfo);

    if (!pipeline.has_value())
    {
        SDL_Log("Failed to create graphics pipeline: %s", SDL_GetError());
        return -1;
    }

    vertexShader->Release();
    fragmentShader->Release();

    SDL_GPUBufferCreateInfo vertexBufferInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = sizeof(_vertices)
    };

    SDL_GPUBuffer* vertexBuffer = SDL_CreateGPUBuffer(graphicsDevice, &vertexBufferInfo);

    if (vertexBuffer == NULL)
    {
        SDL_Log("Failed to create vertex buffer: %s", SDL_GetError());
        return -1;
    }

    GPUUploader uploader(graphicsDevice);

    uploader.AddVertexData(_vertices, sizeof(_vertices), vertexBuffer);
    uploader.Upload();

    while (!_shouldQuit)
    {
        PollEvents();

        SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(graphicsDevice);

        if (commandBuffer == NULL)
        {
            SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
            return -1;
        }

        SDL_GPUTexture* swapchainTexture;

        if (!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, NULL, NULL))
        {
            SDL_Log("Failed to acquire swapchain texture: %s", SDL_GetError());
            return -1;
        }

        if (swapchainTexture != NULL)
        {
            SDL_GPUColorTargetInfo colorTargetInfo = {
                .texture = swapchainTexture,
                .clear_color = { 139.f / 255.f, 139.f / 255.f, 141.f / 255.f, 1.0f },
                .load_op = SDL_GPU_LOADOP_CLEAR,
                .store_op = SDL_GPU_STOREOP_STORE,
            };

            SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);

            SDL_BindGPUGraphicsPipeline(renderPass, pipeline->GetHandle());

            SDL_GPUBufferBinding bufferBinding = { .buffer = vertexBuffer, .offset = 0 };
            SDL_BindGPUVertexBuffers(renderPass, 0, &bufferBinding, 1);
            SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);

            SDL_EndGPURenderPass(renderPass);
        }

        SDL_SubmitGPUCommandBuffer(commandBuffer);
    }

    SDL_ReleaseGPUBuffer(graphicsDevice, vertexBuffer);
    pipeline->Release();
    SDL_ReleaseWindowFromGPUDevice(graphicsDevice, window);
    SDL_DestroyGPUDevice(graphicsDevice);
    SDL_DestroyWindow(window);
    return 0;
}

void ProcessInput(const SDL_Event& event);

void PollEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            _shouldQuit = true;
            break;
        case SDL_EVENT_KEY_DOWN:
            ProcessInput(event);
            break;
        }
    }
}

void ProcessInput(const SDL_Event& event)
{
    if (event.key.key == SDLK_ESCAPE)
    {
        _shouldQuit = true;
    }
}