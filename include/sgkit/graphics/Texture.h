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
    Texture(uint32_t slot = 0u);
    ~Texture();

    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    bool LoadFromFile(const std::string& path);
    bool Create(int width, int height, const void* data,
                TexInternalDataFormat internalFormat = TexInternalDataFormat::RGBA8,
                TexDataFormat format = TexDataFormat::RGBA);

    void Destroy();

    void Bind() const;
    void Unbind() const;

    void SetSlot(uint32_t slot);
    void SetFilterLinear(bool linear) const;
    void SetWrapRepeat(bool repeat) const;

    int      GetWidth() const;
    int      GetHeight() const;
    uint32_t GetHandle() const;
    bool     IsValid() const;

private:
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    uint32_t m_handle;
    uint32_t m_slot;
    int      m_width;
    int      m_height;
    int      m_channels;

    static uint32_t g_currentHandle;
};

}
}
