/******************************************************************************
 * Spine Runtimes License Agreement
 * Last updated January 1, 2020. Replaces all prior versions.
 *
 * Copyright (c) 2013-2020, Esoteric Software LLC
 *
 * Integration of the Spine Runtimes into software or otherwise creating
 * derivative works of the Spine Runtimes is permitted under the terms and
 * conditions of Section 2 of the Spine Editor License Agreement:
 * http://esotericsoftware.com/spine-editor-license
 *
 * Otherwise, it is permitted to integrate the Spine Runtimes into software
 * or otherwise create derivative works of the Spine Runtimes (collectively,
 * "Products"), provided that each user of the Products must obtain their own
 * Spine Editor license and redistribution of the Products in any form must
 * include this license and copyright notice.
 *
 * THE SPINE RUNTIMES ARE PROVIDED BY ESOTERIC SOFTWARE LLC "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ESOTERIC SOFTWARE LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES,
 * BUSINESS INTERRUPTION, OR LOSS OF USE, DATA, OR PROFITS) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THE SPINE RUNTIMES, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include <iostream>
#include <spine/Debug.h>
#include <spine/spine-sdl.h>

using namespace std;
using namespace spine;
#include <stdio.h>
#include <stdlib.h>

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Renderer* spSDL_getRenderer() { return renderer; } // required by spine-sdl

void callback(spAnimationState *state, spEventType type, spTrackEntry *entry, spEvent *event) {
    UNUSED(state);
    const char *animationName = (entry && entry->animation) ? entry->animation->name : 0;

    switch (type) {
        case SP_ANIMATION_START:
            printf("%d start: %s\n", entry->trackIndex, animationName);
            break;
        case SP_ANIMATION_INTERRUPT:
            printf("%d interrupt: %s\n", entry->trackIndex, animationName);
            break;
        case SP_ANIMATION_END:
            printf("%d end: %s\n", entry->trackIndex, animationName);
            break;
        case SP_ANIMATION_COMPLETE:
            printf("%d complete: %s\n", entry->trackIndex, animationName);
            break;
        case SP_ANIMATION_DISPOSE:
            printf("%d dispose: %s\n", entry->trackIndex, animationName);
            break;
        case SP_ANIMATION_EVENT:
            printf("%d event: %s, %s: %d, %f, %s %f %f\n", entry->trackIndex, animationName, event->data->name, event->intValue, event->floatValue,
                   event->stringValue, event->volume, event->balance);
            break;
    }
    fflush(stdout);
}

spSkeletonData *readSkeletonJsonData(const char *filename, spAtlas *atlas, float scale) {
    spSkeletonJson *json = spSkeletonJson_create(atlas);
    json->scale = scale;
    spSkeletonData *skeletonData = spSkeletonJson_readSkeletonDataFile(json, filename);
    if (!skeletonData) {
        printf("%s\n", json->error);
        exit(0);
    }
    spSkeletonJson_dispose(json);
    return skeletonData;
}

spSkeletonData *readSkeletonBinaryData(const char *filename, spAtlas *atlas, float scale) {
    spSkeletonBinary *binary = spSkeletonBinary_create(atlas);
    binary->scale = scale;
    spSkeletonData *skeletonData = spSkeletonBinary_readSkeletonDataFile(binary, filename);
    if (!skeletonData) {
        printf("%s\n", binary->error);
        exit(0);
    }
    spSkeletonBinary_dispose(binary);
    return skeletonData;
}

void testcase(void func(spSkeletonData *skeletonData, spAtlas *atlas),
              const char *jsonName, const char *binaryName, const char *atlasName,
              float scale) {
    spAtlas *atlas = spAtlas_createFromFile(atlasName, 0);

    printf("Running %s\n", jsonName);
    spSkeletonData *skeletonData = readSkeletonJsonData(jsonName, atlas, scale);
    func(skeletonData, atlas);
    spSkeletonData_dispose(skeletonData);

    printf("Running %s\n", binaryName);
    skeletonData = readSkeletonBinaryData(binaryName, atlas, scale);
    func(skeletonData, atlas);
    spSkeletonData_dispose(skeletonData);

    spAtlas_dispose(atlas);
}

void spineboy(spSkeletonData *skeletonData, spAtlas *atlas) {
    UNUSED(atlas);
    // Configure mixing.
    spAnimationStateData *stateData = spAnimationStateData_create(skeletonData);
    spAnimationStateData_setMixByName(stateData, "walk", "jump", 0.2f);
    spAnimationStateData_setMixByName(stateData, "jump", "run", 0.2f);

    SkeletonDrawable drawable(skeletonData, stateData);
    drawable.timeScale = 1;
    drawable.setUsePremultipliedAlpha(true);

    spSkeleton *skeleton = drawable.skeleton;
    spSkeleton_setToSetupPose(skeleton);

    skeleton->x = 320;
    skeleton->y = 590;
    spSkeleton_updateWorldTransform(skeleton);

    drawable.state->listener = callback;
    spAnimationState_addAnimationByName(drawable.state, 0, "walk", true, 0);
    spAnimationState_addAnimationByName(drawable.state, 0, "jump", false, 3);
    spAnimationState_addAnimationByName(drawable.state, 0, "run", true, 0);

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
        while (SDL_PollEvent(&e) != 0)
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) prev_time = 0; // quit
    } while (prev_time > 0);

    spAnimationStateData_dispose(stateData);
}

void ikDemo(spSkeletonData *skeletonData, spAtlas *atlas) {
    UNUSED(atlas);

    // Create the SkeletonDrawable and position it
    spAnimationStateData *stateData = spAnimationStateData_create(skeletonData);
    SkeletonDrawable drawable(skeletonData, stateData);
    drawable.timeScale = 1;
    drawable.setUsePremultipliedAlpha(true);

    spSkeleton *skeleton = drawable.skeleton;
    skeleton->x = 320;
    skeleton->y = 590;

    // Queue the "walk" animation on the first track.
    spAnimationState_setAnimationByName(drawable.state, 0, "walk", true);

    // Queue the "aim" animation on a higher track.
    // It consists of a single frame that positions
    // the back arm and gun such that they point at
    // the "crosshair" bone. By setting this
    // animation on a higher track, it overrides
    // any changes to the back arm and gun made
    // by the walk animation, allowing us to
    // mix the two. The mouse position following
    // is performed in the render() method below.
    spAnimationState_setAnimationByName(drawable.state, 1, "aim", true);

    Uint32 prev_time = 0;
    do {
        Uint32 curr_time = SDL_GetTicks();
        if (prev_time > 0) {
            float delta = (float)(curr_time - prev_time) / 1000.0f;
            // Update and apply the animations to the skeleton,
            // then calculate the world transforms of every bone.
            // This is needed so we can call Bone#worldToLocal()
            // later.
            drawable.update(delta);

            // Position the "crosshair" bone at the mouse
            // location. We do this before calling
            // skeleton.updateWorldTransform() below, so
            // our change is incorporated before the IK
            // constraint is applied.
            //
            // When setting the crosshair bone position
            // to the mouse position, we need to translate
            // from "mouse space" to "local bone space". Note that the local
            // bone space is calculated using the bone's parent
            // worldToLocal() function!
            SDL_Point mouseCoords;
            SDL_GetMouseState(&mouseCoords.x, &mouseCoords.y);
            float boneCoordsX = 0, boneCoordsY = 0;
            spBone *crosshair = spSkeleton_findBone(drawable.skeleton, "crosshair");// Should be cached.
            spBone_worldToLocal(crosshair->parent, mouseCoords.x, mouseCoords.y, &boneCoordsX, &boneCoordsY);
            crosshair->x = boneCoordsX;
            crosshair->y = boneCoordsY;

            // Calculate final world transform with the
            // crosshair bone set to the mouse cursor
            // position.
            spSkeleton_updateWorldTransform(drawable.skeleton);

            SDL_RenderClear(renderer);
            drawable.draw(renderer);
            SDL_RenderPresent(renderer);
        }
        prev_time = curr_time;
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0)
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) prev_time = 0; // quit
    } while (prev_time > 0);

    spAnimationStateData_dispose(stateData);    
}


void goblins(spSkeletonData *skeletonData, spAtlas *atlas) {
    UNUSED(atlas);
    SkeletonDrawable drawable(skeletonData);
    drawable.timeScale = 1;
    drawable.setUsePremultipliedAlpha(true);

    spSkeleton *skeleton = drawable.skeleton;
    spSkeleton_setSkinByName(skeleton, "goblingirl");
    spSkeleton_setSlotsToSetupPose(skeleton);
    skeleton->x = 320;
    skeleton->y = 590;
    spSkeleton_updateWorldTransform(skeleton);

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
        while (SDL_PollEvent(&e) != 0)
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) prev_time = 0; // quit
    } while (prev_time > 0);
}

void raptor(spSkeletonData *skeletonData, spAtlas *atlas) {
    UNUSED(atlas);
    SkeletonDrawable drawable(skeletonData);
    drawable.timeScale = 1;
    drawable.setUsePremultipliedAlpha(true);

    spSwirlVertexEffect *effect = spSwirlVertexEffect_create(400);
    effect->centerY = -200;
    drawable.vertexEffect = &effect->super;

    spSkeleton *skeleton = drawable.skeleton;
    skeleton->x = 320;
    skeleton->y = 590;
    spSkeleton_updateWorldTransform(skeleton);

    spAnimationState_setAnimationByName(drawable.state, 0, "walk", true);
    spAnimationState_addAnimationByName(drawable.state, 1, "gun-grab", false, 2);

    float swirlTime = 0;

    Uint32 prev_time = 0;
    do {
        Uint32 curr_time = SDL_GetTicks();
        if (prev_time > 0) {
            float delta = (float)(curr_time - prev_time) / 1000.0f;

            swirlTime += delta;
            float percent = (float) fmod(swirlTime, 2);
            if (percent > 1) percent = 1 - (percent - 1);
            effect->angle = _spMath_interpolate(_spMath_pow2_apply, -60, 60, percent);

            drawable.update(delta);

            SDL_RenderClear(renderer);
            drawable.draw(renderer);
            SDL_RenderPresent(renderer);
        }
        prev_time = curr_time;
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0)
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) prev_time = 0; // quit
    } while (prev_time > 0);
    spSwirlVertexEffect_dispose(effect);
}

void tank(spSkeletonData *skeletonData, spAtlas *atlas) {
    UNUSED(atlas);
    SkeletonDrawable drawable(skeletonData);
    drawable.timeScale = 1;
    drawable.setUsePremultipliedAlpha(true);

    spSkeleton *skeleton = drawable.skeleton;
    skeleton->x = 500;
    skeleton->y = 590;
    spSkeleton_updateWorldTransform(skeleton);

    spAnimationState_setAnimationByName(drawable.state, 0, "drive", true);

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
        while (SDL_PollEvent(&e) != 0)
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) prev_time = 0; // quit
    } while (prev_time > 0);
}

void vine(spSkeletonData *skeletonData, spAtlas *atlas) {
    UNUSED(atlas);
    SkeletonDrawable drawable(skeletonData);
    drawable.timeScale = 1;
    drawable.setUsePremultipliedAlpha(true);

    spSkeleton *skeleton = drawable.skeleton;
    skeleton->x = 320;
    skeleton->y = 590;
    spSkeleton_updateWorldTransform(skeleton);

    spAnimationState_setAnimationByName(drawable.state, 0, "grow", true);

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
        while (SDL_PollEvent(&e) != 0)
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) prev_time = 0; // quit
    } while (prev_time > 0);
}

void stretchyman(spSkeletonData *skeletonData, spAtlas *atlas) {
    UNUSED(atlas);
    SkeletonDrawable drawable(skeletonData);
    drawable.timeScale = 1;
    drawable.setUsePremultipliedAlpha(true);

    spSkeleton *skeleton = drawable.skeleton;
    skeleton->x = 100;
    skeleton->y = 590;
    spSkeleton_updateWorldTransform(skeleton);

    spAnimationState_setAnimationByName(drawable.state, 0, "sneak", true);

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
        while (SDL_PollEvent(&e) != 0)
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) prev_time = 0; // quit
    } while (prev_time > 0);
}

void coin(spSkeletonData *skeletonData, spAtlas *atlas) {
    UNUSED(atlas);


    SkeletonDrawable drawable(skeletonData);
    drawable.timeScale = 1;
    drawable.setUsePremultipliedAlpha(true);

    spSkeleton *skeleton = drawable.skeleton;
    skeleton->x = 320;
    skeleton->y = 320;
    spSkeleton_updateWorldTransform(skeleton);
    spAnimationState_setAnimationByName(drawable.state, 0, "animation", true);

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
        while (SDL_PollEvent(&e) != 0)
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) prev_time = 0; // quit
    } while (prev_time > 0);
}

void owl(spSkeletonData *skeletonData, spAtlas *atlas) {
    UNUSED(atlas);
    SkeletonDrawable drawable(skeletonData);
    drawable.timeScale = 1;
    drawable.setUsePremultipliedAlpha(true);

    spSkeleton *skeleton = drawable.skeleton;
    skeleton->x = 320;
    skeleton->y = 400;
    spSkeleton_updateWorldTransform(skeleton);

    spAnimationState_setAnimationByName(drawable.state, 0, "idle", true);
    spAnimationState_setAnimationByName(drawable.state, 1, "blink", true);
    spTrackEntry *left = spAnimationState_setAnimationByName(drawable.state, 2, "left", true);
    spTrackEntry *right = spAnimationState_setAnimationByName(drawable.state, 3, "right", true);
    spTrackEntry *up = spAnimationState_setAnimationByName(drawable.state, 4, "up", true);
    spTrackEntry *down = spAnimationState_setAnimationByName(drawable.state, 5, "down", true);

    left->alpha = 0;
    //left->mixBlend = SP_MIX_BLEND_ADD;
    right->alpha = 0;
    //right->mixBlend = SP_MIX_BLEND_ADD;
    up->alpha = 0;
    //up->mixBlend = SP_MIX_BLEND_ADD;
    down->alpha = 0;
    //down->mixBlend = SP_MIX_BLEND_ADD;

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
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) prev_time = 0; // quit
            else if(e.type == SDL_MOUSEMOTION) {
                float x = e.motion.x / 640.0f;
                left->alpha = (MAX(x, 0.5f) - 0.5f) * 2;
                right->alpha = (0.5f - MIN(x, 0.5f)) * 2;

                float y = e.motion.y / 640.0f;
                down->alpha = (MAX(y, 0.5f) - 0.5f) * 2;
                up->alpha = (0.5f - MIN(y, 0.5f)) * 2;
            }
        }
    } while (prev_time > 0);
}

void test(spSkeletonData *skeletonData, spAtlas *atlas) {
    UNUSED(atlas);
    spSkeleton *skeleton = spSkeleton_create(skeletonData);
    spAnimationStateData *animData = spAnimationStateData_create(skeletonData);
    spAnimationState *animState = spAnimationState_create(animData);
    spAnimationState_setAnimationByName(animState, 0, "drive", true);


    float d = 3;
    for (int i = 0; i < 1; i++) {
        spSkeleton_update(skeleton, d);
        spAnimationState_update(animState, d);
        spAnimationState_apply(animState, skeleton);
        spSkeleton_updateWorldTransform(skeleton);
        for (int ii = 0; ii < skeleton->bonesCount; ii++) {
            spBone *bone = skeleton->bones[ii];
            printf("%s %f %f %f %f %f %f\n", bone->data->name, bone->a, bone->b, bone->c, bone->d, bone->worldX, bone->worldY);
        }
        printf("========================================\n");
        d += 0.1f;
    }

    spAnimationStateData_dispose(animData);
    spSkeleton_dispose(skeleton);
}

void testSkinsApi(spSkeletonData *skeletonData, spAtlas *atlas) {
    UNUSED(atlas);
    SkeletonDrawable drawable(skeletonData);
    drawable.timeScale = 1;
    drawable.setUsePremultipliedAlpha(true);

    spSkeleton *skeleton = drawable.skeleton;

    spSkin *skin = spSkin_create("test-skin");
    spSkin_copySkin(skin, spSkeletonData_findSkin(skeletonData, "goblingirl"));
    // spSkin_addSkin(skin, spSkeletonData_findSkin(skeletonData, "goblingirl"));
    spSkeleton_setSkin(skeleton, skin);
    spSkeleton_setSlotsToSetupPose(skeleton);

    skeleton->x = 320;
    skeleton->y = 590;
    spSkeleton_updateWorldTransform(skeleton);

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
        while (SDL_PollEvent(&e) != 0)
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) prev_time = 0; // quit
    } while (prev_time > 0);

    spSkin_clear(skin);
    spSkin_dispose(skin);
}

void testMixAndMatch(spSkeletonData *skeletonData, spAtlas *atlas) {
    UNUSED(atlas);
    SkeletonDrawable drawable(skeletonData);
    drawable.timeScale = 1;
    drawable.setUsePremultipliedAlpha(true);

    spSkeleton *skeleton = drawable.skeleton;

    // Create a new skin, by mixing and matching other skins
    // that fit together. Items making up the girl are individual
    // skins. Using the skin API, a new skin is created which is
    // a combination of all these individual item skins.
    spSkin *skin = spSkin_create("mix-and-match");
    spSkin_addSkin(skin, spSkeletonData_findSkin(skeletonData, "skin-base"));
    spSkin_addSkin(skin, spSkeletonData_findSkin(skeletonData, "nose/short"));
    spSkin_addSkin(skin, spSkeletonData_findSkin(skeletonData, "eyelids/girly"));
    spSkin_addSkin(skin, spSkeletonData_findSkin(skeletonData, "eyes/violet"));
    spSkin_addSkin(skin, spSkeletonData_findSkin(skeletonData, "hair/brown"));
    spSkin_addSkin(skin, spSkeletonData_findSkin(skeletonData, "clothes/hoodie-orange"));
    spSkin_addSkin(skin, spSkeletonData_findSkin(skeletonData, "legs/pants-jeans"));
    spSkin_addSkin(skin, spSkeletonData_findSkin(skeletonData, "accessories/bag"));
    spSkin_addSkin(skin, spSkeletonData_findSkin(skeletonData, "accessories/hat-red-yellow"));
    spSkeleton_setSkin(skeleton, skin);
    spSkeleton_setSlotsToSetupPose(skeleton);

    skeleton->x = 320;
    skeleton->y = 590;
    spSkeleton_updateWorldTransform(skeleton);

    spAnimationState_setAnimationByName(drawable.state, 0, "dance", true);

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
        while (SDL_PollEvent(&e) != 0)
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) prev_time = 0; // quit
    } while (prev_time > 0);

    spSkin_clear(skin);
    spSkin_dispose(skin);
}

int main()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(640, 640, 0, &window, &renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    printf("\nHit the ESC key or click the close button to move to the next test\n");
    testcase(ikDemo, "data/spineboy-pro.json", "data/spineboy-pro.skel", "data/spineboy-pma.atlas", 0.6f);
    testcase(spineboy, "data/spineboy-pro.json", "data/spineboy-pro.skel", "data/spineboy-pma.atlas", 0.6f);
    testcase(coin, "data/coin-pro.json", "data/coin-pro.skel", "data/coin-pma.atlas", 0.5f);
    testcase(testMixAndMatch, "data/mix-and-match-pro.json", "data/mix-and-match-pro.skel", "data/mix-and-match-pma.atlas", 0.5f);
    testcase(test, "data/tank-pro.json", "data/tank-pro.skel", "data/tank-pma.atlas", 1.0f);
    testcase(owl, "data/owl-pro.json", "data/owl-pro.skel", "data/owl-pma.atlas", 0.5f);
    testcase(vine, "data/vine-pro.json", "data/vine-pro.skel", "data/vine-pma.atlas", 0.5f);
    testcase(tank, "data/tank-pro.json", "data/tank-pro.skel", "data/tank-pma.atlas", 0.2f);
    testcase(raptor, "data/raptor-pro.json", "data/raptor-pro.skel", "data/raptor-pma.atlas", 0.5f);
    testcase(goblins, "data/goblins-pro.json", "data/goblins-pro.skel", "data/goblins-pma.atlas", 1.4f);
    testcase(stretchyman, "data/stretchyman-pro.json", "data/stretchyman-pro.skel", "data/stretchyman-pma.atlas", 0.6f);
    testcase(testSkinsApi, "data/goblins-pro.json", "data/goblins-pro.skel", "data/goblins-pma.atlas", 1.4f);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
