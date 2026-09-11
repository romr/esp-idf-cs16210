#include "cs16210.h"
#include "esp_rom_sys.h"
#include "driver/gpio.h"

struct cs16210_dev_t {
    cs16210_config_t config;
    uint16_t shadow_buffer;
};

esp_err_t cs16210_init(cs16210_handle_t *out_handle, const cs16210_config_t *config)
{
    if (out_handle == NULL || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!GPIO_IS_VALID_OUTPUT_GPIO(config->cp_pin) ||
        !GPIO_IS_VALID_OUTPUT_GPIO(config->din_pin) ||
        !GPIO_IS_VALID_OUTPUT_GPIO(config->en_pin)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (config->r_pin != GPIO_NUM_NC && !GPIO_IS_VALID_OUTPUT_GPIO(config->r_pin)) {
        return ESP_ERR_INVALID_ARG;
    }

    cs16210_handle_t dev = (cs16210_handle_t)calloc(1, sizeof(struct cs16210_dev_t));
    if (dev == NULL) {
        return ESP_ERR_NO_MEM;
    }
    
    dev->config = *config;

    uint64_t pin_mask = (1ULL << config->cp_pin) |
                         (1ULL << config->din_pin) |
                         (1ULL << config->en_pin);

    if (config->r_pin != GPIO_NUM_NC) {
        pin_mask |= (1ULL << config->r_pin);
    }
    
    // Configure GPIO pins as outputs
    gpio_config_t io_conf = {
        .pin_bit_mask = pin_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        free(dev);
        return ret;
    };

    // Set default idle signal levels
    gpio_set_level(config->cp_pin, 1);  
    gpio_set_level(config->din_pin, 0); 
    gpio_set_level(config->en_pin, 1);

    if (config->r_pin != GPIO_NUM_NC) {
        gpio_set_level(config->r_pin, 1);
        cs16210_reset(dev);
    }

    *out_handle = dev;
    return cs16210_clear(dev);
}

esp_err_t cs16210_deinit(cs16210_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    cs16210_clear(handle);
    free(handle);
    return ESP_OK;
}

esp_err_t cs16210_reset(cs16210_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (handle->config.r_pin == GPIO_NUM_NC) {
        return ESP_ERR_INVALID_STATE;
    }

    gpio_set_level(handle->config.r_pin, 0);
    esp_rom_delay_us(10);
    gpio_set_level(handle->config.r_pin, 1);
    esp_rom_delay_us(10);

    return ESP_OK;
}

esp_err_t cs16210_write(cs16210_handle_t handle, uint16_t data)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Gate output drivers (En=1) to eliminate switching noise/ghosting
    gpio_set_level(handle->config.en_pin, 1); 

    // Bit-bang 16 bits MSB first
    for (int i = 15; i >= 0; i--) {
        uint8_t bit = (data >> i) & 0x01;
        
        // Present data bit on Din while Cp is HIGH
        gpio_set_level(handle->config.din_pin, bit);
        esp_rom_delay_us(1); // Setup time (>100ns required per datasheet)

        // Falling edge (1 -> 0) triggers sampling by the IC
        gpio_set_level(handle->config.cp_pin, 0);
        esp_rom_delay_us(1); // Clock pulse low width (>400ns required)

        // Return Cp to HIGH state for the next clock cycle
        gpio_set_level(handle->config.cp_pin, 1);
        esp_rom_delay_us(1); 
    }

    // Enable high-voltage drivers (En=0) to present shifted data
    gpio_set_level(handle->config.en_pin, 0);
    handle->shadow_buffer = data;

    return ESP_OK;
}

esp_err_t cs16210_clear(cs16210_handle_t handle) {
    return cs16210_write(handle, 0x0000);
}