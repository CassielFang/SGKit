#pragma once

#include <cstdint>
#include <string>

namespace sgkit {
namespace graphics {

enum class TexInternalDataFormat
{
    Alpha,
    RGB, R3_G3_B2, RGB4, RGB5, RGB8, RGB10, RGB12, RGB16,
    RGBA, RGB5_A1, RGBA8, RGB10_A2, RGBA12, RGBA16
};
enum class TexDataFormat
{
    Alpha, RGB, RGBA
};

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
                TexInternalDataFormat internalFormat = TexInternalDataFormat::RGBA8,
                TexDataFormat format = TexDataFormat::RGBA);

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
