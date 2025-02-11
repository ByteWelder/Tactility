#pragma once

#include <Tactility/RtosCompat.h>
#include <Tactility/Mutex.h>

#include <minmea.h>

#include <ranges>
#include <memory>

namespace tt::hal::gps {

/** Thread-safe storage of recent satellites */
class SatelliteStorage {

public:

    static constexpr size_t recordCount = 32;

private:

    struct SatelliteRecord {
        minmea_sat_info data {
            .nr = 0,
            .elevation = 0,
            .azimuth = 0,
            .snr = 0
        };
        TickType_t lastUpdated = 0;
        bool inUse = false;
    };

    Mutex mutex = Mutex(Mutex::Type::Recursive);
    std::array<SatelliteRecord, recordCount> records;
    uint16_t recycleTimeSeconds;
    uint16_t recentTimeSeconds;

    SatelliteRecord* findRecord(int number);

    SatelliteRecord* findUnusedRecord();

    SatelliteRecord* findRecordToRecycle();

    /** Tries to find an existing record, otherwise return a free one, otherwise return the oldest active one */
    SatelliteRecord* findWithFallback(int number);

public:

    explicit SatelliteStorage(
        uint16_t recycleTimeSeconds = 120,
        uint16_t recentTimeSeconds = 60
    ) : recycleTimeSeconds(recycleTimeSeconds), recentTimeSeconds(recentTimeSeconds) {}

    void notify(const minmea_sat_info& info);

    void getRecords(const std::function<void(const minmea_sat_info&)>& onRecord) const;
};

} // namespace tt::hal::gps
