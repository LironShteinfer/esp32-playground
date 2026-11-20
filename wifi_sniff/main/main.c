#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#define SNIFF_CHANNEL 6   // Wi-Fi channel to listen on (find it with wifi_scan)

#define PKT_QUEUE_LEN 64

/* Only what we print, copied out of the Wi-Fi driver's buffer. */
typedef struct {
    wifi_promiscuous_pkt_type_t type;
    uint16_t len;
    int8_t   rssi;
    uint8_t  channel;
    bool     has_src;      /* frame is long enough to contain address 2 */
    uint8_t  src_mac[6];
} pkt_info_t;

static QueueHandle_t pkt_queue;
static volatile uint32_t dropped_pkts = 0;

/* Runs in the Wi-Fi driver's task: keep it short - copy the fields and return.
 * Printing here would slow the driver down and lose frames. */
static void wifi_sniffer_packet_handler(void *buf, wifi_promiscuous_pkt_type_t type)
{
    const wifi_promiscuous_pkt_t *pkt = (const wifi_promiscuous_pkt_t *)buf;
    pkt_info_t info = {
        .type    = type,
        .len     = pkt->rx_ctrl.sig_len,
        .rssi    = pkt->rx_ctrl.rssi,
        .channel = pkt->rx_ctrl.channel,
    };

    // The transmitter MAC (802.11 address 2) is at bytes 10..15 of the frame.
    // Short control frames (ACK, CTS) end before that, so check the length first.
    info.has_src = (info.len >= 16);
    if (info.has_src) {
        memcpy(info.src_mac, &pkt->payload[10], sizeof(info.src_mac));
    }

    if (xQueueSend(pkt_queue, &info, 0) != pdTRUE) {
        dropped_pkts++;     // queue full: the printer task is behind
    }
}

static void print_task(void *arg)
{
    pkt_info_t info;
    uint32_t last_dropped = 0;

    while (1) {
        if (xQueueReceive(pkt_queue, &info, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        const char *type_str;
        switch (info.type) {
            case WIFI_PKT_MGMT: type_str = "MGMT"; break;
            case WIFI_PKT_DATA: type_str = "DATA"; break;
            case WIFI_PKT_CTRL: type_str = "CTRL"; break;
            default:            type_str = "MISC"; break;
        }

        if (info.has_src) {
            printf("[%s] len=%4u rssi=%4d ch=%2u | src_mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
                   type_str, info.len, info.rssi, info.channel,
                   info.src_mac[0], info.src_mac[1], info.src_mac[2],
                   info.src_mac[3], info.src_mac[4], info.src_mac[5]);
        } else {
            printf("[%s] len=%4u rssi=%4d ch=%2u | src_mac=(none)\n",
                   type_str, info.len, info.rssi, info.channel);
        }

        if (dropped_pkts != last_dropped) {
            last_dropped = dropped_pkts;
            printf("(%lu frames dropped so far - output is slower than the traffic)\n",
                   (unsigned long)last_dropped);
        }
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

    pkt_queue = xQueueCreate(PKT_QUEUE_LEN, sizeof(pkt_info_t));
    if (pkt_queue == NULL) {
        printf("Failed to create the packet queue\n");
        return;
    }
    xTaskCreate(print_task, "print_task", 4096, NULL, 5, NULL);

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