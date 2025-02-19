#pragma once

#include <memory>
#include <Xpt2046Touch.h>

std::shared_ptr<Xpt2046Touch> createTouch();
