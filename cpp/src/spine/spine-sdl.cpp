//
// Steven Burns 2022.
//

#include <spine/spine-sdl.h>

#ifndef SPINE_MESH_VERTEX_COUNT_MAX
#define SPINE_MESH_VERTEX_COUNT_MAX 1000
#endif

namespace blend {
    SDL_BlendMode normal = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_SRC_ALPHA,SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,SDL_BLENDOPERATION_ADD,
                                                      SDL_BLENDFACTOR_SRC_ALPHA,SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,SDL_BLENDOPERATION_ADD);

    SDL_BlendMode additive = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_SRC_ALPHA,SDL_BLENDFACTOR_ONE,SDL_BLENDOPERATION_ADD,
                                                        SDL_BLENDFACTOR_SRC_ALPHA,SDL_BLENDFACTOR_ONE,SDL_BLENDOPERATION_ADD);

    SDL_BlendMode multiply = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_DST_COLOR,SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,SDL_BLENDOPERATION_ADD,
                                                        SDL_BLENDFACTOR_DST_COLOR,SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,SDL_BLENDOPERATION_ADD);

    SDL_BlendMode screen = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE,SDL_BLENDFACTOR_ONE_MINUS_SRC_COLOR,SDL_BLENDOPERATION_ADD,
                                                      SDL_BLENDFACTOR_ONE,SDL_BLENDFACTOR_ONE_MINUS_SRC_COLOR,SDL_BLENDOPERATION_ADD);

    SDL_BlendMode normalPma = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE,SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,SDL_BLENDOPERATION_ADD,
                                                         SDL_BLENDFACTOR_ONE,SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,SDL_BLENDOPERATION_ADD);

    SDL_BlendMode additivePma = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE,SDL_BLENDFACTOR_ONE,SDL_BLENDOPERATION_ADD,
                                                           SDL_BLENDFACTOR_ONE,SDL_BLENDFACTOR_ONE,SDL_BLENDOPERATION_ADD);

    SDL_BlendMode multiplyPma = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_DST_COLOR,SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,SDL_BLENDOPERATION_ADD,
                                                           SDL_BLENDFACTOR_DST_COLOR,SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,SDL_BLENDOPERATION_ADD);

    SDL_BlendMode screenPma = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE,SDL_BLENDFACTOR_ONE_MINUS_SRC_COLOR,SDL_BLENDOPERATION_ADD,
                                                         SDL_BLENDFACTOR_ONE,SDL_BLENDFACTOR_ONE_MINUS_SRC_COLOR,SDL_BLENDOPERATION_ADD);
}


namespace spine {

    SkeletonDrawable::SkeletonDrawable(SkeletonData *skeletonData, AnimationStateData *stateData) : timeScale(1),
                                                                                                    vertexEffect(NULL), worldVertices(), clipper() {
        Bone::setYDown(true);
        worldVertices.ensureCapacity(SPINE_MESH_VERTEX_COUNT_MAX);
        vertexArray.ensureCapacity(SPINE_MESH_VERTEX_COUNT_MAX);
        skeleton = new (__FILE__, __LINE__) Skeleton(skeletonData);
        tempUvs.ensureCapacity(16);
        tempColors.ensureCapacity(16);

        ownsAnimationStateData = stateData == 0;
        if (ownsAnimationStateData) stateData = new (__FILE__, __LINE__) AnimationStateData(skeletonData);

        state = new (__FILE__, __LINE__) AnimationState(stateData);

        quadIndices.add(0);
        quadIndices.add(1);
        quadIndices.add(2);
        quadIndices.add(2);
        quadIndices.add(3);
        quadIndices.add(0);
    }

    SkeletonDrawable::~SkeletonDrawable() {
        // vertexArray takes care of itself (just like worldVertices)
        if (ownsAnimationStateData) delete state->getData();
        delete state;
        delete skeleton;
    }

    void SkeletonDrawable::update(float deltaTime) {
        skeleton->update(deltaTime);
        state->update(deltaTime * timeScale);
        state->apply(*skeleton);
        skeleton->updateWorldTransform();
    }

    void SkeletonDrawable::draw(SDL_Renderer *renderer) const {
        struct { SDL_Texture* texture; SDL_BlendMode blendMode; } states; // keep the syntax as close as possible to spine-sfml
        vertexArray.clear();
        states.texture = NULL;

        // Early out if skeleton is invisible
        if (skeleton->getColor().a == 0) return;

        if (vertexEffect != NULL) vertexEffect->begin(*skeleton);

        SDL_Vertex vertex;
        SDL_Texture *texture = NULL;
        for (unsigned i = 0; i < skeleton->getSlots().size(); ++i) {
            Slot &slot = *skeleton->getDrawOrder()[i];
            Attachment *attachment = slot.getAttachment();
            if (!attachment) continue;

            // Early out if the slot color is 0 or the bone is not active
            if (slot.getColor().a == 0 || !slot.getBone().isActive()) {
                clipper.clipEnd(slot);
                continue;
            }

            Vector<float> *vertices = &worldVertices;
            int verticesCount = 0;
            Vector<float> *uvs = NULL;
            Vector<unsigned short> *indices = NULL;
            int indicesCount = 0;
            Color *attachmentColor;

            if (attachment->getRTTI().isExactly(RegionAttachment::rtti)) {
                RegionAttachment *regionAttachment = (RegionAttachment *) attachment;
                attachmentColor = &regionAttachment->getColor();

                // Early out if the slot color is 0
                if (attachmentColor->a == 0) {
                    clipper.clipEnd(slot);
                    continue;
                }

                worldVertices.setSize(8, 0);
                regionAttachment->computeWorldVertices(slot.getBone(), worldVertices, 0, 2);
                verticesCount = 4;
                uvs = &regionAttachment->getUVs();
                indices = &quadIndices;
                indicesCount = 6;
                texture = (SDL_Texture*) ((AtlasRegion *) regionAttachment->getRendererObject())->page->getRendererObject();

            } else if (attachment->getRTTI().isExactly(MeshAttachment::rtti)) {
                MeshAttachment *mesh = (MeshAttachment *) attachment;
                attachmentColor = &mesh->getColor();

                // Early out if the slot color is 0
                if (attachmentColor->a == 0) {
                    clipper.clipEnd(slot);
                    continue;
                }

                worldVertices.setSize(mesh->getWorldVerticesLength(), 0);
                texture = (SDL_Texture*) ((AtlasRegion *) mesh->getRendererObject())->page->getRendererObject();
                mesh->computeWorldVertices(slot, 0, mesh->getWorldVerticesLength(), worldVertices, 0, 2);
                verticesCount = mesh->getWorldVerticesLength() >> 1;
                uvs = &mesh->getUVs();
                indices = &mesh->getTriangles();
                indicesCount = mesh->getTriangles().size();

            } else if (attachment->getRTTI().isExactly(ClippingAttachment::rtti)) {
                ClippingAttachment *clip = (ClippingAttachment *) slot.getAttachment();
                clipper.clipStart(slot, clip);
                continue;
            } else
                continue;

            Uint8 r = static_cast<Uint8>(skeleton->getColor().r * slot.getColor().r * attachmentColor->r * 255);
            Uint8 g = static_cast<Uint8>(skeleton->getColor().g * slot.getColor().g * attachmentColor->g * 255);
            Uint8 b = static_cast<Uint8>(skeleton->getColor().b * slot.getColor().b * attachmentColor->b * 255);
            Uint8 a = static_cast<Uint8>(skeleton->getColor().a * slot.getColor().a * attachmentColor->a * 255);
            vertex.color.r = r;
            vertex.color.g = g;
            vertex.color.b = b;
            vertex.color.a = a;

            Color light;
            light.r = r / 255.0f;
            light.g = g / 255.0f;
            light.b = b / 255.0f;
            light.a = a / 255.0f;

            SDL_BlendMode blend;
            switch (slot.getData().getBlendMode()) {
                case BlendMode_Normal:
                    blend = usePremultipliedAlpha ? blend::normalPma : blend::normal;
                    break;
                case BlendMode_Additive:
                    blend = usePremultipliedAlpha ? blend::additivePma : blend::additive;
                    break;
                case BlendMode_Multiply:
                    blend = usePremultipliedAlpha ? blend::multiplyPma : blend::multiply;
                    break;
                case BlendMode_Screen:
                    blend = usePremultipliedAlpha ? blend::screenPma : blend::screen;
                    break;
            }

            if (states.texture == 0) states.texture = texture;

            if (states.blendMode != blend || states.texture != texture) {
                if (vertexArray.size() > 0) {
                    SDL_SetTextureBlendMode(states.texture, states.blendMode);
                    SDL_RenderGeometry(renderer, states.texture, vertexArray.buffer(), vertexArray.size(), 0, 0);
                    vertexArray.clear();
                }
                states.blendMode = blend;
                states.texture = texture;
            }

            if (clipper.isClipping()) {
                clipper.clipTriangles(worldVertices, *indices, *uvs, 2);
                vertices = &clipper.getClippedVertices();
                verticesCount = clipper.getClippedVertices().size() >> 1;
                uvs = &clipper.getClippedUVs();
                indices = &clipper.getClippedTriangles();
                indicesCount = clipper.getClippedTriangles().size();
            }

            if (vertexEffect != 0) {
                tempUvs.clear();
                tempColors.clear();
                for (int ii = 0; ii < verticesCount; ii++) {
                    Color vertexColor = light;
                    Color dark;
                    dark.r = dark.g = dark.b = dark.a = 0;
                    int index = ii << 1;
                    float x = (*vertices)[index];
                    float y = (*vertices)[index + 1];
                    float u = (*uvs)[index];
                    float v = (*uvs)[index + 1];
                    vertexEffect->transform(x, y, u, v, vertexColor, dark);
                    (*vertices)[index] = x;
                    (*vertices)[index + 1] = y;
                    tempUvs.add(u);
                    tempUvs.add(v);
                    tempColors.add(vertexColor);
                }

                for (int ii = 0; ii < indicesCount; ++ii) {
                    int index = (*indices)[ii] << 1;
                    vertex.position.x = (*vertices)[index];
                    vertex.position.y = (*vertices)[index + 1];
                    vertex.tex_coord.x = (*uvs)[index];
                    vertex.tex_coord.y = (*uvs)[index + 1];
                    Color vertexColor = tempColors[index >> 1];
                    vertex.color.r = static_cast<Uint8>(vertexColor.r * 255);
                    vertex.color.g = static_cast<Uint8>(vertexColor.g * 255);
                    vertex.color.b = static_cast<Uint8>(vertexColor.b * 255);
                    vertex.color.a = static_cast<Uint8>(vertexColor.a * 255);
                    vertexArray.add(vertex);
                }
            } else {
                for (int ii = 0; ii < indicesCount; ++ii) {
                    int index = (*indices)[ii] << 1;
                    vertex.position.x = (*vertices)[index];
                    vertex.position.y = (*vertices)[index + 1];
                    vertex.tex_coord.x = (*uvs)[index];
                    vertex.tex_coord.y = (*uvs)[index + 1];
                    vertexArray.add(vertex);
                }
            }
            clipper.clipEnd(slot);
        }

        if (vertexArray.size() > 0)
        {
            SDL_SetTextureBlendMode(states.texture, states.blendMode);
            SDL_RenderGeometry(renderer, states.texture, vertexArray.buffer(), vertexArray.size(), 0, 0);
            vertexArray.clear();
        }
        clipper.clipEnd();

        if (vertexEffect != 0) vertexEffect->end();
    }

    void SDLTextureLoader::load(AtlasPage &page, const String &path) {
        SDL_Surface* img = IMG_Load(path.buffer());
        if (!img) {
            printf("Error loading image: %s\n", path.buffer());
            return;
        }
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, img);
        page.setRendererObject(texture);
        page.width = img->w;
        page.height = img->h;
        SDL_FreeSurface(img);
    }

    void SDLTextureLoader::unload(void *texture) {
        if (texture != NULL) SDL_DestroyTexture((SDL_Texture*)texture);
    }

    SpineExtension *getDefaultExtension() {
        return new DefaultSpineExtension();
    }
}// namespace spine
