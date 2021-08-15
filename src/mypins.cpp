#include <Arduino.h>
#include "config.h"
#include "mypins.h"
#include "commands.h"
//timeSet mypins::onTime = {10,10};

Commands *cm;
uint32_t mypins::medianSensVal = 2000;
uint8_t mypins::solarStateRes = 0;

mypins::mypins()
{
    // Zum Ansteuern der Relais auf Ausgang
    pinMode(RELAIS_1, OUTPUT);
    pinMode(RELAIS_2, OUTPUT);
    pinMode(RELAIS_3, OUTPUT);

    // Zum Ansteuern der LED auf Ausgang
    pinMode(RUN_PIN, OUTPUT);
    digitalWrite(RUN_PIN, HIGH);
}

void mypins::setRelais(uint8_t R1, uint8_t R2, uint8_t R3)
{
    if( R1 == 1 )
        digitalWrite( RELAIS_1, HIGH);
    else
        digitalWrite( RELAIS_1, LOW);

    if( R2 == 1 )
        digitalWrite( RELAIS_2, HIGH);
    else
        digitalWrite( RELAIS_2, LOW);

    if( R3 == 1 )
        digitalWrite( RELAIS_3, HIGH);
    else
        digitalWrite( RELAIS_3, LOW);
}

uint8_t mypins::calcSolarState()
{
    static uint8_t updateAllowed = 0;
    static uint32_t value[ANZAHL_MESV];
    static uint8_t count = 0;

    if( count >= ANZAHL_MESV )
    {
        count = 0;
        updateAllowed = 1;
    }

    value[ count ] = analogRead( PV_VOLTAGE_PIN );
    count++;

    medianSensVal = 0;
    for(uint8_t z = 0; z < ANZAHL_MESV; z++)
        medianSensVal += value[ z ];

    medianSensVal /= ANZAHL_MESV;

    if( updateAllowed && ( medianSensVal <= cm->unteresLimitSensor ) )
        solarStateRes = 1;
    else if( ( medianSensVal >= cm->oberesLimitSensor ) )
        solarStateRes = 0;
    else
        solarStateRes = solarStateRes;
    
    return solarStateRes;
}

uint8_t mypins::getSolarState()
{
    return solarStateRes;;
}