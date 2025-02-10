#include "Tactility/hal/gps/Satellites.h"

#include <algorithm>

#define TAG "satellites"

namespace tt::hal::gps {

constexpr inline bool isExpired(TickType_t now, TickType_t timeToCheck, TickType_t expireTimeInTicks) {
    return (expireTimeInTicks < now) && ((now - expireTimeInTicks) > timeToCheck);
}

SatelliteStorage::SatelliteRecord* SatelliteStorage::findRecord(int number) {
    auto result = records | std::views::filter([number](auto& record) {
        return record.inUse && record.data.nr == number;
    });

    if (!result.empty()) {
        return &result.front();
    } else {
        return nullptr;
    }
}

SatelliteStorage::SatelliteRecord* SatelliteStorage::findUnusedRecord() {
    auto lockable = mutex.scoped();
    lockable->lock();

    auto result = records | std::views::filter([](auto& record) {
        return !record.inUse;
    });

    if (!result.empty()) {
        auto* record = &result.front();
        record->inUse = true;
        TT_LOG_D(TAG, "Found unused record");
        return record;
    } else {
        return nullptr;
    }
}

SatelliteStorage::SatelliteRecord* SatelliteStorage::findRecordToRecycle() {
    auto lockable = mutex.scoped();
    lockable->lock();

    int candidate_index = -1;
    auto candidate_age = portMAX_DELAY;
    TickType_t expire_duration = kernel::secondsToTicks(recycleTimeSeconds);
    TickType_t now = kernel::getTicks();
    for (int i = 0; i < records.size(); ++i) {
        // First try to find a record that is "old enough"
        if (isExpired(now, records[i].lastUpdated, expire_duration)) {
            TT_LOG_D(TAG, "! [%d] %lu < %lu", i, records[i].lastUpdated, expire_duration);
            candidate_index = i;
            break;
        }

        // Otherwise keep finding the oldest record
        if (records[i].inUse && records[i].lastUpdated < candidate_age) {
            candidate_index = i;
            candidate_age = records[i].lastUpdated;
            TT_LOG_D(TAG, "? [%d] %lu < %lu", i, records[i].lastUpdated, candidate_age);
        }
    }

    assert(candidate_index != -1);

    TT_LOG_D(TAG, "Recycled record %d", candidate_index);

    return &records[candidate_index];
}

SatelliteStorage::SatelliteRecord* SatelliteStorage::findWithFallback(int number) {
    auto lockable = mutex.scoped();
    lockable->lock();

    if (auto* found_record = findRecord(number)) {
        return found_record;
    } else if (auto* unused_record = findUnusedRecord()) {
        return unused_record;
    } else {
        return findRecordToRecycle();
    }
}

void SatelliteStorage::notify(const minmea_sat_info& data) {
    auto lockable = mutex.scoped();
    lockable->lock();

    auto* record = findWithFallback(data.nr);
    if (record != nullptr) {
        record->inUse = true;
        record->lastUpdated = kernel::getTicks();
        record->data = data;
        TT_LOG_D(TAG, "Updated satellite %d: elevation %d, azimuth %d, snr %d", record->data.nr, record->data.elevation, record->data.elevation, record->data.snr);
    }
}

void SatelliteStorage::getRecords(const std::function<void(const minmea_sat_info&)>& onRecord) const {
    auto lockable = mutex.scoped();
    lockable->lock();

    TickType_t expire_duration = kernel::secondsToTicks(recentTimeSeconds);
    TickType_t now = kernel::getTicks();

    for (auto& record: records) {
        if (record.inUse && !isExpired(now, record.lastUpdated, expire_duration)) {
            onRecord(record.data);
        }
    }
}

} // namespace tt::hal::gps
