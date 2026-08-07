#include <sgkit/scene/Model.h>

#include <sgkit/scene/Scene.h>
#include <sgkit/scene/Mesh.h>

#include <sgkit/framework/DebugOut.h>
#include <sgkit/core/FileSystem.h>
#include <sgkit/sgkit_graphics.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>
#include <assimp/material.h>
#include <assimp/texture.h>
#include <assimp/quaternion.h>
#include <assimp/GltfMaterial.h>

#include <cmath>
#include <cstring>
#include <unordered_map>
#include <unordered_set>

namespace sgkit {
namespace scene {

// -- Internal helpers

namespace {

using TexCache = std::unordered_map<std::string, std::shared_ptr<graphics::Texture>>;
using MatLogSet = std::unordered_set<unsigned int>;

static Entity CreateEntityFromMesh(
    aiMesh*             aiMesh,
    const aiScene*      aiScene,
    const std::string&  directory,
    std::shared_ptr<graphics::Shader> blinnPhongShader,
    std::shared_ptr<graphics::Shader> pbrShader,
    Entity              parent,
    std::vector<Entity>& outMeshes,
    TexCache&           texCache,
    MatLogSet&          loggedMats);

static void ProcessAssimpNode(
    aiNode*             node,
    const aiScene*      aiScene,
    const std::string&  directory,
    std::shared_ptr<graphics::Shader> blinnPhongShader,
    std::shared_ptr<graphics::Shader> pbrShader,
    Entity              parentEntity,
    std::vector<Entity>& outMeshes,
    TexCache&           texCache,
    MatLogSet&          loggedMats);

// -- aiMesh -> SGKit Mesh + Entity

static Entity CreateEntityFromMesh(
    aiMesh*             aiMesh,
    const aiScene*      aiScene,
    const std::string&  directory,
    std::shared_ptr<graphics::Shader> blinnPhongShader,
    std::shared_ptr<graphics::Shader> pbrShader,
    Entity              parent,
    std::vector<Entity>& outMeshes,
    TexCache&           texCache,
    MatLogSet&          loggedMats)
{
    if (!aiMesh->HasPositions() || !aiMesh->HasFaces())
        return Entity::Invalid;

    // a) Vertices: interleave [pos(3) normal(3) tex(2)] = 8 floats
    std::vector<float> verts;
    verts.reserve(static_cast<size_t>(aiMesh->mNumVertices * 8));
    bool hasTex = aiMesh->HasTextureCoords(0);
    bool hasNrm = aiMesh->HasNormals();

    for (unsigned int i = 0; i < aiMesh->mNumVertices; ++i)
    {
        verts.push_back(aiMesh->mVertices[i].x);
        verts.push_back(aiMesh->mVertices[i].y);
        verts.push_back(aiMesh->mVertices[i].z);

        if (hasNrm) {
            verts.push_back(aiMesh->mNormals[i].x);
            verts.push_back(aiMesh->mNormals[i].y);
            verts.push_back(aiMesh->mNormals[i].z);
        } else { verts.insert(verts.end(), {0, 1, 0}); }

        if (hasTex) {
            verts.push_back(aiMesh->mTextureCoords[0][i].x);
            verts.push_back(aiMesh->mTextureCoords[0][i].y);
        } else { verts.insert(verts.end(), {0, 0}); }
    }

    // b) Indices
    std::vector<uint32_t> idx;
    idx.reserve(static_cast<size_t>(aiMesh->mNumFaces * 3));
    for (unsigned int i = 0; i < aiMesh->mNumFaces; ++i)
    {
        const aiFace& f = aiMesh->mFaces[i];
        for (unsigned int j = 0; j < f.mNumIndices; ++j)
            idx.push_back(f.mIndices[j]);
    }

    // c) Graphics objects
    graphics::VertexLayout layout;
    layout.PushFloat(0, 3).PushFloat(1, 3).PushFloat(2, 2);

    auto vb = std::make_shared<graphics::VertexBuffer>();
    vb->Create(verts.data(), static_cast<size_t>(verts.size() * sizeof(float)));

    auto ib = std::make_shared<graphics::IndexBuffer>();
    ib->Create(idx.data(), static_cast<uint32_t>(idx.size()));

    auto va = std::make_shared<graphics::VertexArray>();
    va->Create();
    va->SetVertexBuffer(vb, layout);
    va->SetIndexBuffer(ib);

    // d) Detect lighting model from assimp material
    aiMaterial* aiMat = (aiMesh->mMaterialIndex >= 0)
        ? aiScene->mMaterials[aiMesh->mMaterialIndex] : nullptr;
    unsigned int matIdx = (aiMesh->mMaterialIndex >= 0)
        ? static_cast<unsigned int>(aiMesh->mMaterialIndex) : 0u;

    bool isPBR = false;
    if (aiMat)
    {
        // Primary: check shading model
        int shadingModel = 0;
        if (aiMat->Get(AI_MATKEY_SHADING_MODEL, shadingModel) == AI_SUCCESS)
            isPBR = (shadingModel == aiShadingMode_PBR_BRDF);

        // Fallback: glTF PBR metallic factor exists  ->  PBR workflow
        if (!isPBR)
        {
            float mf;
            if (aiMat->Get(AI_MATKEY_METALLIC_FACTOR, mf) == AI_SUCCESS)
                isPBR = true;
        }
    }

    // Select the correct shader for this sub-mesh
    std::shared_ptr<graphics::Shader> shader = isPBR ? pbrShader : blinnPhongShader;
    if (!shader)
    {
        // Fallback: if the preferred shader is missing, try the other one
        shader = isPBR ? blinnPhongShader : pbrShader;
        if (shader && loggedMats.insert(matIdx).second)
            SGK_LOG_WARN(
                "Model", "  mat[%u] is %s but no %s shader provided; falling back to other shader",
                matIdx, isPBR ? "PBR" : "BlinnPhong", isPBR ? "PBR" : "BlinnPhong");
    }
    if (!shader)
    {
        if (loggedMats.insert(matIdx).second)
            SGK_LOG_ERROR("Model", "  mat[%u] has no shader - skipping mesh", matIdx);
        return Entity::Invalid;
    }

    // e) Generic texture loader (single type, no fallback)
    auto loadTex = [&](aiTextureType type, int slot) -> std::shared_ptr<graphics::Texture>
    {
        if (!aiMat) return nullptr;
        aiString aiPath;
        if (aiMat->GetTexture(type, 0, &aiPath) != AI_SUCCESS || aiPath.length == 0)
            return nullptr;

        std::string key = aiPath.C_Str();
        auto it = texCache.find(key);
        if (it != texCache.end()) return it->second;

        auto tex = std::make_shared<graphics::Texture>(slot);

        if (!key.empty() && key[0] == '*')
        {
            int embIdx = std::stoi(key.substr(1));
            if (embIdx < 0 || embIdx >= (int)aiScene->mNumTextures) return nullptr;
            aiTexture* emb = aiScene->mTextures[embIdx];

            if (emb->mHeight > 0)  // uncompressed BGRA -> RGBA
            {
                std::vector<uint8_t> rgba(emb->mWidth * emb->mHeight * 4);
                for (unsigned int p = 0; p < emb->mWidth * emb->mHeight; ++p)
                {
                    auto* s = reinterpret_cast<const aiTexel*>(emb->pcData) + p;
                    rgba[static_cast<size_t>(p * 4 + 0)] = s->r;
                    rgba[static_cast<size_t>(p * 4 + 1)] = s->g;
                    rgba[static_cast<size_t>(p * 4 + 2)] = s->b;
                    rgba[static_cast<size_t>(p * 4 + 3)] = s->a;
                }
                tex->Create(emb->mWidth, emb->mHeight, rgba.data());
            }
            else  // compressed -> temp file
            {
                std::string ext = emb->achFormatHint;
                if (ext.empty()) ext = "png";
                std::string tmp = directory + "/_emb_" + std::to_string(embIdx) + "." + ext;
                auto* raw = reinterpret_cast<const uint8_t*>(emb->pcData);
                core::FileSystem::WriteBinary(tmp, {raw, raw + emb->mWidth});
                tex->LoadFromFile(tmp);
            }
        }
        else
        {
            if (!tex->LoadFromFile(directory + "/" + key))
                return nullptr;
        }

        texCache[key] = tex;
        return tex;
    };

    auto loadTexOr = [&](aiTextureType primary, aiTextureType fallback,
                         int slot) -> std::shared_ptr<graphics::Texture>
    {
        auto t = loadTex(primary, slot);
        return t ? t : loadTex(fallback, slot);
    };

    // f) Assemble material  (Blinn-Phong or PBR)
    auto mat = std::make_shared<Material>();
    mat->shader = shader;

    if (isPBR)
    {
        mat->lightingModel = LightingModel::PBR;
        mat->albedo    = loadTexOr(aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE, 0);
        mat->metallic  = loadTex(aiTextureType_METALNESS, 1);
        mat->roughness = loadTex(aiTextureType_DIFFUSE_ROUGHNESS, 2);
        mat->normalMap = loadTex(aiTextureType_NORMALS, 3);
        mat->ao        = loadTex(aiTextureType_AMBIENT_OCCLUSION, 4);
        mat->emissive  = loadTex(aiTextureType_EMISSIVE, 5);

        // glTF combined metallicRoughnessTexture detection:
        // assimp returns the same file for METALNESS + DIFFUSE_ROUGHNESS,
        // so the cache gives us the same shared_ptr for both.
        if (mat->metallic && mat->roughness && mat->metallic == mat->roughness)
            mat->pbrCombinedMetallicRoughness = true;

        // PBR factor values
        float f;
        if (aiMat->Get(AI_MATKEY_METALLIC_FACTOR, f) == AI_SUCCESS)
            mat->metallicFactor = f;
        if (aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, f) == AI_SUCCESS)
            mat->roughnessFactor = f;

        // Emissive factor
        aiColor3D emissiveCol(0.0f, 0.0f, 0.0f);
        if (aiMat->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveCol) == AI_SUCCESS)
            mat->emissiveFactor = { emissiveCol.r, emissiveCol.g, emissiveCol.b };

        // Normal scale & AO strength  (glTF textureInfo extensions)
        float ns = 1.0f;
        if (aiMat->Get(AI_MATKEY_GLTF_TEXTURE_STRENGTH(aiTextureType_NORMALS, 0), ns) == AI_SUCCESS)
            mat->normalScale = ns;
        float aos = 1.0f;
        if (aiMat->Get(AI_MATKEY_GLTF_TEXTURE_STRENGTH(aiTextureType_AMBIENT_OCCLUSION, 0), aos) == AI_SUCCESS)
            mat->aoStrength = aos;

        // Read glTF baseColorFactor for the albedo fallback colour
        aiColor4D baseColor(1.0f, 1.0f, 1.0f, 1.0f);
        if (aiMat)
            aiMat->Get(AI_MATKEY_BASE_COLOR, baseColor);

        // Transparency: read opacity and set blend/depth/cull for glass etc.
        {
            float opacity = 1.0f;
            if (aiMat && aiMat->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS
                && opacity < 1.0f)
            {
                mat->alphaFactor = opacity;
                mat->blendMode   = BlendMode::AlphaBlend;
                mat->depthMode   = DepthMode::ReadOnly;
                mat->cullMode    = CullMode::None;
            }

            // glTF alphaMode: MASK uses discard, BLEND uses alpha blending
            aiString alphaMode;
            if (aiMat && aiMat->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS)
            {
                if (strcmp(alphaMode.C_Str(), "MASK") == 0)
                {
                    float cutoff = 0.5f;
                    aiMat->Get(AI_MATKEY_GLTF_ALPHACUTOFF, cutoff);
                    mat->alphaCutoff = cutoff;
                    mat->cullMode    = CullMode::None;  // see both sides of mask
                }
            }

            mat->alphaFactor = baseColor.a;
        }

        // Fallback: default 1×1 textures for any missing PBR maps.
        // Without these, the shader's sampler would read from texture unit 0
        // (usually the albedo map), producing wildly wrong metallic/roughness/ao.
        {
            uint8_t black[]    = {0,   0,   0,   255};
            uint8_t white[]    = {255, 255, 255, 255};
            uint8_t flatNorm[] = {128, 128, 255, 255};  // (0,0,1) in tangent space

            if (!mat->albedo) {
                // baseColor is linear (glTF spec) -> sRGB-encode for upload
                // (shader will pow(2.2)-decode it back to linear)
                uint8_t c[4] = {
                    static_cast<uint8_t>(std::pow(baseColor.r, 1.0f/2.2f) * 255),
                    static_cast<uint8_t>(std::pow(baseColor.g, 1.0f/2.2f) * 255),
                    static_cast<uint8_t>(std::pow(baseColor.b, 1.0f/2.2f) * 255),
                    255
                };
                auto t = std::make_shared<graphics::Texture>(0);
                t->Create(1, 1, c);
                mat->albedo = t;
            }
            if (!mat->metallic) {
                auto t = std::make_shared<graphics::Texture>(1);
                t->Create(1, 1, black);
                mat->metallic = t;
            }
            if (!mat->roughness) {
                auto t = std::make_shared<graphics::Texture>(2);
                t->Create(1, 1, white);
                mat->roughness = t;
            }
            if (!mat->normalMap) {
                auto t = std::make_shared<graphics::Texture>(3);
                t->Create(1, 1, flatNorm);
                mat->normalMap = t;
            }
            if (!mat->ao) {
                auto t = std::make_shared<graphics::Texture>(4);
                t->Create(1, 1, white);
                mat->ao = t;
            }
            if (!mat->emissive) {
                auto t = std::make_shared<graphics::Texture>(5);
                t->Create(1, 1, white);  // white -> emissiveFactor controls colour
                mat->emissive = t;
            }
        }

        if (loggedMats.insert(matIdx).second)
            SGK_LOG_INFO(
                "Model", "  mat[%u] -> PBR (metallic=%.2f roughness=%.2f)",
                matIdx, mat->metallicFactor, mat->roughnessFactor);
    }
    else
    {
        mat->lightingModel = LightingModel::BlinnPhong;
        mat->diffuse   = loadTexOr(aiTextureType_DIFFUSE, aiTextureType_BASE_COLOR, 0);
        mat->specular  = loadTex(aiTextureType_SPECULAR, 1);

        float s = 32.0f;
        if (aiMat) aiMat->Get(AI_MATKEY_SHININESS, s);
        mat->shininess = s;

        if (loggedMats.insert(matIdx).second)
            SGK_LOG_INFO(
                "Model", "  mat[%u] -> BlinnPhong (shininess=%.0f diffuse=%s specular=%s)",
                matIdx, mat->shininess, mat->diffuse  ? "yes" : "no", mat->specular ? "yes" : "no");
    }

    // Honour the two-sided flag from the source asset (applies to both PBR and BlinnPhong)
    if (aiMat)
    {
        int twoSided = 0;
        if (aiMat->Get(AI_MATKEY_TWOSIDED, twoSided) == AI_SUCCESS && twoSided)
            mat->cullMode = CullMode::None;
    }

    auto mesh = std::make_shared<Mesh>();
    mesh->vertexArray = va;
    mesh->material    = mat;

    // g) Create entity
    auto& sm = Scene::instance();
    Entity e = sm.CreateEntity();
    auto* tf = sm.AddComponent<component::Transform>(e);
    tf->parent = parent;
    sm.GetComponent<component::Transform>(parent)->children.push_back(e);
    sm.AddComponent<component::MeshRenderer>(e)->mesh = mesh;

    outMeshes.push_back(e);
    return e;
}

// -- Recursive node walk
//
// Each assimp node has a local transform (node->mTransformation).  We create an
// intermediate entity for every child node so that sub-mesh offsets are preserved
// - otherwise all meshes would stack at the parent origin.
//
// The root node's transform is applied to the pre-created root entity in
// Model::Load() before calling this function.

static void ProcessAssimpNode(
    aiNode*              node,
    const aiScene*       aiScene,
    const std::string&   directory,
    std::shared_ptr<graphics::Shader> blinnPhongShader,
    std::shared_ptr<graphics::Shader> pbrShader,
    Entity               parentEntity,
    std::vector<Entity>& outMeshes,
    TexCache&            texCache,
    MatLogSet&           loggedMats)
{
    auto& sm = Scene::instance();

    // -- Meshes on this node: parent directly to this node's entity ----------
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
        CreateEntityFromMesh(
            aiScene->mMeshes[node->mMeshes[i]], aiScene, directory,
            blinnPhongShader, pbrShader, parentEntity, outMeshes, texCache, loggedMats);

    // -- Children: create an entity per child node, apply its transform ------
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        aiNode* child = node->mChildren[i];

        // Create intermediate entity for this child node
        Entity childEntity = sm.CreateEntity();
        auto* ct = sm.AddComponent<component::Transform>(childEntity);
        ct->parent = parentEntity;
        sm.GetComponent<component::Transform>(parentEntity)->children.push_back(childEntity);

        // Decompose assimp node transform -> position / rotation / scale
        aiVector3D  pos, scl;
        aiQuaternion rot;
        child->mTransformation.Decompose(scl, rot, pos);
        ct->position = { pos.x, pos.y, pos.z };
        ct->scale    = { scl.x, scl.y, scl.z };
        ct->rotation = math::Quaternion(rot.x, rot.y, rot.z, rot.w);

        ProcessAssimpNode(
            child, aiScene, directory, blinnPhongShader, pbrShader,
            childEntity, outMeshes, texCache, loggedMats);
    }
}

} // anonymous namespace

// -- Public API

Model::Result Model::Load( const std::string& filePath,
    std::shared_ptr<graphics::Shader> blinnPhongShader,
    std::shared_ptr<graphics::Shader> pbrShader)
{
    if (!blinnPhongShader && !pbrShader)
    {
        SGK_LOG_ERROR("Model", "At least one shader must be provided");
        return { Entity::Invalid, {} };
    }

    std::string ext = core::FileSystem::GetExtension(filePath);
    bool needFlip = (ext != "glb" && ext != "gltf");

    unsigned int flags
        = aiProcess_Triangulate
        | aiProcess_JoinIdenticalVertices
        | aiProcess_GenSmoothNormals;
    if (needFlip) flags |= aiProcess_FlipUVs;

    Assimp::Importer importer;
    const aiScene* aiScene = importer.ReadFile(filePath, flags);
    if (!aiScene || aiScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aiScene->mRootNode)
    {
        SGK_LOG_ERROR("Model", "Failed to load: %s", importer.GetErrorString());
        return { Entity::Invalid, {} };
    }

    std::string directory = filePath.substr(0, filePath.find_last_of("/\\"));

    auto& sm = Scene::instance();
    Entity root = sm.CreateEntity();
    auto* rootTf = sm.AddComponent<component::Transform>(root);

    // Apply root node transform
    {
        aiVector3D  pos, scl;
        aiQuaternion rot;
        aiScene->mRootNode->mTransformation.Decompose(scl, rot, pos);
        rootTf->position = { pos.x, pos.y, pos.z };
        rootTf->scale    = { scl.x, scl.y, scl.z };
        rootTf->rotation = math::Quaternion(rot.x, rot.y, rot.z, rot.w);
    }

    std::vector<Entity> meshEntities;
    TexCache texCache;
    MatLogSet loggedMats;
    ProcessAssimpNode(
        aiScene->mRootNode, aiScene, directory,
        blinnPhongShader, pbrShader,
        root, meshEntities, texCache, loggedMats);

    SGK_LOG_INFO("Model", "Loaded %s (%d meshes)", filePath.c_str(), (int)meshEntities.size());

    return { root, meshEntities };
}

}
}
