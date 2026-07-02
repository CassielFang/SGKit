#include <sgkit/core/FileSystem.h>

#include <fstream>
#include <filesystem>
#include <algorithm>

namespace sgkit {
namespace core {

namespace FileSystem {

// -- Read

std::optional<std::string> ReadText(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return std::nullopt;

    file.seekg(0, std::ios::end);
    size_t size = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    std::string content(size, '\0');
    file.read(content.data(), static_cast<std::streamsize>(size));

    if (!file)
        return std::nullopt;

    return content;
}

std::optional<std::vector<uint8_t>> ReadBinary(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return std::nullopt;

    file.seekg(0, std::ios::end);
    size_t size = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size));

    if (!file)
        return std::nullopt;

    return data;
}

// -- Write

bool WriteText(const std::string& path, const std::string& content)
{
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open())
        return false;

    file.write(content.data(), static_cast<std::streamsize>(content.size()));
    return file.good();
}

bool WriteBinary(const std::string& path, const std::vector<uint8_t>& data)
{
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open())
        return false;

    file.write(reinterpret_cast<const char*>(data.data()),
        static_cast<std::streamsize>(data.size()));
    return file.good();
}

// -- Query

bool Exists(const std::string& path)
{
    return std::filesystem::exists(path);
}

bool IsDirectory(const std::string& path)
{
    return std::filesystem::is_directory(path);
}

std::string GetDirectory(const std::string& path)
{
    return std::filesystem::path(path).parent_path().string();
}

std::string GetExtension(const std::string& path)
{
    std::string ext = std::filesystem::path(path).extension().string();
    // Remove leading dot
    if (!ext.empty() && ext[0] == '.')
        ext.erase(0, 1);
    // Normalize to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

std::string GetFilename(const std::string& path)
{
    return std::filesystem::path(path).filename().string();
}

std::string GetFilenameWithoutExtension(const std::string& path)
{
    return std::filesystem::path(path).stem().string();
}

}

}
}
