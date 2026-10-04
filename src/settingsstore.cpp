#include "settingsstore.h"
#include <Preferences.h>

static const char *NVS_NAMESPACE = "gartenlicht";
static const char *NVS_KEY = "settings";
static const char *NVS_KEY_VERSION = "version";
// Bei Änderung der Struktur erhöhen, dann werden alte Daten verworfen.
static const uint8_t SETTINGS_VERSION = 1;

bool SettingsStore::load(PersistentSettings &settings)
{
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true))
        return false;

    bool ok = prefs.getUChar(NVS_KEY_VERSION, 0) == SETTINGS_VERSION &&
              prefs.getBytesLength(NVS_KEY) == sizeof(PersistentSettings);
    if (ok)
        prefs.getBytes(NVS_KEY, &settings, sizeof(PersistentSettings));

    prefs.end();
    return ok;
}

bool SettingsStore::save(const PersistentSettings &settings)
{
    PersistentSettings stored;
    if (load(stored) && memcmp(&stored, &settings, sizeof(PersistentSettings)) == 0)
        return true;

    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false))
        return false;

    bool ok = prefs.putBytes(NVS_KEY, &settings, sizeof(PersistentSettings)) == sizeof(PersistentSettings);
    if (ok)
        prefs.putUChar(NVS_KEY_VERSION, SETTINGS_VERSION);

    prefs.end();
    return ok;
}
