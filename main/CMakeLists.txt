idf_component_register(
    SRCS
        "main.c"
        "audio_input.c"
        "audio_analyzer.c"
        "light_effects.c"
        "dmx_output.c"
        "web_server.c"

    INCLUDE_DIRS
        "."

    REQUIRES
        esp_driver_gpio
        esp_driver_i2s
        esp_driver_uart
        esp_wifi
        esp_event
        esp_netif
        esp_http_server
        nvs_flash
)