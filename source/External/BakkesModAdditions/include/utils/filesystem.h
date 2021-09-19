#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>


static inline bool has_extension(const std::string& fileExtension, const std::vector<std::string>& extensions)
{
    // Filter out unwanted file extensions.
    return std::ranges::any_of(extensions, [&](const std::string& extension) {
        return fileExtension == extension;
    });
}


static inline std::vector<std::filesystem::path> iterate_directory(const std::filesystem::path& directory,
    const std::vector<std::string>& extensions, const int depth = 0, const int maxDepth = 3)
{
    if (depth > maxDepth) {
        return std::vector<std::filesystem::path>();
    }

    std::vector<std::filesystem::path> files;
    for (const std::filesystem::directory_entry& file : std::filesystem::directory_iterator(directory)) {
        const std::filesystem::path& filePath = file.path();
        if (file.is_directory()) {
            std::vector<std::filesystem::path> directoryFiles = iterate_directory(
                filePath, extensions, depth + 1, maxDepth);
            // Remove if directory is empty.
            if (!directoryFiles.empty()) {
                files.insert(files.end(), directoryFiles.begin(), directoryFiles.end());
            }
        }
        else if (has_extension(filePath.extension().string(), extensions)) {
            files.push_back(filePath);
        }
    }

    return files;
}


template<class... TArgs, typename = std::enable_if_t<std::conjunction_v<std::is_convertible<TArgs, std::string>...>>>
static inline std::vector<std::filesystem::path> get_files_from_dir(const std::filesystem::path& directory, const int maxDepth, const TArgs... extension)
{
    if (!exists(directory)) {
        return std::vector<std::filesystem::path>();
    }

    const std::vector<std::string> fileExtensions = { extension... };

    return iterate_directory(directory, fileExtensions, 0, maxDepth);
}
