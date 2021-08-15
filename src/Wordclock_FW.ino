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
uint8_t event = 0;
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
void IRAM_ATTR onTimer()
{
    portENTER_CRITICAL_ISR(&timerMux);
    event = 1;
    portEXIT_CRITICAL_ISR(&timerMux);
}

/* START SETUP ************************************************************/
void setup()
{
    Serial.begin(115200);
    Serial.println("hallo welt!");
    InOut = mypins();
    // Timer
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 1000000, true);
    timerAlarmEnable(timer);

    // Zeitfunktionen
    pZeit = new Zeitmaster();
    pZeit->setTimeDate(22, 59, 49, 15, 4, 45); // ToDo: Sollte gelöscht werden, wenn die NTP Zeit bzw. App Zeiteinstellung funktioniert
}

/*************************************************************************************************************************
 * 
**************************************************************************************************************************/
void loop()
{
    if( event )
    {
        InOut.calcSolarState();
        event = 0;
    }
    
    // Eventgetriggerte Steuerung der Bewegung und der LEDs
    if( com.compareTimeToTriggerTheLight() )
    {
        InOut.setRelais(1,1,1);
    }
    else
    {
        if( InOut.getSolarState() && com.controlBySensorAllowed )
            InOut.setRelais(1,1,1);
        else
            InOut.setRelais(0,0,0);
    }

    if (Serial.available() > 0)
    {
        com.readCommandCharFromSerial( Serial.read() );
    }
}
