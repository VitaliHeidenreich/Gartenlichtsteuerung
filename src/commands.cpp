#include "Commands.h"
#include "config.h"
#include "Zeitmaster.h"
#include "mypins.h"
#include "settingsstore.h"
#include "BluetoothSerial.h"

extern BluetoothSerial SerialBT;

Zeitmaster *_interpreterzeitmaster;
mypins *IO;

timeSet Commands::onTime[3] = {{21,00}, {21,00}, {21,00}};
timeSet Commands::offTime[3] = {{3,0}, {3,0}, {3,0}};
uint8_t Commands::controlModeRelais[3] = {CONTROL_MODE_MONTH_TIME, CONTROL_MODE_MONTH_TIME, CONTROL_MODE_MONTH_TIME};
timeSet Commands::monthOnTime[3][12];
timeSet Commands::monthOffTime[3][12];
uint32_t Commands::switchPulseUntil[3] = {0, 0, 0};
uint8_t Commands::lastSwitchState[3] = {HIGH, HIGH, HIGH};
uint32_t Commands::switchDurationMs[3] = {600000UL, 600000UL, 600000UL}; // 10 Minuten
uint16_t Commands::unteresLimitSensor = 800;
uint16_t Commands::oberesLimitSensor = 2200;
uint8_t Commands::controlBySensorAllowed = 0;

Commands::Commands()
{
    _interpreterzeitmaster = nullptr;
    for (uint8_t r = 0; r < 3; r++)
        for (uint8_t m = 0; m < 12; m++)
        {
            monthOnTime[r][m] = onTime[r];
            monthOffTime[r][m] = offTime[r];
        }
}

void Commands::setZeitmaster(Zeitmaster *zeitmaster)
{
    _interpreterzeitmaster = zeitmaster;
}

void Commands::setIO(mypins *io)
{
    IO = io;
}

void Commands::loadSettings()
{
    PersistentSettings s;
    if (!SettingsStore::load(s))
        return;

    memcpy(switchDurationMs, s.switchDurationMs, sizeof(switchDurationMs));
    memcpy(onTime, s.onTime, sizeof(onTime));
    memcpy(offTime, s.offTime, sizeof(offTime));
    memcpy(monthOnTime, s.monthOnTime, sizeof(monthOnTime));
    memcpy(monthOffTime, s.monthOffTime, sizeof(monthOffTime));
    memcpy(controlModeRelais, s.controlModeRelais, sizeof(controlModeRelais));
    unteresLimitSensor = s.unteresLimitSensor;
    oberesLimitSensor = s.oberesLimitSensor;
    controlBySensorAllowed = s.controlBySensorAllowed;
}

void Commands::saveSettings()
{
    PersistentSettings s;
    memset(&s, 0, sizeof(s));
    memcpy(s.switchDurationMs, switchDurationMs, sizeof(switchDurationMs));
    memcpy(s.onTime, onTime, sizeof(onTime));
    memcpy(s.offTime, offTime, sizeof(offTime));
    memcpy(s.monthOnTime, monthOnTime, sizeof(monthOnTime));
    memcpy(s.monthOffTime, monthOffTime, sizeof(monthOffTime));
    memcpy(s.controlModeRelais, controlModeRelais, sizeof(controlModeRelais));
    s.unteresLimitSensor = unteresLimitSensor;
    s.oberesLimitSensor = oberesLimitSensor;
    s.controlBySensorAllowed = controlBySensorAllowed;
    SettingsStore::save(s);
}

/** =========================================================================
 *  | Command structure:                                                    |
 *  |-----------------+--------------+--------------------+-----------------|
 *  | Startsign       | Command type | Command parameters | Endsign         |
 *  |-----------------+-----------------------------------+-----------------+
 *  | 'X'             | '#'          | X5 X4 X3 X2 X1 X0  | '$\n' oder '$\t |
 *  =========================================================================
 *  
 *  Command list follows below (#)
 *  T Set time on DS3231 -- communicate by I2C
 *  I Show info (Debug) -- Rend to Serial BT or UART
 *  N Set on time -- For all 3 relais output on the board
 *  F Set off time -- For all 3 relais output on the board
 *  B/C/D Set on time for relais 1/2/3
 *  E/G/H Set off time for relais 1/2/3
 *  J/K/L Set switch impulse duration for relais 1/2/3 in seconds
 *  M Set control mode: first 3 parameters (relais 1..3), 0=switch (10 min), 1=always on, 2=time range per month
 *  P/Q/R Set on time per month for relais 1/2/3: MMHHMM (MM = month 01..12)
 *  S/V/W Set off time per month for relais 1/2/3: MMHHMM (MM = month 01..12)
 *  O Set upper sensor limit -- 12V Batterie voltage, above that voltage the light can be activated (oberesLimitSensor)
 *  U Set lower sensor limit -- if the 12V Batterie voltage drops below this value (unteresLimitSensor), the light can#t be activated until the voltage rises above the limit defined in oberesLimitSensor
 *  A Allow control by sensor -- default should be disabled
 * 
 *  X5, X4, X3, X2, X1 and X0 represent the command parameters transmitted from the app
 * 
 * Zusammenfügen der einzelnen übertragenen char aus der App in Befehlsbuffer
 * Übergabeparameter: char aus der seriellen BT Übertragung oder per UART
 * Rückgabe: uint8_t zur Anzeige, ob Befehl aktiv war oder nicht
**************************************************************************
*/
uint8_t Commands::readCommandCharFromSerial(char CommandChar)
{
    // static Variablen müssen initialisiert werden
    static char _AppBefehlBuffer[] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}; //10 Elemente
    char _AppBefehl[] = {'0', '0', '0', '0', '0', '0', '0'};                             // 7 Elemente
    uint8_t iRet = 0;
    uint8_t i;

    //Befüllung des Ringbuffers (kopieren von vorne nach hinten, beginnend am Ende)
    for (i = (10 - 1); i > 0; i--)
    {
        _AppBefehlBuffer[i] = _AppBefehlBuffer[i - 1];
    }

    // Neues Zeichen in den Buffer[0] schieben
    _AppBefehlBuffer[0] = CommandChar;

    // Prüfe, ob ein befehl anliegt
    if ((_AppBefehlBuffer[9] == 'X') && (_AppBefehlBuffer[1] == '$') && ((_AppBefehlBuffer[0] == '\n') ))
    {
        // command found
        iRet = 1;

        //Erstellung lokale Kopie für Befehle inkl Drehung der Orientierung
        for (i = 7; i > 1; i--)
        {
            _AppBefehl[7 - i] = _AppBefehlBuffer[i];
        }

        switch (_AppBefehlBuffer[8])
        {
            // T: Uhrzeit der DS3231-Echtzeituhr einstellen.
            case 'T':
                CommandSetTime( _AppBefehl );
                break;
            
            // I: Aktuelle Zeit, Schaltzeiten und Sensorwerte ausgeben.
            case 'I':
                showInfo( );
                break;

            // N: Einschaltzeit für alle drei Relais einstellen.
            case 'N':
                CommandSetOnTime( _AppBefehl );
                break;

            // F: Ausschaltzeit für alle drei Relais einstellen.
            case 'F':
                CommandSetOffTime( _AppBefehl );
                break;

            // B: Einschaltzeit für Relais 1 einstellen.
            case 'B':
                CommandSetOnTime( 0, _AppBefehl );
                break;

            // C: Einschaltzeit für Relais 2 einstellen.
            case 'C':
                CommandSetOnTime( 1, _AppBefehl );
                break;

            // D: Einschaltzeit für Relais 3 einstellen.
            case 'D':
                CommandSetOnTime( 2, _AppBefehl );
                break;

            // E: Ausschaltzeit für Relais 1 einstellen.
            case 'E':
                CommandSetOffTime( 0, _AppBefehl );
                break;

            // G: Ausschaltzeit für Relais 2 einstellen.
            case 'G':
                CommandSetOffTime( 1, _AppBefehl );
                break;

            // H: Ausschaltzeit für Relais 3 einstellen.
            case 'H':
                CommandSetOffTime( 2, _AppBefehl );
                break;

            // J: Impulsdauer für Relais 1 in Sekunden einstellen.
            case 'J':
                CommandSetSwitchDuration( 0, _AppBefehl );
                break;

            // K: Impulsdauer für Relais 2 in Sekunden einstellen.
            case 'K':
                CommandSetSwitchDuration( 1, _AppBefehl );
                break;

            // L: Impulsdauer für Relais 3 in Sekunden einstellen.
            case 'L':
                CommandSetSwitchDuration( 2, _AppBefehl );
                break;

            // M: Steuerungsart je Relais setzen (0 = Taster, 1 = Dauer an, 2 = Zeitbereich je Monat).
            case 'M':
                CommandSetControlMode( _AppBefehl );
                break;

            // P/Q/R: Einschaltzeit je Monat für Relais 1/2/3 (MMHHMM).
            case 'P':
                CommandSetMonthTime( 0, true, _AppBefehl );
                break;
            case 'Q':
                CommandSetMonthTime( 1, true, _AppBefehl );
                break;
            case 'R':
                CommandSetMonthTime( 2, true, _AppBefehl );
                break;

            // S/V/W: Ausschaltzeit je Monat für Relais 1/2/3 (MMHHMM).
            case 'S':
                CommandSetMonthTime( 0, false, _AppBefehl );
                break;
            case 'V':
                CommandSetMonthTime( 1, false, _AppBefehl );
                break;
            case 'W':
                CommandSetMonthTime( 2, false, _AppBefehl );
                break;

            // O: Oberen Grenzwert des Sensors einstellen.
            case 'O':
                oberesLimitSensor = limitseinstellen( _AppBefehl );
                saveSettings();
                break;

            // U: Unteren Grenzwert des Sensors einstellen.
            case 'U':
                unteresLimitSensor = limitseinstellen( _AppBefehl );
                saveSettings();
                break;
            
            // A: Sensorsteuerung aktivieren (Wert 0 deaktiviert sie).
            case 'A':
                controlBySensorAllowed = checkForNotZero( _AppBefehl );
                saveSettings();
                break;

            default:
                // Unbekannten Befehl ignorieren.
                iRet = 0;
                break;
        }
    }

    // Trigger um ein Ereignis auszulösen
    return iRet;
}

/***************************************************************************
 * Funktion zum Konvertieren von (hex)char in (dec)uint8_t
 * in:  hexadezimaler char Zeichen
 * out: zugehöriger dezimaler int Wert
 **************************************************************************/
uint8_t _hexcharToUint8_t(char hexchar)
{
    if (hexchar >= '0' && hexchar <= '9')
        return hexchar - '0';
    if (hexchar >= 'A' && hexchar <= 'F')
        return hexchar - 'A' + 10;
    if (hexchar >= 'a' && hexchar <= 'f')
        return hexchar - 'a' + 10;
    return -1;
}

uint8_t Commands::checkForNotZero( char *value )
{
    uint8_t iRes = 0;
    for(uint8_t i = 1; i < 5; i++ )
        iRes += _hexcharToUint8_t(*(value + i));
    if( iRes )
        return 1;
    else
        return 0;
}

/***************************************************************************
 * Funktion aus dem App-Befehl die Uhrzeit zu setzen
 * Übergabeparameter: Array mit dem entsprechenden Befehl
 * Rückgabe: kein
***************************************************************************/


uint16_t Commands::limitseinstellen( char *c )
{
    return (_hexcharToUint8_t(*c) * 100000 + _hexcharToUint8_t(*(c + 1))*10000 + _hexcharToUint8_t(*(c + 2))*1000 + _hexcharToUint8_t(*(c + 3))*100 + _hexcharToUint8_t(*(c + 4))*10 + _hexcharToUint8_t(*(c + 5)));
}

/***************************************************************************
 * Schreiben der gesamten Zeit und Datums Informationen
 * Übergabeparameter: alle Zeit und Datums Informationen als einzelne Werte
 * Rückgabeparameter: kein
 **************************************************************************/

void Commands::CommandSetTime(char *Uhrzeit)
{
    if (_interpreterzeitmaster == nullptr)
        return;

    uint8_t AppHours;
    uint8_t AppMinutes;
    uint8_t AppSeconds;
    uint8_t AppDate;
    uint8_t AppMonth;
    uint8_t AppYear;

    //Auslesen Stunden
    AppHours = _hexcharToUint8_t(*Uhrzeit) * 10 + _hexcharToUint8_t(*(Uhrzeit + 1));

    //Auslesen Minuten
    AppMinutes = _hexcharToUint8_t(*(Uhrzeit + 2)) * 10 + _hexcharToUint8_t(*(Uhrzeit + 3));

    //Auslesen Sekunden
    AppSeconds = _hexcharToUint8_t(*(Uhrzeit + 4)) * 10 + _hexcharToUint8_t(*(Uhrzeit + 5));

    //Verwerfen der versendeten Appwerte bei Werten außerhalb des Wertebereichs
    if ((AppHours > 23) || (AppMinutes > 59) || (AppSeconds > 59))
    {
        AppHours = _interpreterzeitmaster->getHours();
        AppMinutes = _interpreterzeitmaster->getMinutes();
        AppSeconds = _interpreterzeitmaster->getSeconds();
    }

    //Auslesen der bereits enthaltenen Datumsinformation
    AppDate = _interpreterzeitmaster->getDate();
    AppMonth = _interpreterzeitmaster->getMonth();
    AppYear = _interpreterzeitmaster->getYear();

    //Schreiben der Uhrzeit auf die Echtzeituhr
    _interpreterzeitmaster->setTimeDate(AppHours, AppMinutes, AppSeconds, AppDate, AppMonth, AppYear);
}

void Commands::showInfo( void )
{
    if (_interpreterzeitmaster == nullptr)
        return;

    SerialBT.print("");
    SerialBT.print("---------------------\nAktuelle Zeit:  ");
    if( _interpreterzeitmaster->getHours() < 10)
        SerialBT.print('0');
    SerialBT.print(_interpreterzeitmaster->getHours());SerialBT.print(":");
    if( _interpreterzeitmaster->getMinutes() < 10)
        SerialBT.print('0');
    SerialBT.print(_interpreterzeitmaster->getMinutes());SerialBT.print(":");
    if( _interpreterzeitmaster->getSeconds() < 10)
        SerialBT.print('0');
    SerialBT.println(_interpreterzeitmaster->getSeconds());

    SerialBT.print("Einschaltzeit:  ");
    for (uint8_t relais = 0; relais < 3; relais++)
    {
        SerialBT.print("Relais ");
        SerialBT.print(relais + 1);
        SerialBT.print(" Einschaltzeit: ");
        if( onTime[relais].std < 10)
            SerialBT.print('0');
        SerialBT.print(onTime[relais].std);SerialBT.print(":");
        if( onTime[relais].min < 10)
            SerialBT.print('0');
        SerialBT.print(onTime[relais].min); SerialBT.println(":00");

        SerialBT.print("Relais ");
        SerialBT.print(relais + 1);
        SerialBT.print(" Ausschaltzeit: ");
        if( offTime[relais].std < 10)
            SerialBT.print('0');
        SerialBT.print(offTime[relais].std);SerialBT.print(":");
        if( offTime[relais].min < 10)
            SerialBT.print('0');
        SerialBT.print(offTime[relais].min); SerialBT.println(":00");
    }

    for (uint8_t relais = 1; relais <= 3; relais++)
    {
        SerialBT.print("Relais ");
        SerialBT.print(relais);
        if(compareTimeToTriggerTheLight(relais))
            SerialBT.println(" sollte laut Zeiteinstellung an sein.");
        else
            SerialBT.println(" sollte laut Zeiteinstellung aus sein.");
    }

    if( controlBySensorAllowed && IO != nullptr ) // Später Abfrage der Spannungsmessung
    {

        SerialBT.print("Oberes Sensorlimit:   "); 
        SerialBT.println(oberesLimitSensor);
        SerialBT.print("Unteres Sensorlimit:  "); 
        SerialBT.println(unteresLimitSensor);
        SerialBT.print("Aktueller Sensorwert: "); SerialBT.println(IO->medianSensVal);
        if( IO->getSolarState() )
            SerialBT.println("Laut Sensor sollten die Lichter an sein.");
        else
            SerialBT.println("Laut Sensor sollten die Lichter aus sein.");
        SerialBT.println("---------------------");
    }
}
    

// Return 1 if light should be on and 0 if it should be off
// In this function
// on_time defines the time where te light relais will be turned on -- in the format HHMM
// off_time defines the time where the light relais will be turned off -- in the format HHMM
// Idea: Compare the current time (act) with on_time and off_time to determine if the light should be on or off. If on_time is greater than off_time, it means the light should be on overnight, otherwise it follows the normal schedule.
uint8_t Commands::compareTimeToTriggerTheLight()
{
    return compareTimeToTriggerTheLight(1) &&
           compareTimeToTriggerTheLight(2) &&
           compareTimeToTriggerTheLight(3);
}

bool Commands::usesSwitchControl(uint8_t relais) const
{
    return getControlMode(relais) == CONTROL_MODE_SWITCH;
}

uint8_t Commands::getControlMode(uint8_t relais) const
{
    if (relais < 1 || relais > 3)
        return CONTROL_MODE_MONTH_TIME;

    return controlModeRelais[relais - 1];
}

void Commands::initSwitches()
{
    const uint8_t pins[3] = {SWITCH_1, SWITCH_2, SWITCH_3};
    for (uint8_t i = 0; i < 3; i++)
    {
        pinMode(pins[i], INPUT_PULLUP);
        lastSwitchState[i] = digitalRead(pins[i]);
    }
}

uint8_t Commands::getRelaisState(uint8_t relais)
{
    if (relais < 1 || relais > 3)
        return 0;

    const uint8_t pins[3] = {SWITCH_1, SWITCH_2, SWITCH_3};
    uint8_t index = relais - 1;

    // Taster ist aktiv-low: fallende Flanke startet bzw. verlängert den Impuls.
    uint8_t state = digitalRead(pins[index]);
    if (lastSwitchState[index] == HIGH && state == LOW && controlModeRelais[index] == CONTROL_MODE_SWITCH)
        switchPulseUntil[index] = millis() + switchDurationMs[index];
    lastSwitchState[index] = state;

    switch (controlModeRelais[index])
    {
        case CONTROL_MODE_SWITCH:
            return (int32_t)(switchPulseUntil[index] - millis()) > 0;
        case CONTROL_MODE_ALWAYS_ON:
            return 1;
        default: // CONTROL_MODE_MONTH_TIME
            return compareTimeToTriggerTheLight(relais);
    }
}

uint8_t Commands::isInTimeRange(timeSet on, timeSet off, uint16_t act)
{
    uint16_t on_time = on.std*100 + on.min;
    uint16_t off_time = off.std*100 + off.min;

    // Ist on_time > off_time, läuft der Zeitbereich über Mitternacht.
    if( on_time < off_time )
        return (act >= on_time) && (act < off_time);
    else if( on_time > off_time )
        return (act >= on_time) || (act < off_time);

    return 0;
}

uint32_t Commands::getSwitchDurationMs(uint8_t relais) const
{
    if (relais < 1 || relais > 3)
        return 0;

    return switchDurationMs[relais - 1];
}

uint8_t Commands::compareTimeToTriggerTheLight(uint8_t relais)
{
    if (_interpreterzeitmaster == nullptr)
        return 0;

    if (relais < 1 || relais > 3)
        return 0;

    uint8_t index = relais - 1;

    uint16_t act = _interpreterzeitmaster->getHours()*100 + _interpreterzeitmaster->getMinutes();

    uint8_t month = _interpreterzeitmaster->getMonth();
    if (month < 1 || month > 12)
        return 0;

    return isInTimeRange(monthOnTime[index][month - 1], monthOffTime[index][month - 1], act);
}

void Commands::CommandSetOnTime( char  *_Time )
{
    for (uint8_t relais = 0; relais < 3; relais++)
        CommandSetOnTime(relais, _Time);
}

void Commands::CommandSetOnTime(uint8_t relais, char *_Time)
{
    if (relais >= 3)
        return;

    if (_Time == nullptr ||
        _Time[0] < '0' || _Time[0] > '9' ||
        _Time[1] < '0' || _Time[1] > '9' ||
        _Time[2] < '0' || _Time[2] > '9' ||
        _Time[3] < '0' || _Time[3] > '9')
        return;

    uint8_t hours = (_Time[0] - '0') * 10 + (_Time[1] - '0');
    uint8_t minutes = (_Time[2] - '0') * 10 + (_Time[3] - '0');

    if (hours > 23 || minutes > 59)
        return;

    onTime[relais].std = hours;
    onTime[relais].min = minutes;
    for (uint8_t m = 0; m < 12; m++)
        monthOnTime[relais][m] = onTime[relais];
    saveSettings();
}

void Commands::CommandSetOffTime( char  *_Time )
{
    for (uint8_t relais = 0; relais < 3; relais++)
        CommandSetOffTime(relais, _Time);
}

void Commands::CommandSetOffTime(uint8_t relais, char *_Time)
{
    if (relais >= 3)
        return;

    if (_Time == nullptr ||
        _Time[0] < '0' || _Time[0] > '9' ||
        _Time[1] < '0' || _Time[1] > '9' ||
        _Time[2] < '0' || _Time[2] > '9' ||
        _Time[3] < '0' || _Time[3] > '9')
        return;

    uint8_t hours = (_Time[0] - '0') * 10 + (_Time[1] - '0');
    uint8_t minutes = (_Time[2] - '0') * 10 + (_Time[3] - '0');

    if (hours > 23 || minutes > 59)
        return;

    offTime[relais].std = hours;
    offTime[relais].min = minutes;
    for (uint8_t m = 0; m < 12; m++)
        monthOffTime[relais][m] = offTime[relais];
    saveSettings();
}

void Commands::CommandSetSwitchDuration(uint8_t relais, char *_Duration)
{
    if (relais >= 3)
        return;

    uint32_t seconds = 0;
    for (uint8_t i = 0; i < 6; i++)
    {
        if (_Duration[i] < '0' || _Duration[i] > '9')
            return;
        seconds = seconds * 10 + (_Duration[i] - '0');
    }

    if (seconds == 0)
        return;

    switchDurationMs[relais] = seconds * 1000UL;
    saveSettings();
}

void Commands::CommandSetControlMode(char *_Mode)
{
    for (uint8_t relais = 0; relais < 3; relais++)
    {
        if (_Mode[relais] < '0' || _Mode[relais] > '2')
            return;
    }

    for (uint8_t relais = 0; relais < 3; relais++)
        controlModeRelais[relais] = _Mode[relais] - '0';
    saveSettings();
}

// Parameter: MMHHMM
void Commands::CommandSetMonthTime(uint8_t relais, bool isOn, char *_Param)
{
    if (relais >= 3 || _Param == nullptr)
        return;

    for (uint8_t i = 0; i < 6; i++)
        if (_Param[i] < '0' || _Param[i] > '9')
            return;

    uint8_t month = (_Param[0] - '0') * 10 + (_Param[1] - '0');
    uint8_t hours = (_Param[2] - '0') * 10 + (_Param[3] - '0');
    uint8_t minutes = (_Param[4] - '0') * 10 + (_Param[5] - '0');

    if (month < 1 || month > 12 || hours > 23 || minutes > 59)
        return;

    timeSet &t = isOn ? monthOnTime[relais][month - 1] : monthOffTime[relais][month - 1];
    t.std = hours;
    t.min = minutes;
    saveSettings();
}


