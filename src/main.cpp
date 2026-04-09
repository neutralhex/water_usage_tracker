#include <Arduino.h>
#include <WiFi.h>
#include "secrets.h"
#include "config.h"

volatile int reed_trigger_count = 0;
volatile unsigned long last_interrupt_time = 0;
unsigned long last_send_time_ms = 0;

void connect_wifi(const char *ssid, const char *password);
void upload_to_influxdb();

void IRAM_ATTR reed_switch_isr();

void setup() {
    Serial.begin(115200);

    // this will block forever as 
    // there is no point in not having wifi
    //connect_wifi(SSID, PASSWORD);

    pinMode(REED_SWITCH, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(REED_SWITCH), reed_switch_isr, FALLING);
}

void loop() {
    unsigned long now = millis();
    if (now - last_send_time_ms > DATA_UPLOAD_INTERVAL_MS) {
        last_send_time_ms = now;
        upload_to_influxdb();
        reed_trigger_count = 0;
    }
}

void connect_wifi(const char *ssid, const char *password) {
    WiFi.begin(ssid, password);

    Serial.print("Connecting to wifi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("Connected");
    Serial.println(WiFi.localIP());
}

void IRAM_ATTR reed_switch_isr() {
    unsigned long now = millis();

    if (now - last_interrupt_time >= REED_SWITCH_DEBOUNCE_MS) {
        reed_trigger_count++;
        last_interrupt_time = now; 
    }
}

void upload_to_influxdb() {
    Serial.println("Sending data to influxdb");
    return;
}
