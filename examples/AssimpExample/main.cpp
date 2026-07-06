#include <sgkit/sgkit.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>
#include <assimp/material.h>

#include <unordered_map>

using namespace sgkit;

// -- Global state

static scene::Entity                              s_camera;
static std::shared_ptr<graphics::Shader>          s_shader;
static std::shared_ptr<graphics::Shader>          s_simpleShader;
static std::vector<std::shared_ptr<scene::Mesh>>  s_meshes;
static std::unordered_map<std::string, std::shared_ptr<graphics::Texture>> s_texCache;

// -- Assimp helpers

static std::shared_ptr<scene::Mesh> ProcessMesh(
    aiMesh*             aiMesh,
    const aiScene*      scene,
    const std::string&  directory);

static void ProcessNode(
    aiNode*                     node,
    const aiScene*              scene,
    const std::string&          directory,
    std::vector<std::shared_ptr<scene::Mesh>>& outMeshes);

// -- LoadModel: entry point

static std::vector<std::shared_ptr<scene::Mesh>> LoadModel(const std::string& filePath)
{
    Assimp::Importer importer;
    const aiScene* aiScene = importer.ReadFile(filePath,
        aiProcess_Triangulate
        | aiProcess_JoinIdenticalVertices
        | aiProcess_GenSmoothNormals
        | aiProcess_FlipUVs);

    if (!aiScene || aiScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aiScene->mRootNode)
    {
        std::fprintf(stderr, "ASSIMP ERROR: %s\n", importer.GetErrorString());
        return {};
    }

    // Extract directory from file path for texture lookup
    std::string directory = filePath.substr(0, filePath.find_last_of("/\\"));

    std::vector<std::shared_ptr<scene::Mesh>> meshes;
    ProcessNode(aiScene->mRootNode, aiScene, directory, meshes);
    return meshes;
}

// -- ProcessNode: recursive scene-graph walk

static void ProcessNode(
    aiNode*                     node,
    const aiScene*              scene,
    const std::string&          directory,
    std::vector<std::shared_ptr<scene::Mesh>>& outMeshes)
{
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* aiMesh = scene->mMeshes[node->mMeshes[i]];
        auto sgMesh = ProcessMesh(aiMesh, scene, directory);
        if (sgMesh)
            outMeshes.push_back(sgMesh);
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
        ProcessNode(node->mChildren[i], scene, directory, outMeshes);
}

// -- ProcessMesh: aiMesh -> SGKit Mesh

static std::shared_ptr<scene::Mesh> ProcessMesh(
    aiMesh*             aiMesh,
    const aiScene*      scene,
    const std::string&  directory)
{
    if (!aiMesh->HasPositions() || !aiMesh->HasFaces())
        return nullptr;

    // -- a) Extract vertices --
    std::vector<float> vertices;
    vertices.reserve(aiMesh->mNumVertices * 8); // 3pos + 3nrm + 2tex

    bool hasTexCoords = aiMesh->HasTextureCoords(0);
    bool hasNormals   = aiMesh->HasNormals();

    for (unsigned int i = 0; i < aiMesh->mNumVertices; ++i)
    {
        // position
        vertices.push_back(aiMesh->mVertices[i].x);
        vertices.push_back(aiMesh->mVertices[i].y);
        vertices.push_back(aiMesh->mVertices[i].z);

        // normal
        if (hasNormals)
        {
            vertices.push_back(aiMesh->mNormals[i].x);
            vertices.push_back(aiMesh->mNormals[i].y);
            vertices.push_back(aiMesh->mNormals[i].z);
        }
        else
        {
            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
            vertices.push_back(0.0f);
        }

        // texcoord
        if (hasTexCoords)
        {
            vertices.push_back(aiMesh->mTextureCoords[0][i].x);
            vertices.push_back(aiMesh->mTextureCoords[0][i].y);
        }
        else
        {
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
    }

    // -- b) Extract indices
    std::vector<uint32_t> indices;
    indices.reserve(aiMesh->mNumFaces * 3);
    for (unsigned int i = 0; i < aiMesh->mNumFaces; ++i)
    {
        const aiFace& face = aiMesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
            indices.push_back(face.mIndices[j]);
    }

    // -- c) Build SGKit graphics objects
    graphics::VertexLayout layout;
    layout.PushFloat(0, 3).PushFloat(1, 3).PushFloat(2, 2);

    auto vb = std::make_shared<graphics::VertexBuffer>();
    vb->Create(vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(float)));

    auto ib = std::make_shared<graphics::IndexBuffer>();
    ib->Create(indices.data(), static_cast<uint32_t>(indices.size()));

    auto va = std::make_shared<graphics::VertexArray>();
    va->Create();
    va->SetVertexBuffer(vb, layout);
    va->SetIndexBuffer(ib);

    // -- d) Load material textures (cached)
    std::shared_ptr<graphics::Texture> diffuseTex;
    std::shared_ptr<graphics::Texture> specularTex;

    auto loadTex = [&](aiTextureType type, int slot) -> std::shared_ptr<graphics::Texture>
    {
        if (aiMesh->mMaterialIndex < 0) return nullptr;
        aiMaterial* mat = scene->mMaterials[aiMesh->mMaterialIndex];
        aiString aiPath;
        if (mat->GetTexture(type, 0, &aiPath) != AI_SUCCESS) return nullptr;

        std::string fullPath = directory + "/" + aiPath.C_Str();

        auto it = s_texCache.find(fullPath);
        if (it != s_texCache.end())
            return it->second;

        auto tex = std::make_shared<graphics::Texture>(slot);
        if (!tex->LoadFromFile(fullPath))
        {
            std::fprintf(stderr, "  Failed to load texture: %s\n", fullPath.c_str());
            return nullptr;
        }
        s_texCache[fullPath] = tex;
        return tex;
    };

    diffuseTex  = loadTex(aiTextureType_DIFFUSE, 0);
    specularTex = loadTex(aiTextureType_SPECULAR, 1);

    // -- e) Assemble SGKit Mesh
    auto material = std::make_shared<scene::Material>();
    material->shader    = s_shader;
    material->diffuse   = diffuseTex;
    material->specular  = specularTex;
    material->shininess = 32.0f;

    auto mesh = std::make_shared<scene::Mesh>();
    mesh->vertexArray = va;
    mesh->material    = material;
    return mesh;
}

// -- CreateApplication

ApplicationConfig sgkit::CreateApplication()
{
    ApplicationConfig cfg{};
    cfg.title  = "SGKit Assimp Example";
    cfg.width  = 1280;
    cfg.height = 720;
    cfg.fullscreen = true;
    cfg.fullscreenBolderless = true;

    cfg.onInit = [&]() -> bool
    {
        // 1. Load the shared Phong shader
        s_shader = std::make_shared<graphics::Shader>();
        if (!s_shader->LoadFromFile("assets/shaders/light.vert",
                                    "assets/shaders/light.frag"))
        {
            std::fprintf(stderr, "Failed to load shaders!\n");
            return false;
        }

        // 2. Load the 3D model via assimp
        s_meshes = LoadModel("assets/backpack/backpack.obj");
        if (s_meshes.empty())
        {
            std::fprintf(stderr, "Failed to load model!\n");
            return false;
        }

        // 3. Create entities for each mesh
        auto& sceneMgr = scene::Scene::instance();
        for (size_t i = 0; i < s_meshes.size(); ++i)
        {
            scene::Entity e = sceneMgr.CreateEntity();
            auto* tf = sceneMgr.AddComponent<scene::component::Transform>(e);
            tf->position = { 0.0f, 0.0f, 0.0f };
            tf->scale    = { 1.5f, 1.5f, 1.5f };
            sceneMgr.AddComponent<scene::component::MeshRenderer>(e)->mesh = s_meshes[i];
        }

        // 4. Create camera
        s_camera = sceneMgr.CreateEntity();
        auto* camTf = sceneMgr.AddComponent<scene::component::Transform>(s_camera);
        camTf->position = { 0.0f, 0.5f, 6.0f };
        sceneMgr.AddComponent<scene::component::Camera>(s_camera);

        // 5. Load simple shader for light markers
        s_simpleShader = std::make_shared<graphics::Shader>();
        s_simpleShader->LoadFromFile("assets/shaders/simple.vert",
                                     "assets/shaders/simple.frag");

        // 6. Create lights with visible markers
        {
            // -- Shared cube geometry for light markers
            constexpr float cubeVerts[] = {
                -0.5f,-0.5f, 0.5f, 0,0,1, 0,0,  0.5f,-0.5f, 0.5f, 0,0,1, 1,0,
                 0.5f, 0.5f, 0.5f, 0,0,1, 1,1, -0.5f, 0.5f, 0.5f, 0,0,1, 0,1,
                 0.5f,-0.5f,-0.5f, 0,0,-1,0,0, -0.5f,-0.5f,-0.5f, 0,0,-1,1,0,
                -0.5f, 0.5f,-0.5f, 0,0,-1,1,1,  0.5f, 0.5f,-0.5f, 0,0,-1,0,1,
                -0.5f, 0.5f, 0.5f, 0,1,0, 0,0,  0.5f, 0.5f, 0.5f, 0,1,0, 1,0,
                 0.5f, 0.5f,-0.5f, 0,1,0, 1,1, -0.5f, 0.5f,-0.5f, 0,1,0, 0,1,
                -0.5f,-0.5f,-0.5f, 0,-1,0,0,0,  0.5f,-0.5f,-0.5f, 0,-1,0,1,0,
                 0.5f,-0.5f, 0.5f, 0,-1,0,1,1, -0.5f,-0.5f, 0.5f, 0,-1,0,0,1,
                 0.5f,-0.5f, 0.5f, 1,0,0, 0,0,  0.5f,-0.5f,-0.5f, 1,0,0, 1,0,
                 0.5f, 0.5f,-0.5f, 1,0,0, 1,1,  0.5f, 0.5f, 0.5f, 1,0,0, 0,1,
                -0.5f,-0.5f,-0.5f,-1,0,0, 0,0, -0.5f,-0.5f, 0.5f,-1,0,0, 1,0,
                -0.5f, 0.5f, 0.5f,-1,0,0, 1,1, -0.5f, 0.5f,-0.5f,-1,0,0, 0,1,
            };
            constexpr uint32_t cubeIdx[] = {
                0,1,2, 2,3,0,  4,5,6, 6,7,4,  8,9,10, 10,11,8,
                12,13,14, 14,15,12,  16,17,18, 18,19,16,  20,21,22, 22,23,20,
            };

            auto makeLightCube = [&](math::Vector3 pos, math::Vector3 color) -> std::shared_ptr<scene::Mesh>
            {
                graphics::VertexLayout layout;
                layout.PushFloat(0, 3).PushFloat(1, 3).PushFloat(2, 2);
                auto vb = std::make_shared<graphics::VertexBuffer>();
                vb->Create(cubeVerts, sizeof(cubeVerts));
                auto ib = std::make_shared<graphics::IndexBuffer>();
                ib->Create(cubeIdx, sizeof(cubeIdx) / sizeof(uint32_t));
                auto va = std::make_shared<graphics::VertexArray>();
                va->Create();
                va->SetVertexBuffer(vb, layout);
                va->SetIndexBuffer(ib);

                auto mat = std::make_shared<scene::Material>();
                mat->shader    = s_simpleShader;
                mat->blendMode = scene::BlendMode::Additive;
                mat->depthMode = scene::DepthMode::ReadOnly;
                mat->cullMode  = scene::CullMode::None;

                auto mesh = std::make_shared<scene::Mesh>();
                mesh->vertexArray = va;
                mesh->material    = mat;
                return mesh;
            };

            // -- Narrow red SpotLight (laser-like)
            {
                auto& sm = scene::Scene::instance();
                scene::Entity e = sm.CreateEntity();
                auto* tf = sm.AddComponent<scene::component::Transform>(e);
                tf->position = { 0.0f, 2.0f, 3.0f };
                tf->scale    = { 0.3f, 0.3f, 0.3f };
                sm.AddComponent<scene::component::MeshRenderer>(e)->mesh = makeLightCube({}, {});

                auto* lt = sm.AddComponent<scene::component::Light>(e);
                lt->type       = scene::component::Light::Type::SpotLight;
                lt->direction  = math::Vector3{ 0.0f, -2.0f, -3.0f }.Normalized();
                lt->cutOff      = 0.985f;  // cos(~10) - tight beam
                lt->outerCutOff = 0.95f;   // cos(~18) - crisp edge
                lt->ambient     = { 0.0f, 0.0f, 0.0f };
                lt->diffuse     = { 3.0f, 3.0f, 3.0f };   // bright red
                lt->specular    = { 2.0f, 0.2f, 0.2f };
                lt->linear     = 0.027f;
                lt->quadratic  = 0.008f;
            }

            // -- Soft white Point light (fill)
            {
                auto& sm = scene::Scene::instance();
                scene::Entity e = sm.CreateEntity();
                auto* tf = sm.AddComponent<scene::component::Transform>(e);
                tf->position = { -4.0f, 2.0f, 3.0f };
                tf->scale    = { 0.12f, 0.12f, 0.12f };
                sm.AddComponent<scene::component::MeshRenderer>(e)->mesh = makeLightCube({}, {});

                auto* lt = sm.AddComponent<scene::component::Light>(e);
                lt->type      = scene::component::Light::Type::Point;
                lt->ambient   = { 0.3f, 0.3f, 0.3f };
                lt->diffuse   = { 2.0f, 2.0f, 2.0f };
                lt->specular  = { 1.0f, 1.0f, 1.0f };
                lt->linear    = 0.07f;
                lt->quadratic = 0.018f;
            }
        }

        return true;
    };

    cfg.onUpdate = [&]()
    {
        auto* cameraTransform = scene::Scene::instance()
            .GetComponent<scene::component::Transform>(s_camera);

        math::Vector3 forward = cameraTransform->rotation * math::Vector3::k_Forward;
        math::Vector3 right   = cameraTransform->rotation * math::Vector3::k_Right;
        float speed = 5.0f * framework::Clock::GetFrameDeltaSeconds();

        auto& in = core::Input::instance();
        if (in.IsKeyDown(core::KeyCode::W)) cameraTransform->position += forward * speed;
        if (in.IsKeyDown(core::KeyCode::S)) cameraTransform->position -= forward * speed;
        if (in.IsKeyDown(core::KeyCode::A)) cameraTransform->position -= right * speed;
        if (in.IsKeyDown(core::KeyCode::D)) cameraTransform->position += right * speed;
        if (in.IsKeyDown(core::KeyCode::Q)) cameraTransform->position.y -= speed;
        if (in.IsKeyDown(core::KeyCode::E)) cameraTransform->position.y += speed;

        if (in.IsMouseButtonDown(core::MouseButton::Left))
        {
            float s     = 0.002f;
            float yaw   = -in.GetMouseDeltaX() * s;
            float pitch = -in.GetMouseDeltaY() * s;
            cameraTransform->rotation
                = math::Quaternion::FromEulerAngles(0, yaw, 0)
                * cameraTransform->rotation
                * math::Quaternion::FromEulerAngles(pitch, 0, 0);
            cameraTransform->rotation.Normalize();
        }

        core::Window& window = core::Window::instance();
        if (in.IsKeyPressed(core::KeyCode::Space)) window.SetFullscreen(true);
        if (in.IsKeyPressed(core::KeyCode::Z))     window.SetFullscreen(false);
        if (in.IsKeyDown(core::KeyCode::V))        window.SetCursorVisible(false);
        if (in.IsKeyReleased(core::KeyCode::V))    window.SetCursorVisible(true);
    };

    cfg.onRender = [&]()
    {
        scene::Scene::instance().Render(s_camera);
    };

    cfg.onShutdown = [&]()
    {
        s_meshes.clear();
        s_texCache.clear();
        s_shader.reset();
        s_simpleShader.reset();
    };

    return cfg;
}
