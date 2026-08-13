#include <sgkit/graphics/FrameBuffer.h>

#include <sgkit/framework/DebugOut.h>
#include <glad/glad.h>

namespace sgkit {
namespace graphics {

FrameBuffer::FrameBuffer()
    : m_fbo(0)
    , m_depthTexture(0)
    , m_width(0)
    , m_height(0) {}

FrameBuffer::~FrameBuffer()
{
    Destroy();
}

FrameBuffer::FrameBuffer(FrameBuffer&& other) noexcept
    : m_fbo(other.m_fbo), m_depthTexture(other.m_depthTexture)
    , m_width(other.m_width), m_height(other.m_height)
{
    other.m_fbo = 0;
    other.m_depthTexture = 0;
    SGK_LOG_INFO("FBO", "Moved (fbo=%u, depthTex=%u, %dx%d)", m_fbo, m_depthTexture, m_width, m_height);
}

FrameBuffer& FrameBuffer::operator=(FrameBuffer&& other) noexcept
{
    if (this != &other)
    {
        Destroy();
        m_fbo = other.m_fbo;
        m_depthTexture = other.m_depthTexture;
        m_width = other.m_width;
        m_height = other.m_height;
        other.m_fbo = 0;
        other.m_depthTexture = 0;
        SGK_LOG_INFO("FBO", "Move-assigned (fbo=%u, depthTex=%u, %dx%d)", m_fbo, m_depthTexture, m_width, m_height);
    }
    return *this;
}

bool FrameBuffer::Create(int width, int height)
{
    Destroy();

    m_width  = width;
    m_height = height;

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // Depth texture
    glGenTextures(1, &m_depthTexture);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

#ifdef _DEBUG
    if (ok)
    {
        SGK_LOG_INFO("FBO", "Created (fbo=%u, depthTex=%u, %dx%d)", m_fbo, m_depthTexture, m_width, m_height);
    }
#endif

    if (!ok)
    {
        SGK_LOG_ERROR("FBO", "Failed to create (fbo=%u, %dx%d) -- framebuffer incomplete", m_fbo, m_width, m_height);
        Destroy();
    }

    return ok;
}

bool FrameBuffer::CreateColorDepth(int width, int height, bool hdr)
{
    Destroy();

    m_width    = width;
    m_height   = height;
    m_hasColor = true;

    GLenum colorFmt = hdr ? GL_RGBA16F : GL_RGBA8;

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // Color texture
    glGenTextures(1, &m_colorTexture);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, colorFmt, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);

    // Depth RBO
    m_depthIsRBO = true;
    glGenRenderbuffers(1, &m_depthTexture);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthTexture);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthTexture);

    bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (ok)
    {
        SGK_LOG_INFO("FBO", "Created color+depth (fbo=%u, color=%u, %dx%d %s)",
            m_fbo, m_colorTexture, width, height, hdr ? "HDR" : "LDR");
    }
    else {
        SGK_LOG_ERROR("FBO", "Failed color+depth create");
        Destroy();
    }
    return ok;
}

bool FrameBuffer::CreateCubemap(int size)
{
    Destroy();

    m_width     = size;
    m_height    = size;
    m_isCubemap = true;

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // Cubemap depth texture - 6 faces
    glGenTextures(1, &m_depthTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_depthTexture);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
            size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (ok)
    {
        SGK_LOG_INFO("FBO", "Created cubemap (fbo=%u, depthCube=%u, %dx%d)",
            m_fbo, m_depthTexture, m_width, m_height);
    }
    else
    {
        SGK_LOG_ERROR("FBO", "Failed cubemap create");
        Destroy();
    }
    return ok;
}

void FrameBuffer::Destroy()
{
    SGK_LOG_INFO("FBO", "Destroyed (fbo=%u, color=%u, depth=%u, %dx%d)",
        m_fbo, m_colorTexture, m_depthTexture, m_width, m_height);
    if (m_colorTexture)
    {
        glDeleteTextures(1, &m_colorTexture);
        m_colorTexture = 0;
    }
    if (m_depthTexture)
    {
        if (m_depthIsRBO)
        {
            glDeleteRenderbuffers(1, &m_depthTexture);
        }
        else
        {
            glDeleteTextures(1, &m_depthTexture);
        }
        m_depthTexture = 0;
    }
    if (m_fbo)
    {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    m_isCubemap  = false;
    m_hasColor   = false;
    m_depthIsRBO = false;
}

uint32_t FrameBuffer::GetColorTexture() const { return m_colorTexture; }
bool     FrameBuffer::IsCubemap()       const { return m_isCubemap; }
bool     FrameBuffer::HasColor()        const { return m_hasColor; }

bool FrameBuffer::IsValid() const
{
    return m_fbo != 0;
}

void FrameBuffer::Bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
}

void FrameBuffer::Unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

uint32_t FrameBuffer::GetDepthTexture() const
{
    return m_depthTexture;
}

int FrameBuffer::GetWidth()  const
{
    return m_width;
}

int FrameBuffer::GetHeight() const
{
    return m_height;
}

}
}
