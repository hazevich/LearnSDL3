#include <print>
#include "SDL3/SDL.h"
#include "Common/misc.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

float _vertices[] = {
    // vertex              // color             // texture coordinates
    -0.5f,  0.5f, 0.0f,    0.0f, 1.0f, 1.0f,    0.0f, 1.0f, // top left
    -0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, // bottom left
     0.5f, -0.5f, 0.0f,    1.0f, 0.0f, 0.0f,    1.0f, 0.0f, // bottom right
     0.5f,  0.5f, 0.0f,    1.0f, 1.0f, 0.0f,    1.0f, 1.0f, // top right
};

uint32_t _indices[] = {
    0, 1, 2,
    0, 2, 3,
};

bool _shouldQuit;

Time _time;

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
        Shader::FromSPV(graphicsDevice, "shaders/vertex_shader.spv", { .ShaderStage = SDL_GPU_SHADERSTAGE_VERTEX, .NumUniformBuffers = 1 });

    if (vertexShader == std::nullopt)
    {
        SDL_Log("Failed to create vertex shader: %s", SDL_GetError());
        return -1;
    }

    std::optional<Shader> fragmentShader = 
        Shader::FromSPV(graphicsDevice, "shaders/fragment_shader.spv", { .ShaderStage = SDL_GPU_SHADERSTAGE_FRAGMENT, .NumSamplers = 2 });

    if (fragmentShader == std::nullopt)
    {
        SDL_Log("Failed to create fragment shader: %s", SDL_GetError());
        return -1;
    }

    PipelineCreateInfo pipelineInfo = {
        .VertexShader = &vertexShader.value(),
        .FragmentShader = &fragmentShader.value(),
        .VertexBufferDescription = {.Slot = 0, .Pitch = 8 * sizeof(float) },
        .VertexAttributes = {
            SDL_GPUVertexAttribute{.location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = 0 },
            SDL_GPUVertexAttribute{.location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = 3 * sizeof(float)},
            SDL_GPUVertexAttribute{.location = 2, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = 6 * sizeof(float)},
        },
    };

    std::optional<Pipeline> pipeline = Pipeline::Create(graphicsDevice, window, pipelineInfo);

    if (pipeline == std::nullopt)
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

    SDL_GPUBufferCreateInfo indexBufferInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = sizeof(_indices),
    };

    SDL_GPUBuffer* indexBuffer = SDL_CreateGPUBuffer(graphicsDevice, &indexBufferInfo);

    if (indexBuffer == NULL)
    {
        SDL_Log("Failed to create index buffer: %s", SDL_GetError());
        return -1;
    }

    Image* containerImage = LoadImage("assets/container.png");

    SDL_GPUTextureCreateInfo containerTextureInfo = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = containerImage->Width,
        .height = containerImage->Height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };

    SDL_GPUTexture* containerTexture = SDL_CreateGPUTexture(graphicsDevice, &containerTextureInfo);

    Image* awesomefaceImage = LoadImage("assets/awesomeface.png");

    SDL_GPUTextureCreateInfo awesomefaceTextureInfo = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = awesomefaceImage->Width,
        .height = awesomefaceImage->Height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };

    SDL_GPUTexture* awesomefaceTexture = SDL_CreateGPUTexture(graphicsDevice, &awesomefaceTextureInfo);

    GPUUploader gpuUploader(graphicsDevice);

    gpuUploader.AddVertexData(_vertices, sizeof(_vertices), vertexBuffer, 0);
    gpuUploader.AddIndexData(_indices, sizeof(_indices), indexBuffer, 0);
    gpuUploader.AddTextureData(containerImage->Data, containerImage->Width, containerImage->Height, containerTexture);
    gpuUploader.AddTextureData(awesomefaceImage->Data, awesomefaceImage->Width, awesomefaceImage->Height, awesomefaceTexture);

    if (!gpuUploader.Upload())
    {
        SDL_Log("Could not upload data to GPU: %s", SDL_GetError());
        return -1;
    }

    delete containerImage;
    delete awesomefaceImage;

    SDL_GPUSamplerCreateInfo samplerInfo = {
        .min_filter = SDL_GPU_FILTER_LINEAR,
        .mag_filter = SDL_GPU_FILTER_LINEAR,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    };

    SDL_GPUSampler* sampler = SDL_CreateGPUSampler(graphicsDevice, &samplerInfo);

    while (!_shouldQuit)
    {
        TickTime(_time);

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

            SDL_GPUBufferBinding vertexBufferBinding = { .buffer = vertexBuffer, .offset = 0 };
            SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);

            SDL_GPUBufferBinding indexBufferBinding = { .buffer = indexBuffer, .offset = 0 };
            SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

            SDL_GPUTextureSamplerBinding textureSamplers[] = {
                { .texture = containerTexture, .sampler = sampler },
                { .texture = awesomefaceTexture, .sampler = sampler },
            };
            SDL_BindGPUFragmentSamplers(renderPass, 0, textureSamplers, 2);


            glm::mat4 transformation = glm::mat4(1.0f);
            // we translate and only then rotate translated coordinates, which makes the object rotate around the origin
            // instead of itself
            transformation = glm::rotate(transformation, _time.TotalTime, glm::vec3(0.0f, 0.0f, 1.0f));
            transformation = glm::translate(transformation, glm::vec3(0.5f, -0.5f, 0.0f));
            SDL_PushGPUVertexUniformData(commandBuffer, 0, glm::value_ptr(transformation), sizeof(transformation));

            SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);

            SDL_EndGPURenderPass(renderPass);
        }

        SDL_SubmitGPUCommandBuffer(commandBuffer);
    }

    SDL_ReleaseGPUBuffer(graphicsDevice, vertexBuffer);
    SDL_ReleaseGPUBuffer(graphicsDevice, indexBuffer);
    SDL_ReleaseGPUTexture(graphicsDevice, containerTexture);
    SDL_ReleaseGPUTexture(graphicsDevice, awesomefaceTexture);
    pipeline->Release();
    SDL_ReleaseGPUSampler(graphicsDevice, sampler);
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