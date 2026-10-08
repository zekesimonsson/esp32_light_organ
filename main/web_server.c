#include "web_server.h"

#include <string.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "WEB";

static const char html_page[] =
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<style>"
    "body{background:#141b29;color:white;"
    "font-family:Arial;text-align:center;padding:30px}"
    "button{background:#3760aa;color:white;"
    "border:0;border-radius:8px;padding:16px;"
    "margin:8px;font-size:16px}"
    "</style>"
    "</head>"
    "<body>"
    "<h1>Light Organ</h1>"
    "<p>ESP32-S3 DMX Controller</p>"
    "<h2>Effect mode</h2>"
    "<button>Classic</button>"
    "<button>Beat Chase</button>"
    "<button>Alternate</button>"
    "<p>Web server is running!</p>"
    "</body>"
    "</html>";

static esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html_page,
                           HTTPD_RESP_USE_STRLEN);
}

void web_server_init(void)
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_ap();

    wifi_init_config_t wifi_cfg =
        WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(
        esp_wifi_init(&wifi_cfg));

    wifi_config_t ap_cfg = {
        .ap = {
            .ssid = "LightOrgan",
            .ssid_len = 0,
            .password = "LightOrgan123",
            .channel = 1,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(
        esp_wifi_set_mode(WIFI_MODE_AP));

    ESP_ERROR_CHECK(
        esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));

    ESP_ERROR_CHECK(esp_wifi_start());

    httpd_config_t http_cfg =
        HTTPD_DEFAULT_CONFIG();

    httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(
        httpd_start(&server, &http_cfg));

    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = NULL,
    };

    ESP_ERROR_CHECK(
        httpd_register_uri_handler(server, &root));

    ESP_LOGI(TAG, "Wi-Fi: LightOrgan");
    ESP_LOGI(TAG, "Open http://192.168.4.1");
}