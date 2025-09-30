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
    { -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f },
    { 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f },
};

struct UniformBuffer
{
    float time;
};

static UniformBuffer _timeUniform{};

SDL_Window* _window;
SDL_GPUDevice* _graphics;
SDL_GPUBuffer* _vertexBuffer;
SDL_GPUTransferBuffer* _transferBuffer;
SDL_GPUGraphicsPipeline* _graphicsPipeline;

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

    SDL_GPUShader* vertexShader = SDL_CreateGPUShader(_graphics, &vertexShaderInfo);

    SDL_free(vertexCode);

    size_t fragmentCodeSize;
    void* fragmentCode = SDL_LoadFile("shaders/fragment.spv", &fragmentCodeSize);

    SDL_GPUShaderCreateInfo fragmentShaderInfo = {
        .code_size = fragmentCodeSize,
        .code = (Uint8*)fragmentCode,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
        .num_samplers = 0,
        .num_storage_textures = 0,
        .num_storage_buffers = 0,
        .num_uniform_buffers = 1,
    };

    SDL_GPUShader* fragmentShader = SDL_CreateGPUShader(_graphics, &fragmentShaderInfo);

    SDL_free(fragmentCode);

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {
        .vertex_shader = vertexShader,
        .fragment_shader = fragmentShader,
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
    };

    SDL_GPUVertexBufferDescription vertexBufferDescriptions[1];
    vertexBufferDescriptions[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDescriptions[0].instance_step_rate = 0;
    vertexBufferDescriptions[0].pitch = sizeof(Vertex);
    vertexBufferDescriptions[0].slot = 0;

    SDL_GPUVertexAttribute vertexAttributes[2];

    vertexAttributes[0].buffer_slot = 0;
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertexAttributes[0].location = 0;
    vertexAttributes[0].offset = 0;

    vertexAttributes[1].buffer_slot = 0;
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    vertexAttributes[1].location = 1;
    vertexAttributes[1].offset = sizeof(float) * 3;

    pipelineInfo.vertex_input_state.num_vertex_attributes = 2;
    pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes;

    pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
    pipelineInfo.vertex_input_state.vertex_buffer_descriptions = vertexBufferDescriptions;

    SDL_GPUColorTargetDescription colorTargetDescriptions[1];
    colorTargetDescriptions[0] = {
        .format = SDL_GetGPUSwapchainTextureFormat(_graphics, _window),
        .blend_state = {
            .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
            .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA,
            .color_blend_op = SDL_GPU_BLENDOP_ADD,

            .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
            .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA,
            .alpha_blend_op = SDL_GPU_BLENDOP_ADD,

            .enable_blend = true,
        },
    };

    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = colorTargetDescriptions;


    _graphicsPipeline = SDL_CreateGPUGraphicsPipeline(_graphics, &pipelineInfo);

    SDL_ReleaseGPUShader(_graphics, vertexShader);
    SDL_ReleaseGPUShader(_graphics, fragmentShader);

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

    SDL_BindGPUGraphicsPipeline(renderPass, _graphicsPipeline);

    SDL_GPUBufferBinding bufferBindings[1];
    bufferBindings[0].buffer = _vertexBuffer;
    bufferBindings[0].offset = 0;

    SDL_BindGPUVertexBuffers(renderPass, 0, bufferBindings, 1);

    _timeUniform.time = SDL_GetTicksNS() / 1e9f;
    SDL_PushGPUFragmentUniformData(commandBuffer, 0, &_timeUniform, sizeof(UniformBuffer));

    SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);

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
    SDL_ReleaseGPUGraphicsPipeline(_graphics, _graphicsPipeline);

    SDL_DestroyWindow(_window);
    SDL_DestroyGPUDevice(_graphics);
}
