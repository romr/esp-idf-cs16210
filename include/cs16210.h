#pragma once

#include "driver/gpio.h" // IWYU pragma: keep
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque handle representing a CS16210 driver instance.
 */
typedef struct cs16210_dev_t* cs16210_handle_t;

/**
 * @brief Configuration structure for the CS16210 VFD driver pins.
 */
typedef struct {
    gpio_num_t cp_pin;   /*!< Clock pulse pin (Cp), data sampled on falling edge */
    gpio_num_t din_pin;  /*!< Serial data input pin (Din) */
    gpio_num_t en_pin;   /*!< Output enable pin (En), active LOW */
    gpio_num_t r_pin;    /**< Reset pin (R), active LOW. Set to GPIO_NUM_NC if hardwired to VDD */
} cs16210_config_t;

/**
 * @brief Initialize CS16210 driver and allocate instance handle.
 *
 * Validates output GPIO pins, allocates internal handle state, sets initial line levels,
 * triggers hardware reset if `r_pin` is configured, and clears output registers.
 *
 * @param[out] out_handle Pointer to receive the allocated driver handle.
 * @param[in]  config     Pointer to configuration structure.
 *
 * @return
 *     - ESP_OK: Driver initialized successfully.
 *     - ESP_ERR_INVALID_ARG: Null pointer or invalid/non-output GPIO assigned.
 *     - ESP_ERR_NO_MEM: Memory allocation for handle failed.
 */
esp_err_t cs16210_init(cs16210_handle_t *out_handle, const cs16210_config_t *config);

/**
 * @brief Deinitialize CS16210 driver and free allocated resources.
 *
 * @param[in] handle Driver instance handle.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if handle is NULL.
 */
esp_err_t cs16210_deinit(cs16210_handle_t handle);

/**
 * @brief Trigger a hardware reset sequence via the R pin.
 *
 * @param[in] handle Driver instance handle.
 * @return 
 *     - ESP_OK: Reset pulse sent.
 *     - ESP_ERR_INVALID_STATE: Reset pin set to GPIO_NUM_NC.
 *     - ESP_ERR_INVALID_ARG: Invalid handle.
 */
esp_err_t cs16210_reset(cs16210_handle_t handle);

/**
 * @brief Write a 16-bit word to output channels (Out16..Out1).
 *
 * Transmits MSB first. Disables output (En=1) while shifting bits to eliminate ghosting,
 * then latches data to outputs (En=0).
 *
 * @param[in] handle Driver instance handle.
 * @param[in] data   16-bit channel mask (Bit 15 = Out16, Bit 0 = Out1).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if handle is NULL.
 */
esp_err_t cs16210_write(cs16210_handle_t handle, uint16_t data);

/**
 * @brief Turn off all VFD output channels.
 *
 * @param[in] handle Driver instance handle.
 * @return ESP_OK on success.
 */
esp_err_t cs16210_clear(cs16210_handle_t handle);

#ifdef __cplusplus
}
#endif