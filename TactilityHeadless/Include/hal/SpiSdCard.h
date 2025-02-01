#ifdef ESP_PLATFORM

#pragma once

#include "hal/SdCard.h"

#include <sd_protocol_types.h>
#include <utility>
#include <vector>
#include <hal/spi_types.h>
#include <soc/gpio_num.h>

namespace tt::hal {

/**
 * SD card interface at the default SPI interface
 */
class SpiSdCard : public tt::hal::SdCard {
public:
    struct Config {
        Config(
            gpio_num_t spiPinCs,
            gpio_num_t spiPinCd,
            gpio_num_t spiPinWp,
            gpio_num_t spiPinInt,
            MountBehaviour mountBehaviourAtBoot,
            std::shared_ptr<Lockable> lockable = nullptr,
            std::vector<gpio_num_t> csPinWorkAround = std::vector<gpio_num_t>(),
            spi_host_device_t spiHost = SPI2_HOST,
            int spiFrequencyKhz = SDMMC_FREQ_DEFAULT
        ) : spiFrequencyKhz(spiFrequencyKhz),
            spiPinCs(spiPinCs),
            spiPinCd(spiPinCd),
            spiPinWp(spiPinWp),
            spiPinInt(spiPinInt),
            mountBehaviourAtBoot(mountBehaviourAtBoot),
            lockable(std::move(lockable)),
            csPinWorkAround(std::move(csPinWorkAround)),
            spiHost(spiHost)
        {}

        int spiFrequencyKhz;
        gpio_num_t spiPinCs; // Clock
        gpio_num_t spiPinCd; // Card detect
        gpio_num_t spiPinWp; // Write-protect
        gpio_num_t spiPinInt; // Interrupt
        SdCard::MountBehaviour mountBehaviourAtBoot;
        std::shared_ptr<Lockable> _Nullable lockable;
        std::vector<gpio_num_t> csPinWorkAround;
        spi_host_device_t spiHost;
        bool formatOnMountFailed = false;
        uint16_t maxOpenFiles = 4;
        uint16_t allocUnitSize = 16 * 1024;
        bool statusCheckEnabled = false;
    };

private:

    std::string mountPoint;
    sdmmc_card_t* card = nullptr;
    std::shared_ptr<Config> config;

    bool applyGpioWorkAround();
    bool mountInternal(const char* mount_point);

public:

    explicit SpiSdCard(std::unique_ptr<Config> config) :
        SdCard(config->mountBehaviourAtBoot),
        config(std::move(config))
    {}

    bool mount(const char* mountPath) override;
    bool unmount() override;
    State getState() const override;

    sdmmc_card_t* _Nullable getCard() { return card; }
};

}

#endif
