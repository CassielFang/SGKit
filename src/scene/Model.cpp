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

#include <unordered_map>

namespace sgkit {
namespace scene {

// -- Internal helpers

namespace {

using TexCache = std::unordered_map<std::string, std::shared_ptr<graphics::Texture>>;

static void ProcessAssimpNode(
    aiNode*             node,
    const aiScene*      aiScene,
    const std::string&  directory,
    std::shared_ptr<graphics::Shader> shader,
    Entity              parentEntity,
    std::vector<Entity>& outMeshes,
    TexCache&           texCache);

// -- aiMesh -> SGKit Mesh + Entity

static Entity CreateEntityFromMesh(
    aiMesh*             aiMesh,
    const aiScene*      aiScene,
    const std::string&  directory,
    std::shared_ptr<graphics::Shader> shader,
    Entity              parent,
    std::vector<Entity>& outMeshes,
    TexCache&           texCache)
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

    // d) Textures (cached)
    auto tryGet = [](aiMaterial* mat, aiTextureType type, aiString& out) -> bool
    {
        if (mat->GetTexture(type, 0, &out) == AI_SUCCESS && out.length > 0) return true;
        if (type == aiTextureType_DIFFUSE
            && mat->GetTexture(aiTextureType_BASE_COLOR, 0, &out) == AI_SUCCESS
            && out.length > 0) return true;
        return false;
    };

    auto loadTex = [&](aiTextureType type, int slot) -> std::shared_ptr<graphics::Texture>
    {
        if (aiMesh->mMaterialIndex < 0) return nullptr;
        aiMaterial* mat = aiScene->mMaterials[aiMesh->mMaterialIndex];
        aiString aiPath;
        if (!tryGet(mat, type, aiPath)) return nullptr;

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

    // e) Assemble
    auto mat = std::make_shared<Material>();
    mat->shader    = shader;
    mat->diffuse   = loadTex(aiTextureType_DIFFUSE, 0);
    mat->specular  = loadTex(aiTextureType_SPECULAR, 1);
    mat->shininess = 32.0f;

    auto mesh = std::make_shared<Mesh>();
    mesh->vertexArray = va;
    mesh->material    = mat;

    // f) Create entity
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

static void ProcessAssimpNode(
    aiNode*             node,
    const aiScene*      aiScene,
    const std::string&  directory,
    std::shared_ptr<graphics::Shader> shader,
    Entity              parentEntity,
    std::vector<Entity>& outMeshes,
    TexCache&           texCache)
{
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
        CreateEntityFromMesh(aiScene->mMeshes[node->mMeshes[i]], aiScene, directory, shader, parentEntity, outMeshes, texCache);

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
        ProcessAssimpNode(node->mChildren[i], aiScene, directory, shader, parentEntity, outMeshes, texCache);
}

}

// -- Public API

Model::Result Model::Load(const std::string& filePath,
                           std::shared_ptr<graphics::Shader> shader)
{
    std::string ext = core::FileSystem::GetExtension(filePath);
    bool needFlip = (ext != "glb" && ext != "gltf");

    unsigned int flags = aiProcess_Triangulate
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
    sm.AddComponent<component::Transform>(root);

    std::vector<Entity> meshEntities;
    TexCache texCache;
    ProcessAssimpNode(aiScene->mRootNode, aiScene, directory, shader, root, meshEntities, texCache);

    SGK_LOG_INFO("Model", "Loaded %s (%d meshes)", filePath.c_str(), (int)meshEntities.size());

    return { root, meshEntities };
}

}
}
