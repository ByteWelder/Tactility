#pragma once

#include <Axp192.h>

bool initAxp();

// Must call initAxp() first before this returns a non-nullptr response
std::shared_ptr<Axp192> getAxp192();

