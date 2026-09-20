#include "Commands.h"
#include "config.h"
#include "Zeitmaster.h"
#include "mypins.h"
#include "BluetoothSerial.h"

extern BluetoothSerial SerialBT;

Zeitmaster *_interpreterzeitmaster;
mypins *IO;

timeSet Commands::onTime[3] = {{22,10}, {22,10}, {22,10}};
timeSet Commands::offTime[3] = {{5,1}, {5,1}, {5,1}};
bool Commands::switchControl[3] = {false, false, false};
uint32_t Commands::switchDurationMs[3] = {5000, 5000, 5000};
uint16_t Commands::unteresLimitSensor = 800;
uint16_t Commands::oberesLimitSensor = 2200;
uint8_t Commands::controlBySensorAllowed = 0;

Commands::Commands()
{
    _interpreterzeitmaster = nullptr;
}

void Commands::setZeitmaster(Zeitmaster *zeitmaster)
{
    _interpreterzeitmaster = zeitmaster;
}

void Commands::setIO(mypins *io)
{
    IO = io;
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
 *  M Set control mode: first 3 parameters, 0=time and 1=switch
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
            // Einstellen der Zeit des DS3231
            case 'T':
                    CommandSetTime( _AppBefehl );
                    break;
            
            // Rueckgage der Zeit (Debug)
            case 'I':
                    showInfo( );
                    break;

            // Einstellen der Einschaltzeit
            case 'N':
                    CommandSetOnTime( _AppBefehl );
                    break;

            // Einstellen der Ausschaltzeit
            case 'F':
                    CommandSetOffTime( _AppBefehl );
                    break;

                case 'B':
                    CommandSetOnTime( 0, _AppBefehl );
                    break;

                case 'C':
                    CommandSetOnTime( 1, _AppBefehl );
                    break;

                case 'D':
                    CommandSetOnTime( 2, _AppBefehl );
                    break;

                case 'E':
                    CommandSetOffTime( 0, _AppBefehl );
                    break;

                case 'G':
                    CommandSetOffTime( 1, _AppBefehl );
                    break;

                case 'H':
                    CommandSetOffTime( 2, _AppBefehl );
                    break;

                    case 'J':
                        CommandSetSwitchDuration( 0, _AppBefehl );
                        break;

                    case 'K':
                        CommandSetSwitchDuration( 1, _AppBefehl );
                        break;

                    case 'L':
                        CommandSetSwitchDuration( 2, _AppBefehl );
                        break;

                    case 'M':
                        CommandSetControlMode( _AppBefehl );
                        break;

            case 'O':
                    oberesLimitSensor = limitseinstellen( _AppBefehl );
                    break;

            case 'U':
                    unteresLimitSensor = limitseinstellen( _AppBefehl );
                    break;
            
            case 'A':
                    controlBySensorAllowed = checkForNotZero( _AppBefehl );
                    break;

            default:
                // unknown command
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
    return relais >= 1 && relais <= 3 && switchControl[relais - 1];
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
    uint16_t on_time = onTime[index].std*100 + onTime[index].min;
    uint16_t off_time = offTime[index].std*100 + offTime[index].min;

    // compare the on_time and off_time in 24h format -- HHMM
    // If the ontime is greater than the off_time, it means the light should be on overnight. Otherwise, it follows the normal schedule.
    if( on_time < off_time )
        return (act >= on_time) && (act < off_time);
    else if( on_time > off_time )
        return (act >= on_time) || (act < off_time);

    return 0;
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
}

void Commands::CommandSetControlMode(char *_Mode)
{
    for (uint8_t relais = 0; relais < 3; relais++)
    {
        if (_Mode[relais] != '0' && _Mode[relais] != '1')
            return;
    }

    for (uint8_t relais = 0; relais < 3; relais++)
        switchControl[relais] = (_Mode[relais] == '1');
}


