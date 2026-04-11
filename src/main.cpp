#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "secrets.h"

volatile bool wifi_connected = false;
volatile int reed_trigger_count = 0;
volatile unsigned long last_interrupt_time = 0;
unsigned long last_send_time_ms = 0;

void IRAM_ATTR reed_switch_isr();
void wifi_event_cb(WiFiEvent_t event);
void status_led_task(void *parameters);

void setup() {
    Serial.begin(115200);

    pinMode(LED, OUTPUT);
    pinMode(REED_SWITCH, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(REED_SWITCH), reed_switch_isr, FALLING);

    xTaskCreatePinnedToCore(
        status_led_task,
        "Status LED",
        1024,
        NULL,
        1,
        NULL,
        1
    );

    WiFi.mode(WIFI_MODE_STA);
    WiFi.onEvent(wifi_event_cb);
    WiFi.begin(SSID, PASSWORD);
}

void loop() { }

void IRAM_ATTR reed_switch_isr() {
    unsigned long now = millis();

    if (now - last_interrupt_time >= REED_SWITCH_DEBOUNCE_MS) {
        reed_trigger_count++;
        last_interrupt_time = now; 
    }
}

void wifi_event_cb(WiFiEvent_t event) {
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

void status_led_task(void *parameters) {
    int delay;

    while(1) {
        if (wifi_connected)
            delay = 2000;
        else
            delay = 100;

        digitalWrite(LED, 1);
        vTaskDelay(delay / portTICK_PERIOD_MS);
        digitalWrite(LED, 0);
        vTaskDelay(delay / portTICK_PERIOD_MS);
    }
}
