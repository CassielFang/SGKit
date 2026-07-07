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
    : m_fbo(other.m_fbo), m_depthTexture(other.m_depthTexture),
      m_width(other.m_width), m_height(other.m_height)
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D, m_depthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

#ifdef _DEBUG
    if (ok)
        SGK_LOG_INFO("FBO", "Created (fbo=%u, depthTex=%u, %dx%d)", m_fbo, m_depthTexture, m_width, m_height);
#endif

    if (!ok)
    {
        SGK_LOG_ERROR("FBO", "Failed to create (fbo=%u, %dx%d) -- framebuffer incomplete", m_fbo, m_width, m_height);
        Destroy();
    }

    return ok;
}

void FrameBuffer::Destroy()
{
    SGK_LOG_INFO("FBO", "Destroyed (fbo=%u, depthTex=%u, %dx%d)", m_fbo, m_depthTexture, m_width, m_height);
    if (m_depthTexture) { glDeleteTextures(1, &m_depthTexture); m_depthTexture = 0; }
    if (m_fbo)          { glDeleteFramebuffers(1, &m_fbo); m_fbo = 0; }
}

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
