#include "Drv2605.h"

bool Drv2605::init() {
    uint8_t status;
    if (!readRegister8(static_cast<uint8_t>(Register::Status), status)) {
        TT_LOG_E(TAG, "Failed to read status");
        return false;
    }
    status >>= 5;

    ChipId chip_id = static_cast<ChipId>(status);
    if (chip_id != ChipId::DRV2604 && chip_id != ChipId::DRV2604L && chip_id != ChipId::DRV2605 && chip_id != ChipId::DRV2605L) {
        TT_LOG_E(TAG, "Unknown chip id %02x", chip_id);
        return false;
    }

    writeRegister(Register::Mode, 0x00); // Get out of standby

    writeRegister(Register::RealtimePlaybackInput, 0x00); // Disable

    writeRegister(Register::WaveSequence1, 1); // Strong click
    writeRegister(Register::WaveSequence2, 0); // End sequence

    writeRegister(Register::OverdriveTimeOffset, 0); // No overdrive

    writeRegister(Register::SustainTimeOffsetPostivie, 0);
    writeRegister(Register::SustainTimeOffsetNegative, 0);
    writeRegister(Register::BrakeTimeOffset, 0);
    writeRegister(Register::AudioInputLevelMax, 0x64);

    // ERM open loop

    uint8_t feedback;
    if (!readRegister(Register::Feedback, feedback)) {
        TT_LOG_E(TAG, "Failed to read feedback");
        return false;
    }

    writeRegister(Register::Feedback, feedback & 0x7F); // N_ERM_LRA off

    bitOnByIndex(static_cast<uint8_t>(Register::Control3), 5); // ERM_OPEN_LOOP on

    return true;
}

void Drv2605::setWaveForm(uint8_t slot, uint8_t waveform) {
    writeRegister8(static_cast<uint8_t>(Register::WaveSequence1) + slot, waveform);
}

void Drv2605::selectLibrary(uint8_t library) {
    writeRegister(Register::WaveLibrarySelect, library);
}

void Drv2605::startPlayback() {
    writeRegister(Register::Go, 0x01);
}

void Drv2605::stopPlayback() {
    writeRegister(Register::Go, 0x00);
}

