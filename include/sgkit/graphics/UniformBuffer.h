#pragma once

#include <cstdint>
#include <cstddef>

namespace sgkit {
namespace graphics {

// OpenGL Uniform Buffer Object - upload once, share across shaders.
class UniformBuffer
{
public:
    UniformBuffer() = default;
    ~UniformBuffer();

    UniformBuffer(UniformBuffer&&) noexcept;
    UniformBuffer& operator=(UniformBuffer&&) noexcept;

    bool Create(size_t size, uint32_t bindingPoint);
    void Destroy();
    bool IsValid() const;

    void Bind() const;
    void Unbind() const;
    void Upload(const void* data, size_t size, size_t offset = 0) const;

    uint32_t GetHandle() const;

private:
    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;

    uint32_t m_handle = 0;
    uint32_t m_binding = 0;
    size_t   m_size = 0;
};

}
}
