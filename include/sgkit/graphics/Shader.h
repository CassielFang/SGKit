#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <unordered_map>

#include <sgkit/math/Matrix4.h>

namespace sgkit {
namespace graphics {

class Shader
{
public:
    Shader();
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    bool LoadFromFile(const std::string& vertexPath, const std::string& fragmentPath);
    bool LoadFromSource(const std::string& vertexSource, const std::string& fragmentSource);

    void Bind() const;
    void Unbind() const;

    uint32_t GetHandle() const { return m_programID; }
    bool IsValid() const { return m_programID != 0; }

    // -- Uniform helpers ------------------------------------------
    void SetInt(const std::string& name, int value);
    void SetFloat(const std::string& name, float value);
    void SetVector2(const std::string& name, const math::Vector2& value);
    void SetVector3(const std::string& name, const math::Vector3& value);
    void SetVector4(const std::string& name, const math::Vector4& value);
    void SetMatrix4(const std::string& name, const math::Matrix4& value);

private:
    uint32_t m_programID = 0;

    int GetUniformLocation(const std::string& name) const;
    mutable std::unordered_map<std::string, int> m_uniformCache;

    static uint32_t CompileShader(uint32_t type, const std::string& source);
    void Release();
};

} // namespace graphics
} // namespace sgkit
