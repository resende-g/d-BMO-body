// Mapa de pinos canônico do d-BMO v1 (ESP32-S3-WROOM-1-N16R8 / DevKitC-1 N16R8).
//
// Este arquivo é a fonte de verdade dos GPIOs para o firmware próprio. A documentação
// (docs/hardware/GPIO_MAP.md), o build do Bruce (firmware/bruce/bmo.ini) e o alvo do
// Retro-Go (firmware/retro-go/) são conferidos contra ele por tools/check_pinmap.py.
//
// Existem dois perfis:
//   - target: a pinagem PROPOSTA para o v1 completo (README > Pinagem). Não é definitiva:
//     só os itens marcados BenchValidated foram ligados e testados.
//   - bench:  a fiação que existe hoje na protoboard (tela + 3 botões diretos), igual ao
//     firmware/bruce/bmo.ini.
//
// Sem dependência do Arduino: compila no computador para os testes.
#pragma once

#include <cstddef>
#include <cstdint>

namespace dbmo::config {

enum class PinStatus : uint8_t {
  ChipFixed,       // função fixa do SoC/módulo (datasheet da Espressif)
  BenchValidated,  // ligado e testado na bancada
  Proposed,        // proposta; pendente de validação física
  ProposedAtRisk,  // proposta com conflito/risco aberto (ver docs/hardware/GPIO_MAP.md)
};

struct PinAssignment {
  const char *signal;
  int8_t gpio;
  PinStatus status;
};

// GPIOs utilizáveis no módulo ESP32-S3-WROOM-1-N16R8: 0–21 e 38–48.
// 22–25 não existem no ESP32-S3; 26–32 são da flash; 33 e 34 não saem no módulo; 35–37
// saem no módulo (e no conector da DevKitC), mas no N16R8 pertencem à PSRAM octal.
// Fontes em docs/hardware/sources/ESP32-S3.md.
constexpr bool isUsableGpio(int gpio) {
  return (gpio >= 0 && gpio <= 21) || (gpio >= 38 && gpio <= 48);
}

// Pinos de strapping do ESP32-S3 (lidos no reset).
constexpr bool isStrappingGpio(int gpio) {
  return gpio == 0 || gpio == 3 || gpio == 45 || gpio == 46;
}

// ADC1 (GPIO 1–10). O ADC2 (GPIO 11–20) não é usado: disputa com o WiFi.
constexpr bool isAdc1Gpio(int gpio) { return gpio >= 1 && gpio <= 10; }

// GPIOs RTC (0–21): os únicos que acordam o chip do deep sleep por nível (ext0/ext1).
constexpr bool isRtcGpio(int gpio) { return gpio >= 0 && gpio <= 21; }

// ---------------------------------------------------------------------------
// Perfil target: pinagem proposta do v1 (README > Pinagem).
// ---------------------------------------------------------------------------
namespace target {

// Desde 2026-11-26 a ordem dos GPIOs segue a placa principal de uma face
// (hardware/placa-principal), com a tela encostada no topo: cada módulo pega os pinos da DevKitC
// que ficam do lado dele, para a placa precisar de menos fios. Os pinos de SPI (11, 12 e 13, IO_MUX
// nativo do FSPI) não mudaram. Os outros são PROPOSTOS até a placa ser montada, menos os que caíram
// no mesmo GPIO já validado na bancada (microfone no 7, IR RX no 4). A bancada continua no perfil
// bench.
inline constexpr int8_t kI2cScl = 1;
// Trava da placa de energia. GPIO RTC: segura o nível em deep sleep.
inline constexpr int8_t kPowerHold = 2;
inline constexpr int8_t kIrRx = 4;
inline constexpr int8_t kIrTx = 5;
inline constexpr int8_t kI2cSda = 6;
inline constexpr int8_t kI2sMicData = 7;  // INMP441 SD
// Status do carregador, da placa de energia. GPIO RTC: acorda ao ligar o carregador.
inline constexpr int8_t kChargerStatus = 8;
inline constexpr int8_t kVbatSense = 9;  // ADC1 canal 8; divisor 100k/100k, na placa de energia
// Botão de energia: GPIO RTC fora dos pinos de strapping, acorda do deep sleep (risco 1).
inline constexpr int8_t kPowerButton = 10;
inline constexpr int8_t kSpiMosi = 11;
inline constexpr int8_t kSpiSck = 12;
inline constexpr int8_t kSpiMiso = 13;
inline constexpr int8_t kDisplayBacklight = 14;
inline constexpr int8_t kAmpShutdown = 15;     // MAX98357A SD_MODE
inline constexpr int8_t kI2sSpeakerData = 16;  // MAX98357A DIN
inline constexpr int8_t kI2sBclk = 17;
inline constexpr int8_t kI2sLrclk = 18;
inline constexpr int8_t kUsbDm = 19;
inline constexpr int8_t kUsbDp = 20;
inline constexpr int8_t kCc1101Gdo0 = 21;
inline constexpr int8_t kDisplayDc = 39;
inline constexpr int8_t kDisplayReset = 40;
inline constexpr int8_t kDisplayCs = 41;
inline constexpr int8_t kSdCs = 47;
inline constexpr int8_t kCc1101Cs = 48;

inline constexpr PinAssignment kPins[] = {
    {"I2C_SCL", kI2cScl, PinStatus::Proposed},
    {"POWER_HOLD", kPowerHold, PinStatus::Proposed},
    {"IR_RX", kIrRx, PinStatus::BenchValidated},
    {"IR_TX", kIrTx, PinStatus::Proposed},
    {"I2C_SDA", kI2cSda, PinStatus::Proposed},
    {"I2S_MIC_DATA", kI2sMicData, PinStatus::BenchValidated},
    {"CHARGER_STATUS", kChargerStatus, PinStatus::Proposed},
    {"VBAT_SENSE", kVbatSense, PinStatus::Proposed},
    {"POWER_BUTTON", kPowerButton, PinStatus::Proposed},
    {"SPI_MOSI", kSpiMosi, PinStatus::BenchValidated},
    {"SPI_SCK", kSpiSck, PinStatus::BenchValidated},
    {"SPI_MISO", kSpiMiso, PinStatus::BenchValidated},
    {"DISPLAY_BACKLIGHT", kDisplayBacklight, PinStatus::Proposed},
    {"AMP_SHUTDOWN", kAmpShutdown, PinStatus::Proposed},
    {"I2S_SPEAKER_DATA", kI2sSpeakerData, PinStatus::Proposed},
    {"I2S_BCLK", kI2sBclk, PinStatus::Proposed},
    {"I2S_LRCLK", kI2sLrclk, PinStatus::Proposed},
    {"USB_DM", kUsbDm, PinStatus::ChipFixed},
    {"USB_DP", kUsbDp, PinStatus::ChipFixed},
    {"CC1101_GDO0", kCc1101Gdo0, PinStatus::Proposed},
    {"DISPLAY_DC", kDisplayDc, PinStatus::Proposed},
    {"DISPLAY_RESET", kDisplayReset, PinStatus::Proposed},
    {"DISPLAY_CS", kDisplayCs, PinStatus::Proposed},
    {"SD_CS", kSdCs, PinStatus::Proposed},
    {"CC1101_CS", kCc1101Cs, PinStatus::Proposed},
};

// Livres no perfil target. 0, 3, 45 e 46 são de strapping: o 0 fica só como BOOT (pad de teste
// ou botão interno), e 45 e 46 não podem ser puxados no reset. 38 é o LED RGB da DevKitC v1.1.
// 43 e 44 são a UART0, ligada ao conversor USB-serial da DevKitC. 42 sobrou na troca de
// 2026-11-26.
inline constexpr int8_t kFreePins[] = {0, 3, 38, 42, 43, 44, 45, 46};

}  // namespace target

// ---------------------------------------------------------------------------
// Perfil bench: a protoboard de hoje (igual ao firmware/bruce/bmo.ini). Até 2026-11-26 a tela,
// o SD, o CC1101, o I2C e o I2S estavam nos mesmos GPIOs do target; desde a troca para a placa
// de uma face, só o SPI (11, 12, 13) continua igual. Os números abaixo são os da fiação da
// protoboard, validados na bancada.
// ---------------------------------------------------------------------------
namespace bench {

inline constexpr int8_t kButtonUp = 39;
inline constexpr int8_t kButtonOk = 2;  // GPIO RTC: acorda do deep sleep
inline constexpr int8_t kButtonDown = 47;
// Botão de energia da bancada: o BOOT da DevKitC.
inline constexpr int8_t kPowerButton = 0;
// IR no Bruce de bancada.
inline constexpr int8_t kIrRx = 4;
inline constexpr int8_t kIrTx = 5;
inline constexpr int8_t kI2sMicData = 7;
inline constexpr int8_t kI2cSda = 8;
inline constexpr int8_t kI2cScl = 9;
inline constexpr int8_t kDisplayCs = 10;
inline constexpr int8_t kSpiMosi = 11;  // igual ao target
inline constexpr int8_t kSpiSck = 12;   // igual ao target
inline constexpr int8_t kSpiMiso = 13;  // igual ao target
inline constexpr int8_t kCc1101Cs = 14;
inline constexpr int8_t kCc1101Gdo0 = 15;
inline constexpr int8_t kDisplayBacklight = 16;
inline constexpr int8_t kDisplayDc = 17;
inline constexpr int8_t kDisplayReset = 18;
inline constexpr int8_t kSdCs = 21;
inline constexpr int8_t kI2sBclk = 40;
inline constexpr int8_t kI2sLrclk = 41;
inline constexpr int8_t kI2sSpeakerData = 42;

inline constexpr PinAssignment kPins[] = {
    {"BUTTON_OK", kButtonOk, PinStatus::BenchValidated},
    {"POWER_BUTTON", kPowerButton, PinStatus::Proposed},
    {"IR_RX", kIrRx, PinStatus::BenchValidated},
    {"IR_TX", kIrTx, PinStatus::Proposed},
    {"I2C_SDA", kI2cSda, PinStatus::Proposed},
    {"I2C_SCL", kI2cScl, PinStatus::Proposed},
    {"DISPLAY_CS", kDisplayCs, PinStatus::BenchValidated},
    {"SPI_MOSI", kSpiMosi, PinStatus::BenchValidated},
    {"SPI_SCK", kSpiSck, PinStatus::BenchValidated},
    {"SPI_MISO", kSpiMiso, PinStatus::BenchValidated},
    {"CC1101_CS", kCc1101Cs, PinStatus::BenchValidated},
    {"CC1101_GDO0", kCc1101Gdo0, PinStatus::Proposed},
    {"DISPLAY_BACKLIGHT", kDisplayBacklight, PinStatus::Proposed},
    {"DISPLAY_DC", kDisplayDc, PinStatus::BenchValidated},
    {"DISPLAY_RESET", kDisplayReset, PinStatus::BenchValidated},
    {"USB_DM", target::kUsbDm, PinStatus::ChipFixed},
    {"USB_DP", target::kUsbDp, PinStatus::ChipFixed},
    {"SD_CS", kSdCs, PinStatus::BenchValidated},
    {"BUTTON_UP", kButtonUp, PinStatus::BenchValidated},
    {"BUTTON_DOWN", kButtonDown, PinStatus::BenchValidated},
    // Microfone INMP441 e amplificador MAX98357A. O SD_MODE do amplificador não vai a um GPIO na
    // bancada: a placa dele o mantém ligado.
    {"I2S_BCLK", kI2sBclk, PinStatus::BenchValidated},
    {"I2S_LRCLK", kI2sLrclk, PinStatus::BenchValidated},
    {"I2S_MIC_DATA", kI2sMicData, PinStatus::BenchValidated},
    {"I2S_SPEAKER_DATA", kI2sSpeakerData, PinStatus::Proposed},
};

}  // namespace bench

// ---------------------------------------------------------------------------
// Barramentos e endereços.
// ---------------------------------------------------------------------------
namespace i2c {
inline constexpr uint8_t kMcp23017Buttons = 0x20;
inline constexpr uint8_t kMcp23017Aux = 0x21;  // opcional: vibração e GPIOs do cartucho
inline constexpr uint8_t kPn532 = 0x24;
inline constexpr uint8_t kCartridgeEeprom = 0x50;  // 24C02 no cartucho (proposta)
inline constexpr uint32_t kBusHz = 100000;
}  // namespace i2c

namespace spi {
// 10 MHz foi o que rodou nos jumpers da protoboard (teste_tela); na PCB pode subir.
inline constexpr uint32_t kDisplayHz = 10000000;
inline constexpr uint32_t kCc1101Hz = 1000000;  // valor do teste_cc1101; não validado
inline constexpr uint32_t kSdInitHz = 400000;
}  // namespace spi

// Botões no MCP23017 (0x20): bit 0–7 = GPA0–7, bit 8–15 = GPB0–7. Proposta; a ordem
// pode mudar com o roteamento da PCB (basta alterar esta tabela).
//
// O datasheet do MCP23017 (Microchip DS20001952, revisão D) declara GPA7 e GPB7 como
// somente saída. Por isso os bits 7 e 15 não recebem botão: um MCP23017 oferece 14
// entradas. Ver docs/hardware/sources/PERIPHERALS.md (MCP-03 a MCP-06).
// São 10 botões; sobram 0, 1, 8 e 11 (GPA0, GPA1, GPB0 e GPB3).
namespace mcp_buttons {
// Ordem escolhida em 2026-11-26 pela placa de uma face (cada botão no pino do MCP23017 mais
// perto dele no roteamento).
inline constexpr uint8_t kSelect = 2;     // GPA2
inline constexpr uint8_t kDpadUp = 3;     // GPA3
inline constexpr uint8_t kDpadRight = 4;  // GPA4
inline constexpr uint8_t kDpadLeft = 5;   // GPA5
inline constexpr uint8_t kDpadDown = 6;   // GPA6
inline constexpr uint8_t kActionB = 9;    // GPB1
inline constexpr uint8_t kActionA = 10;   // GPB2
inline constexpr uint8_t kActionY = 12;   // GPB4
inline constexpr uint8_t kActionX = 13;   // GPB5
inline constexpr uint8_t kStart = 14;     // GPB6
// GPA7 e GPB7: não usar como entrada.
inline constexpr uint16_t kOutputOnlyMask = 0x8080;
}  // namespace mcp_buttons

// ---------------------------------------------------------------------------
// Verificações em tempo de compilação.
// ---------------------------------------------------------------------------
namespace detail {

template <size_t N>
constexpr bool allUsable(const PinAssignment (&pins)[N]) {
  for (size_t i = 0; i < N; ++i)
    if (!isUsableGpio(pins[i].gpio)) return false;
  return true;
}

template <size_t N>
constexpr bool noDuplicates(const PinAssignment (&pins)[N]) {
  for (size_t i = 0; i < N; ++i)
    for (size_t j = i + 1; j < N; ++j)
      if (pins[i].gpio == pins[j].gpio) return false;
  return true;
}

}  // namespace detail

static_assert(detail::allUsable(target::kPins), "perfil target usa GPIO indisponível no N16R8");
static_assert(detail::noDuplicates(target::kPins), "perfil target tem GPIO duplicado");
static_assert(detail::allUsable(bench::kPins), "perfil bench usa GPIO indisponível no N16R8");
static_assert(detail::noDuplicates(bench::kPins), "perfil bench tem GPIO duplicado");
static_assert(isAdc1Gpio(target::kVbatSense),
              "a leitura da bateria precisa estar no ADC1 (GPIO 1-10)");
static_assert(!isStrappingGpio(target::kPowerButton),
              "botao de energia no target fora dos pinos de strapping (GPIO_MAP.md, risco 1)");
static_assert(isRtcGpio(target::kPowerButton) && isRtcGpio(target::kChargerStatus) &&
                  isRtcGpio(bench::kPowerButton) && isRtcGpio(bench::kButtonOk),
              "o pino que acorda do deep sleep precisa ser GPIO RTC (0-21)");

}  // namespace dbmo::config
