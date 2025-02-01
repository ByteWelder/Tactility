#pragma once

#include "Tactility/app/AppInstance.h"

namespace tt::app {

class AppInstancePaths final : public Paths {

private:

    const AppManifest& manifest;

public:

    explicit AppInstancePaths(const AppManifest& manifest) : manifest(manifest) {}
    ~AppInstancePaths() final = default;

    std::string getDataDirectory() const final;
    std::string getDataDirectoryLvgl() const final;
    std::string getDataPath(const std::string& childPath) const final;
    std::string getDataPathLvgl(const std::string& childPath) const final;
    std::string getSystemDirectory() const final;
    std::string getSystemDirectoryLvgl() const final;
    std::string getSystemPath(const std::string& childPath) const final;
    std::string getSystemPathLvgl(const std::string& childPath) const final;
};

}