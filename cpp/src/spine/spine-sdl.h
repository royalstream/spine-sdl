//
// Steven Burns 2022.
//

#ifndef SPINE_SDL_H_
#define SPINE_SDL_H_

#include <SDL.h>
#include <SDL_image.h>
#include <spine/spine.h>

namespace spine {

    class SkeletonDrawable {
    public:
        Skeleton *skeleton;
        AnimationState *state;
        float timeScale;
        mutable Vector<SDL_Vertex> vertexArray;
        VertexEffect *vertexEffect;

        SkeletonDrawable(SkeletonData *skeleton, AnimationStateData *stateData = 0);

        ~SkeletonDrawable();

        void update(float deltaTime);

        virtual void draw(SDL_Renderer* renderer) const;

        void setUsePremultipliedAlpha(bool usePMA) { usePremultipliedAlpha = usePMA; };

        bool getUsePremultipliedAlpha() const { return usePremultipliedAlpha; };

    private:
        mutable bool ownsAnimationStateData;
        mutable Vector<float> worldVertices;
        mutable Vector<float> tempUvs;
        mutable Vector<Color> tempColors;
        mutable Vector<unsigned short> quadIndices;
        mutable SkeletonClipping clipper;
        mutable bool usePremultipliedAlpha;
    };

    class SDLTextureLoader : public TextureLoader {
    public:
        virtual void load(AtlasPage &page, const String &path);

        virtual void unload(void *texture);

        SDLTextureLoader(SDL_Renderer* sdl_renderer) : renderer(sdl_renderer) {}
    private:
        SDL_Renderer* renderer;
    };

} /* namespace spine */
#endif /* SPINE_SDL_H_ */
