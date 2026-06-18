#include <sgkit/graphics/Shader.h>
#include <sgkit/core/FileSystem.h>

#include <glad/glad.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

namespace sgkit {
namespace graphics {

Shader::Shader() = default;

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
        std::fprintf(stderr, "SGKit: Shader compilation failed (%s):\n%s\n",
                     (type == GL_VERTEX_SHADER) ? "vertex" : "fragment", infoLog);
        glDeleteShader(id);
        return 0;
    }
    return id;
}

bool Shader::LoadFromFile(const std::string& vertexPath, const std::string& fragmentPath)
{
    auto vertSrc = core::FileSystem::ReadText(vertexPath);
    auto fragSrc = core::FileSystem::ReadText(fragmentPath);

    if (!vertSrc || !fragSrc)
    {
        std::fprintf(stderr, "SGKit: Failed to load shader files\n");
        return false;
    }
    return LoadFromSource(*vertSrc, *fragSrc);
}

bool Shader::LoadFromSource(const std::string& vertexSource, const std::string& fragmentSource)
{
    Release();

    uint32_t vs = CompileShader(GL_VERTEX_SHADER, vertexSource);
    uint32_t fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

    if (!vs || !fs)
    {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return false;
    }

    m_programID = glCreateProgram();
    glAttachShader(m_programID, vs);
    glAttachShader(m_programID, fs);
    glLinkProgram(m_programID);

    int success;
    glGetProgramiv(m_programID, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(m_programID, 512, nullptr, infoLog);
        std::fprintf(stderr, "SGKit: Shader linking failed:\n%s\n", infoLog);
        glDeleteProgram(m_programID);
        m_programID = 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
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

int Shader::GetUniformLocation(const std::string& name) const
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

} // namespace graphics
} // namespace sgkit
