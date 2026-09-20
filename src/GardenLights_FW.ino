#include <Arduino.h>
#include "BluetoothSerial.h"
#include "config.h"
#include "mypins.h"
#include "commands.h"
#include "zeitmaster.h"

// Anlegend er Klassen Instanzen
mypins InOut;
Commands com;
Zeitmaster *pZeit;
BluetoothSerial SerialBT;

char c;
volatile uint8_t event = 0;
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
void IRAM_ATTR onTimer()
{
    portENTER_CRITICAL_ISR(&timerMux);
    event = 1;
    portEXIT_CRITICAL_ISR(&timerMux);
}

/* START SETUP *****************************************************************************************************/
void setup()
{
    Serial.begin(115200);
    Serial.println("hallo welt!");
    SerialBT.setPin(BLUETOOTH_PIN);
    SerialBT.begin(BLUETOOTH_DEVICE_NAME);
    Serial.println("Bluetooth gestartet: Gartenlichtsteuerung");
    InOut = mypins();
    InOut.setCommands(&com);
    com.setIO(&InOut);
    // Timer
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 1000000, true);
    timerAlarmEnable(timer);

    // Zeitfunktionen
    pZeit = new Zeitmaster();
    com.setZeitmaster(pZeit);
    if (pZeit->hadLostPower())
        pZeit->setTimeDate(22, 59, 49, 15, 4, 45);
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
    
    // Eventgetriggerte Steuerung der Relais und LEDs
    for (uint8_t relais = 1; relais <= 3; relais++)
    {
        uint8_t relaisOn = com.compareTimeToTriggerTheLight(relais);
        if (!relaisOn && InOut.getSolarState() && com.controlBySensorAllowed)
            relaisOn = 1;

        if (relais == 1)
            InOut.setRelais1(relaisOn);
        else if (relais == 2)
            InOut.setRelais2(relaisOn);
        else
            InOut.setRelais3(relaisOn);
    }

    // Einlesen und Auswerten der seriellen Befehle
    if ( SerialBT.available() > 0 )
    {
        com.readCommandCharFromSerial( SerialBT.read() );
    }
}
//*************************************************************************************************************************