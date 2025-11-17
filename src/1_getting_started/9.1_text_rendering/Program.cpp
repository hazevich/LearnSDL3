#include <print>
#include "SDL3/SDL.h"
#include "Common/misc.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "stb_truetype.h"
#include <queue>
#include <map>
#include <tuple>

uint32_t AtlasSize = 512;

//float _vertices[] = {
//     // vertex          // texture coordinates
//     0.0f,    0.0f,    0.0f, 1.0f, // top left
//     0.0f,  -100.0f,    0.0f, 0.0f, // bottom left
//     100.0f,-100.0f,    1.0f, 0.0f, // bottom right
//     100.0f,  0.0f,    1.0f, 1.0f, // top right
//};

float _vertices[] = {
    // vertex         // texture coordinates
    -50.5f,  50.5f,     0.0f, 0.0f, // top left
    -50.5f, -50.5f,     0.0f, 1.0f, // bottom left
     50.5f, -50.5f,     1.0f, 1.0f, // bottom right
     50.5f,  50.5f,     1.0f, 0.0f, // top right
};

uint32_t _indices[] = {
    0, 1, 2,
    0, 2, 3,
};

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

struct Character
{
    char Value;
    int32_t FontSize;

    uint32_t X;
    uint32_t Y;

    uint32_t Width;
    uint32_t Height;

    int32_t BearingX;
    int32_t BearingY;

    uint32_t Advance;
};

std::map<std::tuple<char, int32_t>, Character> _characters;
std::map<std::tuple<char, char>, float> _kerningPairs;

Character* GetCharacter(char character, int32_t fontSize);

bool _shouldQuit;

Time _time;

glm::vec3 _cameraPosition = glm::vec3(0.0f, 0.0f, 700.0f);
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
struct Glyph
{
    int32_t Index;
    void* Pixels;
    uint32_t Width;
    uint32_t Height;
    int32_t OffsetX;
    int32_t OffsetY;
};

std::queue<char> chars{};
stbtt_fontinfo fontInfo;
uint8_t* fontImage;
GPUUploader* gpuUploader;
FontAtlas fontAtlas;
SDL_GPUTexture* fontTexture;

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

    SDL_Window* window = SDL_CreateWindow("LearnSDL3", 2560, 1440, SDL_WINDOW_RESIZABLE);

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
        .VertexBufferDescription = {.Slot = 0, .Pitch = 4 * sizeof(float) },
        .VertexAttributes = {
            SDL_GPUVertexAttribute{.location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = 0 },
            SDL_GPUVertexAttribute{.location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = 2 * sizeof(float)},
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

    size_t fontDataSize;
    void* fontData = SDL_LoadFile("assets/Roboto-Regular.ttf", &fontDataSize);
    if (!stbtt_InitFont(&fontInfo, (unsigned char*) fontData, stbtt_GetFontOffsetForIndex((unsigned char*) fontData, 0)))
    {
        SDL_Log("Could not init font");
        return -1;
    }


    fontAtlas = CreateFontAtlas(AtlasSize, AtlasSize);

    fontImage = new uint8_t[AtlasSize * AtlasSize * 4];

    SDL_GPUTextureCreateInfo fontTextureInfo = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = (uint32_t) AtlasSize,
        .height = (uint32_t) AtlasSize,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };

    fontTexture = SDL_CreateGPUTexture(graphicsDevice, &fontTextureInfo);
 
    gpuUploader = new GPUUploader(graphicsDevice);

    //std::string text = "!\"#$%&'()*+\n,-./0123456789\n:;<=>?@ABCDEFGHIJKL\nMNOPQRSTUVWXYZ[\\]\n^_`abcdefghi\njklmnopqrstuvwxy\nz{|}~";
    std::string text = R"(
    for (size_t i = 0; i < text.size(); i++)
    {
        if (text[i] == '\n')
        {
            y -= (ascent - descent + lineGap) * scale;
            x = -50.0f;
            continue;
        }

        Character* character = GetCharacter(text[i], 20);
        if (character)
        {
            float minu = character->X / (float) AtlasSize;
            float minv = character->Y / (float) AtlasSize;
            float maxu = minu + (character->Width / (float) AtlasSize);
            float maxv = minv + (character->Height / (float) AtlasSize);

            float mincx = x + character->BearingX;
            float mincy = y - ((int32_t) character->Height + character->BearingY);
            float maxcx = mincx + character->Width;
            float maxcy = mincy + character->Height;

            x += character->Advance;

            vertices.push_back(mincx); vertices.push_back(maxcy); vertices.push_back(minu); vertices.push_back(minv);
            vertices.push_back(mincx); vertices.push_back(mincy); vertices.push_back(minu); vertices.push_back(maxv);
            vertices.push_back(maxcx); vertices.push_back(mincy); vertices.push_back(maxu); vertices.push_back(maxv);
            vertices.push_back(maxcx); vertices.push_back(maxcy); vertices.push_back(maxu); vertices.push_back(minv);

            indecies.push_back(_indices[0] + charIndex * 4);
            indecies.push_back(_indices[1] + charIndex * 4);
            indecies.push_back(_indices[2] + charIndex * 4);
            indecies.push_back(_indices[3] + charIndex * 4);
            indecies.push_back(_indices[4] + charIndex * 4);
            indecies.push_back(_indices[5] + charIndex * 4);
            charIndex++;
        }
    }

)";

    std::vector<float> vertices;
    std::vector<uint32_t> indecies;

    float y = 50.0f;
    float x = -50.0f;

    const int32_t fontSize = 40;

    int32_t ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
    float scale = stbtt_ScaleForPixelHeight(&fontInfo, fontSize);

    int32_t charIndex = 0;

    for (size_t i = 0; i < text.size(); i++)
    {
        if (text[i] == '\n')
        {
            y -= (ascent - descent + lineGap) * scale;
            x = -50.0f;
            continue;
        }

        Character* character = GetCharacter(text[i], fontSize);
        if (character)
        {
            float kerning = 0.0f;

            if (i < text.size() - 1)
            {
                std::tuple<char, char> kerningKey = { text[i], text[i + 1] };
                if (_kerningPairs.contains(kerningKey))
                {
                    kerning = _kerningPairs[kerningKey];
                }
                else 
                {
                    kerning = stbtt_GetCodepointKernAdvance(&fontInfo, text[i], text[i + 1]);
                    _kerningPairs.insert(std::pair<std::tuple<char, char>, float>(kerningKey, kerning));
                }
            }

            kerning *= scale;

            float minu = character->X / (float) AtlasSize;
            float minv = character->Y / (float) AtlasSize;
            float maxu = minu + (character->Width / (float) AtlasSize);
            float maxv = minv + (character->Height / (float) AtlasSize);

            float mincx = x + character->BearingX;
            float mincy = y - ((int32_t) character->Height + character->BearingY);
            float maxcx = mincx + character->Width;
            float maxcy = mincy + character->Height;

            x += character->Advance + kerning;

            vertices.push_back(mincx); vertices.push_back(maxcy); vertices.push_back(minu); vertices.push_back(minv);
            vertices.push_back(mincx); vertices.push_back(mincy); vertices.push_back(minu); vertices.push_back(maxv);
            vertices.push_back(maxcx); vertices.push_back(mincy); vertices.push_back(maxu); vertices.push_back(maxv);
            vertices.push_back(maxcx); vertices.push_back(maxcy); vertices.push_back(maxu); vertices.push_back(minv);

            indecies.push_back(_indices[0] + charIndex * 4);
            indecies.push_back(_indices[1] + charIndex * 4);
            indecies.push_back(_indices[2] + charIndex * 4);
            indecies.push_back(_indices[3] + charIndex * 4);
            indecies.push_back(_indices[4] + charIndex * 4);
            indecies.push_back(_indices[5] + charIndex * 4);
            charIndex++;
        }
    }

    y = 50.0f;
    x = -1050.0f;



    SDL_GPUBufferCreateInfo vertexBufferInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = (uint32_t) (vertices.size() * sizeof(float))
    };

    SDL_GPUBuffer* vertexBuffer = SDL_CreateGPUBuffer(graphicsDevice, &vertexBufferInfo);

    if (vertexBuffer == NULL)
    {
        SDL_Log("Failed to create vertex buffer: %s", SDL_GetError());
        return -1;
    }

    SDL_GPUBufferCreateInfo indexBufferInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = (uint32_t) (indecies.size() * sizeof(uint32_t))
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

    SDL_GPUTextureCreateInfo depthTextureInfo = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = 2560,
        .height = 1440,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };

    SDL_GPUTexture* depthTexture = SDL_CreateGPUTexture(graphicsDevice, &depthTextureInfo);


    const char* toEnqueue = "1234567890-=!@#$%^&*()_+qwertyuiopasdfghjkl[];'\\|\":/.,<>?mnbvcxzQWERTYUIOPASDFGHJKLZXCVBNM";

    for (size_t i = 0; i < 91; i++)
    {
        chars.push(toEnqueue[i]);
    }

    gpuUploader->AddVertexData(vertices.data(), vertices.size() * sizeof(float), vertexBuffer, 0);
    gpuUploader->AddIndexData(indecies.data(), indecies.size() * sizeof(uint32_t), indexBuffer, 0);
    gpuUploader->AddTextureData(containerImage->Data, containerImage->Width, containerImage->Height, containerTexture);
    gpuUploader->AddTextureData(awesomefaceImage->Data, awesomefaceImage->Width, awesomefaceImage->Height, awesomefaceTexture);
    gpuUploader->AddTextureData(fontImage, AtlasSize, AtlasSize, fontTexture);

    if (!gpuUploader->Upload())
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

            SDL_GPUBufferBinding indexBufferBinding = { .buffer = indexBuffer, .offset = 0 };
            SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

            SDL_GPUTextureSamplerBinding textureSamplers[] = {
                { .texture = containerTexture, .sampler = sampler },
                { .texture = fontTexture, .sampler = sampler },
            };
            SDL_BindGPUFragmentSamplers(renderPass, 0, textureSamplers, 2);

            glm::mat4 model = glm::mat4(1.0f);

            const float radius = 10.0f;
            float cameraX = sin(_time.TotalTime) * radius;
            float cameraZ = cos(_time.TotalTime) * radius;
            glm::mat4 view = glm::mat4(1.0f);
            view = glm::translate(view, glm::vec3(2560.0f / 2, 1440.0f / 2, 0.0f));
            //view = glm::lookAt(_cameraPosition, _cameraPosition + _cameraFront, _cameraUp);

            //glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f/600.0f, 0.1f, 10000.0f);
            glm::mat4 projection = glm::ortho(0.0f, 2560.0f, 0.0f, 1440.0f);

            MatrixUniform matrixUniform{ .Model = model, .View = view, .Projection = projection };

            SDL_PushGPUVertexUniformData(commandBuffer, 0, value_ptr(matrixUniform), sizeof(matrixUniform));
            SDL_DrawGPUIndexedPrimitives(renderPass, indecies.size(), 1, 0, 0, 0);

            SDL_EndGPURenderPass(renderPass);
        }

        SDL_SubmitGPUCommandBuffer(commandBuffer);
    }

    delete fontImage;
    SDL_free(fontData);
    delete gpuUploader;

    SDL_ReleaseGPUBuffer(graphicsDevice, vertexBuffer);
    SDL_ReleaseGPUBuffer(graphicsDevice, indexBuffer);
    SDL_ReleaseGPUTexture(graphicsDevice, containerTexture);
    SDL_ReleaseGPUTexture(graphicsDevice, awesomefaceTexture);
    SDL_ReleaseGPUTexture(graphicsDevice, depthTexture);
    SDL_ReleaseGPUTexture(graphicsDevice, fontTexture);
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

bool _wasXPressed = false;

void ProcessKeyboardState()
{
    const bool* keyStates = SDL_GetKeyboardState(nullptr);

    float speed = 100.0f * _time.DeltaTime;

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

    if (!_wasXPressed && keyStates[SDL_SCANCODE_X] && !chars.empty())
    {
        char c = chars.front();
        chars.pop();
        GetCharacter(c, 126);
    }

    _wasXPressed = keyStates[SDL_SCANCODE_X];
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

Glyph get_glyph(stbtt_fontinfo* fontInfo, char character, int32_t fontSize)
{
    int32_t fontWidth, fontHeight, fontOffsetX, fontOffsetY;
    int glyphIndex = stbtt_FindGlyphIndex(fontInfo, character);
    unsigned char* bitmap = stbtt_GetGlyphBitmap(fontInfo, 0, stbtt_ScaleForPixelHeight(fontInfo, fontSize), glyphIndex, &fontWidth, &fontHeight, &fontOffsetX, &fontOffsetY);

    int32_t size = fontWidth * fontHeight * 4;
    uint8_t* fontRgba = new uint8_t[fontWidth * fontHeight * 4];

    for (size_t i = 0; i < fontWidth * fontHeight; i++)
    {
        fontRgba[i * 4 + 0] = 255;
        fontRgba[i * 4 + 1] = 255;
        fontRgba[i * 4 + 2] = 255;
        fontRgba[i * 4 + 3] = bitmap[i];
    }
    void* userdata = nullptr;
    stbtt_FreeBitmap(bitmap, userdata);

    return Glyph{ .Index = glyphIndex, .Pixels = fontRgba, .Width = (uint32_t) fontWidth, .Height = (uint32_t) fontHeight, .OffsetX = fontOffsetX, .OffsetY = fontOffsetY, };
}

void CopyPixels(uint8_t* fontImage, Glyph& font, FontAtlasNode& packedFont)
{
    const uint32_t Width = AtlasSize;

    size_t yOffset = packedFont.Y * 4;
    size_t xOffset = packedFont.X * 4;
    size_t length = font.Width * 4;

    for (size_t y = 0; y < font.Height; y++)
    {
        size_t yCoordinate = y * 4;

        size_t sourceIndex = font.Width * (yCoordinate);
        size_t destinationIndex = Width * (yCoordinate + yOffset) + xOffset;
        SDL_memcpy(fontImage + destinationIndex, (uint8_t*) font.Pixels + sourceIndex, length);
    }
}

Character* GetCharacter(char character, int32_t fontSize)
{
    std::tuple<char, int32_t> key = { character, fontSize };

    if (!_characters.contains(key))
    {
        Glyph glyph = get_glyph(&fontInfo, character, fontSize);
        std::optional<FontAtlasNode> packedGlyph = PackTexture(fontAtlas, glyph.Width, glyph.Height);

        if (packedGlyph.has_value())
        {
            CopyPixels(fontImage, glyph, packedGlyph.value());
            delete glyph.Pixels;

            gpuUploader->AddTextureData(fontImage, AtlasSize, AtlasSize, fontTexture);
            gpuUploader->Upload();

            int32_t advance;
            int32_t bearingX;
            stbtt_GetGlyphHMetrics(&fontInfo, glyph.Index, &advance, &bearingX);

            advance *= stbtt_ScaleForPixelHeight(&fontInfo, fontSize);

            Character c = Character{
                .Value = character,
                .FontSize = fontSize,
                .X = packedGlyph->X,
                .Y = packedGlyph->Y,
                .Width = glyph.Width,
                .Height = glyph.Height,
                .BearingX = glyph.OffsetX,
                .BearingY = glyph.OffsetY,
                .Advance = (uint32_t)advance,
            };

            _characters.insert(std::pair<std::tuple<char, int32_t>, Character>(key, c));
        }
    }

    return &_characters[key];
}
