#include <print>
#include "SDL3/SDL.h"
#include "Common/misc.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

float _vertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f};

struct MatrixUniform
{
    glm::mat4 Model;
    glm::mat4 View;
    glm::mat4 Projection;
};

glm::f32* value_ptr(MatrixUniform& matrixUniform)
{
    return glm::value_ptr(matrixUniform.Model);
}

uint32_t _indices[] = {
    0, 1, 2,
    0, 2, 3,
};

bool _shouldQuit;

Time _time;

glm::vec3 _cameraPosition = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 _cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 _cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float _cameraPitch;
float _cameraYaw = -90.0f;
float _cameraRoll;

struct InputContext
{
    bool IsFowardDown;
    bool IsBackDown;
    bool IsLeftDown;
    bool IsRightDown;
};

struct MouseContext
{
    float LastX, LastY;
};

InputContext _input;
MouseContext _mouse;

void PollEvents(SDL_Window* window);

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
        .VertexBufferDescription = {.Slot = 0, .Pitch = 5 * sizeof(float) },
        .VertexAttributes = {
            SDL_GPUVertexAttribute{.location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = 0 },
            SDL_GPUVertexAttribute{.location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = 3 * sizeof(float)},
        },
        .DepthStencilFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
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

    SDL_GPUTextureCreateInfo depthTextureInfo = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = 800,
        .height = 600,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };

    SDL_GPUTexture* depthTexture = SDL_CreateGPUTexture(graphicsDevice, &depthTextureInfo);

    GPUUploader gpuUploader(graphicsDevice);

    gpuUploader.AddVertexData(_vertices, sizeof(_vertices), vertexBuffer, 0);
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

    glm::vec3 cubePositions[] = {
        glm::vec3(0.0f,  0.0f,  0.0f),
        glm::vec3(2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3(2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f,  3.0f, -7.5f),
        glm::vec3(1.3f, -2.0f, -2.5f),
        glm::vec3(1.5f,  2.0f, -2.5f),
        glm::vec3(1.5f,  0.2f, -1.5f),
        glm::vec3(-1.3f,  1.0f, -1.5f)
    };

    while (!_shouldQuit)
    {
        TickTime(_time);

        PollEvents(window);

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

            SDL_GPUDepthStencilTargetInfo depthTargetInfo = {
                .texture = depthTexture,
                .clear_depth = 1,
                .load_op = SDL_GPU_LOADOP_CLEAR,
                .store_op = SDL_GPU_STOREOP_DONT_CARE,
            };

            SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, &depthTargetInfo);

            SDL_BindGPUGraphicsPipeline(renderPass, pipeline->GetHandle());

            SDL_GPUBufferBinding vertexBufferBinding = { .buffer = vertexBuffer, .offset = 0 };
            SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);

            SDL_GPUTextureSamplerBinding textureSamplers[] = {
                { .texture = containerTexture, .sampler = sampler },
                { .texture = awesomefaceTexture, .sampler = sampler },
            };
            SDL_BindGPUFragmentSamplers(renderPass, 0, textureSamplers, 2);

            for (size_t i = 0; i < 10; i++)
            {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, cubePositions[i]);
                float angle = i * 20.0f;
                model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

                const float radius = 10.0f;
                float cameraX = sin(_time.TotalTime) * radius;
                float cameraZ = cos(_time.TotalTime) * radius;
                glm::mat4 view = glm::mat4(1.0f);
                view = glm::lookAt(_cameraPosition, _cameraPosition + _cameraFront, _cameraUp);

                glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

                MatrixUniform matrixUniform{ .Model = model, .View = view, .Projection = projection };

                SDL_PushGPUVertexUniformData(commandBuffer, 0, value_ptr(matrixUniform), sizeof(matrixUniform));
                SDL_DrawGPUPrimitives(renderPass, 36, 1, 0, 0);
            }

            SDL_EndGPURenderPass(renderPass);
        }

        SDL_SubmitGPUCommandBuffer(commandBuffer);
    }

    SDL_ReleaseGPUBuffer(graphicsDevice, vertexBuffer);
    SDL_ReleaseGPUTexture(graphicsDevice, containerTexture);
    SDL_ReleaseGPUTexture(graphicsDevice, awesomefaceTexture);
    SDL_ReleaseGPUTexture(graphicsDevice, depthTexture);
    pipeline->Release();
    SDL_ReleaseGPUSampler(graphicsDevice, sampler);
    SDL_ReleaseWindowFromGPUDevice(graphicsDevice, window);
    SDL_DestroyGPUDevice(graphicsDevice);
    SDL_DestroyWindow(window);
    return 0;
}

void ProcessKeyDown(const SDL_Event& event);
void ProcessKeyboardState();
void ProcessMouseMotion(SDL_Window* window);

void PollEvents(SDL_Window* window)
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
            ProcessKeyDown(event);
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            SDL_Log("resized");
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_MOTION:
            ProcessMouseMotion(window);
            break;
        }
    }

    ProcessKeyboardState();
}

void ProcessKeyDown(const SDL_Event& event)
{
    if (event.key.key == SDLK_ESCAPE)
    {
        _shouldQuit = true;
    }
}

void ProcessKeyboardState()
{
    const bool* keyStates = SDL_GetKeyboardState(nullptr);

    float speed = 10 * _time.DeltaTime;

    if (keyStates[SDL_SCANCODE_W])
    {
        _cameraPosition += speed * _cameraFront;
    }
    if (keyStates[SDL_SCANCODE_S])
    {
        _cameraPosition -= speed * _cameraFront;
    }
    if (keyStates[SDL_SCANCODE_A])
    {
        _cameraPosition += speed * glm::cross(_cameraUp, _cameraFront);
    }
    if (keyStates[SDL_SCANCODE_D])
    {
        _cameraPosition -= speed * glm::cross(_cameraUp, _cameraFront);
    }
    if (keyStates[SDL_SCANCODE_Q])
    {
        _cameraPosition -= speed * _cameraUp;
    }
    if (keyStates[SDL_SCANCODE_E])
    {
        _cameraPosition += speed * _cameraUp;
    }
}

bool _isFirst = true;
bool _wasMousePressed = false;

void ProcessMouseMotion(SDL_Window* window)
{
    float mouseX, mouseY;
    SDL_MouseButtonFlags mouseFlags = SDL_GetMouseState(&mouseX, &mouseY);

    bool isMousePressed = mouseFlags == SDL_BUTTON_RMASK;
    if (!isMousePressed)
    {
        SDL_SetWindowRelativeMouseMode(window, false);
        _wasMousePressed = false;
        return;
    }

    _isFirst = !_wasMousePressed;
    _wasMousePressed = true;

    if (_isFirst)
    {
        _mouse.LastX = mouseX;
        _mouse.LastY = mouseY;
        _isFirst = false;
        SDL_SetWindowRelativeMouseMode(window, true);
    }

    float deltaX = mouseX - _mouse.LastX;
    float deltaY = mouseY - _mouse.LastY;

    const float sensitivity = 10.0f;

    _cameraPitch -= deltaY * sensitivity * _time.DeltaTime;
    _cameraYaw += deltaX * sensitivity * _time.DeltaTime;

    if (_cameraPitch > 89.0f)
    {
        _cameraPitch = 89.0f;
    }
    else if (_cameraPitch < -89.0f)
    {
        _cameraPitch = -89.0f;
    }

    glm::vec3 direction;
    direction.x = cos(glm::radians(_cameraYaw)) * cos(glm::radians(_cameraPitch));
    direction.y = sin(glm::radians(_cameraPitch));
    direction.z = sin(glm::radians(_cameraYaw)) * cos(glm::radians(_cameraPitch));

    _cameraFront = glm::normalize(direction);

    SDL_WarpMouseInWindow(window, _mouse.LastX, _mouse.LastY);
}
