# Spine Runtime for SDL2

This is the version based on [spine-sfml/cpp](https://github.com/EsotericSoftware/spine-runtimes/tree/4.0/spine-sfml/cpp) (which in turn uses [spine-cpp](https://github.com/EsotericSoftware/spine-runtimes/tree/4.0/spine-cpp))


## Documentation

I'm providing the same examples included with **spine-sfml** but modified to use **spine-sdl**

A minimalistic example would look like this:

```C++
#include <spine/spine-sdl.h>

using namespace spine;

int main() {
    SDL_Window *window;
    SDL_Renderer *renderer;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(640, 640, 0, &window, &renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 64, 255);

    SDLTextureLoader textureLoader(renderer);
    Atlas atlas("data/spineboy-pma.atlas", &textureLoader);

    SkeletonBinary binary(&atlas);
    binary.setScale(0.6f);
    auto skeletonData = binary.readSkeletonDataFile("data/spineboy-pro.skel");
    // Alternatively you can load the json:
    // SkeletonJson json(&atlas);
    // json.setScale(0.6f);
    // auto skeletonData = json.readSkeletonDataFile("data/spineboy-pro.json");
    if (skeletonData) {
        AnimationStateData stateData(skeletonData);
        SkeletonDrawable drawable(skeletonData, &stateData);
        drawable.timeScale = 1;
        drawable.setUsePremultipliedAlpha(true);
        drawable.skeleton->setPosition(320, 590);
        drawable.state->setAnimation(0, "walk", true);

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
    }
    else printf("Error loading skeleton\n");

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
```
