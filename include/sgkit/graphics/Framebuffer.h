#pragma once

#include <cstdint>

namespace sgkit {
namespace graphics {

class FrameBuffer
{
public:
    FrameBuffer();
    ~FrameBuffer();

    FrameBuffer(FrameBuffer&&) noexcept;
    FrameBuffer& operator=(FrameBuffer&&) noexcept;

    bool Create(int width, int height);
    bool CreateCubemap(int size);   // point-light shadow cubemap
    void Destroy();
    bool IsValid() const;

    void Bind() const;
    void Unbind() const;

    uint32_t GetDepthTexture() const;
    int GetWidth() const;
    int GetHeight() const;
    bool IsCubemap() const;

private:
    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;

    uint32_t m_fbo;
    uint32_t m_depthTexture;
    int m_width;
    int m_height;
    bool m_isCubemap = false;
};

}
}
