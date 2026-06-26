#include <sgkit/graphics/Framebuffer.h>

#include <glad/glad.h>

namespace sgkit {
namespace graphics {

Framebuffer::Framebuffer() = default;

Framebuffer::~Framebuffer()
{
    Destroy();
}

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : m_fbo(other.m_fbo), m_depthTexture(other.m_depthTexture),
      m_width(other.m_width), m_height(other.m_height)
{
    other.m_fbo = 0;
    other.m_depthTexture = 0;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept
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
    }
    return *this;
}

bool Framebuffer::Create(int width, int height)
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

    if (!ok)
        Destroy();

    return ok;
}

void Framebuffer::Destroy()
{
    if (m_depthTexture) { glDeleteTextures(1, &m_depthTexture); m_depthTexture = 0; }
    if (m_fbo)          { glDeleteFramebuffers(1, &m_fbo); m_fbo = 0; }
}

void Framebuffer::Bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
}

void Framebuffer::Unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace graphics
} // namespace sgkit
