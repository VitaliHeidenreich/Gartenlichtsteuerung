#include <Arduino.h>
#include "config.h"
#include "mypins.h"
#include "commands.h"
#include "zeitmaster.h"

// Anlegend er Klassen Instanzen
mypins InOut;
Commands com;
Zeitmaster *pZeit;

char c;

/* START SETUP ************************************************************/
void setup()
{
    Serial.begin(115200);
    Serial.println("hallo welt!");
    InOut = mypins();

    // Zeitfunktionen
    pZeit = new Zeitmaster();
    pZeit->setTimeDate(22, 59, 49, 15, 4, 45); // ToDo: Sollte gelöscht werden, wenn die NTP Zeit bzw. App Zeiteinstellung funktioniert
}

/*************************************************************************************************************************
 * 
**************************************************************************************************************************/
void loop()
{
    // Eventgetriggerte Steuerung der Bewegung und der LEDs
    if( com.compareTimeToTriggerTheLight() )
    {
        InOut.setRelais(1,1,1);
    }
    else
    {
        InOut.setRelais(0,0,0);
    }

    if (Serial.available() > 0)
    {
        com.readCommandCharFromSerial( Serial.read() );
    }
}
