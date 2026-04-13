#ifndef CONFIG_H
#define CONFIG_H

#define LED 2
#define REED_SWITCH 5
#define REED_SWITCH_DEBOUNCE_MS 10
#define DATA_UPLOAD_INTERVAL_MS 60 * 1000
#define BAUD_RATE 115200

const char *influx_server = "http://localhost:8086";

#endif
