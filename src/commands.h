#ifndef COMMANDS_H
#define COMMANDS_H
#pragma once

#include "Arduino.h"
#include "mypins.h"
#include "Zeitmaster.h"

#define DEBUG_APPINTERPRETER 0

class Commands
{
public:
     //Konstruktor
     Commands();
     void setZeitmaster(Zeitmaster *zeitmaster);
     void setIO(mypins *io);

     /****************************************
     * App Befehle einlesen
     ***************************************/
     uint8_t readCommandCharFromSerial(char CommandChar);
     uint8_t compareTimeToTriggerTheLight(); 
     uint8_t compareTimeToTriggerTheLight(uint8_t relais);
     bool usesSwitchControl(uint8_t relais) const;
     uint32_t getSwitchDurationMs(uint8_t relais) const;

     void CommandSetOnTime( char  *_Time );
     void CommandSetOffTime( char  *_Time );

     static uint16_t unteresLimitSensor;
     static uint16_t oberesLimitSensor;

     static uint8_t controlBySensorAllowed;

private:
     void CommandSetTime( char *Uhrzeit );
     void showInfo( void );
     void GetTime( timeSet t );
     static timeSet onTime[3];
     static timeSet offTime[3];
     static bool switchControl[3];
     static uint32_t switchDurationMs[3];
     uint8_t checkForNotZero( char *value );
     void CommandSetOnTime(uint8_t relais, char *_Time);
     void CommandSetOffTime(uint8_t relais, char *_Time);
     void CommandSetSwitchDuration(uint8_t relais, char *_Duration);
     void CommandSetControlMode(char *_Mode);
     
     uint16_t limitseinstellen( char *c );

};


#endif /* COMMANDS_H */