# Spine Runtime for SDL2

This is the version based on [spine-sfml/c](https://github.com/EsotericSoftware/spine-runtimes/tree/4.0/spine-sfml/c) (which in turn uses [spine-c](https://github.com/EsotericSoftware/spine-runtimes/tree/4.0/spine-c))


## Documentation

I'm providing the same examples included with **spine-sfml** but modified to use **spine-sdl**

A minimalistic example would look like this:

```C++
#include <spine/spine-sdl.h>

using namespace spine;

SDL_Renderer *renderer;
SDL_Renderer* spSDL_getRenderer() { return renderer; } // required by spine-sdl

int main() {
    SDL_Window *window;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(640, 640, 0, &window, &renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 64, 255);

    spAtlas *atlas = spAtlas_createFromFile("data/spineboy-pma.atlas", 0);

    spSkeletonBinary *binary = spSkeletonBinary_create(atlas);
    binary->scale = 0.6f;
    spSkeletonData *skeletonData = spSkeletonBinary_readSkeletonDataFile(binary, "data/spineboy-pro.skel");
    // Alternatively, you can load the json:
    // spSkeletonJson *json = spSkeletonJson_create(atlas);
    // json->scale = 0.6f;
    // spSkeletonData *skeletonData = spSkeletonJson_readSkeletonDataFile(json, "data/spineboy-pro.json");
    if (skeletonData) {
        spAnimationStateData *stateData = spAnimationStateData_create(skeletonData);
        SkeletonDrawable drawable(skeletonData, stateData);
        drawable.timeScale = 1;
        drawable.setUsePremultipliedAlpha(true);
        drawable.skeleton->x = 320;
        drawable.skeleton->y = 590;
        spAnimationState_setAnimationByName(drawable.state, 0, "walk", true);

        Uint32 prev_time = 0;
        do {
            Uint32 curr_time = SDL_GetTicks();
            if (prev_time > 0) {
                float delta = (float)(curr_time - prev_time) / 1000.0f;
                drawable.update(delta);
                SDL_RenderClear(renderer);
                drawable.draw(renderer);
                SDL_RenderPresent(renderer);
            }
            prev_time = curr_time;
            SDL_Event e;
            while (SDL_PollEvent(&e) != 0) if (e.type == SDL_QUIT) prev_time = 0; // quit
        } while (prev_time > 0);
        spAnimationStateData_dispose(stateData);
    }
    else printf("Error loading skeleton\n");

    spSkeletonData_dispose(skeletonData);
    spAtlas_dispose(atlas);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
```