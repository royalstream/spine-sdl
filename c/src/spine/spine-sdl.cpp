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

// operator== is required by _SP_ARRAY_IMPLEMENT_TYPE's name##_contains method
// That said, the compiler will optimize it away because we don't use it.
bool operator==(spColor c1, spColor c2) {
    return c1.r == c2.r &&
           c1.g == c2.g &&
           c1.b == c2.b &&
           c1.a == c2.a;
}
_SP_ARRAY_IMPLEMENT_TYPE(spColorArray, spColor)

bool operator==(SDL_Vertex v1, SDL_Vertex v2) { 
    return v1.color.r == v2.color.r &&
           v1.color.g == v2.color.g &&
           v1.color.b == v2.color.b &&
           v1.color.a == v2.color.a &&
           v1.position.x == v2.position.x &&
           v1.position.y == v2.position.y &&
           v1.tex_coord.x == v2.tex_coord.x &&
           v1.tex_coord.y == v2.tex_coord.y;
}
_SP_ARRAY_IMPLEMENT_TYPE(spVertexArray, SDL_Vertex)

extern SDL_Renderer* spSDL_getRenderer(); // to be implemented by end users

void _spAtlasPage_createTexture(spAtlasPage *self, const char *path) {
    SDL_Surface* img = IMG_Load(path);
    if (!img) {
        printf("Error loading image: %s\n", path);
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(spSDL_getRenderer(), img);
    self->rendererObject = texture;
    self->width = img->w;
    self->height = img->h;
    SDL_FreeSurface(img);
}

void _spAtlasPage_disposeTexture(spAtlasPage *self) {
    if (self->rendererObject != 0) SDL_DestroyTexture((SDL_Texture*)self->rendererObject);
}

char *_spUtil_readFile(const char *path, int *length) {
    SDL_RWops* file = SDL_RWFromFile(path, "rb");
    if (!file) {
        printf("Error reading file: %s\n", path);
        return 0;
    }
    *length = (int)SDL_RWsize(file);
    char* data = MALLOC(char, *length);
    SDL_RWread(file, data, 1, *length);
    SDL_RWclose(file);
    return data;
}

namespace spine {
    SkeletonDrawable::SkeletonDrawable(spSkeletonData *skeletonData, spAnimationStateData *stateData)
    : timeScale(1), vertexEffect(0), clipper(0), usePremultipliedAlpha(false)
    {
        spBone_setYDown(true);
        worldVertices = MALLOC(float, SPINE_MESH_VERTEX_COUNT_MAX);
        skeleton = spSkeleton_create(skeletonData);
        tempUvs = spFloatArray_create(16);
        tempColors = spColorArray_create(16);
        vertexArray = spVertexArray_create(SPINE_MESH_VERTEX_COUNT_MAX);

        ownsAnimationStateData = stateData == 0;
        if (ownsAnimationStateData) stateData = spAnimationStateData_create(skeletonData);

        state = spAnimationState_create(stateData);

        clipper = spSkeletonClipping_create();
    }

    SkeletonDrawable::~SkeletonDrawable() {
        spVertexArray_dispose(vertexArray);
        FREE(worldVertices);
        if (ownsAnimationStateData) spAnimationStateData_dispose(state->data);
        spAnimationState_dispose(state);
        spSkeleton_dispose(skeleton);
        spSkeletonClipping_dispose(clipper);
        spFloatArray_dispose(tempUvs);
        spColorArray_dispose(tempColors);
    }

    void SkeletonDrawable::update(float deltaTime) {
        spSkeleton_update(skeleton, deltaTime);
        spAnimationState_update(state, deltaTime * timeScale);
        spAnimationState_apply(state, skeleton);
        spSkeleton_updateWorldTransform(skeleton);
    }

    void SkeletonDrawable::draw(SDL_Renderer *renderer) const {
        struct { SDL_Texture* texture; SDL_BlendMode blendMode; } states; // keep the syntax as close as possible to spine-sfml
        spVertexArray_clear(vertexArray);
        states.texture = 0;
        unsigned short quadIndices[6] = {0, 1, 2, 2, 3, 0};

        // Early out if skeleton is invisible
        if (skeleton->color.a == 0) return;

        if (vertexEffect != 0) vertexEffect->begin(vertexEffect, skeleton);

        SDL_Vertex vertex;
        SDL_Texture *texture = 0;
        for (int i = 0; i < skeleton->slotsCount; ++i) {
            spSlot *slot = skeleton->drawOrder[i];
            spAttachment *attachment = slot->attachment;
            if (!attachment) continue;

            // Early out if slot is invisible
            if (slot->color.a == 0 || !slot->bone->active) {
                spSkeletonClipping_clipEnd(clipper, slot);
                continue;
            }

            float *vertices = worldVertices;
            int verticesCount = 0;
            float *uvs = 0;
            unsigned short *indices = 0;
            int indicesCount = 0;
            spColor *attachmentColor;

            if (attachment->type == SP_ATTACHMENT_REGION) {
                spRegionAttachment *regionAttachment = (spRegionAttachment *) attachment;
                attachmentColor = &regionAttachment->color;

                // Early out if slot is invisible
                if (attachmentColor->a == 0) {
                    spSkeletonClipping_clipEnd(clipper, slot);
                    continue;
                }

                spRegionAttachment_computeWorldVertices(regionAttachment, slot->bone, vertices, 0, 2);
                verticesCount = 4;
                uvs = regionAttachment->uvs;
                indices = quadIndices;
                indicesCount = 6;
                texture = (SDL_Texture*) ((spAtlasRegion *) regionAttachment->rendererObject)->page->rendererObject;

            } else if (attachment->type == SP_ATTACHMENT_MESH) {
                spMeshAttachment *mesh = (spMeshAttachment *) attachment;
                attachmentColor = &mesh->color;

                // Early out if slot is invisible
                if (attachmentColor->a == 0) {
                    spSkeletonClipping_clipEnd(clipper, slot);
                    continue;
                }

                if (mesh->super.worldVerticesLength > SPINE_MESH_VERTEX_COUNT_MAX) continue;
                texture = (SDL_Texture*) ((spAtlasRegion *) mesh->rendererObject)->page->rendererObject;
                spVertexAttachment_computeWorldVertices(SUPER(mesh), slot, 0, mesh->super.worldVerticesLength, worldVertices, 0, 2);
                verticesCount = mesh->super.worldVerticesLength >> 1;
                uvs = mesh->uvs;
                indices = mesh->triangles;
                indicesCount = mesh->trianglesCount;

            } else if (attachment->type == SP_ATTACHMENT_CLIPPING) {
                spClippingAttachment *clip = (spClippingAttachment *) slot->attachment;
                spSkeletonClipping_clipStart(clipper, slot, clip);
                continue;
            } else
                continue;

            Uint8 r = static_cast<Uint8>(skeleton->color.r * slot->color.r * attachmentColor->r * 255);
            Uint8 g = static_cast<Uint8>(skeleton->color.g * slot->color.g * attachmentColor->g * 255);
            Uint8 b = static_cast<Uint8>(skeleton->color.b * slot->color.b * attachmentColor->b * 255);
            Uint8 a = static_cast<Uint8>(skeleton->color.a * slot->color.a * attachmentColor->a * 255);
            vertex.color.r = r;
            vertex.color.g = g;
            vertex.color.b = b;
            vertex.color.a = a;

            spColor light;
            light.r = r / 255.0f;
            light.g = g / 255.0f;
            light.b = b / 255.0f;
            light.a = a / 255.0f;

            SDL_BlendMode blend;
            switch (slot->data->blendMode) {
                case SP_BLEND_MODE_NORMAL:
                    blend = usePremultipliedAlpha ? blend::normalPma : blend::normal;
                    break;
                case SP_BLEND_MODE_ADDITIVE:
                    blend = usePremultipliedAlpha ? blend::additivePma : blend::additive;
                    break;
                case SP_BLEND_MODE_MULTIPLY:
                    blend = usePremultipliedAlpha ? blend::multiplyPma : blend::multiply;
                    break;
                case SP_BLEND_MODE_SCREEN:
                    blend = usePremultipliedAlpha ? blend::screenPma : blend::screen;
                    break;
            }

            if (states.texture == 0) states.texture = texture;

            if (states.blendMode != blend || states.texture != texture) {
                if (vertexArray->size > 0) {
                    SDL_SetTextureBlendMode(states.texture, states.blendMode);
                    SDL_RenderGeometry(renderer, states.texture, vertexArray->items, vertexArray->size, 0, 0);
                    spVertexArray_clear(vertexArray);
                }
                states.blendMode = blend;
                states.texture = texture;
            }

            if (spSkeletonClipping_isClipping(clipper)) {
                spSkeletonClipping_clipTriangles(clipper, vertices, verticesCount << 1, indices, indicesCount, uvs, 2);
                vertices = clipper->clippedVertices->items;
                verticesCount = clipper->clippedVertices->size >> 1;
                uvs = clipper->clippedUVs->items;
                indices = clipper->clippedTriangles->items;
                indicesCount = clipper->clippedTriangles->size;
            }

            if (vertexEffect != 0) {
                spFloatArray_clear(tempUvs);
                spColorArray_clear(tempColors);
                for (int j = 0; j < verticesCount; j++) {
                    spColor vertexColor = light;
                    spColor dark;
                    dark.r = dark.g = dark.b = dark.a = 0;
                    int index = j << 1;
                    float x = vertices[index];
                    float y = vertices[index + 1];
                    float u = uvs[index];
                    float v = uvs[index + 1];
                    vertexEffect->transform(vertexEffect, &x, &y, &u, &v, &vertexColor, &dark);
                    vertices[index] = x;
                    vertices[index + 1] = y;
                    spFloatArray_add(tempUvs, u);
                    spFloatArray_add(tempUvs, v);
                    spColorArray_add(tempColors, vertexColor);
                }

                for (int j = 0; j < indicesCount; ++j) {
                    int index = indices[j] << 1;
                    vertex.position.x = vertices[index];
                    vertex.position.y = vertices[index + 1];
                    vertex.tex_coord.x = uvs[index];
                    vertex.tex_coord.y = uvs[index + 1];
                    spColor vertexColor = tempColors->items[index >> 1];
                    vertex.color.r = static_cast<Uint8>(vertexColor.r * 255);
                    vertex.color.g = static_cast<Uint8>(vertexColor.g * 255);
                    vertex.color.b = static_cast<Uint8>(vertexColor.b * 255);
                    vertex.color.a = static_cast<Uint8>(vertexColor.a * 255);
                    spVertexArray_add(vertexArray, vertex);
                }
            } else {
                for (int j = 0; j < indicesCount; ++j)
                {
                    int index = indices[j] << 1;
                    vertex.position.x = vertices[index];
                    vertex.position.y = vertices[index + 1];
                    vertex.tex_coord.x = uvs[index];
                    vertex.tex_coord.y = uvs[index + 1];
                    spVertexArray_add(vertexArray, vertex);
                }
            }

            spSkeletonClipping_clipEnd(clipper, slot);
        }

        if (vertexArray->size > 0)
        {
            SDL_SetTextureBlendMode(states.texture, states.blendMode);
            SDL_RenderGeometry(renderer, states.texture, vertexArray->items, vertexArray->size, 0, 0);
            spVertexArray_clear(vertexArray);
        }

        spSkeletonClipping_clipEnd2(clipper);

        if (vertexEffect != 0) vertexEffect->end(vertexEffect);
    }

} /* namespace spine */

