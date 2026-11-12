// SPDX-License-Identifier: GPL-2.0-only
// Licença: GPL-2.0, a mesma do Retro-Go (ver firmware/retro-go/LICENSE).
//
// Substitutos mínimos dos símbolos do ESP-IDF que o config.h do alvo dbmo-v1 cita.
// Servem só para conferir a SINTAXE das macros com o compilador do computador
// (tests/host_check.sh). Não são o ESP-IDF e não provam que o firmware compila.
#pragma once

#include <stddef.h>
#include <stdint.h>

typedef int gpio_num_t;
enum {
    GPIO_NUM_NC = -1,
    GPIO_NUM_0 = 0, GPIO_NUM_1, GPIO_NUM_2, GPIO_NUM_3, GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_6, GPIO_NUM_7,
    GPIO_NUM_8, GPIO_NUM_9, GPIO_NUM_10, GPIO_NUM_11, GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_15,
    GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_20, GPIO_NUM_21,
    GPIO_NUM_38 = 38, GPIO_NUM_39, GPIO_NUM_40, GPIO_NUM_41, GPIO_NUM_42, GPIO_NUM_43, GPIO_NUM_44,
    GPIO_NUM_45, GPIO_NUM_46, GPIO_NUM_47, GPIO_NUM_48
};
enum { GPIO_MODE_INPUT = 1, GPIO_MODE_OUTPUT = 2 };
enum { GPIO_PULLUP_ONLY = 0 };
enum { ADC_UNIT_1 = 1, ADC_UNIT_2 = 2 };
enum { ADC_CHANNEL_0 = 0, ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3, ADC_CHANNEL_4, ADC_CHANNEL_5 };
#define ADC_ATTEN_DB_11 3 /* o rg_input.c do Retro-Go redefine assim */
enum { SPI2_HOST = 1, SPI3_HOST = 2 };
#define SPI_MASTER_FREQ_10M (80 * 1000 * 1000 / 8)
#define SPI_MASTER_FREQ_40M (80 * 1000 * 1000 / 2)
#define SDMMC_FREQ_DEFAULT 20000

int gpio_reset_pin(gpio_num_t gpio_num);
int gpio_set_level(gpio_num_t gpio_num, uint32_t level);
int gpio_set_direction(gpio_num_t gpio_num, int mode);
int gpio_set_pull_mode(gpio_num_t gpio_num, int pull);
