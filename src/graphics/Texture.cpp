#include <sgkit/graphics/Texture.h>
#include <sgkit/core/FileSystem.h>

#include <glad/glad.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>

namespace sgkit {
namespace graphics {

Texture::Texture() = default;

Texture::~Texture()
{
    Destroy();
}

Texture::Texture(Texture&& other) noexcept
    : m_handle(other.m_handle), m_width(other.m_width),
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
    std::string ext = core::FileSystem::GetExtension(path);

    if (ext == "bmp")
        return LoadBMP(path);

    std::fprintf(stderr, "SGKit: Unsupported texture format: %s\n", ext.c_str());
    return false;
}

bool Texture::Create(int width, int height, const void* data,
                     TexInternalDataFormat internalFormat, TexDataFormat format)
{
    Destroy();

    GLenum internalFmt = 0;
    switch (internalFormat)
    {
    case TexInternalDataFormat::Alpha: internalFmt = GL_ALPHA; break;
    case TexInternalDataFormat::RGB: internalFmt = GL_RGB; break;
    case TexInternalDataFormat::R3_G3_B2: internalFmt = GL_R3_G3_B2; break;
    case TexInternalDataFormat::RGB4: internalFmt = GL_RGB4; break;
    case TexInternalDataFormat::RGB5: internalFmt = GL_RGB5; break;
    case TexInternalDataFormat::RGB8: internalFmt = GL_RGB8; break;
    case TexInternalDataFormat::RGB10: internalFmt = GL_RGB10; break;
    case TexInternalDataFormat::RGB12: internalFmt = GL_RGB12; break;
    case TexInternalDataFormat::RGB16: internalFmt = GL_RGB16; break;
    case TexInternalDataFormat::RGBA: internalFmt = GL_RGBA; break;
    case TexInternalDataFormat::RGB5_A1: internalFmt = GL_RGB5_A1; break;
    case TexInternalDataFormat::RGBA8: internalFmt = GL_RGBA8; break;
    case TexInternalDataFormat::RGB10_A2: internalFmt = GL_RGB10_A2; break;
    case TexInternalDataFormat::RGBA12: internalFmt = GL_RGBA12; break;
    case TexInternalDataFormat::RGBA16: internalFmt = GL_RGBA16; break;
    default: internalFmt = GL_RGBA8;
    }
    GLenum fmt = 0;
    switch (format)
    {
    case TexDataFormat::Alpha: fmt = GL_ALPHA; break;
    case TexDataFormat::RGB: fmt = GL_RGB; break;
    case TexDataFormat::RGBA: fmt = GL_RGBA; break;
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

void Texture::Bind(int slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_handle);
}

void Texture::Unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::SetFilterLinear(bool linear)
{
    if (!m_handle) return;
    GLint filter = linear ? GL_LINEAR : GL_NEAREST;
    GLint mipFilter = linear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST;

    Bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    Unbind();
}

void Texture::SetWrapRepeat(bool repeat)
{
    if (!m_handle) return;
    GLint wrap = repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;

    Bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    Unbind();
}

// ===================================================================
//  BMP Loader — minimal, uncompressed 24/32-bit BMP only
// ===================================================================

#pragma pack(push, 1)
struct BMPHeader
{
    uint16_t signature;   // 'BM'
    uint32_t fileSize;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t dataOffset;
};

struct BMPInfoHeader
{
    uint32_t size;
    int32_t  width;
    int32_t  height;
    uint16_t planes;
    uint16_t bitCount;
    uint32_t compression;
    uint32_t imageSize;
    int32_t  xPixelsPerMeter;
    int32_t  yPixelsPerMeter;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
};
#pragma pack(pop)

bool Texture::LoadBMP(const std::string& path)
{
    auto data = core::FileSystem::ReadBinary(path);
    if (!data)
    {
        std::fprintf(stderr, "SGKit: Failed to read BMP file: %s\n", path.c_str());
        return false;
    }

    if (data->size() < sizeof(BMPHeader) + sizeof(BMPInfoHeader))
    {
        std::fprintf(stderr, "SGKit: BMP file too small\n");
        return false;
    }

    const BMPHeader* header = reinterpret_cast<const BMPHeader*>(data->data());
    if (header->signature != 0x4D42)  // 'BM'
    {
        std::fprintf(stderr, "SGKit: Not a valid BMP file\n");
        return false;
    }

    const BMPInfoHeader* info = reinterpret_cast<const BMPInfoHeader*>(
        data->data() + sizeof(BMPHeader));

    if (info->compression != 0)
    {
        std::fprintf(stderr, "SGKit: Only uncompressed BMP is supported\n");
        return false;
    }

    int width    = info->width;
    int height   = std::abs(info->height);
    bool topDown = info->height < 0;
    int channels = info->bitCount / 8;

    if (channels != 3 && channels != 4)
    {
        std::fprintf(stderr, "SGKit: Only 24/32-bit BMP is supported\n");
        return false;
    }

    // BMP rows are 4-byte aligned
    int rowSize = (width * channels + 3) & ~3;
    const uint8_t* pixelData = data->data() + header->dataOffset;

    // Copy pixels (BMP is stored BGR->RGB, bottom-up by default)
    std::vector<uint8_t> pixels(width * height * channels);
    for (int y = 0; y < height; ++y)
    {
        // BMP is already bottom-up; OpenGL also expects bottom-up.  No flip needed.
        // Top-down BMPs (negative height in DIB header) get flipped once.
        int srcY = topDown ? (height - 1 - y) : y;
        const uint8_t* src = pixelData + srcY * rowSize;
        uint8_t* dst = pixels.data() + y * width * channels;

        for (int x = 0; x < width; ++x)
        {
            dst[x * channels + 0] = src[x * channels + 2];  // R
            dst[x * channels + 1] = src[x * channels + 1];  // G
            dst[x * channels + 2] = src[x * channels + 0];  // B
            if (channels == 4)
                dst[x * channels + 3] = src[x * channels + 3];  // A
        }
    }

    TexDataFormat format =
        (channels == 4) ? TexDataFormat::RGBA : TexDataFormat::RGB;
    TexInternalDataFormat internalFormat =
        (channels == 4) ? TexInternalDataFormat::RGBA8 : TexInternalDataFormat::RGB8;

    return Create(width, height, pixels.data(), internalFormat, format);
}

} // namespace graphics
} // namespace sgkit
