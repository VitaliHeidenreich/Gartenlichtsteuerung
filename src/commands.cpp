#include "Commands.h"
#include "config.h"
#include "Zeitmaster.h"
#include "mypins.h"


Zeitmaster *_interpreterzeitmaster;
mypins *IO;

timeSet Commands::onTime = {22,10};
timeSet Commands::offTime = {5,1};
uint16_t Commands::unteresLimitSensor = 800;
uint16_t Commands::oberesLimitSensor = 2200;

Commands::Commands()
{
    //_interpreterzeitmaster = new Zeitmaster();
}

/** ===========================================================
 *  | Befehlsstruktur:                                        |
 *  |-----------------+---------------------+-----------------|
 *  | Start/Erkennung | Eigentlicher Befehl | Ende            |
 *  |-----------------+---------------------+-----------------+
 *  | "X"             | # X X X X X X       | '$\n' oder '$\t |
 *  |-----------------+---------------------+-----------------|
 *  
 *  Details in der Befehlsliste
*/

/***************************************************************************
 * Zusammenfügen der einzelnen übertragenen char aus der App in Befehlsbuffer
 * Übergabeparameter: char aus der seriellen BT Übertragung
 * Rückgabe: uint8_t zur Anzeige ob Befehl aktiv war oder nicht
***************************************************************************/
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
        // Vorläufig wurde was erkannt
        iRet = 1;

        //Erstellung lokale Kopie für Befehle inkl Drehung der Orientierung
        for (i = 7; i > 1; i--)
        {
            _AppBefehl[7 - i] = _AppBefehlBuffer[i];
        }

        switch (_AppBefehlBuffer[8])
        {
            // einstellen der Zeit
            case 'T':
                    CommandSetTime( _AppBefehl );
                    break;
            
            // Rückgage der Zeit (Debug)
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

            case 'O':
                    oberesLimitSensor = limitseinstellen( _AppBefehl );
                    break;

            case 'U':
                    unteresLimitSensor = limitseinstellen( _AppBefehl );
                    break;

            default:
                iRet = 0;
                break;
        }
    }

    // Trigger um ein Ereignis auszulösen
    return iRet;
}

/***************************************************************************
 * Funktion aus dem App-Befehl die Uhrzeit zu setzen
 * Übergabeparameter: Array mit dem entsprechenden Befehl
 * Rückgabe: kein
***************************************************************************/
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
    Serial.print("");
    Serial.print("---------------------\nAktuelle Zeit:  ");
    if( _interpreterzeitmaster->getHours() < 10)
        Serial.print('0');
    Serial.print(_interpreterzeitmaster->getHours());Serial.print(":");
    if( _interpreterzeitmaster->getMinutes() < 10)
        Serial.print('0');
    Serial.print(_interpreterzeitmaster->getMinutes());Serial.print(":");
    if( _interpreterzeitmaster->getSeconds() < 10)
        Serial.print('0');
    Serial.println(_interpreterzeitmaster->getSeconds());

    Serial.print("Einschaltzeit:  ");
    if( onTime.std < 10)
        Serial.print('0');
    Serial.print(onTime.std);Serial.print(":");
    if( onTime.min < 10)
        Serial.print('0');
    Serial.print(onTime.min); Serial.println(":00");

    Serial.print("Ausschaltzeit: ");
    if( offTime.std < 10)
        Serial.print('0');
    Serial.print(offTime.std);Serial.print(":");
    if( offTime.min < 10)
        Serial.print('0');
    Serial.print(offTime.min); Serial.println(":00");

    if(compareTimeToTriggerTheLight())
        Serial.println("Laut Zeiteinstellung sollten die Lichter an sein.");
    else
        Serial.println("Laut Zeiteinstellung sollten die Lichter aus sein.");

    if( 1 ) // Später Abfrage der Spannungsmessung
    {
        Serial.print("Oberes Sensorlimit:   "); 
        Serial.println(oberesLimitSensor);
        Serial.print("Unteres Sensorlimit:  "); 
        Serial.println(unteresLimitSensor);
        Serial.print("Aktueller Sensorwert: "); Serial.println(IO->getSolarState());
        Serial.println("---------------------");
    }
}
    

// Return 1 if light should be on
uint8_t Commands::compareTimeToTriggerTheLight()
{
    uint8_t iRet = 0;

    uint16_t act = _interpreterzeitmaster->getHours()*100 + _interpreterzeitmaster->getMinutes();
    uint16_t on = onTime.std*100 + onTime.min;
    uint16_t off = offTime.std*100 + offTime.min;

    // Licht brennt ueber Mitternacht
    if( on > off )
    {
            if( (act >= on) || (off > act) )
                iRet = 1;
            else
                iRet = 0;
    }
    else
    {
            if( (act >= on) || (off > act) )
                iRet = 1;
            else
                iRet = 0;
    }

    //act = 
    // convert time to decimal values
    

    return iRet;
}

void Commands::CommandSetOnTime( char  *_Time )
{
    //Auslesen Stunden
    onTime.std = _hexcharToUint8_t(*_Time) * 10 + _hexcharToUint8_t(*(_Time + 1));
    //Auslesen Minuten
    onTime.min = _hexcharToUint8_t(*(_Time + 2)) * 10 + _hexcharToUint8_t(*(_Time + 3));
}

void Commands::CommandSetOffTime( char  *_Time )
{
    //Auslesen Stunden
    offTime.std = _hexcharToUint8_t(*_Time) * 10 + _hexcharToUint8_t(*(_Time + 1));
    //Auslesen Minuten
    offTime.min = _hexcharToUint8_t(*(_Time + 2)) * 10 + _hexcharToUint8_t(*(_Time + 3));
}


