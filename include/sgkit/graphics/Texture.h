#pragma once

#include <glad/glad.h>

#include <cstdint>
#include <string>

namespace sgkit {
namespace graphics {

class Texture
{
public:
    Texture();
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    bool LoadFromFile(const std::string& path);
    bool Create(int width, int height, const void* data = nullptr,
                uint32_t internalFormat = GL_RGBA8, uint32_t format = GL_RGBA);

    void Destroy();

    void Bind(int slot = 0) const;
    void Unbind() const;

    void SetFilterLinear(bool linear);
    void SetWrapRepeat(bool repeat);

    int      GetWidth() const    { return m_width; }
    int      GetHeight() const   { return m_height; }
    uint32_t GetHandle() const   { return m_handle; }
    bool     IsValid() const     { return m_handle != 0; }

private:
    uint32_t m_handle   = 0;
    int      m_width    = 0;
    int      m_height   = 0;
    int      m_channels = 0;

    bool LoadBMP(const std::string& path);
};

} // namespace graphics
} // namespace sgkit
