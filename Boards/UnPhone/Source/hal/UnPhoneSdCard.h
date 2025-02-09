#pragma once

#include <Tactility/hal/sdcard/SdCard.h>

using tt::hal::sdcard::SdCard;

std::shared_ptr<SdCard> createUnPhoneSdCard();
