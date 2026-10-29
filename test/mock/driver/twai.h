#pragma once
#include <cstdint>
typedef int esp_err_t; typedef int gpio_num_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define pdMS_TO_TICKS(x) (x)
typedef enum { TWAI_MODE_NORMAL, TWAI_MODE_NO_ACK, TWAI_MODE_LISTEN_ONLY } twai_mode_t;
typedef enum { TWAI_STATE_STOPPED, TWAI_STATE_RUNNING, TWAI_STATE_BUS_OFF, TWAI_STATE_RECOVERING } twai_state_t;
typedef struct { union { struct { uint32_t extd : 1; uint32_t rtr : 1; }; uint32_t flags; }; uint32_t identifier; uint8_t data_length_code; uint8_t data[8]; } twai_message_t;
typedef struct { int tx, rx; twai_mode_t mode; int rx_queue_len, tx_queue_len; } twai_general_config_t;
typedef struct { int r; } twai_timing_config_t;
typedef struct { int f; } twai_filter_config_t;
typedef struct { twai_state_t state; } twai_status_info_t;
#define TWAI_GENERAL_CONFIG_DEFAULT(t, r, m) {t, r, m, 5, 5}
#define TWAI_TIMING_CONFIG_500KBITS() {500}
#define TWAI_TIMING_CONFIG_250KBITS() {250}
#define TWAI_FILTER_CONFIG_ACCEPT_ALL() {0}
esp_err_t twai_driver_install(const twai_general_config_t *, const twai_timing_config_t *, const twai_filter_config_t *);
esp_err_t twai_driver_uninstall();
esp_err_t twai_start();
esp_err_t twai_stop();
esp_err_t twai_transmit(const twai_message_t *, uint32_t);
esp_err_t twai_receive(twai_message_t *, uint32_t);
esp_err_t twai_get_status_info(twai_status_info_t *);
esp_err_t twai_initiate_recovery();
