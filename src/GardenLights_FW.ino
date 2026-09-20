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
const uint8_t switchPins[3] = {SWITCH_1, SWITCH_2, SWITCH_3};
uint8_t lastSwitchState[3];
uint32_t switchPulseUntil[3] = {0, 0, 0};
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
    for (uint8_t switchIndex = 0; switchIndex < 3; switchIndex++)
    {
        pinMode(switchPins[switchIndex], INPUT_PULLUP);
        lastSwitchState[switchIndex] = digitalRead(switchPins[switchIndex]);
    }
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
        uint8_t relaisOn = 0;
        uint8_t switchIndex = relais - 1;

        uint8_t currentSwitchState = digitalRead(switchPins[switchIndex]);
        if (currentSwitchState != lastSwitchState[switchIndex])
        {
            lastSwitchState[switchIndex] = currentSwitchState;
            if (com.usesSwitchControl(relais))
                switchPulseUntil[switchIndex] = millis() + com.getSwitchDurationMs(relais);
        }

        if (com.usesSwitchControl(relais))
            relaisOn = (int32_t)(switchPulseUntil[switchIndex] - millis()) > 0;
        else
            relaisOn = com.compareTimeToTriggerTheLight(relais);

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