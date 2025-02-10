#pragma once

#include "../Device.h"
#include "../uart/Uart.h"
#include "GpsInit.h"
#include "Satellites.h"

#include <Tactility/Mutex.h>
#include <Tactility/Thread.h>

#include <minmea.h>
#include <utility>


namespace tt::hal::gps {

class GpsDevice : public Device {

private:


public:

    struct Configuration {
        std::string name;
        uart_port_t uartPort;
        std::function<bool(uart_port_t)> initFunction = initGpsDefault;
    };

private:

    const Configuration configuration;
    Mutex mutex;
    std::unique_ptr<Thread> thread;
    bool threadInterrupted = false;
    SatelliteStorage satelliteStorage;

    static int32_t threadMainStatic(void* parameter);
    int32_t threadMain();

    bool isThreadInterrupted() const;

    TickType_t rmcTime = 0;
    minmea_sentence_rmc rmc;

    public:

    explicit GpsDevice(Configuration configuration) : configuration(std::move(configuration)) {
        assert(this->configuration.initFunction != nullptr);
    }

    ~GpsDevice() override = default;

    Type getType() const override { return Type::Gps; }

    std::string getName() const override { return configuration.name; }
    std::string getDescription() const override { return ""; }

    bool start();
    bool stop();

    bool isStarted() const;
};

}