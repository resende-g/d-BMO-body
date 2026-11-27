// SPDX-License-Identifier: GPL-2.0-only
// Licença: GPL-2.0, a mesma do Retro-Go (ver firmware/retro-go/LICENSE).
//
// Alvo do Retro-Go para o d-BMO v1 (ESP32-S3-WROOM-1-N16R8).
//
// Derivado de dois alvos do Retro-Go (ducalex/retro-go, commit 4ced120669750ca7228fd0414211430c1d923166):
//   - targets/esp32-s3-devkit/config.h: estrutura, sequência de init do ILI9341 320x240, DAC I2S externo;
//   - targets/fri3d-2024/config.h: mesmo módulo N16R8, tela e cartão SD no mesmo host SPI.
//
// ESTADO: a imagem completa compila (ESP-IDF 4.4.8, perfil de bancada) e o launcher do perfil
// padrão compila na CI; nunca executado no hardware. A pinagem é a
// do perfil "target" de firmware/config/include/dbmo/config/pins.h (PROPOSTA do v1); na bancada
// só existem a tela e três botões diretos.
// Ver firmware/retro-go/README.md (tabela de status, limitações e pendências).
//
// Convenção para conferência automática do mapa de pinos: toda linha que fixa um GPIO termina
// com o comentário "pins.h <namespace>::<constante>".

// ---------------------------------------------------------------------------
// Opções de build do d-BMO (todas DESLIGADAS por padrão).
// O firmware/retro-go/build.sh gera o arquivo dbmo_build_flags.h ao lado deste a partir das
// variáveis de ambiente de mesmo nome. Sem esse arquivo, valem os padrões abaixo.
// ---------------------------------------------------------------------------
#if defined(__has_include)
#if __has_include("dbmo_build_flags.h")
#include "dbmo_build_flags.h"
#endif
#endif

// 1 = perfil de bancada (pins.h namespace bench): tela + 3 botões diretos, sem MCP23017.
#ifndef DBMO_RG_BENCH
#define DBMO_RG_BENCH 0
#endif
// 1 = leitura da bateria no ADC1 canal 8 (GPIO 9, divisor 100k/100k). O divisor ainda não existe:
// com o pino solto o ADC leria ruído e o Retro-Go mostraria um nível de bateria falso.
#ifndef DBMO_RG_ENABLE_BATTERY_ADC
#define DBMO_RG_ENABLE_BATTERY_ADC 0
#endif
// 1 = aciona o power-latch (GPIO 2) no início do boot. O circuito não está decidido: a
// polaridade tem que ser informada em DBMO_RG_POWER_HOLD_LEVEL (0 ou 1), sem valor padrão.
#ifndef DBMO_RG_ENABLE_POWER_HOLD
#define DBMO_RG_ENABLE_POWER_HOLD 0
#endif

#if DBMO_RG_ENABLE_POWER_HOLD && !defined(DBMO_RG_POWER_HOLD_LEVEL)
#error "DBMO_RG_ENABLE_POWER_HOLD exige DBMO_RG_POWER_HOLD_LEVEL (0 ou 1): o circuito do power-latch não está decidido."
#endif

// Registrador MADCTL (0x36) do ILI9341. 0x28 = MV|BGR: é o que o TFT_eSPI do Bruce 1.16.1 envia
// na rotação 1 (firmware/bruce/bmo.ini, ROTATION=1), a orientação usada na bancada. Bits:
// 0x80 = MY, 0x40 = MX, 0x20 = MV, 0x08 = BGR. Pendente de validação física com o Retro-Go.
#ifndef DBMO_RG_LCD_MADCTL
#define DBMO_RG_LCD_MADCTL 0x28
#endif
// Clock SPI da tela, em Hz. 40 MHz é o valor dos alvos de origem; no d-BMO só 10 MHz foi
// validado, e em jumpers de protoboard (constante kDisplayHz do mapa de pinos canônico).
// ATENÇÃO: o datasheet do ILI9341 limita a escrita serial a 10 MHz (ciclo mínimo de 100 ns;
// docs/hardware/sources/PERIPHERALS.md, ILI-07). Os 40 MHz do perfil sem DBMO_RG_BENCH estão
// FORA dessa especificação: são o que os emuladores precisam para uma taxa de quadros
// utilizável (a 10 MHz, um quadro inteiro de 320 x 240 leva cerca de 120 ms) e o que outros
// alvos do Retro-Go usam com este controlador, mas é um risco PENDENTE DE VALIDAÇÃO FÍSICA.
#ifndef DBMO_RG_SCREEN_SPEED_HZ
#if DBMO_RG_BENCH
#define DBMO_RG_SCREEN_SPEED_HZ SPI_MASTER_FREQ_10M
#else
#define DBMO_RG_SCREEN_SPEED_HZ SPI_MASTER_FREQ_40M
#endif
#endif

// ---------------------------------------------------------------------------
// Definição do alvo
// ---------------------------------------------------------------------------
#define RG_TARGET_NAME             "DBMO-V1"

// Sem verificação de atualização: o atualizador do Retro-Go consulta a API do GitHub e o
// projeto não depende de nuvem. Só tem efeito quando a rede é compilada (DBMO_RG_NETWORKING=1).
#define RG_UPDATER_ENABLE           0

// Segurar MENU no boot abre o menu de recuperação (o padrão do Retro-Go é qualquer tecla).
#define RG_RECOVERY_BTN             RG_KEY_MENU

// ---------------------------------------------------------------------------
// Armazenamento: cartão SD em modo SPI, no MESMO host SPI da tela.
// O Retro-Go monta o SD antes de iniciar a tela (rg_system.c), e é o rg_storage.c que inicializa
// o barramento; o driver da tela aceita o barramento já inicializado (ESP_ERR_INVALID_STATE).
// Não definir RG_STORAGE_SDSPI_HOLD_CS: ele só serve para barramento não compartilhado.
// ---------------------------------------------------------------------------
#define RG_STORAGE_ROOT             "/sd"
#define RG_STORAGE_SDSPI_HOST       SPI2_HOST
#define RG_STORAGE_SDSPI_SPEED      SDMMC_FREQ_DEFAULT
#if DBMO_RG_BENCH
// Na bancada o SD pode não estar ligado: se a imagem tiver a partição FAT interna "vfs"
// (rg_tool.py --fatsize), o Retro-Go a monta quando o SD falha.
#define RG_STORAGE_FLASH_PARTITION  "vfs"
#endif

// ---------------------------------------------------------------------------
// Extensor de GPIO (botões): MCP23017 em 0x20. Driver 3 = MCP23017 (rg_i2c.c).
// ---------------------------------------------------------------------------
#if !DBMO_RG_BENCH
#define RG_I2C_GPIO_DRIVER          3
#define RG_I2C_GPIO_ADDR            0x20 // pins.h i2c::kMcp23017Buttons
#endif

// ---------------------------------------------------------------------------
// Áudio: DAC I2S externo (MAX98357A). O ESP32-S3 não tem DAC interno.
// ---------------------------------------------------------------------------
#define RG_AUDIO_USE_INT_DAC        0   // 0 = desligado
#define RG_AUDIO_USE_EXT_DAC        1   // 1 = I2S externo

// ---------------------------------------------------------------------------
// Vídeo: ILI9341 320x240 por SPI
// ---------------------------------------------------------------------------
#define RG_SCREEN_DRIVER            0   // 0 = ILI9341/ST7789
#define RG_SCREEN_HOST              SPI2_HOST
#define RG_SCREEN_SPEED             DBMO_RG_SCREEN_SPEED_HZ
#define RG_SCREEN_BACKLIGHT         1
#define RG_SCREEN_WIDTH             320
#define RG_SCREEN_HEIGHT            240
#define RG_SCREEN_ROTATE            0
#define RG_SCREEN_VISIBLE_AREA      {0, 0, 0, 0}
#define RG_SCREEN_SAFE_AREA         {0, 0, 0, 0}
// Sequência do alvo esp32-s3-devkit; o MADCTL (0x36) e o 2º byte do 0xB6 são do d-BMO. O alvo de
// origem usa 0xB6 = 0x0A, 0xA2 (SS=1, colunas do painel invertidas). Com o MADCTL 0x28, tirado de
// um driver com SS=0, isso deixou a imagem de cabeça para baixo e espelhada na bancada em
// 25/11/2026. 0x82 (SS=0) é o valor do Adafruit_ILI9341 do firmware próprio, cuja imagem saiu certa.
#define RG_SCREEN_INIT()                                                                                         \
    ILI9341_CMD(0xCF, 0x00, 0xc3, 0x30);                                                                         \
    ILI9341_CMD(0xED, 0x64, 0x03, 0x12, 0x81);                                                                   \
    ILI9341_CMD(0xE8, 0x85, 0x00, 0x78);                                                                         \
    ILI9341_CMD(0xCB, 0x39, 0x2c, 0x00, 0x34, 0x02);                                                             \
    ILI9341_CMD(0xF7, 0x20);                                                                                     \
    ILI9341_CMD(0xEA, 0x00, 0x00);                                                                               \
    ILI9341_CMD(0xC0, 0x1B);                 /* Power control   //VRH[5:0] */                                    \
    ILI9341_CMD(0xC1, 0x12);                 /* Power control   //SAP[2:0];BT[3:0] */                            \
    ILI9341_CMD(0xC5, 0x32, 0x3C);           /* VCM control */                                                   \
    ILI9341_CMD(0xC7, 0x91);                 /* VCM control2 */                                                  \
    ILI9341_CMD(0x36, DBMO_RG_LCD_MADCTL);   /* Memory Access Control */                                         \
    ILI9341_CMD(0xB1, 0x00, 0x10);           /* Frame Rate Control (1B=70, 1F=61, 10=119) */                     \
    ILI9341_CMD(0xB6, 0x0A, 0x82);           /* Display Function Control: SS=0, como o Adafruit (ver acima) */   \
    ILI9341_CMD(0xF6, 0x01, 0x30);                                                                               \
    ILI9341_CMD(0xF2, 0x00);                 /* 3Gamma Function Disable */                                       \
    ILI9341_CMD(0x26, 0x01);                 /* Gamma curve selected */                                          \
    ILI9341_CMD(0xE0, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00); \
    ILI9341_CMD(0xE1, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F);

// ---------------------------------------------------------------------------
// Entrada
// O Retro-Go tem 14 teclas (rg_input.h), mas nenhum emulador do commit fixado lê X, Y, L ou R
// (o Mega Drive usa A, B e SELECT). O d-BMO tem 10 botões e nenhum gatilho de ombro desde
// 2026-11-25; por isso os botões X e Y viram MENU e OPTION.
// ---------------------------------------------------------------------------
#if DBMO_RG_BENCH

// Bancada: 3 botões ligando o GPIO ao GND, pull-up interno do ESP32-S3.
#define RG_GAMEPAD_GPIO_MAP {\
    {RG_KEY_UP,   .num = GPIO_NUM_39, .pullup = 1, .level = 0}, /* pins.h bench::kButtonUp */ \
    {RG_KEY_A,    .num = GPIO_NUM_2,  .pullup = 1, .level = 0}, /* pins.h bench::kButtonOk */ \
    {RG_KEY_DOWN, .num = GPIO_NUM_47, .pullup = 1, .level = 0}, /* pins.h bench::kButtonDown */ \
}
// Combinações para navegar com 3 botões. Cima+baixo = voltar, como no Bruce de bancada.
#define RG_GAMEPAD_VIRT_MAP {\
    {RG_KEY_B,      .src = RG_KEY_UP | RG_KEY_DOWN},\
    {RG_KEY_RIGHT,  .src = RG_KEY_A | RG_KEY_UP},\
    {RG_KEY_MENU,   .src = RG_KEY_A | RG_KEY_DOWN},\
    {RG_KEY_OPTION, .src = RG_KEY_A | RG_KEY_UP | RG_KEY_DOWN},\
}

#else

// 10 botões no MCP23017: botão ao GND, pull-up interno do MCP (.pullup = 1), ativo em nível
// baixo (.level = 0). .num é o bit: 0-7 = GPA0-7, 8-15 = GPB0-7. Os bits 7 e 15 (GPA7 e GPB7)
// ficam sem botão: o datasheet do MCP23017 (DS20001952, rev. D) os declara somente saída.
#define RG_GAMEPAD_I2C_MAP {\
    {RG_KEY_UP,     .num = 14, .pullup = 1, .level = 0}, /* pins.h mcp_buttons::kDpadUp */ \
    {RG_KEY_DOWN,   .num = 4,  .pullup = 1, .level = 0}, /* pins.h mcp_buttons::kDpadDown */ \
    {RG_KEY_LEFT,   .num = 6,  .pullup = 1, .level = 0}, /* pins.h mcp_buttons::kDpadLeft */ \
    {RG_KEY_RIGHT,  .num = 0,  .pullup = 1, .level = 0}, /* pins.h mcp_buttons::kDpadRight */ \
    {RG_KEY_A,      .num = 10, .pullup = 1, .level = 0}, /* pins.h mcp_buttons::kActionA */ \
    {RG_KEY_B,      .num = 11, .pullup = 1, .level = 0}, /* pins.h mcp_buttons::kActionB */ \
    {RG_KEY_MENU,   .num = 12, .pullup = 1, .level = 0}, /* pins.h mcp_buttons::kActionX */ \
    {RG_KEY_OPTION, .num = 8,  .pullup = 1, .level = 0}, /* pins.h mcp_buttons::kActionY */ \
    {RG_KEY_SELECT, .num = 5,  .pullup = 1, .level = 0}, /* pins.h mcp_buttons::kSelect */ \
    {RG_KEY_START,  .num = 1,  .pullup = 1, .level = 0}, /* pins.h mcp_buttons::kStart */ \
}

#endif

// ---------------------------------------------------------------------------
// Bateria: ADC1 canal 8 = GPIO 9, divisor 100k/100k (por isso o "* 2").
// PROPOSTA: o divisor não existe ainda (hardware ausente), por isso fica atrás de uma opção.
// A faixa 3,5 V = 0 % a 4,2 V = 100 % é a aproximação linear do alvo esp32-s3-devkit; não é uma
// curva medida da célula do d-BMO.
// ---------------------------------------------------------------------------
#if DBMO_RG_ENABLE_BATTERY_ADC
#define RG_BATTERY_DRIVER           1
#define RG_BATTERY_ADC_UNIT         ADC_UNIT_1
#define RG_BATTERY_ADC_CHANNEL      ADC_CHANNEL_8 // GPIO 9: pins.h target::kVbatSense
#define RG_BATTERY_CALC_PERCENT(raw) (((raw) * 2.f - 3500.f) / (4200.f - 3500.f) * 100.f)
#define RG_BATTERY_CALC_VOLTAGE(raw) ((raw) * 2.f * 0.001f)
#else
// 0 = sem driver: rg_input_read_battery_raw() devolve false e o ícone da bateria não aparece.
#define RG_BATTERY_DRIVER           0
#endif

// LED de status: o d-BMO não tem (RG_GPIO_LED fica indefinido).

// ---------------------------------------------------------------------------
// Pinos. Desde 2026-11-26 o perfil target (a placa-base, hardware/placa-principal) e a
// bancada (a protoboard de hoje) só têm em comum o SPI (11, 12, 13). DBMO_RG_BENCH escolhe a
// fiação da protoboard; sem ele, vale a da placa. tools/check_pinmap.py confere os dois blocos.
// ---------------------------------------------------------------------------
#define RG_GPIO_LCD_MISO            GPIO_NUM_13  // pins.h target::kSpiMiso (igual na bancada)
#define RG_GPIO_LCD_MOSI            GPIO_NUM_11  // pins.h target::kSpiMosi (igual na bancada)
#define RG_GPIO_LCD_CLK             GPIO_NUM_12  // pins.h target::kSpiSck (igual na bancada)
// Cartão SD em modo SPI: mesmos MOSI/MISO/SCK da tela, CS próprio.
#define RG_GPIO_SDSPI_MISO          RG_GPIO_LCD_MISO
#define RG_GPIO_SDSPI_MOSI          RG_GPIO_LCD_MOSI
#define RG_GPIO_SDSPI_CLK           RG_GPIO_LCD_CLK
// DAC I2S externo (MAX98357A). SD_MODE em nível alto liga o amplificador, então não se define
// RG_GPIO_SND_AMP_ENABLE_INVERT.

#if DBMO_RG_BENCH  // pinos da bancada (pins.h bench)
#define RG_GPIO_LCD_CS              GPIO_NUM_10  // pins.h bench::kDisplayCs
#define RG_GPIO_LCD_DC              GPIO_NUM_17  // pins.h bench::kDisplayDc
#define RG_GPIO_LCD_BCKL            GPIO_NUM_16  // pins.h bench::kDisplayBacklight
#define RG_GPIO_LCD_RST             GPIO_NUM_18  // pins.h bench::kDisplayReset
#define RG_GPIO_SDSPI_CS            GPIO_NUM_21  // pins.h bench::kSdCs
#define RG_GPIO_SND_I2S_BCK         GPIO_NUM_40  // pins.h bench::kI2sBclk
#define RG_GPIO_SND_I2S_WS          GPIO_NUM_41  // pins.h bench::kI2sLrclk
#define RG_GPIO_SND_I2S_DATA        GPIO_NUM_42  // pins.h bench::kI2sSpeakerData
#define DBMO_GPIO_CC1101_CS         GPIO_NUM_14  // pins.h bench::kCc1101Cs
// Na bancada o SD_MODE do amplificador não vai a um GPIO (a placa dele o mantém ligado) e o
// MCP23017 não existe: sem RG_GPIO_SND_AMP_ENABLE nem I2C.
#else  // pinos da placa-base (pins.h target)
#define RG_GPIO_I2C_SDA             GPIO_NUM_6   // pins.h target::kI2cSda
#define RG_GPIO_I2C_SCL             GPIO_NUM_1   // pins.h target::kI2cScl
#define RG_GPIO_LCD_CS              GPIO_NUM_41  // pins.h target::kDisplayCs
#define RG_GPIO_LCD_DC              GPIO_NUM_39  // pins.h target::kDisplayDc
#define RG_GPIO_LCD_BCKL            GPIO_NUM_14  // pins.h target::kDisplayBacklight
#define RG_GPIO_LCD_RST             GPIO_NUM_40  // pins.h target::kDisplayReset
#define RG_GPIO_SDSPI_CS            GPIO_NUM_47  // pins.h target::kSdCs
#define RG_GPIO_SND_I2S_BCK         GPIO_NUM_17  // pins.h target::kI2sBclk
#define RG_GPIO_SND_I2S_WS          GPIO_NUM_18  // pins.h target::kI2sLrclk
#define RG_GPIO_SND_I2S_DATA        GPIO_NUM_16  // pins.h target::kI2sSpeakerData
#define RG_GPIO_SND_AMP_ENABLE      GPIO_NUM_15  // pins.h target::kAmpShutdown
#define DBMO_GPIO_CC1101_CS         GPIO_NUM_48  // pins.h target::kCc1101Cs
#endif
// Pino do d-BMO que o Retro-Go não conhece, tratado em RG_CUSTOM_PLATFORM_INIT().
#define DBMO_GPIO_POWER_HOLD        GPIO_NUM_2   // pins.h target::kPowerHold

#if DBMO_RG_ENABLE_POWER_HOLD
#define DBMO_RG_POWER_HOLD_INIT()                                  \
    gpio_reset_pin(DBMO_GPIO_POWER_HOLD);                          \
    gpio_set_level(DBMO_GPIO_POWER_HOLD, DBMO_RG_POWER_HOLD_LEVEL); \
    gpio_set_direction(DBMO_GPIO_POWER_HOLD, GPIO_MODE_OUTPUT);
#else
#define DBMO_RG_POWER_HOLD_INIT()
#endif

// Volta ao d-BMO. Na imagem combinada (tools/combine_retrogo.py do d-BMO-body), o firmware
// próprio fica na partição factory "dbmo" e o item Jogos dele aponta o boot ao "launcher". Aqui,
// em todo boot do Retro-Go, o boot volta a apontar para "dbmo": um reset sai dos jogos. As
// trocas do próprio Retro-Go (launcher <-> jogo) gravam o boot de novo antes de reiniciar
// (update_boot_config em rg_system.c), por isso continuam funcionando. RG_APP_FACTORY também
// liga a opção "Reboot to factory" do menu de recuperação. Sem a partição (imagem só do
// Retro-Go), nada muda.
#define RG_APP_FACTORY "dbmo"
#define DBMO_RG_RETURN_TO_DBMO()                                                             \
    {                                                                                        \
        const esp_partition_t *dbmo_part = esp_partition_find_first(                         \
            ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, RG_APP_FACTORY);       \
        const esp_partition_t *boot_part = esp_ota_get_boot_partition();                     \
        if (dbmo_part && (!boot_part || boot_part->address != dbmo_part->address))           \
            esp_ota_set_boot_partition(dbmo_part);                                           \
    }

// Executado pelo Retro-Go dentro de platform_init(), antes do SD e da tela (rg_system.c).
// O barramento SPI do d-BMO também tem o CC1101: o CS dele fica em nível alto para não
// responder durante a inicialização do SD. O próprio Retro-Go já faz isso com os CS
// da tela e do SD. O pull-up interno no MISO repete o que o alvo t-deck-plus faz; ele não
// substitui o resistor externo que o cartão SD pede.
#define RG_CUSTOM_PLATFORM_INIT()                                 \
    DBMO_RG_POWER_HOLD_INIT()                                     \
    DBMO_RG_RETURN_TO_DBMO()                                      \
    gpio_reset_pin(DBMO_GPIO_CC1101_CS);                          \
    gpio_set_level(DBMO_GPIO_CC1101_CS, 1);                       \
    gpio_set_direction(DBMO_GPIO_CC1101_CS, GPIO_MODE_OUTPUT);    \
    gpio_set_pull_mode(RG_GPIO_SDSPI_MISO, GPIO_PULLUP_ONLY);
