#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
            esp_bd_addr_t *addr = &param->scan_rst.bda;
            printf("Found device | MAC: %02x:%02x:%02x:%02x:%02x:%02x | RSSI: %4d dBm | Addr type: %d\n",
                   (*addr)[0], (*addr)[1], (*addr)[2], (*addr)[3], (*addr)[4], (*addr)[5],
                   param->scan_rst.rssi,
                   param->scan_rst.ble_addr_type);
        }
        break;
    }
    default:
        break;
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));

    esp_ble_ext_scan_params_t scan_params = {
        .own_addr_type  = BLE_ADDR_TYPE_PUBLIC,
        .filter_policy  = BLE_SCAN_FILTER_ALLOW_ALL,
        .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE,
        .cfg_mask       = ESP_BLE_GAP_EXT_SCAN_CFG_UNCODE_MASK,
        .uncoded_cfg = {
            .scan_type     = BLE_SCAN_TYPE_ACTIVE,
            .scan_interval = 0x50,
            .scan_window   = 0x30,
        },
    };
    ESP_ERROR_CHECK(esp_ble_gap_set_ext_scan_params(&scan_params));

    printf("\n=== Starting BLE Scan ===\n\n");
    ESP_ERROR_CHECK(esp_ble_gap_start_ext_scan(0, 0)); // Scan continuously.

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}