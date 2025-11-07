#pragma once

#include <cstdint>
#include <string>
#include <optional>
#include <vector>

#include <SDL3/SDL.h>

static const float NanosecondInSecond = 1e9f;

inline float NanosecondsToSeconds(float nanoseconds)
{
    return nanoseconds / NanosecondInSecond;
}

inline float SDL_GetTimeSeconds()
{
    return NanosecondsToSeconds(SDL_GetTicksNS());
}

struct ShaderCreateInfo 
{
    SDL_GPUShaderStage ShaderStage;
    uint32_t NumSamplers = 0;
    uint32_t NumStorageTextures = 0;
    uint32_t NumStorageBuffers = 0;
    uint32_t NumUniformBuffers = 0;
};

class Shader
{
public:
    static std::optional<Shader> FromSPV(SDL_GPUDevice* graphicsDevice, const std::string& shaderPath, const ShaderCreateInfo& shaderCreateInfo);

    SDL_GPUShader* GetHandle() const;

    void Release();

private:
    SDL_GPUDevice* GraphicsDeviceHandle;
    SDL_GPUShader* ShaderHandle;

    Shader(SDL_GPUDevice* graphicsDeviceHandle, SDL_GPUShader* shaderHandle);
};

struct VertexBufferDescription
{
    uint32_t Slot;
    uint32_t Pitch;
};

struct PipelineCreateInfo
{
    Shader* VertexShader;
    Shader* FragmentShader;
    VertexBufferDescription VertexBufferDescription;
    std::vector<SDL_GPUVertexAttribute> VertexAttributes;
};

class Pipeline
{
public:
    static std::optional<Pipeline> Create(SDL_GPUDevice* graphicsDevice, SDL_Window* window, const PipelineCreateInfo& createinfo);

    void Release();

    SDL_GPUGraphicsPipeline* GetHandle() const;

private:
    SDL_GPUGraphicsPipeline* PipelineHandle;
    SDL_GPUDevice* GraphicsDeviceHandle;

    Pipeline(SDL_GPUDevice* graphicsDevice, SDL_GPUGraphicsPipeline* pipelineHandle);
};

class GPUUploader
{
public:
    GPUUploader(SDL_GPUDevice* graphicsDevice);

    void AddVertexData(float vertices[], uint32_t size, SDL_GPUBuffer* vertexBuffer, uint32_t bufferOffset = 0);
    void AddIndexData(uint32_t indecies[], uint32_t size, SDL_GPUBuffer* indexBuffer, uint32_t bufferOffset = 0);
    void AddTextureData(void* pixels, uint32_t width, uint32_t height, SDL_GPUTexture* texture);
    bool Upload();

private:
    struct VertexData
    {
        float* Vertices;
        uint32_t Size;
        SDL_GPUBuffer* VertexBuffer;
        uint32_t BufferOffset;
    };

    struct IndexData
    {
        uint32_t* Indecies;
        uint32_t Size;
        SDL_GPUBuffer* IndexBuffer;
        uint32_t BufferOffset;
    };

    struct TextureData
    {
        void* Pixels;
        uint32_t Width;
        uint32_t Height;
        SDL_GPUTexture* Texture;
    };

    std::vector<VertexData> Vertices;
    std::vector<IndexData> Indecies;
    std::vector<TextureData> Textures;

    SDL_GPUDevice* _graphicsDevice;
};

struct Image
{
    void* Data;
    uint32_t Width;
    uint32_t Height;
    uint32_t Channels;

    Image(void* data, uint32_t width, uint32_t height, uint32_t channels);
    ~Image();
};

Image* LoadImage(const std::string& filePath);
