#pragma once

#include <Axp2101.h>
#include <Aw9523.h>

extern std::shared_ptr<Axp2101> axp2101;
extern std::shared_ptr<Aw9523> aw9523;

bool initBoot();
