#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#define SNIFF_CHANNEL 6   // Wi-Fi channel to listen on (find it with wifi_scan)

static void wifi_sniffer_packet_handler(void *buf, wifi_promiscuous_pkt_type_t type)
{
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    wifi_pkt_rx_ctrl_t *ctrl = &pkt->rx_ctrl;

    const char *type_str;
    switch (type) {
        case WIFI_PKT_MGMT: type_str = "MGMT"; break;
        case WIFI_PKT_DATA: type_str = "DATA"; break;
        case WIFI_PKT_CTRL: type_str = "CTRL"; break;
        default:            type_str = "MISC"; break;
    }

    // The transmitter MAC (802.11 address 2) starts 10 bytes into the frame
    const uint8_t *payload = pkt->payload;

    printf("[%s] len=%4d rssi=%4d ch=%2d | src_mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
           type_str,
           ctrl->sig_len,
           ctrl->rssi,
           ctrl->channel,
           payload[10], payload[11], payload[12],
           payload[13], payload[14], payload[15]);
}

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

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));  // neither STA nor AP - pure sniffer
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_wifi_set_channel(SNIFF_CHANNEL, WIFI_SECOND_CHAN_NONE));

    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler));

    printf("\n=== Sniffing on channel %d ===\n\n", SNIFF_CHANNEL);

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}