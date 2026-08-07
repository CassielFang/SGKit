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

    // Depth-only 2D shadow map
    bool Create(int width, int height);
    // HDR colour + depth  (hdr=true -> GL_RGBA16F, else GL_RGBA8)
    bool CreateColorDepth(int width, int height, bool hdr = false);
    // Depth-only cubemap shadow map
    bool CreateCubemap(int size);

    void Destroy();
    bool IsValid() const;

    void Bind() const;
    void Unbind() const;

    uint32_t GetColorTexture()  const;
    uint32_t GetDepthTexture()  const;
    int  GetWidth()  const;
    int  GetHeight() const;
    bool IsCubemap() const;
    bool HasColor()  const;

private:
    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;

    uint32_t m_fbo = 0;
    uint32_t m_colorTexture = 0;
    uint32_t m_depthTexture = 0;     // texture (2D or cubemap) or RBO
    int  m_width  = 0;
    int  m_height = 0;
    bool m_isCubemap   = false;
    bool m_hasColor    = false;
    bool m_depthIsRBO  = false;      // true when depth is a renderbuffer
};

}
}
