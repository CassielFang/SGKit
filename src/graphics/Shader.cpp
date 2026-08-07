#include <sgkit/graphics/Shader.h>

#include <sgkit/core/FileSystem.h>
#include <sgkit/framework/DebugOut.h>
#include <glad/glad.h>

namespace sgkit {
namespace graphics {

Shader::Shader() : m_programID(0) {}

Shader::~Shader()
{
    Release();
}

Shader::Shader(Shader&& other) noexcept
    : m_programID(other.m_programID)
    , m_uniformCache(std::move(other.m_uniformCache))
{
    other.m_programID = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other)
    {
        Release();
        m_programID    = other.m_programID;
        m_uniformCache = std::move(other.m_uniformCache);
        other.m_programID = 0;
    }
    return *this;
}

void Shader::Release()
{
    if (m_programID)
    {
        SGK_LOG_INFO("Shader", "Destroyed (program=%u)", m_programID);
        glDeleteProgram(m_programID);
        m_programID = 0;
    }
    m_uniformCache.clear();
}

uint32_t Shader::CompileShader(uint32_t type, const std::string& source)
{
    uint32_t id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(id, 512, nullptr, infoLog);
        const char* typeName = "unknown";
        switch (type)
        {
        case GL_VERTEX_SHADER:          typeName = "vertex";           break;
        case GL_FRAGMENT_SHADER:        typeName = "fragment";         break;
        case GL_TESS_CONTROL_SHADER:    typeName = "tess control";     break;
        case GL_TESS_EVALUATION_SHADER: typeName = "tess evaluation";  break;
        case GL_COMPUTE_SHADER:         typeName = "compute";          break;
        }
        SGK_LOG_ERROR("GLSL", "shader compilation failed (%s): %s", typeName, infoLog);
        glDeleteShader(id);
        return 0;
    }
    return id;
}

bool Shader::LoadFromFile(const std::string& vertexPath, const std::string& fragmentPath,
                           const std::string& geometryPath)
{
    auto vertSrc = core::FileSystem::ReadText(vertexPath);
    auto fragSrc = core::FileSystem::ReadText(fragmentPath);
    if (!vertSrc || !fragSrc) { SGK_LOG_ERROR("Shader", "failed to load shader files"); return false; }

    std::string geomSrc;
    if (!geometryPath.empty())
    {
        auto gs = core::FileSystem::ReadText(geometryPath);
        if (!gs) { SGK_LOG_ERROR("Shader", "failed to load geometry shader: %s", geometryPath.c_str()); return false; }
        geomSrc = *gs;
    }
    return LoadFromSource(*vertSrc, *fragSrc, geomSrc);
}

bool Shader::LoadFromSource(const std::string& vertexSource, const std::string& fragmentSource,
                             const std::string& geometrySource)
{
    Release();

    uint32_t vs = CompileShader(GL_VERTEX_SHADER, vertexSource);
    uint32_t fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
    uint32_t gs = 0;
    if (!geometrySource.empty())
        gs = CompileShader(GL_GEOMETRY_SHADER, geometrySource);

    if (!vs || !fs || (!geometrySource.empty() && !gs))
    {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        if (gs) glDeleteShader(gs);
        return false;
    }

    m_programID = glCreateProgram();
    glAttachShader(m_programID, vs);
    glAttachShader(m_programID, fs);
    if (gs) glAttachShader(m_programID, gs);
    glLinkProgram(m_programID);

    int success;
    glGetProgramiv(m_programID, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(m_programID, 512, nullptr, infoLog);
        SGK_LOG_ERROR("GLSL", "shader linking failed: %s", infoLog);
        glDeleteProgram(m_programID);
        m_programID = 0;
    }
#ifdef _DEBUG
    else
    {
        SGK_LOG_INFO("Shader", "Created (program=%u)", m_programID);
    }
#endif

    glDeleteShader(vs);
    glDeleteShader(fs);
    if (gs) glDeleteShader(gs);
    return success;
}

void Shader::Bind() const
{
    glUseProgram(m_programID);
}

void Shader::Unbind() const
{
    glUseProgram(0);
}

uint32_t Shader::GetHandle() const
{
    return m_programID;
}

bool Shader::IsValid() const
{
    return m_programID != 0;
}

int Shader::GetUniformLocation(const std::string& name)
{
    auto it = m_uniformCache.find(name);
    if (it != m_uniformCache.end())
        return it->second;

    int loc = glGetUniformLocation(m_programID, name.c_str());
    m_uniformCache[name] = loc;
    return loc;
}

void Shader::SetInt(const std::string& name, int value)
{
    glUniform1i(GetUniformLocation(name), value);
}

void Shader::SetFloat(const std::string& name, float value)
{
    glUniform1f(GetUniformLocation(name), value);
}

void Shader::SetVector2(const std::string& name, const math::Vector2& value)
{
    glUniform2f(GetUniformLocation(name), value.x, value.y);
}

void Shader::SetVector3(const std::string& name, const math::Vector3& value)
{
    glUniform3f(GetUniformLocation(name), value.x, value.y, value.z);
}

void Shader::SetVector4(const std::string& name, const math::Vector4& value)
{
    glUniform4f(GetUniformLocation(name), value.x, value.y, value.z, value.w);
}

void Shader::SetMatrix4(const std::string& name, const math::Matrix4& value)
{
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, value.Data());
}

}
}
