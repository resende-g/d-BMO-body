// Licença: AGPL-3.0, a mesma do Bruce (ver firmware/bruce/LICENSE).
// Interface do d-BMO v1 para o Bruce. Base: boards/ESP32-C5-tft/interface.cpp do Bruce 1.16.1
// (ILI9341 no SPI compartilhado + 3 botões), sem o touch, que o d-BMO não usa.
//
// Flags DBMO_* (todas desligadas por padrão; tabela e pendências em firmware/bruce/README.md):
//   DBMO_INPUT_MCP23017                              botões pelo MCP23017 em vez dos 3 GPIOs
//   DBMO_VBAT_ADC_PIN                                leitura da bateria pelo divisor no ADC
//   DBMO_CHG_STAT_PIN + DBMO_CHG_STAT_ACTIVE         pino de status do carregador
//   DBMO_POWER_HOLD_PIN + DBMO_POWER_HOLD_ACTIVE     power-latch
// Nada deste arquivo foi compilado nem testado em hardware na sessão em que foi escrito.
#include "core/powerSave.h"
#include "core/utils.h"
#include "driver/rtc_io.h"
#include <interface.h>

#ifdef DBMO_INPUT_MCP23017
#include "core/bus_HAL.h"
#include "dbmo/config/pins.h"
#include <Wire.h>
#endif

#if defined(DBMO_INPUT_MCP23017) && defined(HAS_3_BUTTONS)
#error "DBMO_INPUT_MCP23017 usa o perfil HAS_5_BUTTONS do Bruce: retire HAS_3_BUTTONS (env bmo-v1-mcp)"
#endif
#if defined(DBMO_INPUT_MCP23017) && (SYS_I2C_SDA < 0 || SYS_I2C_SCL < 0)
#error "DBMO_INPUT_MCP23017 exige SYS_I2C_SDA e SYS_I2C_SCL (env bmo-v1-mcp)"
#endif
#if defined(DBMO_CHG_STAT_PIN) != defined(DBMO_CHG_STAT_ACTIVE)
#error "defina DBMO_CHG_STAT_PIN e DBMO_CHG_STAT_ACTIVE (HIGH ou LOW) juntos"
#endif
#if defined(DBMO_POWER_HOLD_PIN) != defined(DBMO_POWER_HOLD_ACTIVE)
#error "defina DBMO_POWER_HOLD_PIN e DBMO_POWER_HOLD_ACTIVE (HIGH ou LOW) juntos"
#endif

// O ext0 só acorda por GPIO RTC (0 a 21 no ESP32-S3), e goToDeepSleep() liga o pull-up desse
// pino, o que só serve para botão que fecha para o GND.
static_assert(DEEPSLEEP_WAKEUP_PIN >= 0 && DEEPSLEEP_WAKEUP_PIN <= 21, "DEEPSLEEP_WAKEUP_PIN não é GPIO RTC");
static_assert(DEEPSLEEP_PIN_ACT == LOW, "goToDeepSleep() supõe botão de acordar ativo em nível baixo");

#ifdef DBMO_VBAT_ADC_PIN
namespace {
// Divisor 100k/100k do README: a bateria chega ao pino pela metade.
constexpr int kVbatDividerRatio = 2;
constexpr int kVbatSamples = 8;
// Abaixo disto não há célula ligada ao divisor (ou a leitura é inválida): sem informação.
constexpr int kVbatAbsentMv = 2500;
// APROXIMAÇÃO NÃO CALIBRADA: reta entre 3300 mV (o mesmo "vazio" de src/core/utils.cpp do
// Bruce) e 4200 mV (carga completa nominal de uma célula). A descarga de uma LiPo não é
// linear e a tensão cai sob carga, então o número serve de indicação, não de medida.
constexpr int kVbatEmptyMv = 3300;
constexpr int kVbatFullMv = 4200;
} // namespace
#endif

#ifdef DBMO_INPUT_MCP23017
namespace {
namespace btn = dbmo::config::mcp_buttons;
constexpr uint8_t kMcpAddress = dbmo::config::i2c::kMcp23017Buttons;

// Registradores do MCP23017 com IOCON.BANK = 0. O código supõe o IOCON do reset do CI, em
// que o ponteiro de registrador avança sozinho (SEQOP = 0) e A e B saem numa transação só:
// suposição ainda a conferir no datasheet (README.md > Verificação).
constexpr uint8_t kMcpRegIodirA = 0x00; // IODIRA, IODIRB, IPOLA, IPOLB
constexpr uint8_t kMcpRegGppuA = 0x0C;  // GPPUA, GPPUB
constexpr uint8_t kMcpRegGpioA = 0x12;  // GPIOA, GPIOB
// O datasheet atual do MCP23017 (DS20001952D) declara GPA7 e GPB7 somente saída e manda
// programar o bit 7 de IODIR como saída. O mapa de pins.h não põe botão nos bits 7 e 15.
constexpr uint8_t kMcpPortInputs = 0x7F;
constexpr uint16_t kMcpInputMask = (kMcpPortInputs << 8) | kMcpPortInputs;

constexpr bool mcpIsInput(uint8_t bit) { return (kMcpInputMask >> bit) & 1; }
static_assert(
    mcpIsInput(btn::kDpadUp) && mcpIsInput(btn::kDpadDown) && mcpIsInput(btn::kDpadLeft) &&
        mcpIsInput(btn::kDpadRight) && mcpIsInput(btn::kActionA) && mcpIsInput(btn::kActionB) &&
        mcpIsInput(btn::kL1) && mcpIsInput(btn::kR1),
    "botão de navegação mapeado em GPA7/GPB7, que o MCP23017 não lê"
);

bool mcpReady = false;

bool mcpWrite(uint8_t reg, const uint8_t *data, size_t len) {
    Wire.beginTransmission(kMcpAddress);
    Wire.write(reg);
    Wire.write(data, len);
    return Wire.endTransmission() == 0;
}

// Entradas com pull-up interno (botão ao GND), sem inversão de polaridade.
bool mcpConfigure() {
    const uint8_t direction[] = {kMcpPortInputs, kMcpPortInputs, 0x00, 0x00};
    const uint8_t pullups[] = {kMcpPortInputs, kMcpPortInputs};
    return mcpWrite(kMcpRegIodirA, direction, sizeof(direction)) &&
           mcpWrite(kMcpRegGppuA, pullups, sizeof(pullups));
}

// Um bit em 1 por botão apertado (bit 0 a 7 = GPA, 8 a 15 = GPB).
bool mcpReadPressed(uint16_t &pressed) {
    Wire.beginTransmission(kMcpAddress);
    Wire.write(kMcpRegGpioA);
    Wire.endTransmission(false);
    if (Wire.requestFrom(kMcpAddress, (uint8_t)2) != 2) return false;
    uint16_t levels = Wire.read();
    levels |= (uint16_t)Wire.read() << 8;
    pressed = ~levels & kMcpInputMask;
    return true;
}
} // namespace
#endif

void _setup_gpio() {
#ifdef DBMO_POWER_HOLD_PIN
    // Antes de tudo: segura o power-latch, para a placa continuar ligada depois que o botão
    // de energia for solto.
    pinMode(DBMO_POWER_HOLD_PIN, OUTPUT);
    digitalWrite(DBMO_POWER_HOLD_PIN, DBMO_POWER_HOLD_ACTIVE);
#endif
    // Todo CS do barramento compartilhado começa em HIGH, para ninguém responder fora da vez.
    for (int cs : {TFT_CS, CC1101_SS_PIN, SDCARD_CS}) {
        pinMode(cs, OUTPUT);
        digitalWrite(cs, HIGH);
    }
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
#ifdef DBMO_INPUT_MCP23017
    pinMode(DEEPSLEEP_WAKEUP_PIN, INPUT_PULLUP);
    // O MCP23017 divide os pinos com o I2C das funções do Bruce (PN532). Declarado como
    // sys_i2c (SYS_I2C_SDA/SCL), o Bruce reaproveita este Wire e nunca o desliga.
    setSysI2CBus(&Wire);
    Wire.begin(SYS_I2C_SDA, SYS_I2C_SCL);
    mcpReady = mcpConfigure();
#else
    for (int btn : {UP_BTN, SEL_BTN, DW_BTN}) pinMode(btn, INPUT_PULLUP);
#endif
#ifdef DBMO_VBAT_ADC_PIN
    pinMode(DBMO_VBAT_ADC_PIN, INPUT);
#endif
#ifdef DBMO_CHG_STAT_PIN
    // Pull contrário ao nível ativo: pino solto ou saída em alta impedância lê "não carregando".
    pinMode(DBMO_CHG_STAT_PIN, DBMO_CHG_STAT_ACTIVE == LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
#endif
    bruceConfig.colorInverted = 0;
    // Padrões de fábrica (o /brucePins.conf salvo, lido depois, prevalece): sem isto o Bruce
    // parte do módulo de RF de um pino só, cujo TX/RX padrão são os pinos do I2C.
    bruceConfigPins.rfModule = CC1101_SPI_MODULE;
    bruceConfigPins.rfidModule = PN532_I2C_MODULE;
}

void _post_setup_gpio() {}

// 0 = "sem informação de bateria": o Bruce não desenha o ícone (drawStatusBar, display.cpp).
// Uma leitura válida, por isso, nunca devolve menos que 1.
int getBattery() {
#ifdef DBMO_VBAT_ADC_PIN
    uint32_t pinMv = 0;
    for (int i = 0; i < kVbatSamples; i++) pinMv += analogReadMilliVolts(DBMO_VBAT_ADC_PIN);
    const int vbatMv = (int)(pinMv / kVbatSamples) * kVbatDividerRatio;
    if (vbatMv < kVbatAbsentMv) return 0;
    return constrain((vbatMv - kVbatEmptyMv) * 100 / (kVbatFullMv - kVbatEmptyMv), 1, 100);
#else
    return 0;
#endif
}

// O Bruce só consulta isto quando getBattery() devolve mais que 0 (drawBatteryStatus).
bool isCharging() {
#ifdef DBMO_CHG_STAT_PIN
    return digitalRead(DBMO_CHG_STAT_PIN) == DBMO_CHG_STAT_ACTIVE;
#else
    return false;
#endif
}

void _setBrightness(uint8_t brightval) {
    if (brightval == 0) {
        analogWrite(TFT_BL, brightval);
    } else {
        int bl = MINBRIGHT + round(((255 - MINBRIGHT) * brightval / 100));
        analogWrite(TFT_BL, bl);
    }
}

#ifdef DBMO_INPUT_MCP23017
// Mapa do perfil de 5 botões do Bruce (boards/smoochiee-board/interface.cpp): esquerda/direita
// = anterior/próximo, cima/baixo, A = OK, B = voltar, L1/R1 = página. Os demais botões só
// acordam a tela.
void InputHandler(void) {
    static unsigned long tm = 0;
    if (millis() - tm < 200 && !LongPress) return;

    uint16_t pressed = 0;
    if (!mcpReady) mcpReady = mcpConfigure();
    if (!mcpReady || !mcpReadPressed(pressed)) {
        // Sem resposta do MCP23017: nenhum botão, e nova configuração na próxima tentativa.
        mcpReady = false;
        tm = millis();
        return;
    }
    auto isDown = [pressed](uint8_t bit) { return (pressed & (1u << bit)) != 0; };

    bool anyPressed = pressed != 0;
    if (anyPressed) tm = millis();
    if (anyPressed && wakeUpScreen()) return;

    AnyKeyPress = anyPressed;
    PrevPress = isDown(btn::kDpadLeft);
    NextPress = isDown(btn::kDpadRight);
    UpPress = isDown(btn::kDpadUp);
    DownPress = isDown(btn::kDpadDown);
    SelPress = isDown(btn::kActionA);
    EscPress = isDown(btn::kActionB);
    PrevPagePress = isDown(btn::kL1);
    NextPagePress = isDown(btn::kR1);
}
#else
void InputHandler(void) {
    static unsigned long tm = 0;
    if (millis() - tm < 200 && !LongPress) return;

    bool upPressed = digitalRead(UP_BTN) == BTN_ACT;
    bool selPressed = digitalRead(SEL_BTN) == BTN_ACT;
    bool dwPressed = digitalRead(DW_BTN) == BTN_ACT;

    bool anyPressed = upPressed || selPressed || dwPressed;
    if (anyPressed) tm = millis();
    if (anyPressed && wakeUpScreen()) return;

    AnyKeyPress = anyPressed;
    if (upPressed && dwPressed) {
        EscPress = true;
        return;
    }
    PrevPress = upPressed;
    NextPress = dwPressed;
    SelPress = selPressed;
}
#endif

// Deep sleep no padrão das placas do Bruce (lilygo-t-display-s3, ES3C28P): backlight apagado,
// painel em SLPIN e ext0 no botão de acordar. O chip reinicia ao acordar.
void goToDeepSleep() {
    // O botão que acorda pode ser o mesmo que acabou de escolher esta opção: dormir com ele
    // ainda apertado acordaria na hora. A espera tem limite: com o botão preso (ou em curto),
    // o aparelho desiste de dormir em vez de ficar parado aqui para sempre.
    const unsigned long waitStart = millis();
    while (digitalRead(DEEPSLEEP_WAKEUP_PIN) == DEEPSLEEP_PIN_ACT) {
        if (millis() - waitStart > 10000UL) return;
        delay(10);
    }
    delay(200);
    _setBrightness(0);
    tft.writecommand(0x10); // SLPIN
    // Com o ext0 o pino passa ao domínio RTC, onde o pull-up do pinMode() não vale no ESP32-S3.
    rtc_gpio_pullup_en((gpio_num_t)DEEPSLEEP_WAKEUP_PIN);
    rtc_gpio_pulldown_dis((gpio_num_t)DEEPSLEEP_WAKEUP_PIN);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)DEEPSLEEP_WAKEUP_PIN, DEEPSLEEP_PIN_ACT);
    esp_deep_sleep_start();
}

void powerOff() {
#ifdef DBMO_POWER_HOLD_PIN
    // Solta o power-latch: o circuito corta a alimentação. Se a placa continuar viva (pela
    // USB, por exemplo), segue para o deep sleep.
    digitalWrite(DBMO_POWER_HOLD_PIN, !DBMO_POWER_HOLD_ACTIVE);
    delay(500);
#endif
    goToDeepSleep();
}

// Vazio: os gestos que o Bruce usa aqui (cima+baixo, botão de energia) já são "voltar" ou ainda não existem.
void checkReboot() {}
