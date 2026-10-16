#include "light_control.h"

#include "audio_analyzer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "nvs.h"
#include "esp_log.h"

#define LIGHT_CONTROL_QUEUE_LENGTH 10

static QueueHandle_t command_queue = NULL;
static SemaphoreHandle_t status_mutex = NULL;

static light_control_status_t current_status = {
    .mode = LIGHT_EFFECT_ALTERNATE,
    .brightness = 30,
    .beat_brightness = 45,
    .blackout = false,
    .noise_gate = true
};

static const char *TAG = "LIGHT_CTRL";

static void load_settings(void)
{
    nvs_handle_t handle;

    esp_err_t err = nvs_open(
        "light_organ",
        NVS_READONLY,
        &handle);

    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGI(TAG, "No saved settings, using defaults");
        return;
    }

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS open failed: %s",
                 esp_err_to_name(err));
        return;
    }

    uint8_t value;

    if (nvs_get_u8(handle, "mode", &value) == ESP_OK &&
        value < LIGHT_EFFECT_COUNT)
    {
        current_status.mode = (light_effect_mode_t)value;
    }

    if (nvs_get_u8(handle, "brightness", &value) == ESP_OK &&
        value <= 100)
    {
        current_status.brightness = value;
    }

    if (nvs_get_u8(handle, "beat_bright", &value) == ESP_OK &&
        value <= 100)
    {
        current_status.beat_brightness = value;
    }

    nvs_close(handle);

    ESP_LOGI(TAG, "Loaded: mode=%d brightness=%u beat=%u",
             (int)current_status.mode,
             (unsigned)current_status.brightness,
             (unsigned)current_status.beat_brightness);
}

static void save_settings(void)
{
    nvs_handle_t handle;

    esp_err_t err = nvs_open(
        "light_organ",
        NVS_READWRITE,
        &handle);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS open failed: %s",
                 esp_err_to_name(err));
        return;
    }

    light_control_status_t status;
    light_control_get_status(&status);

    err = nvs_set_u8(handle, "mode", (uint8_t)status.mode);

    if (err == ESP_OK)
        err = nvs_set_u8(handle, "brightness", status.brightness);

    if (err == ESP_OK)
        err = nvs_set_u8(handle, "beat_bright", status.beat_brightness);

    if (err == ESP_OK)
        err = nvs_commit(handle);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to save settings: %s",
                 esp_err_to_name(err));
    }

    nvs_close(handle);
}

void light_control_init(void)
{
    command_queue = xQueueCreate(
        LIGHT_CONTROL_QUEUE_LENGTH,
        sizeof(light_control_command_t));

    status_mutex = xSemaphoreCreateMutex();

    configASSERT(command_queue != NULL);
    configASSERT(status_mutex != NULL);
    
    load_settings();

    light_effects_set_mode(current_status.mode);
    light_effects_set_brightness(current_status.brightness);
    light_effects_set_beat_brightness(
        current_status.beat_brightness);
    audio_analyzer_set_noise_gate(current_status.noise_gate);
}

bool light_control_send(
    const light_control_command_t *command)
{
    if ((command_queue == NULL) || (command == NULL))
    {
        return false;
    }

    return xQueueSend(
        command_queue,
        command,
        0) == pdTRUE;
}

void light_control_process(void)
{
    light_control_command_t command;

    if (command_queue == NULL)
    {
        return;
    }

    while (xQueueReceive(
        command_queue,
        &command,
        0) == pdTRUE)
    {
        bool valid = true;

        xSemaphoreTake(status_mutex, portMAX_DELAY);

        switch (command.type)
        {
            case LIGHT_CONTROL_SET_MODE:
                if (command.value < LIGHT_EFFECT_COUNT)
                {
                    current_status.mode =
                        (light_effect_mode_t)command.value;

                    light_effects_set_mode(
                        current_status.mode);
                }
                else
                {
                    valid = false;
                }
                break;

            case LIGHT_CONTROL_SET_BRIGHTNESS:
                if (command.value <= 100)
                {
                    current_status.brightness = command.value;
                    light_effects_set_brightness(command.value);
                }
                else
                {
                    valid = false;
                }
                break;

            case LIGHT_CONTROL_SET_BEAT_BRIGHTNESS:
                if (command.value <= 100)
                {
                    current_status.beat_brightness = command.value;
                    light_effects_set_beat_brightness(command.value);
                }
                else
                {
                    valid = false;
                }
                break;

            case LIGHT_CONTROL_SET_BLACKOUT:
                if (command.value <= 1)
                {
                    current_status.blackout =
                        (command.value != 0);
                }
                else
                {
                    valid = false;
                }
                break;

            case LIGHT_CONTROL_SET_NOISE_GATE:
                if (command.value <= 1)
                {
                    current_status.noise_gate =
                        (command.value != 0);

                    audio_analyzer_set_noise_gate(
                        current_status.noise_gate);
                }
                else
                {
                    valid = false;
                }
                break;

            default:
                valid = false;
                break;
        }

        xSemaphoreGive(status_mutex);

        if (valid &&
            command.type != LIGHT_CONTROL_SET_BLACKOUT &&
            command.type != LIGHT_CONTROL_SET_NOISE_GATE)
        {
            save_settings();
        }
    }
}

void light_control_get_status(
    light_control_status_t *status)
{
    if ((status == NULL) || (status_mutex == NULL))
    {
        return;
    }

    xSemaphoreTake(status_mutex, portMAX_DELAY);
    *status = current_status;
    xSemaphoreGive(status_mutex);
}