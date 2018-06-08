#define DEBUG_PRINT

#include <Servo.h>
#include "pins.h"
#include "credentials.h"
#include "consts.h"
#include "wifi.h"

int control = 0; //to enable direct control over tab angle
int lift = 0; //0 to produce no lift 1 to produce lift
int drag = 0;
int windSide = 0; //0 for wind from port 1 for wind from starboard

int heelAngle = 0; //mapped heel angle, 0 degrees is straight up 90 would be on its side
int maxHeelAngle = 30;//settable max heel angle

int angleIn;//reading from wind direction sensor on the front of the sail
int readAttackAngle; //mapped value from wind sensor
int sentAttackAngle; //value mapped to correct sending format

int controlAngle = 0; //manual angle set by boat

int tabAngle = 0; //angle of tab relative to centered being 0

int state;
int printing = 1;
int connectionCount = 0;

int ledState = LOW;
int servoAngle;

IntervalTimer LEDtimer;
IntervalTimer servoTimer;

Servo servo;

void setup() {
  //init
  //pinMode(POT_PIN, INPUT);
  pinMode(WIFILED_PIN, OUTPUT);
  pinMode(WIFILED_GND_PIN, OUTPUT);
  pinMode(POWERLED_PIN, OUTPUT);
  pinMode(POWERLED_GND_PIN, OUTPUT);
  //pinMode(VIN_PIN, INPUT);
  servo.attach(SERVO_PIN);

  digitalWrite(POWERLED_GND_PIN, LOW);
  digitalWrite(WIFILED_GND_PIN, LOW);

  digitalWrite(POWERLED_PIN, HIGH);// turn on power led

  // Initialize Everything
  initializeComs();
  initializeWifi();

  // Connect to the network
  digitalWrite(WIFILED_PIN, LOW);
  connectToNetwork(SSID, PASS);
  digitalWrite(WIFILED_PIN, HIGH);// turn on power led

  LEDtimer.begin(blinkState, 916682);
  servoTimer.begin(servoControl, 50000);

  servo.write(servoOffset); //in place so lift starts at 0 degrees or neutral state

  



  // LEDs are wired to digial pins for their ground reference to save board space

}

void loop() {
  //delay(50);  for serial testing no wifi
  //----------------------------------------------------------------------
  //Wifi communication and message parsing

  if (Serial.available() > 0) {
    // read the incoming byte:
    state = Serial.read() - 48;

    Serial.print("State:");
    Serial.print(state);
  }

  int vIn = analogRead(VIN_PIN);

  if (windSide) {
    servoAngle = tabAngle + 60;
  }
  else {
    servoAngle = -tabAngle + 60;
  }


  sentAttackAngle = (360 + readAttackAngle) % 360;

  if (printing) {
    Serial.print("  Angle of Attack:");
    Serial.print(readAttackAngle);

    Serial.print("  Servo Angle:");
    Serial.println(tabAngle);
  }

  stateSet();

  if (connectedTCP()) {
    connectionCount = 0;
    digitalWrite(WIFILED_PIN, HIGH);

    sendBoatMessage(sentAttackAngle, servoAngle, vIn);  //message sent to hull
    delay(10);                    //delay for message to send before recieving

    if (readMessage(250) && printing) {
      Serial.print("S: ");
      Serial.print(state);
      Serial.print(", A:");
      Serial.print(heelAngle);
      Serial.print(", B:");
      Serial.print(maxHeelAngle);
      Serial.print(", C:");
      Serial.println(controlAngle);
    }

  } else {
    connectionCount++;
    if (connectionCount >= 4) {
      control = 0;
      lift = 0;
      drag = 0;
    }
    openTCP(DST_IP, DST_PORT);    //if no message is recieved than there is no connection so the port is openend
    delay(50);
  }
#define DEBUG_PRINT
}

void sendBoatMessage(int wind, int servoPos, int volt) {
  String msg = "[";
  msg += addZerosToString(wind, 3) + ",";
  msg += addZerosToString(servoPos, 3) + ",";
  msg += addZerosToString(volt, 3) + "]";

  sendTCPMessage(msg);
}


String addZerosToString(int n, int z) {
  String result = String(n);
  if (n > 999) {
    return "999";
  }

  int s = 10;

  while (s < pow(10, z)) {
    if (s >= n) {
      result = "0" + result;
    }
    s = s * 10;
  }

  return result;
}

// This initializes the serial buses
int initializeComs() {
  Serial.begin(115200);
  //while(!Serial){}
  Serial2.begin(115200);
  while(!Serial2){}

  if (printing) Serial.println("Communication Initialized");

  return 0;
}

bool readMessage(int timeout) {
  int start_time = millis();

  bool recievedNewData = false;
  String data = "";

  //  "[1,180,180,100]"

  while (millis() < start_time + timeout) {
    if (Serial2.available()) {
      data += Serial2.readString();
      Serial.println(data);

      for (int i = 0; i < data.length(); i++) {
        if (data.substring(i, i + 1) == "[") {
          if (data.length() > i + 15) {
            String validData = data.substring(i, i + 15);

            // Serial.println("Special string: " + validData);
            state =             validData.substring(1, 2).toInt();
            heelAngle =         validData.substring(3, 6).toInt();
            maxHeelAngle =      validData.substring(7, 10).toInt();
            controlAngle =   validData.substring(11, 14).toInt() - 50;

            recievedNewData = true;
            return true;
          }
        }
      }
    }
  }

  return recievedNewData;
}



void blinkState() {
  if (ledState == LOW) {
    ledState = HIGH;
  } else {
    ledState = LOW;
  }
  tempBlinkWifiLEDWrapper(ledState);
}

void stateSet() {
  if (state == 0) {
    control = 0;
    lift = 0;
    drag = 0;
  }
  else if (state == 1) {
    control = 0;
    lift = 1;
    drag = 0;
    windSide = 1;
  }
  else if (state == 2) {
    control = 0;
    lift = 1;
    drag = 0;
    windSide = 0;
  }
  else if (state == 3) {
    control = 0;
    lift = 0;
    drag = 1;
    windSide = 1;
  }
  else if (state == 4) {
    control = 0;
    lift = 0;
    drag = 1;
    windSide = 0;
  }
  else if (state == 7) {
    control = 1;
    lift = 0;
    drag = 0;
  }
}

void servoControl() {

  angleIn = analogRead(POT_PIN); // reads angle of attack data
  readAttackAngle = (float)(angleIn-947) * 0.3442;
  //---------------------------------------------------------------------------------------------------
  //set for manual control
  if (control) {
    servo.write(servoOffset + controlAngle);
  }

  //------------------------------------------------------------------------------------------------------
  //when lift is desired
  if (lift) {

    if (!windSide) {
      readAttackAngle = readAttackAngle * -1;
    }

    //if the lift angle isnt enough and the heel angle isnt too much the angle of attack is increased
    if ((maxLiftAngle > readAttackAngle+1)) {  //&& (abs(heelAngle) <= maxHeelAngle))) {
      if (tabAngle >= 55) { }
      else {
        tabAngle++;
      }
    }

    //if the lift angle is too much or the max heel angle is too much the sail lightens up
    else if ((maxLiftAngle < readAttackAngle)) {  //&& (abs(heelAngle) <= maxHeelAngle)) || (abs(heelAngle) >= maxHeelAngle)) {
      if (tabAngle <= -55) {  }
      else {
        tabAngle--;
      }
    }

    //if the angle of attack is correct
    else if (maxLiftAngle == readAttackAngle) { }

    //to adjust tab angle according to wind side
    if (windSide) {
      servo.write(servoOffset + tabAngle);
    }
    else {
      servo.write(servoOffset - tabAngle);
    }
  }
  //-----------------------------------------------------------------------------------------------------------
  //while drag if desired
  if (drag) {

    //set sail to most possible angle of attack with respect to direction of wind
    if (windSide) {
      servo.write(servoOffset + 55);
    }
    else if (!windSide) {
      servo.write(servoOffset - 55);
    }
  }
  //----------------------------------------------------------------------------------
  //minimum lift (windvane)
  if (!lift && !drag && !control) {
    servo.write(servoOffset);
    /*
      if (readAttackAngle < 2 && readAttackAngle > -2) {  }            // if angle of attack is within -2 to 2 do nothing
      else if (readAttackAngle > 2 && tabAngle < 60) {       // if angle of attack is to much adjust
      tabAngle--;
      }
      else if (readAttackAngle < -2 && tabAngle > -60) { // if angle of attack is to much adjust
      tabAngle++;
      }
      servo.write(servoOffset + tabAngle);
    */
  }
}
