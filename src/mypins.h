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

class mypins {
    private:
        uint8_t myVal;
    public:
        mypins( );
        void setRelais(uint8_t R1, uint8_t R2, uint8_t R3);
        uint8_t getSolarState();

        //static timeSet onTime; 
};

#endif /* MYHEADER_H */