
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "wifi_scan";

void app_main(void)
{

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    printf("\n=== Starting Wi-Fi Scan ===\n\n");

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true
    };

    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));

    uint16_t ap_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    printf("Found %d access points:\n", ap_count);
    
    if (ap_count == 0) {
        printf("No networks found.\n");
    } else {
        wifi_ap_record_t *ap_list = malloc(sizeof(wifi_ap_record_t) * ap_count);
        if (ap_list == NULL) {
            ESP_LOGE(TAG, "Not enough memory for %d scan records", ap_count);
            return;
        }
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_list));

        for (int i = 0; i < ap_count; i++) {
            printf("%2d: SSID: %-32s | RSSI: %4d dBm | Channel: %2d | BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n",
                   i + 1,
                   ap_list[i].ssid,
                   ap_list[i].rssi,
                   ap_list[i].primary,
                   ap_list[i].bssid[0], ap_list[i].bssid[1], ap_list[i].bssid[2],
                   ap_list[i].bssid[3], ap_list[i].bssid[4], ap_list[i].bssid[5]);
        }
        free(ap_list);
    }

    printf("\n=== Wi-Fi Scan Complete ===\n");
}

