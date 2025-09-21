#pragma once

#include "Tactility/app/AppInstance.h"

namespace tt::app {

class AppInstancePaths final : public Paths {

    const AppManifest& manifest;

public:

    explicit AppInstancePaths(const AppManifest& manifest) : manifest(manifest) {}
    ~AppInstancePaths() override = default;

    std::string getDataDirectory() const override;
    std::string getDataPath(const std::string& childPath) const override;
    std::string getSystemDirectory() const override;
    std::string getSystemPath(const std::string& childPath) const override;
};

}