#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "secrets.h"

static volatile bool wifi_connected = false;
static volatile uint16_t reed_trigger_count = 0;
static QueueHandle_t reed_count_queue;

static void wifi_event_cb(WiFiEvent_t event);
static void status_led_task(void *parameters);
static void reed_poll_task(void *parameters);
static void aggregation_task(void *parameters);
static void influxdb_http_post_task(void *parameters);

typedef enum {
    IDLE,
    DEBOUNCING,
    TRIGGERED
} ReedSwitchState_t;

void setup() {
    Serial.begin(BAUD_RATE);

    WiFi.mode(WIFI_MODE_STA);
    WiFi.onEvent(wifi_event_cb);
    WiFi.begin(SSID, PASSWORD);

    pinMode(LED, OUTPUT);
    pinMode(REED_SWITCH, INPUT_PULLUP);

    reed_count_queue = xQueueCreate(QUEUE_LENGTH, sizeof(uint16_t));

    xTaskCreatePinnedToCore(
        status_led_task,
        "Status LED",
        1024,
        NULL,
        2,
        NULL,
        1
    );

    xTaskCreatePinnedToCore(
        reed_poll_task,
        "Polling the Reed Switch",
        2048,
        NULL,
        1,
        NULL,
        1
    );

    xTaskCreatePinnedToCore(
        aggregation_task,
        "Queues the current trigger count",
        2048,
        NULL,
        1,
        NULL,
        1
    );

    xTaskCreatePinnedToCore(
        influxdb_http_post_task,
        "Uploads the value to influx",
        4096,
        NULL,
        1,
        NULL,
        1
    );
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
    const TickType_t period = pdMS_TO_TICKS(10);
    const TickType_t debounce = pdMS_TO_TICKS(50);

    ReedSwitchState_t state = IDLE;
    TickType_t last_change_time = 0;

    while (1) {
        int8_t level = digitalRead(REED_SWITCH);
        TickType_t now = xTaskGetTickCount();

        switch (state) {
            case IDLE:
                if (level == LOW) {
                    state = DEBOUNCING;
                    last_change_time = now;
                }
            break;
            case DEBOUNCING:
                if (level == HIGH) {
                    state = IDLE;
                } else if (now - last_change_time >= debounce) {
                    state = TRIGGERED;
                }
             break;
            case TRIGGERED:
                if (level == HIGH) {
                    reed_trigger_count++;
                    Serial.printf("PULSE DETECTED, count: %d\n", reed_trigger_count);
                    state = IDLE;
                }
            break;
        }

        vTaskDelay(period);
    }
}

static void aggregation_task(void *parameters) {
    //consider renaming data upload interval
    const TickType_t delay = pdMS_TO_TICKS(DATA_UPLOAD_INTERVAL_MS);

    while(1) {
        vTaskDelay(delay);

        uint16_t value = reed_trigger_count;
        xQueueSend(reed_count_queue, &value, 0);
        reed_trigger_count = 0;
    }
}

static void influxdb_http_post_task(void *parameters) {
    const TickType_t waiting_period = pdMS_TO_TICKS(1000);
    uint16_t value;

    char auth_header[128];
    snprintf(auth_header, sizeof(auth_header),
             "Token %s",
             INFLUXDB_TOKEN);

    while(1) {
        if (xQueueReceive(reed_count_queue, &value, 0) == pdTRUE) {
            Serial.printf("Value received from queue: %d\n", value);
            WiFiClient wifi_client;
            HTTPClient http;

            http.begin(wifi_client, INFLUXDB_URL);
            http.addHeader("Content-Type", "text/plain");
            http.addHeader("Authorization", auth_header);

            char payload[128];
            snprintf(
                payload,
                sizeof(payload),
                "water_meter,device=%s reed_trigger_count=%di",
                DEVICE_NAME,
                value
             );

            Serial.println(payload);
            uint32_t response = http.POST(payload);
            if (response > 0) {
                Serial.print("HTTP Response: ");
                Serial.println(response);
            } else {
                Serial.print("Error: ");
                Serial.println(response);
            }

            http.end();
        }

        vTaskDelay(waiting_period);
    }
}
