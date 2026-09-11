# CS16210 VFD Driver for ESP-IDF

A lightweight, bit-banged ESP-IDF driver for the **Semico CS16210EP (CD16210GP)** 16-bit high-voltage Vacuum Fluorescent Display (VFD) driver IC.

For details, see the [CS16210EP datasheet](https://www.alldatasheet.com/datasheet-pdf/pdf/116303/ETC1/CS16210EP.html).

## Overview

The CS16210EP is a 16-bit serial-in/parallel-out shift register designed for driving VFD displays with high-voltage outputs (up to -30V). This component provides a clean, flicker-free C API for ESP-IDF (v5.x+) using a 3-wire bit-banging interface (`Cp`, `Din`, `En`).

---

## Hardware Pinout & Wiring

> [!WARNING] 
> CS16210 operates on $V_{DD} = 5\text{V}$. Datasheet input threshold $V_{IH}$ is $0.7 \times V_{DD} = 3.5\text{V}$.
> ESP32 GPIO outputs strictly 3.3V. While some PCB revisions tolerate 3.3V signals directly, **a logic level shifter (3.3V $\to$ 5V) is strongly recommended** to prevent intermittent bit-flip errors in production environments.

| CS16210 Pin | Name | Type | Description | ESP32 Connection |
| :---: | :---: | :---: | :--- | :--- |
| **1** | `Cp` | Input | Clock Pulse (Samples data on falling edge) | Any Output GPIO |
| **2** | `Din` | Input | Serial Data Input | Any Output GPIO |
| **3** | `En` | Input | Output Enable (Active LOW) | Any Output GPIO |
| **12** | `Vdd` | Power | Logic Supply Voltage (+5V) | +5V Rail |
| **21** | `Vdisp` | Power | VFD High-Voltage Drive (-30V) | VFD Filament/Bias Power |
| **23** | `R` | Input | Reset Signal (Active LOW) | Dedicated GPIO **OR** Pull-up to +5V ($V_{DD}$) |
| **24** | `Vss` | Power | Logic Ground | GND |

---

## Quick Start Example
```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cs16210.h"

#define PIN_CP   GPIO_NUM_18
#define PIN_DIN  GPIO_NUM_19
#define PIN_EN   GPIO_NUM_21
#define PIN_R    GPIO_NUM_NC

void app_main(void) {
    // 1. Configure driver pins
    cs16210_config_t config = {
        .cp_pin = PIN_CP,
        .din_pin = PIN_DIN,
        .en_pin = PIN_EN,
        r_pin = PIN_R
    };

    cs16210_handle_t vfd_handle = NULL;

    // 2. Initialize driver
    if (cs16210_init(&vfd_handle, &config) != ESP_OK) {
        printf("CS16210 initialization failed!\n");
        return;
    }

    while (1) {
        // Enable Out1 through Out16 (0xFFFF = All Bits Set)
        cs16210_write(vfd_handle, 0xFFFF);
        vTaskDelay(pdMS_TO_TICKS(2000));

        // Clear all outputs
        cs16210_clear(vfd_handle);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    // 3. Cleanup
    cs16210_deinit(vfd_handle);
}
```

### Mapping Data Bits to Output Pins
Transmissions are MSB-first. Bit 15 maps to Out16, and Bit 0 maps to Out1.

| Data Bit: | 15 | 14 | 13 | ... | 2 | 1 | 0 |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| Output Pin: | Out16 | Out15 | Out14 | ... | Out3 | Out2 | Out1 |

For example, to turn on Out1, Out2, and Out8:

```c
uint16_t mask = (1 << 0) | (1 << 1) | (1 << 7); // Bit 0, Bit 1, Bit 7
cs16210_write(vfd_handle, mask);
```