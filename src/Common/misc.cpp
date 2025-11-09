#include "misc.h"
#include "SDL3/SDL.h"
#include "stb_image.h"

std::optional<Shader> Shader::FromSPV(SDL_GPUDevice* graphicsDevice, const std::string& shaderPath, const ShaderCreateInfo& shaderCreateInfo)
{
    size_t shaderCodeSize;
    void* shaderCode = SDL_LoadFile(shaderPath.c_str(), &shaderCodeSize);

    if (shaderCode == NULL)
    {
        SDL_Log("Failed to load shader");
        return std::nullopt;
    }

    SDL_GPUShaderCreateInfo shaderInfo = {
        .code_size = shaderCodeSize,
        .code = (uint8_t*)shaderCode,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = shaderCreateInfo.ShaderStage,
        .num_samplers = shaderCreateInfo.NumSamplers,
        .num_storage_textures = shaderCreateInfo.NumStorageTextures,
        .num_storage_buffers = shaderCreateInfo.NumStorageBuffers,
        .num_uniform_buffers = shaderCreateInfo.NumUniformBuffers,
    };

    SDL_GPUShader* vertexShader = SDL_CreateGPUShader(graphicsDevice, &shaderInfo);

    if (vertexShader == NULL)
    {
        SDL_Log("Failed to create shader: %s", SDL_GetError());
        return std::nullopt;
    }

    SDL_free(shaderCode);

    Shader shader(graphicsDevice, vertexShader);

    return shader;
}

Shader::Shader(SDL_GPUDevice* graphicsDeviceHandle, SDL_GPUShader* shaderHandle)
    : GraphicsDeviceHandle(graphicsDeviceHandle), ShaderHandle(shaderHandle)
{ }

SDL_GPUShader* Shader::GetHandle() const
{
    return ShaderHandle;
}

void Shader::Release()
{
    SDL_ReleaseGPUShader(GraphicsDeviceHandle, ShaderHandle);
}

Pipeline::Pipeline(SDL_GPUDevice* graphicsDevice, SDL_GPUGraphicsPipeline* pipelineHandle)
    : GraphicsDeviceHandle(graphicsDevice), PipelineHandle(pipelineHandle)
{
}

void Pipeline::Release()
{
    SDL_ReleaseGPUGraphicsPipeline(GraphicsDeviceHandle, PipelineHandle);
}

SDL_GPUGraphicsPipeline* Pipeline::GetHandle() const
{
    return PipelineHandle;
}

std::optional<Pipeline> Pipeline::Create(SDL_GPUDevice* graphicsDevice, SDL_Window* window, const PipelineCreateInfo& createInfo)
{
    const VertexBufferDescription& vertexBufferDescription = createInfo.VertexBufferDescription;

    SDL_GPUVertexBufferDescription vertexBufferDescriptions[] = {
        {
            .slot = vertexBufferDescription.Slot,
            .pitch = vertexBufferDescription.Pitch,
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            .instance_step_rate = 0
        }
    };

    SDL_GPUColorTargetDescription colorTargetDescriptions[1];
    colorTargetDescriptions[0] = {
        .format = SDL_GetGPUSwapchainTextureFormat(graphicsDevice, window)
    };
        
    const std::vector<SDL_GPUVertexAttribute>& vertexAttributes = createInfo.VertexAttributes;

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {
        .vertex_shader = createInfo.VertexShader->GetHandle(),
        .fragment_shader = createInfo.FragmentShader->GetHandle(),
        .vertex_input_state = {
            .vertex_buffer_descriptions = vertexBufferDescriptions,
            .num_vertex_buffers = 1,
            .vertex_attributes = vertexAttributes.data(),
            .num_vertex_attributes = (uint32_t) vertexAttributes.size()
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .target_info = {
            .color_target_descriptions = colorTargetDescriptions,
            .num_color_targets = 1,
        },
    };

    if (createInfo.DepthStencilFormat.has_value())
    {
        pipelineInfo.depth_stencil_state = {
            .compare_op = SDL_GPU_COMPAREOP_LESS,
            .enable_depth_test = true,
            .enable_depth_write = true,
        };

        pipelineInfo.target_info.has_depth_stencil_target = true;
        pipelineInfo.target_info.depth_stencil_format = createInfo.DepthStencilFormat.value();
    }

    SDL_GPUGraphicsPipeline* pipelineHandle = SDL_CreateGPUGraphicsPipeline(graphicsDevice, &pipelineInfo);

    if (pipelineHandle == NULL)
    {
        SDL_Log("Failed to create graphics pipeline: %s", SDL_GetError());
        return std::nullopt;
    }

    return Pipeline(graphicsDevice, pipelineHandle);
}

GPUUploader::GPUUploader(SDL_GPUDevice* graphicsDevice)
    : _graphicsDevice(graphicsDevice)
{

}

void GPUUploader::AddVertexData(float vertices[], uint32_t size, SDL_GPUBuffer* vertexBuffer, uint32_t bufferOffset)
{
    Vertices.push_back(VertexData{ .Vertices = vertices, .Size = size, .VertexBuffer = vertexBuffer, .BufferOffset = bufferOffset });
}

void GPUUploader::AddIndexData(uint32_t indecies[], uint32_t size, SDL_GPUBuffer* indexBuffer, uint32_t bufferOffset)
{
    Indecies.push_back(IndexData{ .Indecies = indecies, .Size = size, .IndexBuffer = indexBuffer, .BufferOffset = bufferOffset });
}

void GPUUploader::AddTextureData(void* pixels, uint32_t width, uint32_t height, SDL_GPUTexture* texture)
{
    Textures.push_back(TextureData{ .Pixels = pixels, .Width = width, .Height = height, .Texture = texture });
}

bool GPUUploader::Upload()
{
    uint32_t totalSize = 0;

    for (VertexData& vertexData : Vertices)
    {
        totalSize += vertexData.Size;
    }

    for (IndexData& indexData : Indecies)
    {
        totalSize += indexData.Size;
    }

    for (TextureData& textureData : Textures)
    {
        totalSize += textureData.Width * textureData.Height * 4;
    }

    SDL_GPUTransferBufferCreateInfo createInfo = { .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = totalSize };
    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(_graphicsDevice, &createInfo);

    if (transferBuffer == NULL)
    {
        SDL_Log("Failed to create transfer buffer: %s", SDL_GetError());
        return false;
    }

    void* transferData = SDL_MapGPUTransferBuffer(_graphicsDevice, transferBuffer, false);
    void* currentTransferData = transferData;

    for (VertexData& vertexData : Vertices)
    {
        SDL_memcpy(currentTransferData, vertexData.Vertices, vertexData.Size);
        currentTransferData = (float*) currentTransferData + vertexData.Size / sizeof(float);
    }

    for (IndexData& indexData : Indecies)
    {
        SDL_memcpy(currentTransferData, indexData.Indecies, indexData.Size);
        currentTransferData = (uint32_t*) currentTransferData + indexData.Size / sizeof(uint32_t);
    }

    for (TextureData& textureData : Textures)
    {
        SDL_memcpy(currentTransferData, textureData.Pixels, textureData.Width * textureData.Height * 4);
        currentTransferData = (float*) currentTransferData + textureData.Width * textureData.Height;
    }

    SDL_UnmapGPUTransferBuffer(_graphicsDevice, transferBuffer);

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(_graphicsDevice);
    if (commandBuffer == NULL)
    {
        SDL_Log("Failed to acquire a command buffer: %s", SDL_GetError());
        return false;
    }

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    uint32_t transferDataOffsetBytes = 0;

    for (VertexData& vertexData : Vertices)
    {
        SDL_GPUTransferBufferLocation transferBufferLocation = {
            .transfer_buffer = transferBuffer,
            .offset = transferDataOffsetBytes,
        };

        SDL_GPUBufferRegion bufferRegion = {
            .buffer = vertexData.VertexBuffer,
            .offset = vertexData.BufferOffset,
            .size = vertexData.Size,
        };

        SDL_UploadToGPUBuffer(copyPass, &transferBufferLocation, &bufferRegion, false);

        transferDataOffsetBytes += vertexData.Size;
    }

    for (IndexData& indexData : Indecies)
    {
        SDL_GPUTransferBufferLocation transferBufferLocation = {
            .transfer_buffer = transferBuffer,
            .offset = transferDataOffsetBytes,
        };

        SDL_GPUBufferRegion bufferRegion = {
            .buffer = indexData.IndexBuffer,
            .offset = indexData.BufferOffset,
            .size = indexData.Size,
        };

        SDL_UploadToGPUBuffer(copyPass, &transferBufferLocation, &bufferRegion, false);

        transferDataOffsetBytes += indexData.Size;
    }

    for (TextureData& textureData : Textures)
    {
        SDL_GPUTextureTransferInfo transferLocation = {
            .transfer_buffer = transferBuffer,
            .offset = transferDataOffsetBytes,
        };
        
        SDL_GPUTextureRegion textureRegion = {
            .texture = textureData.Texture,
            .w = textureData.Width,
            .h = textureData.Height,
            .d = 1
        };

        SDL_UploadToGPUTexture(copyPass, &transferLocation, &textureRegion, false);

        transferDataOffsetBytes += textureData.Width * textureData.Height * 4;
    }

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
    SDL_ReleaseGPUTransferBuffer(_graphicsDevice, transferBuffer);

    Vertices.clear();
    Indecies.clear();

    return true;
}

Image::Image(void* data, uint32_t width, uint32_t height, uint32_t channels)
    : Data(data), Width(width), Height(height), Channels(channels)
{

}

Image::~Image()
{
    stbi_image_free(Data);
}

Image* LoadImage(const std::string& filePath)
{
    stbi_set_flip_vertically_on_load(true);

    int32_t width, height, channels;
    void* data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);

    return new Image(data, width, height, channels);
}

void TickTime(Time& time)
{
    time.PreviousTicksNS = time.CurrentTicksNS;
    time.CurrentTicksNS = SDL_GetTicksNS();
    time.DeltaTime = NanosecondsToSeconds(time.CurrentTicksNS - time.PreviousTicksNS);
    time.TotalTime = NanosecondsToSeconds(time.CurrentTicksNS);
}
