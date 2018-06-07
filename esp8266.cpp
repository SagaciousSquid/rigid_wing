#include "esp8266.h"

int tcpConnection = 0;

void sendMessageToESP(String commandToSend) {
  Serial4.println(commandToSend);

  #ifdef DEBUG_PRINT
  Serial.println("--- " + commandToSend);
  #endif
}

int initializeWifi() {

  // Reset the module
  sendMessageToESP("AT+RST");

  #ifdef DEBUG_PRINT
  Serial.println("Resetting Wifi Module");
  #endif

  // wait for a "ready" command
  bool reset_successful = waitForStringSerial4("ready", 3000);

  if (reset_successful) {
    #ifdef DEBUG_PRINT
    Serial.println("Wifi Reset Successfully");
    #endif
    return 0;

  } else {
    #ifdef DEBUG_PRINT
    Serial.println("Wifi Reset Failed");
    #endif
    return 1;
  }
}

// This initializes the ESP8266 module
// This attempts to connect to a network. If it is succesful, True is returned
bool connectToNetwork(String ssid, String password) {

  #ifdef DEBUG_PRINT
  Serial.println("Attempting to connect to " + ssid);
  Serial.println("Password is " + password);
  #endif

  // Maybe search for network to see it it's available first?

  // Set the operating mode to Client
  // Client = 1, AP = 2, Client and AP = 3
  sendMessageToESP("AT+CWMODE=1");

  // Build the message to connect to the given ssid with the password
  String cmd = "AT+CWJAP=\"" + ssid + "\",\"" + password + "\"";
  sendMessageToESP(cmd);

  // wait for a "OK" command
  bool connection_successful = waitForStringSerial4("OK", 3000);

  cmd = String("AT+CIPSTA=\"") + THIS_DEVICE_IP + "\"";
  sendMessageToESP(cmd);

  // wait for a "OK" command
  connection_successful = connection_successful && waitForStringSerial4("OK", 3000);

  if (connection_successful) {
    #ifdef DEBUG_PRINT
    Serial.println("Connection Successful");
    #endif
    return true;

  } else {
    #ifdef DEBUG_PRINT
    Serial.println("Connection Failed");
    #endif
    return false;
  }
}

// Open a TCP connection
// A returned value of True indicates it was successful
boolean openTCP(String ip, int port) {
  // Set transparent mode to 1 so that messages recieved will be sent directly to serial
  // Set transparent mode to 0
  //  sendMessageToESP("AT+CIPMODE=0", printing);

  // build command
  String cmd = "AT+CIPSTART=\"TCP\",\"" + ip + "\"," + port;

  sendMessageToESP(cmd);
  //  Serial.println(cmd);

  // wait for a "OK" command
  bool connection_successful = waitForStringSerial4("OK", 3000);

  if (connection_successful) {
    #ifdef DEBUG_PRINT
    Serial.println("TCP Connection to " + ip + " port number " + String(port) + " successful");
    #endif
    return true;
  } else {
    #ifdef DEBUG_PRINT
    Serial.println("TCP Connection to " + ip + " port number " + String(port) + " failed");
    #endif
    return false;
  }
}

// Return true if connected to TCP, false otherwise
bool connectedTCP() {
  sendMessageToESP("AT+CIPSTATUS");

  if (waitForStringSerial4("STATUS:3", 500)) {
    #ifdef DEBUG_PRINT
    Serial.println("TCP still connected");
    #endif
    tcpConnection = 1;
    return true;
  }
  #ifdef DEBUG_PRINT
  Serial.println("TCP connection lost");
  #endif

  tcpConnection = 0;
  return false;
}

// Send a message over TCP()
void sendTCPMessage(String msg) {

  // build initial message
  String instructionToSend = "AT+CIPSEND=" + String(msg.length());

  #ifdef DEBUG_PRINT
  Serial.println("Sending message: " + msg);
  #endif

  // Send the message
  sendMessageToESP(instructionToSend);
  delay(20);
  sendMessageToESP(msg);
}


// Close the current TCP connection
// #TODO Unused! Should this be implemented somewhere?
int closeTCP() {
  sendMessageToESP("AT+CIPCLOSE");

  #ifdef DEBUG_PRINT
  Serial.println("TCP Closed");
  #endif

  return 0;
}

// This method scans the input from Serial4 for a specific key
// If this key is found before the timeout, true is returned.
// Othertime false is returned
bool waitForStringSerial4(String key, int timeout) {

  int start_time = millis();

  while (millis() < start_time + timeout) {
    if (Serial4.available()) {
      String data = Serial4.readString();
      // Serial.println(data);

      for (int i = 0; i < data.length() - key.length(); i++) {
        if (data.substring(i, i + key.length()) == key) {
          return true;
        }
      }
    }
  }

  return false;
}

// #TODO temporary wrapper for maintaining original code
void tempBlinkWifiLEDWrapper(int ledState) {
  if (!tcpConnection) {
    digitalWrite(WIFILED_PIN, ledState);
  }
}
