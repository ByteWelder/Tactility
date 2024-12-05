#pragma once

#include "service/ServiceManifest.h"

namespace tt::service {

typedef void (*ManifestCallback)(const ServiceManifest*, void* context);

void initRegistry();

void addService(const ServiceManifest* manifest);
void removeService(const ServiceManifest* manifest);

bool startService(const std::string& id);
bool stopService(const std::string& id);

const ServiceManifest* _Nullable findManifestId(const std::string& id);
ServiceContext* _Nullable findServiceById(const std::string& id);

} // namespace
