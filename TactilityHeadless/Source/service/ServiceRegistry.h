#pragma once

#include "Manifest.h"

namespace tt::service {

typedef void (*ManifestCallback)(const Manifest*, void* context);

void initRegistry();

void addService(const Manifest* manifest);
void removeService(const Manifest* manifest);

bool startService(const std::string& id);
bool stopService(const std::string& id);

const Manifest* _Nullable findManifestId(const std::string& id);
Service* _Nullable findServiceById(const std::string& id);

} // namespace
