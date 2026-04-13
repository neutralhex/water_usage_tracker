#include <Arduino.h>
#include <WiFi.h>
#include <cstdint>
#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "freertos/portmacro.h"
#include "secrets.h"

static volatile bool wifi_connected = false;
static volatile uint16_t reed_trigger_count = 0;

static void wifi_event_cb(WiFiEvent_t event);
static void status_led_task(void *parameters);
static void influxdb_http_post_task(void *parameters);
static void reed_poll_task(void *parameters);

typedef enum {
    IDLE,
    DEBOUNCING
} state_t;

void setup() {
    Serial.begin(BAUD_RATE);

    pinMode(LED, OUTPUT);
    pinMode(REED_SWITCH, INPUT_PULLUP);

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

static void wifi_event_cb(WiFiEvent_t event) {
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

static void status_led_task(void *parameters) {
    uint16_t delay;

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

void reed_poll_task(void *parameters) {
    const TickType_t period_ms = 10;
    const TickType_t debounce_ms = 15;

    state_t state = IDLE;
    TickType_t last_change_time = 0;

    int8_t last_stable_level = HIGH;

    while (1) {
        int8_t level = digitalRead(REED_SWITCH);
        TickType_t now = xTaskGetTickCount();

        switch (state) {
            case IDLE:
                if (last_stable_level == HIGH && level == LOW) {
                    state = DEBOUNCING;
                    last_change_time = now;
                }
            break;
            case DEBOUNCING:
                if ((now - last_change_time) >= debounce_ms) {

                    int stable_level = digitalRead(REED_SWITCH);

                    if (stable_level == LOW && last_stable_level == HIGH) {
                        last_stable_level = LOW;

                        reed_trigger_count++;
                    } else {
                        last_stable_level = stable_level;
                    }

                    state = IDLE;
                }
            break;
        }
    }

    vTaskDelay(period_ms);
}
