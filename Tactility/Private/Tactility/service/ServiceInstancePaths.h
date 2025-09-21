#pragma once

#include "Tactility/service/ServiceInstance.h"

namespace tt::service {

class ServiceInstancePaths final : public Paths {

    std::shared_ptr<const ServiceManifest> manifest;

public:

    explicit ServiceInstancePaths(std::shared_ptr<const ServiceManifest> manifest) : manifest(std::move(manifest)) {}
    ~ServiceInstancePaths() override = default;

    std::string getDataDirectory() const override;
    std::string getDataPath(const std::string& childPath) const override;
    std::string getSystemDirectory() const override;
    std::string getSystemPath(const std::string& childPath) const override;
};

}