#include <Arduino.h>
#include <WiFi.h>
#include "IPAddress.h"
#include "config.h"
#include "secrets.h"

//WiFiClient wifi_client;
//HTTPClient http_client;


volatile bool wifi_connected = false;
volatile int reed_trigger_count = 0;
volatile unsigned long last_interrupt_time = 0;
unsigned long last_send_time_ms = 0;

void IRAM_ATTR reed_switch_isr();
void onWifiEvent(WiFiEvent_t event);

void setup() {
    Serial.begin(115200);

    WiFi.mode(WIFI_MODE_STA);
    WiFi.onEvent(onWifiEvent);
    WiFi.begin(SSID, PASSWORD);

    pinMode(REED_SWITCH, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(REED_SWITCH), reed_switch_isr, FALLING);
}

void loop() { }

void IRAM_ATTR reed_switch_isr() {
    unsigned long now = millis();

    if (now - last_interrupt_time >= REED_SWITCH_DEBOUNCE_MS) {
        reed_trigger_count++;
        last_interrupt_time = now; 
    }
}

void onWifiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_AP_START:
            Serial.println("Wifi started.");
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("Wifi connected.");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("IP received: ");
            Serial.println(WiFi.localIP());
            wifi_connected = true;
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("Wifi disconnected.");
            wifi_connected = false;
            break;
        default:
            break;
    }
}
