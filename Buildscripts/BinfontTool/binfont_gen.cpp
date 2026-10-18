// SPDX-License-Identifier: Apache-2.0
// Host tool that rasterizes TrueType fonts into binfont files with the same generator as the firmware.
//
// Usage: binfont_gen <jobs-file> <output-folder>
// Each line of the jobs file has tab separated fields: name, TTF path, size, bpp and a codepoints file.
// The codepoints file has one decimal codepoint per line, "-" means all codepoints of the TTF.
// The result of a job is written to <output-folder>/<name>.bin.

#include <binfont/generator.h>

#include <tactility/memory.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Normally defined by the kernel's memory.cpp, which depends on more than this tool needs
extern "C" const struct MemoryPolicy MEMORY_POLICY_DEFAULT = { 0, 0, 0 };

namespace {

bool readCodepoints(const std::string& path, std::vector<uint32_t>& codepoints) {
    std::ifstream file(path);
    if (!file) {
        return false;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            codepoints.push_back(static_cast<uint32_t>(std::stoul(line)));
        }
    }
    return true;
}

bool runJob(const std::string& line, const std::string& outputFolder) {
    std::istringstream fields(line);
    std::string name, ttfPath, size, bpp, codepointsPath;
    if (!std::getline(fields, name, '\t') || !std::getline(fields, ttfPath, '\t') || !std::getline(fields, size, '\t') ||
        !std::getline(fields, bpp, '\t') || !std::getline(fields, codepointsPath, '\t')) {
        fprintf(stderr, "Invalid job: %s\n", line.c_str());
        return false;
    }

    std::vector<uint32_t> codepoints;
    if (codepointsPath != "-" && !readCodepoints(codepointsPath, codepoints)) {
        fprintf(stderr, "Failed to read %s\n", codepointsPath.c_str());
        return false;
    }

    const BinFontGeneratorConfig config = {
        .ttf_path = ttfPath.c_str(),
        .ttf_data = nullptr,
        .ttf_size = 0,
        .size = static_cast<uint16_t>(std::stoul(size)),
        .bpp = static_cast<uint8_t>(std::stoul(bpp)),
        .codepoints = codepoints.empty() ? nullptr : codepoints.data(),
        .codepoint_count = codepoints.size(),
    };

    uint8_t* data = nullptr;
    size_t dataSize = 0;
    const error_t error = binfont_generate(&config, &data, &dataSize);
    if (error != ERROR_NONE) {
        fprintf(stderr, "Failed to generate %s (error %d)\n", name.c_str(), static_cast<int>(error));
        return false;
    }

    const std::string outputPath = outputFolder + "/" + name + ".bin";
    FILE* output = fopen(outputPath.c_str(), "wb");
    const bool written = output != nullptr && fwrite(data, 1, dataSize, output) == dataSize;
    if (output != nullptr) {
        fclose(output);
    }
    memory_free(data);
    if (!written) {
        fprintf(stderr, "Failed to write %s\n", outputPath.c_str());
    }
    return written;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <jobs-file> <output-folder>\n", argv[0]);
        return 1;
    }
    std::ifstream jobs(argv[1]);
    if (!jobs) {
        fprintf(stderr, "Failed to read %s\n", argv[1]);
        return 1;
    }
    std::string line;
    while (std::getline(jobs, line)) {
        if (!line.empty() && !runJob(line, argv[2])) {
            return 1;
        }
    }
    return 0;
}
