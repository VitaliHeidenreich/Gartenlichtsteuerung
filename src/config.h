#ifndef CONFIG_H
#define CONFIG_H

#pragma once
#include <Arduino.h>


/***************************************************************************
 * Serielle Verbindung
 **************************************************************************/
#define SERIAL_SPEED 115200
#define BLUETOOTH_DEVICE_NAME "Gartenlichtsteuerung"
#define BLUETOOTH_PIN "0175"

//Relais Ports
#define RELAIS_1        19      
#define RELAIS_2        18
#define RELAIS_3        5

#define SWITCH_1        17
#define SWITCH_2        16
#define SWITCH_3        4

//#define BATTERYVOLTAGE  

#define RUN_PIN         12

typedef struct
{
   uint8_t std;
   uint8_t min;
} timeSet;


/***************************************************************************
 * App Interpreter Einstellungen --> Communication sign
 **************************************************************************/
#define CONV_VERSION 2


#endif /* CONFIG_H */