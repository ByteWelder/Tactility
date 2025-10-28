#pragma once

#include <memory>
#include <Xpt2046Touch.h>

extern std::shared_ptr<Xpt2046Touch> touchInstance;

std::shared_ptr<Xpt2046Touch> createTouch();
