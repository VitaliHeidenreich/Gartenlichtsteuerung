#ifndef MYPINS_H
#define MYPINS_H
#pragma once

#include "config.h"
#include "Arduino.h"

#define GET_TVSTATE (digitalRead(TVSTATE))
#define OUT_SENSSTATE (digitalRead(ET2))
#define IN_SENSSTATE (digitalRead(ET1))

#define SOLAR_ON_VAL    2000
#define SOLAR_OFF_VAL   1800
#define PV_VOLTAGE_PIN  35
#define ANZAHL_MESV     120

class mypins {
    private:
        uint8_t myVal;
        static uint8_t solarStateRes;
    public:
        mypins( );
        void setRelais(uint8_t R1, uint8_t R2, uint8_t R3);
        uint8_t calcSolarState();
        uint8_t getSolarState();
        static uint32_t medianSensVal;
};

#endif /* MYHEADER_H */