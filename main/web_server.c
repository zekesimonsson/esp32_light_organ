#include "web_server.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "light_control.h"
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
    "font-family:Arial;text-align:center;padding:25px}"
    "button{background:#3760aa;color:white;"
    "border:0;border-radius:8px;padding:14px;"
    "margin:5px;font-size:15px}"
    "input{width:85%;max-width:350px}"
    ".selected{background:#16836f}"
    "#blackoutBtn{background:#a52b36;margin-top:22px;"
    "padding:17px 35px;font-weight:bold}"
    "#blackoutBtn.active{background:#16836f}"    
    "#noiseGateBtn{background:#16836f;min-width:180px}"
    "#noiseGateBtn.off{background:#a52b36}"
    "</style>"
    "</head>"
    "<body>"
    "<h1>Zeke's Light Machine</h1>"
    "<p>ESP32-S3 DMX Controller</p>"

    "<h2>Effect mode</h2>"
    "<button id='m0' onclick='setMode(0)'>Classic</button>"
    "<button id='m1' onclick='setMode(1)'>Beat Chase</button>"
    "<button id='m2' onclick='setMode(2)'>Alternate</button>"
    "<button id='m3' onclick='setMode(3)'>AUTO</button>"
    "<button id='m4' onclick='setMode(4)'>Color Chase</button>"
    "<button id='m5' onclick='setMode(5)'>Color Wave</button>"
    "<button id='m6' onclick='setMode(6)'>Color Chase Flash</button>"
    "<button id='m7' onclick='setMode(7)'>Frequency Spectrum</button>"
    "<button id='m8' onclick='setMode(8)'>Spectrum Chase</button>"

    "<h2>Master brightness: <span id='bv'>30</span>%</h2>"
    "<input id='brightness' type='range' min='0' max='100' value='30'"
    " oninput='document.getElementById(\"bv\").textContent=this.value'"
    " onchange='send(\"/api/brightness\",this.value)'>"

    "<h2>Beat brightness: <span id='wv'>45</span>%</h2>"
    "<input id='beatBrightness' type='range' min='0' max='100' value='45'"
    " oninput='document.getElementById(\"wv\").textContent=this.value'"
    " onchange='send(\"/api/beat-brightness\",this.value)'>"
    "<h2>Audio settings</h2>"
    
    "<div>"
    "<button id='noiseGateBtn' onclick='toggleNoiseGate()'>"
    "Noise Gate: ON</button>"
    "</div>"
    
    "<div>"
    "<button id='blackoutBtn' onclick='toggleBlackout()'>BLACKOUT</button>"
    "</div>"

    "<p id='status'>Connecting...</p>"

    "<script>"

    "let blackout=false;"

    "let noiseGate=true;"

    "function updateNoiseGateButton(){"
    "const b=document.getElementById('noiseGateBtn');"
    "b.textContent=noiseGate?'Noise Gate: ON':'Noise Gate: OFF';"
    "b.classList.toggle('off',!noiseGate);"
    "}"

    "async function toggleNoiseGate(){"
    "const newState=!noiseGate;"
    "if(await send('/api/noise-gate',newState?1:0)){"
    "noiseGate=newState;"
    "updateNoiseGateButton();"
    "}}"

    "async function send(url,value){"
    "try{"
    "let r=await fetch(url,{method:'POST',body:String(value)});"
    "if(!r.ok)throw Error('HTTP '+r.status);"
    "document.getElementById('status').textContent='Updated';"
    "return true;"
    "}catch(e){"
    "document.getElementById('status').textContent='Error: '+e.message;"
    "return false;"
    "}}"

    "function updateMode(mode){"
    "document.querySelectorAll('[id^=m]').forEach("
    "b=>b.classList.remove('selected'));"
    "document.getElementById('m'+mode).classList.add('selected');"
    "}"

    "async function setMode(mode){"
    "if(await send('/api/mode',mode)){"
    "updateMode(mode);"
    "}}"

    "function updateBlackoutButton(){"
    "const b=document.getElementById('blackoutBtn');"
    "b.textContent=blackout?'ENABLE LIGHTS':'BLACKOUT';"
    "b.classList.toggle('active',blackout);"
    "}"

    "async function toggleBlackout(){"
    "const newState=!blackout;"
    "if(await send('/api/blackout',newState?1:0)){"
    "blackout=newState;"
    "updateBlackoutButton();"
    "}}"

    "async function loadStatus(){"
    "try{"
    "const r=await fetch('/api/status');"
    "if(!r.ok)throw Error('HTTP '+r.status);"
    "const s=await r.json();"
    "updateMode(s.mode);"
    "document.getElementById('brightness').value=s.brightness;"
    "document.getElementById('bv').textContent=s.brightness;"
    "document.getElementById('beatBrightness').value=s.beat_brightness;"
    "document.getElementById('wv').textContent=s.beat_brightness;"
    "blackout=s.blackout;"
    "updateBlackoutButton();"
    "noiseGate=s.noise_gate;"
    "updateNoiseGateButton();"
    "document.getElementById('status').textContent='Ready';"
    "}catch(e){"
    "document.getElementById('status').textContent='Status error: '+e.message;"
    "}}"

    "loadStatus();"

    "</script>"
    "</body>"
    "</html>";

static esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html_page,
                           HTTPD_RESP_USE_STRLEN);
}

static esp_err_t control_handler(httpd_req_t *req)
{
    char buffer[16];

    int length = httpd_req_recv(
        req, buffer, sizeof(buffer) - 1);

    if (length <= 0)
    {
        return ESP_FAIL;
    }

    buffer[length] = '\0';

    char *end;
    long value = strtol(buffer, &end, 10);

    if ((end == buffer) || (*end != '\0') ||
        (value < 0) || (value > 100))
    {
        httpd_resp_send_err(
            req, HTTPD_400_BAD_REQUEST,
            "Invalid value");
        return ESP_OK;
    }

    light_control_command_t command = {
        .value = (uint8_t)value
    };

    if (strcmp(req->uri, "/api/mode") == 0)
    {
        if (value >= LIGHT_EFFECT_COUNT)
        {
            httpd_resp_send_err(
                req, HTTPD_400_BAD_REQUEST,
                "Invalid mode");
            return ESP_OK;
        }

        command.type = LIGHT_CONTROL_SET_MODE;
    }
    else if (strcmp(req->uri, "/api/brightness") == 0)
    {
        command.type = LIGHT_CONTROL_SET_BRIGHTNESS;
    }
    else if (strcmp(req->uri, "/api/beat-brightness") == 0)
    {
        command.type = LIGHT_CONTROL_SET_BEAT_BRIGHTNESS;
    }
    else if (strcmp(req->uri, "/api/blackout") == 0)
    {
        if (value > 1)
        {
            httpd_resp_send_err(
                req, HTTPD_400_BAD_REQUEST,
                "Invalid blackout value");
            return ESP_OK;
        }

        command.type = LIGHT_CONTROL_SET_BLACKOUT;
    }
    else if (strcmp(req->uri, "/api/noise-gate") == 0)
    {
        if (value > 1)
        {
            httpd_resp_send_err(
                req, HTTPD_400_BAD_REQUEST,
                "Invalid noise gate value");
            return ESP_OK;
        }

        command.type = LIGHT_CONTROL_SET_NOISE_GATE;
    }
    else
    {
        httpd_resp_send_err(
            req, HTTPD_404_NOT_FOUND,
            "Unknown command");
        return ESP_OK;
    }

    if (!light_control_send(&command))
    {
        httpd_resp_send_err(
            req, HTTPD_500_INTERNAL_SERVER_ERROR,
            "Command queue full");
        return ESP_OK;
    }

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, "OK");

    return ESP_OK;
}

static esp_err_t status_handler(httpd_req_t *req)
{
    light_control_status_t status;
    light_control_get_status(&status);

    char response[160];

    snprintf(
        response,
        sizeof(response),
        "{"
        "\"mode\":%d,"
        "\"brightness\":%u,"
        "\"beat_brightness\":%u,"
        "\"blackout\":%s,"
        "\"noise_gate\":%s"
        "}",
        (int)status.mode,
        (unsigned)status.brightness,
        (unsigned)status.beat_brightness,
        status.blackout ? "true" : "false",
        status.noise_gate ? "true" : "false");


    httpd_resp_set_type(req, "application/json");

    return httpd_resp_sendstr(req, response);
}

void web_server_init(void)
{
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

    httpd_uri_t mode_uri = {
        .uri = "/api/mode",
        .method = HTTP_POST,
        .handler = control_handler,
    };

    httpd_uri_t brightness_uri = {
        .uri = "/api/brightness",
        .method = HTTP_POST,
        .handler = control_handler,
    };

    httpd_uri_t beat_uri = {
        .uri = "/api/beat-brightness",
        .method = HTTP_POST,
        .handler = control_handler,
    };

    ESP_ERROR_CHECK(
        httpd_register_uri_handler(server, &mode_uri));

    ESP_ERROR_CHECK(
        httpd_register_uri_handler(server, &brightness_uri));

    ESP_ERROR_CHECK(
        httpd_register_uri_handler(server, &beat_uri));

    httpd_uri_t blackout_uri = {
        .uri = "/api/blackout",
        .method = HTTP_POST,
        .handler = control_handler,
    };

    ESP_ERROR_CHECK(
        httpd_register_uri_handler(server, &blackout_uri));

    httpd_uri_t noise_gate_uri = {
        .uri = "/api/noise-gate",
        .method = HTTP_POST,
        .handler = control_handler,
    };

    ESP_ERROR_CHECK(
        httpd_register_uri_handler(server, &noise_gate_uri));

    httpd_uri_t status_uri = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = status_handler,
    };

    ESP_ERROR_CHECK(
        httpd_register_uri_handler(server, &status_uri));
}