# Spine Runtime for SDL2

Port of the official [spine-sfml](https://github.com/EsotericSoftware/spine-runtimes/tree/4.0/spine-sfml) runtime to use SDL2 instead of SFML. 
It uses the new [SDL_RenderGeometry](https://wiki.libsdl.org/SDL_RenderGeometry) functionality introduced in SDL2 2.0.18.

Just like **spine-sfml**, two different flavors are offered: one based on [spine-c](https://github.com/EsotericSoftware/spine-runtimes/tree/4.0/spine-c) and one based on [spine-cpp](https://github.com/EsotericSoftware/spine-runtimes/tree/4.0/spine-cpp). They are in the [c](/c) and [cpp](/cpp) folders respectively.

## Version Requirements

- Spine animations exported for runtime 4.0.*
- SDL2 ≥ 2.0.18
- I also tested it with Emscripten 3.1.9.

## Implementation Details

1. The starting point was [spine-sfml](https://github.com/EsotericSoftware/spine-runtimes/tree/4.0/spine-sfml) from the 4.0 branch. Since this implementation is part of the official runtimes, I tried to keep the logic unchanged.

2. I made some small improvements, such as:
    - Made `getUsePremultipliedAlpha()` const
    - Refactored the blend mode logic into a single switch statement. Same performance, half the lines, more readable.
    - **spine-c** version only:
        - Removed the manual implementation for `spColorArray` and used the `_SP_ARRAY_IMPLEMENT_TYPE` macro instead. This change required the `operator==` to be defined, but this is cleaner than writing the entire implementation by hand.

3. Replaced SFML with SDL2 as follows:
    - Using the **_SP_ARRAY** macros, I created a new array type `spVertexArray` based on `SDL_Vertex` and replaced all references to `sf::VertexArray` with it.
    - Defined the blend modes with `SDL_ComposeCustomBlendMode` using [GerogeChong's code](https://github.com/GerogeChong/spine-sdl) as a starting point.
    - Used `SDL_SetTextureBlendMode` and `SDL_RenderGeometry` for drawing. It's roughtly equivalent to what [rmg-nik's](https://github.com/rmg-nik/sdl_spine_demo/tree/render_geometry) did.
    - **spine-c** version only:
        - Implemented `_spUtil_readFile` using `SDL_RWFromFile`, `SDL_RWread`, etc. 
        - Implemented `_spAtlasPage_createTexture`/`_spAtlasPage_disposeTexture` using `IMG_Load` and `SDL_CreateTextureFromSurface`. 
        - **Important:** the previous step depends on a external function called `spSDL_getRenderer` that you have to implement yourself, but it's very easy: just return a pointer to the current **SDL_Renderer**
    - **spine-cpp** version only:
        - Replaced `SFMLTextureLoader` with a new class `SDLTextureLoader` implemented using `IMG_Load` and `SDL_CreateTextureFromSurface`. The constructor takes a pointer to the current

4. Copied the examples from **spine-sfml** and made them use **spine-sdl**: 
    - Replaced the SFML loops with standard SDL loops.
    - Replaced `sf::Mouse::getPosition` with `SDL_GetMouseState`, `window.clear()` with `SDL_RenderClear`, and `window.display()` with `SDL_RenderPresent`
    - Commented out the mix blends in the **owl** example because they don't seem to be working as intended.
    - Fixes to the **c** examples:
        - Leak #1: Calls to `spAnimationStateData_dispose` were missing.
        - Leak #2: `SkeletonDrawable` instances never get deleted, but we can create them on the stack like the **cpp** examples do.

## Documentation

See the [c](/c) or [cpp](/cpp) folders for examples and documentation.
