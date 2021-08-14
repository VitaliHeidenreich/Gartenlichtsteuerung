#include <Arduino.h>
#include "config.h"
#include "mypins.h"

//timeSet mypins::onTime = {10,10};

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

uint8_t getSolarState()
{
    uint8_t iRet = 0;

    return iRet;
}