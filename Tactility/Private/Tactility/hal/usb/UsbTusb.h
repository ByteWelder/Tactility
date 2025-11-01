#pragma once

bool tusbIsSupported();
bool tusbStartMassStorageWithSdmmc();
bool tusbStartMassStorageWithFlash();
void tusbStop();
bool tusbCanStartMassStorageWithFlash();