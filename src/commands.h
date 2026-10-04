#ifndef COMMANDS_H
#define COMMANDS_H
#pragma once

#include "Arduino.h"
#include "mypins.h"
#include "Zeitmaster.h"
#include "settingsstore.h"

#define DEBUG_APPINTERPRETER 0

// Steuerungsarten je Relais
#define CONTROL_MODE_SWITCH     0   // Taster: Relais für switchDurationMs aktiv
#define CONTROL_MODE_ALWAYS_ON  1   // Dauer aktiv
#define CONTROL_MODE_MONTH_TIME 2   // Zeitbereich je Monat

class Commands
{
public:
     //Konstruktor
     Commands();
     void setZeitmaster(Zeitmaster *zeitmaster);
     void setIO(mypins *io);

     // Einstellungen beim Start laden; Änderungen danach automatisch speichern
     void loadSettings();
     void saveSettingsIfChanged();

     /****************************************
     * App Befehle einlesen
     ***************************************/
     uint8_t readCommandCharFromSerial(char CommandChar);
     uint8_t compareTimeToTriggerTheLight(); 
     uint8_t compareTimeToTriggerTheLight(uint8_t relais);
     bool usesSwitchControl(uint8_t relais) const;
     uint32_t getSwitchDurationMs(uint8_t relais) const;
     uint8_t getControlMode(uint8_t relais) const;

     // Schalter-Pins initialisieren
     void initSwitches();
     // Gewünschter Relaiszustand (1 = an) abhängig von der Steuerungsart
     uint8_t getRelaisState(uint8_t relais);

     void CommandSetOnTime( char  *_Time );
     void CommandSetOffTime( char  *_Time );

     static uint16_t unteresLimitSensor;
     static uint16_t oberesLimitSensor;

     static uint8_t controlBySensorAllowed;

private:
     void buildSnapshot(PersistentSettings &s);
     static PersistentSettings lastSavedSettings;
     static bool settingsValid;
     void CommandSetTime( char *Uhrzeit );
     void showInfo( void );
     void GetTime( timeSet t );
     static timeSet onTime[3];
     static timeSet offTime[3];
     static uint8_t controlModeRelais[3];
     static timeSet monthOnTime[3][12];
     static timeSet monthOffTime[3][12];
     static uint32_t switchPulseUntil[3];
     static uint8_t lastSwitchState[3];
     static uint8_t isInTimeRange(timeSet on, timeSet off, uint16_t act);
     void CommandSetMonthTime(uint8_t relais, bool isOn, char *_Param);
     static uint32_t switchDurationMs[3];
     uint8_t checkForNotZero( char *value );
     void CommandSetOnTime(uint8_t relais, char *_Time);
     void CommandSetOffTime(uint8_t relais, char *_Time);
     void CommandSetSwitchDuration(uint8_t relais, char *_Duration);
     void CommandSetControlMode(char *_Mode);
     
     uint16_t limitseinstellen( char *c );

};


#endif /* COMMANDS_H */