#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace sgkit {
namespace core {

namespace FileSystem
{
    // -- Read
    std::optional<std::string>          ReadText(const std::string& path);
    std::optional<std::vector<uint8_t>> ReadBinary(const std::string& path);

    // -- Write
    bool WriteText(const std::string& path, const std::string& content);
    bool WriteBinary(const std::string& path, const std::vector<uint8_t>& data);

    // -- Query
    bool        Exists(const std::string& path);
    bool        IsDirectory(const std::string& path);
    std::string GetDirectory(const std::string& path);
    std::string GetExtension(const std::string& path);
    std::string GetFilename(const std::string& path);
    std::string GetFilenameWithoutExtension(const std::string& path);
}

}
}
