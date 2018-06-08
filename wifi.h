#ifndef __ESP8266_H__
#define __ESP8266_H__
#include "Arduino.h"
#include "pins.h"

// IP address of the BeagleBone
#define DST_IP "192.168.1.156"
// Port to connect to on the BeagleBone
#define DST_PORT 3333
// Static IP of the rigid wing
#define THIS_DEVICE_IP "192.168.1.157"

#define DEBUG_PRINT


void sendMessageToESP(String commandToSend);
int initializeWifi();
bool connectToNetwork(String ssid, String password);
bool openTCP(String ip, int port);
bool connectedTCP();
void sendTCPMessage(String msg);
int closeTCP();
bool waitForStringSerial2(String key, int timeout);
void tempBlinkWifiLEDWrapper(int ledState);

#endif
