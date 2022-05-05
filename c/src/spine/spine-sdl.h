//
// Steven Burns 2022.
//

#ifndef SPINE_SDL_H_
#define SPINE_SDL_H_

#include <SDL.h>
#include <SDL_image.h>
#include <spine/extension.h>
#include <spine/spine.h>

_SP_ARRAY_DECLARE_TYPE(spColorArray, spColor)
_SP_ARRAY_DECLARE_TYPE(spVertexArray, SDL_Vertex)

namespace spine {

    class SkeletonDrawable {
    public:
        spSkeleton *skeleton;
        spAnimationState *state;
        float timeScale;
        spVertexArray *vertexArray;
        spVertexEffect *vertexEffect;

        explicit SkeletonDrawable(spSkeletonData *skeleton, spAnimationStateData *stateData = 0);
        ~SkeletonDrawable();

        void update(float deltaTime);

        virtual void draw(SDL_Renderer* renderer) const;

        void setUsePremultipliedAlpha(bool usePMA) { usePremultipliedAlpha = usePMA; };
        bool getUsePremultipliedAlpha() const { return usePremultipliedAlpha; };

    private:
        bool ownsAnimationStateData;
        float *worldVertices;
        spFloatArray *tempUvs;
        spColorArray *tempColors;
        spSkeletonClipping *clipper;
        bool usePremultipliedAlpha;
    };

} /* namespace spine */
#endif /* SPINE_SDL_H_ */
