// LearnSDL3.cpp : Defines the entry point for the application.
//

#define SDL_MAIN_USE_CALLBACKS

#include "LearnSDL3.h"
#include "SDL3/SDL_main.h"
#include "SDL3/SDL.h"

using namespace std;

SDL_Window* _window;
SDL_GPUDevice* _graphics;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
    _window = SDL_CreateWindow("LearnSDL3", 1920, 1080, SDL_WINDOW_RESIZABLE);
    _graphics = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    SDL_ClaimWindowForGPUDevice(_graphics, _window);

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
    SDL_DestroyWindow(_window);
    SDL_DestroyGPUDevice(_graphics);
}
