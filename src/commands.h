#ifndef COMMANDS_H
#define COMMANDS_H
#pragma once

#include "Arduino.h"
#include "mypins.h"

#define DEBUG_APPINTERPRETER 0

class Commands
{
public:
     //Konstruktor
     Commands();

     /****************************************
     * App Befehle einlesen
     ***************************************/
     uint8_t readCommandCharFromSerial(char CommandChar);
     uint8_t compareTimeToTriggerTheLight(); 

     void CommandSetOnTime( char  *_Time );
     void CommandSetOffTime( char  *_Time );




private:
     void CommandSetTime( char *Uhrzeit );
     void showInfo( void );
     void GetTime( timeSet t );
     static timeSet onTime;
     static timeSet offTime;
     static uint16_t unteresLimitSensor;
     static uint16_t oberesLimitSensor;
     uint16_t limitseinstellen( char *c );

};


#endif /* COMMANDS_H */