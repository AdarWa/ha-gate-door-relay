#include <Arduino.h>
#include <Ethernet.h>
#include <ArduinoHA.h>
#include <ArduinoOTA.h>
#include <Ticker.h>
#include "secrets.h"

#define relay1 3
#define relay2 2
#define relay3 1
#define relay4 0

#define CAMERA_TIMER_MS 120000
#define GATE_TIMER_MS 30000
#define DOOR_TIMER_MS 1000

byte mac[] = {0x00, 0x10, 0xFA, 0x6E, 0x38, 0x4A};

EthernetClient client;
HADevice device(mac, sizeof(mac));
HAMqtt mqtt(client, device);

HAButton door("mainDoor");
HASwitch gate("mainGate");
HASensor camera("camera");
HAButton cameraOn("cameraOn");
HAButton cameraOff("cameraOff");
HAButton restart("restart");

void cameraTimerCb();
void gateTimerCb();
void doorTimerCb();

Ticker cameraTimer(cameraTimerCb, CAMERA_TIMER_MS, 0, MILLIS);
Ticker gateTimer(gateTimerCb, GATE_TIMER_MS, 0, MILLIS);
Ticker doorTimer(doorTimerCb, DOOR_TIMER_MS, 0, MILLIS);

unsigned long lastReconnectAttempt = 0;

// ---------------------- RECONNECT HANDLER -----------------------
bool reconnectMQTT() {
    if (!mqtt.isConnected()) {
        Serial.println("MQTT disconnected. Reconnecting...");
        if (mqtt.begin(BROKER_ADDR, BROKER_USER, BROKER_PASS)) {
            Serial.println("MQTT reconnected!");
            gate.setState(gate.getCurrentState(), true);
            return true;
        }
        Serial.println("MQTT reconnect failed");
        return false;
    }
    return true;
}

void cameraTimerCb(){
  camera.setValue("off");
  digitalWrite(relay3, LOW);
  cameraTimer.stop();
}

void gateTimerCb(){
  gate.setState(false);
  digitalWrite(relay1, LOW);
  gateTimer.stop();
}

void doorTimerCb(){
  digitalWrite(relay2, LOW);
  doorTimer.stop();
}

bool reconnectEthernet() {
    if (Ethernet.linkStatus() == LinkOFF) {
        Serial.println("Ethernet link down. Reinitializing...");
        Ethernet.begin(mac);
        delay(2000);
        return true;
    }
    return false;
}

// ----------------------- SWITCH CALLBACK ------------------------
void onSwitchCommand(bool state, HASwitch* sender)
{
    Serial.println("Got MQTT Command");
    if (sender == &gate) {
        Serial.println(String("Gate -> ") + (state ? "on" : "off"));
        digitalWrite(relay1, state);
        gateTimer.stop();
        gateTimer.start();
    }
    sender->setState(state, true);
}

void onButtonCommand(HAButton* sender){
    Serial.println("Got MQTT Command");
    if(sender == &restart){
      Serial.println("Restarting...");
      NVIC_SystemReset();
    }else if(sender == &cameraOn){
      Serial.println("Camera On");
      camera.setValue("on");
      digitalWrite(relay3, HIGH);
      cameraTimer.stop();
      cameraTimer.start();
    }else if(sender == &cameraOff){
      Serial.println("Camera Off");
      camera.setValue("off");
      digitalWrite(relay3, LOW);
      cameraTimer.stop();
    } else if (sender == &door) {
        Serial.println("Door On");
        digitalWrite(relay2, HIGH);
        doorTimer.stop();
        doorTimer.start();
    } 
}

// --------------------------- SETUP ------------------------------
void setup() {
    pinMode(relay1, OUTPUT);
    pinMode(relay2, OUTPUT);
    pinMode(relay3, OUTPUT);
    pinMode(relay4, OUTPUT);

    Serial.begin(9600);
    Serial1.begin(9600);

    Ethernet.init(5);
    Ethernet.begin(mac);
    delay(1000);
    ArduinoOTA.begin(Ethernet.localIP(), "Arduino", "12345", InternalStorage);

    device.setName("Intercom");
    device.setManufacturer("Wasserman Inc.");
    device.setModel("Seeeduino XIAO");
    device.setSoftwareVersion("1.0.1");

    door.setName("Door");
    door.setIcon("mdi:door");
    door.onCommand(onButtonCommand);

    gate.setName("Gate");
    gate.setIcon("mdi:gate");
    gate.onCommand(onSwitchCommand);    

    camera.setName("Camera");
    camera.setIcon("mdi:cctv");
    camera.setValue("off");

    cameraOn.setName("Camera On");
    cameraOn.setIcon("mdi:cctv");
    cameraOn.onCommand(onButtonCommand);

    cameraOff.setName("Camera Off");
    cameraOff.setIcon("mdi:cctv");
    cameraOff.onCommand(onButtonCommand);

    restart.setName("Restart");
    restart.setIcon("mdi:restart");
    restart.onCommand(onButtonCommand);

    mqtt.begin(BROKER_ADDR, BROKER_USER, BROKER_PASS);

    device.enableSharedAvailability();
    device.enableLastWill();
    device.setAvailability(true);
}

// ---------------------------- LOOP ------------------------------
void loop() {
    ArduinoOTA.handle();
    // Check Ethernet
    reconnectEthernet();

    // MQTT Reconnect every 5 Seconds if disconnected
    if (!mqtt.isConnected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            reconnectMQTT();
        }
    }

    mqtt.loop();
    gateTimer.update();
    doorTimer.update();
    cameraTimer.update();
}
