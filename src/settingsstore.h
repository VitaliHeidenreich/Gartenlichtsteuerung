#ifndef SETTINGSSTORE_H
#define SETTINGSSTORE_H
#pragma once

#include <Arduino.h>
#include "config.h"

// Alle dauerhaft zu speichernden Einstellungen (Felder nach Größe sortiert, damit kein Padding entsteht).
struct PersistentSettings
{
    uint32_t switchDurationMs[3];
    uint16_t unteresLimitSensor;
    uint16_t oberesLimitSensor;
    timeSet  onTime[3];
    timeSet  offTime[3];
    timeSet  monthOnTime[3][12];
    timeSet  monthOffTime[3][12];
    uint8_t  controlModeRelais[3];
    uint8_t  controlBySensorAllowed;
};

class SettingsStore
{
public:
    // Liest die Einstellungen aus dem NVS. Rückgabe false, wenn nichts oder Ungültiges gespeichert ist.
    static bool load(PersistentSettings &settings);
    // Schreibt die Einstellungen ins NVS, aber nur bei Änderung (schont den Flash).
    static bool save(const PersistentSettings &settings);
};

#endif /* SETTINGSSTORE_H */
