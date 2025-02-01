#pragma once

#include "Tactility/service/ServiceInstance.h"

namespace tt::service {

class ServiceInstancePaths final : public Paths {

private:

    std::shared_ptr<const ServiceManifest> manifest;

public:

    explicit ServiceInstancePaths(std::shared_ptr<const ServiceManifest> manifest) : manifest(std::move(manifest)) {}
    ~ServiceInstancePaths() final = default;

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