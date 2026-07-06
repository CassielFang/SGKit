#include <sgkit/graphics/Texture.h>

#include <sgkit/core/FileSystem.h>
#include <glad/glad.h>
#include <stb/stb_image.h>

#include <cstdio>

namespace sgkit {
namespace graphics {

Texture::Texture(int slot)
    : m_handle(0)
    , m_slot(slot)
    , m_width(0)
    , m_height(0)
    , m_channels(0) {}

Texture::~Texture()
{
    Destroy();
}

Texture::Texture(Texture&& other) noexcept
    : m_handle(other.m_handle), m_slot(other.m_slot), m_width(other.m_width),
      m_height(other.m_height), m_channels(other.m_channels)
{
    other.m_handle = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other)
    {
        Destroy();
        m_handle   = other.m_handle;
        m_slot     = other.m_slot;
        m_width    = other.m_width;
        m_height   = other.m_height;
        m_channels = other.m_channels;
        other.m_handle = 0;
    }
    return *this;
}

void Texture::Destroy()
{
    if (m_handle)
    {
        glDeleteTextures(1, &m_handle);
        m_handle = 0;
    }
}

bool Texture::LoadFromFile(const std::string& path)
{
    // Read the whole file into memory first (stbi can also load from file directly,
    // but going through FileSystem keeps asset-path resolution consistent).
    auto fileData = core::FileSystem::ReadBinary(path);
    if (!fileData)
    {
        std::fprintf(stderr, "SGKit: Failed to read texture: %s\n", path.c_str());
        return false;
    }

    // stb_image flips the image so that the first row is the bottom row
    // (OpenGL convention).  Must be called before every stbi_load* call.
    stbi_set_flip_vertically_on_load(true);

    int width, height, channels;
    unsigned char* pixels = stbi_load_from_memory(
        fileData->data(),
        static_cast<int>(fileData->size()),
        &width, &height, &channels,
        0);  // 0 = desired_channels -> keep original

    if (!pixels)
    {
        std::fprintf(stderr, "SGKit: stb_image failed: %s  (%s)\n",
                     stbi_failure_reason(), path.c_str());
        return false;
    }

    TexDataFormat format;
    TexInternalDataFormat internalFormat;
    switch (channels)
    {
    case 1:
        format         = TexDataFormat::Alpha;
        internalFormat = TexInternalDataFormat::Alpha;
        break;
    case 3:
        format         = TexDataFormat::RGB;
        internalFormat = TexInternalDataFormat::RGB8;
        break;
    case 4:
        format         = TexDataFormat::RGBA;
        internalFormat = TexInternalDataFormat::RGBA8;
        break;
    default:
        std::fprintf(stderr, "SGKit: Unexpected channel count %d: %s\n",
                     channels, path.c_str());
        stbi_image_free(pixels);
        return false;
    }

    bool ok = Create(width, height, pixels, internalFormat, format);
    stbi_image_free(pixels);

    if (ok)
    {
        m_channels = channels;
        std::printf("SGKit: Loaded texture  %s  (%dx%d, %d ch)\n",
                    path.c_str(), width, height, channels);
    }
    return ok;
}

bool Texture::Create(int width, int height, const void* data,
                     TexInternalDataFormat internalFormat, TexDataFormat format)
{
    Destroy();

    GLenum internalFmt = 0;
    switch (internalFormat)
    {
    case TexInternalDataFormat::Alpha:    internalFmt = GL_ALPHA;    break;
    case TexInternalDataFormat::RGB:      internalFmt = GL_RGB;      break;
    case TexInternalDataFormat::R3_G3_B2: internalFmt = GL_R3_G3_B2; break;
    case TexInternalDataFormat::RGB4:     internalFmt = GL_RGB4;     break;
    case TexInternalDataFormat::RGB5:     internalFmt = GL_RGB5;     break;
    case TexInternalDataFormat::RGB8:     internalFmt = GL_RGB8;     break;
    case TexInternalDataFormat::RGB10:    internalFmt = GL_RGB10;    break;
    case TexInternalDataFormat::RGB12:    internalFmt = GL_RGB12;    break;
    case TexInternalDataFormat::RGB16:    internalFmt = GL_RGB16;    break;
    case TexInternalDataFormat::RGBA:     internalFmt = GL_RGBA;     break;
    case TexInternalDataFormat::RGB5_A1:  internalFmt = GL_RGB5_A1;  break;
    case TexInternalDataFormat::RGBA8:    internalFmt = GL_RGBA8;    break;
    case TexInternalDataFormat::RGB10_A2: internalFmt = GL_RGB10_A2; break;
    case TexInternalDataFormat::RGBA12:   internalFmt = GL_RGBA12;   break;
    case TexInternalDataFormat::RGBA16:   internalFmt = GL_RGBA16;   break;
    default: internalFmt = GL_RGBA8;
    }

    GLenum fmt = 0;
    switch (format)
    {
    case TexDataFormat::Alpha: fmt = GL_ALPHA; break;
    case TexDataFormat::RGB:   fmt = GL_RGB;   break;
    case TexDataFormat::RGBA:  fmt = GL_RGBA;  break;
    default: fmt = GL_RGBA;
    }

    glGenTextures(1, &m_handle);
    glBindTexture(GL_TEXTURE_2D, m_handle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFmt),
                 width, height, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    m_width    = width;
    m_height   = height;
    m_channels = (fmt == GL_RGBA) ? 4 : ((fmt == GL_RGB) ? 3 : 1);

    return true;
}

void Texture::Bind() const
{
    glActiveTexture(GL_TEXTURE0 + m_slot);
    glBindTexture(GL_TEXTURE_2D, m_handle);
}

void Texture::Unbind() const
{
    glActiveTexture(GL_TEXTURE0 + m_slot);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::SetSlot(int slot)
{
    m_slot = slot;
}

void Texture::SetFilterLinear(bool linear) const
{
    if (!m_handle) return;
    GLint filter = linear ? GL_LINEAR : GL_NEAREST;
    GLint mipFilter = linear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST;

    Bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    //Unbind();
}

void Texture::SetWrapRepeat(bool repeat) const
{
    if (!m_handle) return;
    GLint wrap = repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;

    Bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    //Unbind();
}

int Texture::GetWidth() const
{
    return m_width;
}

int Texture::GetHeight() const
{
    return m_height;
}

int Texture::GetSlot() const
{
    return m_slot;
}

uint32_t Texture::GetHandle() const
{
    return m_handle;
}

bool Texture::IsValid() const {
    return m_handle != 0;
}

}
}
