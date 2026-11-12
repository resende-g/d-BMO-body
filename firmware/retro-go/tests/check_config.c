// SPDX-License-Identifier: GPL-2.0-only
// Licença: GPL-2.0, a mesma do Retro-Go (ver firmware/retro-go/LICENSE).
//
// Conferência de SINTAXE do config.h do alvo dbmo-v1 no compilador do computador, contra os
// tipos reais do rg_input.h do Retro-Go e com substitutos dos símbolos do ESP-IDF
// (idf_stubs.h). Usado por tests/host_check.sh com -fsyntax-only. Não é a compilação do
// firmware: não usa o ESP-IDF nem o compilador do ESP32-S3.
//
// Dois modos:
//   -DCHECK_CHAIN: inclui components/retro-go/config.h do Retro-Go já preparado pelo build.sh
//                  (confere o registro do alvo e o dbmo_build_flags.h gerado);
//   sem ele:       inclui direto o arquivo indicado em DBMO_CONFIG_H (confere as opções passadas
//                  com -D).
#include "idf_stubs.h"
#include "rg_input.h" /* do Retro-Go: só os tipos */

#ifdef CHECK_CHAIN
#include "config.h" /* components/retro-go/config.h do Retro-Go */
#else
#include DBMO_CONFIG_H
#endif

#define RG_COUNT(array) (sizeof(array) / sizeof((array)[0]))

/* Mesmas declarações de rg_input.c (linhas 23 a 40) */
#ifdef RG_GAMEPAD_ADC_MAP
static rg_keymap_adc_t keymap_adc[] = RG_GAMEPAD_ADC_MAP;
#endif
#ifdef RG_GAMEPAD_GPIO_MAP
static rg_keymap_gpio_t keymap_gpio[] = RG_GAMEPAD_GPIO_MAP;
#endif
#ifdef RG_GAMEPAD_I2C_MAP
static rg_keymap_i2c_t keymap_i2c[] = RG_GAMEPAD_I2C_MAP;
#endif
#ifdef RG_GAMEPAD_VIRT_MAP
static rg_keymap_virt_t keymap_virt[] = RG_GAMEPAD_VIRT_MAP;
#endif

/* Mesma macro de drivers/display/ili9341.h (linhas 15 a 21) */
void spi_queue_transaction(const void *data, size_t length, uint32_t type);
#define ILI9341_CMD(cmd, data...)                    \
    {                                                \
        const uint8_t c = cmd, x[] = {data};         \
        spi_queue_transaction(&c, 1, 0);             \
        if (sizeof(x))                               \
            spi_queue_transaction(&x, sizeof(x), 1); \
    }

static const char *const target_name = RG_TARGET_NAME;
static const int screen[] = {RG_SCREEN_DRIVER, RG_SCREEN_HOST, RG_SCREEN_SPEED, RG_SCREEN_WIDTH,
                             RG_SCREEN_HEIGHT, RG_SCREEN_BACKLIGHT, RG_GPIO_LCD_MISO, RG_GPIO_LCD_MOSI,
                             RG_GPIO_LCD_CLK, RG_GPIO_LCD_CS, RG_GPIO_LCD_DC, RG_GPIO_LCD_BCKL,
                             RG_GPIO_LCD_RST};
static const int storage[] = {RG_STORAGE_SDSPI_HOST, RG_STORAGE_SDSPI_SPEED, RG_GPIO_SDSPI_MISO,
                              RG_GPIO_SDSPI_MOSI, RG_GPIO_SDSPI_CLK, RG_GPIO_SDSPI_CS};
static const int audio[] = {RG_AUDIO_USE_INT_DAC, RG_AUDIO_USE_EXT_DAC, RG_GPIO_SND_I2S_BCK,
                            RG_GPIO_SND_I2S_WS, RG_GPIO_SND_I2S_DATA, RG_GPIO_SND_AMP_ENABLE};
static const struct { int left, top, right, bottom; } areas[] = {RG_SCREEN_VISIBLE_AREA, RG_SCREEN_SAFE_AREA};
static const int recovery = RG_RECOVERY_BTN;
#if !DBMO_RG_BENCH
static const int i2c_bus[] = {RG_I2C_GPIO_DRIVER, RG_I2C_GPIO_ADDR, RG_GPIO_I2C_SDA, RG_GPIO_I2C_SCL};
#endif

void use_everything(void)
{
    /* Como em rg_system.c (platform_init) e em ili9341.h (lcd_init) */
    RG_CUSTOM_PLATFORM_INIT();
    RG_SCREEN_INIT();
#if RG_BATTERY_DRIVER == 1
    {
        uint32_t raw = 2000;
        float percent = RG_BATTERY_CALC_PERCENT(raw), volts = RG_BATTERY_CALC_VOLTAGE(raw);
        int unit = RG_BATTERY_ADC_UNIT, channel = RG_BATTERY_ADC_CHANNEL;
        (void)percent, (void)volts, (void)unit, (void)channel;
    }
#endif
    (void)target_name, (void)screen, (void)storage, (void)audio, (void)areas, (void)recovery;
#if !DBMO_RG_BENCH
    (void)i2c_bus;
#endif
#ifdef RG_GAMEPAD_ADC_MAP
    (void)keymap_adc;
#endif
#ifdef RG_GAMEPAD_GPIO_MAP
    (void)keymap_gpio;
#endif
#ifdef RG_GAMEPAD_I2C_MAP
    (void)keymap_i2c;
#endif
#ifdef RG_GAMEPAD_VIRT_MAP
    (void)keymap_virt;
#endif
}

/* O que cada perfil tem que conter */
_Static_assert(sizeof(RG_TARGET_NAME) == sizeof("DBMO-V1"), "a cadeia de alvos nao escolheu o dbmo-v1");
_Static_assert(RG_UPDATER_ENABLE == 0, "o atualizador deve ficar desligado");
_Static_assert(RG_SCREEN_WIDTH == 320 && RG_SCREEN_HEIGHT == 240, "tela 320x240");

#if DBMO_RG_BENCH
_Static_assert(RG_COUNT(keymap_gpio) == 3 && RG_COUNT(keymap_virt) == 4, "bancada: 3 botoes e 4 combinacoes");
#ifdef RG_GAMEPAD_I2C_MAP
#error "o perfil de bancada nao deve definir o mapa I2C"
#endif
#ifdef RG_I2C_GPIO_DRIVER
#error "o perfil de bancada nao deve selecionar o extensor de GPIO"
#endif
#else
_Static_assert(RG_COUNT(keymap_i2c) == 14, "o mapa I2C deve ter as 14 teclas do Retro-Go");
_Static_assert(RG_I2C_GPIO_DRIVER == 3 && RG_I2C_GPIO_ADDR == 0x20, "MCP23017 (driver 3) em 0x20");
#endif

#ifdef EXPECT_BENCH
#if !DBMO_RG_BENCH
#error "esperava o perfil de bancada (dbmo_build_flags.h nao foi lido?)"
#endif
#endif
