#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace sgkit {
namespace core {

class FileSystem
{
public:
    // -- Read -----------------------------------------------------
    static std::optional<std::string>          ReadText(const std::string& path);
    static std::optional<std::vector<uint8_t>> ReadBinary(const std::string& path);

    // -- Write ----------------------------------------------------
    static bool WriteText(const std::string& path, const std::string& content);
    static bool WriteBinary(const std::string& path, const std::vector<uint8_t>& data);

    // -- Query ----------------------------------------------------
    static bool        Exists(const std::string& path);
    static bool        IsDirectory(const std::string& path);
    static std::string GetDirectory(const std::string& path);
    static std::string GetExtension(const std::string& path);
    static std::string GetFilename(const std::string& path);
    static std::string GetFilenameWithoutExtension(const std::string& path);
};

} // namespace core
} // namespace sgkit
