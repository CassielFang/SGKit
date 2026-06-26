#pragma once

#include <cstdint>

namespace sgkit {
namespace graphics {

class Framebuffer
{
public:
    Framebuffer();
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&&) noexcept;
    Framebuffer& operator=(Framebuffer&&) noexcept;

    bool Create(int width, int height);
    void Destroy();
    bool IsValid() const { return m_fbo != 0; }

    void Bind() const;
    void Unbind() const;

    uint32_t GetDepthTexture() const { return m_depthTexture; }
    int GetWidth()  const { return m_width; }
    int GetHeight() const { return m_height; }

private:
    uint32_t m_fbo         = 0;
    uint32_t m_depthTexture = 0;
    int m_width  = 0;
    int m_height = 0;
};

} // namespace graphics
} // namespace sgkit
