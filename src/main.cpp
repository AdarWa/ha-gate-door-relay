#include <Arduino.h>
#include <Ethernet.h>
#include <ArduinoHA.h>
#include "secrets.h"

#define relay1 3
#define relay2 2
#define relay3 1
#define relay4 0

byte mac[] = {0x00, 0x10, 0xFA, 0x6E, 0x38, 0x4A}; //17:4a:7a:10:ba:50

EthernetClient client;
HADevice device(mac, sizeof(mac));
HAMqtt mqtt(client, device);

HASwitch door("mainDoor");
HASwitch gate("mainGate");
HASwitch camera("camera");

void onSwitchCommand(bool state, HASwitch* sender)
{
    Serial.println("Got MQTT Command");
    if (sender == &door) {
        Serial.println("Got MQTT command for door: " + state ? "ON" : "OFF");
        digitalWrite(relay2, state);
    } else if (sender == &gate) {
        Serial.println("Got MQTT command for gate: " + state ? "ON" : "OFF");
        digitalWrite(relay1, state);
    } else if (sender == &camera){
      Serial.println("Got MQTT command for camera: " + state ? "ON" : "OFF");
      digitalWrite(relay3, state);
    }

    sender->setState(state);
}

void setup() {
    pinMode(relay1, OUTPUT);
    pinMode(relay2, OUTPUT);
    pinMode(relay3, OUTPUT);
    pinMode(relay4, OUTPUT);
    Serial.begin(9600);
    Serial1.begin(9600);
    Ethernet.init(5);
    Ethernet.begin(mac);

    device.setName("Intercom");
    device.setManufacturer("Wasserman Inc.");
    device.setModel("Seeeduino XIAO");
    device.setSoftwareVersion("1.0.0");

    door.setName("Door");
    door.setIcon("mdi:door");
    door.onCommand(onSwitchCommand);

    gate.setName("Gate");
    gate.setIcon("mdi:gate");
    gate.onCommand(onSwitchCommand);    

    camera.setName("Camera");
    camera.setIcon("mdi:cctv");
    camera.onCommand(onSwitchCommand);

    mqtt.begin(BROKER_ADDR, BROKER_USER, BROKER_PASS);
}

void loop() {
    while(Serial1.available() > 0){
      Serial.print(Serial1.read());
    }
    Ethernet.maintain();
    mqtt.loop();
}