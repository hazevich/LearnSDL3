#include <print>
#include "SDL3/SDL.h"

float _vertices[] = {
    -0.5f,  0.5f, 0.0f, // top left
    -0.5f, -0.5f, 0.0f, // bottom left
     0.5f, -0.5f, 0.0f, // bottom right
     0.5f,  0.5f, 0.0f, // top right
};

uint32_t _indices[] = {
    0, 1, 2,
    0, 2, 3,
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

    size_t vertexShaderCodeSize;
    void* vertexShaderCode = SDL_LoadFile("shaders/vertex_shader.spv", &vertexShaderCodeSize);

    if (vertexShaderCode == NULL)
    {
        SDL_Log("Failed to load vertex shader");
        return -1;
    }

    SDL_GPUShaderCreateInfo vertexShaderInfo = {
        .code_size = vertexShaderCodeSize,
        .code = (uint8_t*)vertexShaderCode,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = SDL_GPU_SHADERSTAGE_VERTEX,
        .num_samplers = 0,
        .num_storage_textures = 0,
        .num_storage_buffers = 0,
        .num_uniform_buffers = 0
    };

    SDL_GPUShader* vertexShader = SDL_CreateGPUShader(graphicsDevice, &vertexShaderInfo);

    if (vertexShader == NULL)
    {
        SDL_Log("Failed to create vertex shader: %s", SDL_GetError());
        return -1;
    }

    SDL_free(vertexShaderCode);

    size_t fragmentShaderCodeSize;
    void* fragmentShaderCode = SDL_LoadFile("shaders/fragment_shader.spv", &fragmentShaderCodeSize);

    if (fragmentShaderCode == NULL)
    {
        SDL_Log("Failed to load fragment shader");
        return -1;
    }

    SDL_GPUShaderCreateInfo fragmentShaderInfo = {
        .code_size = fragmentShaderCodeSize,
        .code = (uint8_t*)fragmentShaderCode,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
        .num_samplers = 0,
        .num_storage_textures = 0,
        .num_storage_buffers = 0,
        .num_uniform_buffers = 0
    };

    SDL_GPUShader* fragmentShader = SDL_CreateGPUShader(graphicsDevice, &fragmentShaderInfo);

    if (fragmentShader == NULL)
    {
        SDL_Log("Failed to create fragment shader: %s", SDL_GetError());
        return -1;
    }

    SDL_free(fragmentShaderCode);

    SDL_GPUVertexBufferDescription vertexBufferDescriptions[1];
    vertexBufferDescriptions[0] = {
        .slot = 0,
        .pitch = 3 * sizeof(float),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        .instance_step_rate = 0,
    };

    SDL_GPUVertexAttribute vertexAttributes[1];
    vertexAttributes[0] = {
        .location = 0,
        .buffer_slot = 0,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
        .offset = 0
    };

    SDL_GPUColorTargetDescription colorTargetDescriptions[1];
    colorTargetDescriptions[0] = {
        .format = SDL_GetGPUSwapchainTextureFormat(graphicsDevice, window)
    };
    
    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {
        .vertex_shader = vertexShader,
        .fragment_shader = fragmentShader,
        .vertex_input_state = {
            .vertex_buffer_descriptions = vertexBufferDescriptions,
            .num_vertex_buffers = 1,
            .vertex_attributes = vertexAttributes,
            .num_vertex_attributes = 1,
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .target_info = {
            .color_target_descriptions = colorTargetDescriptions,
            .num_color_targets = 1,
        },
    };

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(graphicsDevice, &pipelineInfo);

    if (pipeline == NULL)
    {
        SDL_Log("Failed to create graphics pipeline: %s", SDL_GetError());
        return -1;
    }

    SDL_ReleaseGPUShader(graphicsDevice, vertexShader);
    vertexShader = NULL;

    SDL_ReleaseGPUShader(graphicsDevice, fragmentShader);
    fragmentShader = NULL;

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

    SDL_GPUTransferBufferCreateInfo transferBufferInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = sizeof(_vertices) + sizeof(_indices)
    };

    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(graphicsDevice, &transferBufferInfo);

    if (transferBuffer == NULL)
    {
        SDL_Log("Failed to create transfer buffer: %s", SDL_GetError());
        return -1;
    }

    void* transferData = SDL_MapGPUTransferBuffer(graphicsDevice, transferBuffer, false);
    SDL_memcpy(transferData, _vertices, sizeof(_vertices));
    SDL_memcpy((uint32_t*) transferData + std::size(_vertices), _indices, sizeof(_indices));

    SDL_UnmapGPUTransferBuffer(graphicsDevice, transferBuffer);

    SDL_GPUCommandBuffer* uploadCommandBuffer = SDL_AcquireGPUCommandBuffer(graphicsDevice);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);

    SDL_GPUTransferBufferLocation vertexTransferLocation = {
        .transfer_buffer = transferBuffer,
        .offset = 0,
    };

    SDL_GPUBufferRegion vertexBufferRegion = {
        .buffer = vertexBuffer,
        .offset = 0,
        .size = sizeof(_vertices),
    };

    SDL_UploadToGPUBuffer(
        copyPass,
        &vertexTransferLocation,
        &vertexBufferRegion,
        false
    );

    SDL_GPUTransferBufferLocation indexTransferLocation = {
        .transfer_buffer = transferBuffer,
        .offset = sizeof(_vertices),
    };

    SDL_GPUBufferRegion indexBufferRegion = {
        .buffer = indexBuffer,
        .offset = 0,
        .size = sizeof(_indices),
    };

    SDL_UploadToGPUBuffer(
        copyPass,
        &indexTransferLocation,
        &indexBufferRegion,
        false
    );

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCommandBuffer);

    SDL_ReleaseGPUTransferBuffer(graphicsDevice, transferBuffer);
    transferBuffer = NULL;

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

            SDL_BindGPUGraphicsPipeline(renderPass, pipeline);

            SDL_GPUBufferBinding vertexBufferBinding = { .buffer = vertexBuffer, .offset = 0 };
            SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);

            SDL_GPUBufferBinding indexBufferBinding = { .buffer = indexBuffer, .offset = 0 };
            SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

            SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);

            SDL_EndGPURenderPass(renderPass);
        }

        SDL_SubmitGPUCommandBuffer(commandBuffer);
    }

    SDL_ReleaseGPUTransferBuffer(graphicsDevice, transferBuffer);
    SDL_ReleaseGPUBuffer(graphicsDevice, vertexBuffer);
    SDL_ReleaseGPUBuffer(graphicsDevice, indexBuffer);
    SDL_ReleaseGPUGraphicsPipeline(graphicsDevice, pipeline);
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