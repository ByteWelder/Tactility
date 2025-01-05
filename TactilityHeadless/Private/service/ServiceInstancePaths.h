#pragma once

#include "service/ServiceInstance.h"

namespace tt::service {

class ServiceInstancePaths final : public Paths {

private:

    const ServiceManifest& manifest;

public:

    explicit ServiceInstancePaths(const ServiceManifest& manifest) : manifest(manifest) {}
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