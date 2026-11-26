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

// GPIO RTC fora dos pinos de strapping: acorda do deep sleep e não é lido como strap no reset.
// Estava no GPIO 0 até 2026-11-25 (risco 1 do GPIO_MAP.md); o GPIO 0 ficou só como BOOT.
inline constexpr int8_t kPowerButton = 2;
inline constexpr int8_t kVbatSense = 1;  // ADC1; divisor 100k/100k, na placa de energia
// Status do carregador, da placa de energia. GPIO RTC (acorda ao ligar o carregador) fora dos
// pinos que a bancada usa (o IR da bancada está no 4 e no 5).
inline constexpr int8_t kChargerStatus = 6;
inline constexpr int8_t kAmpShutdown = 3;  // MAX98357A SD_MODE; strapping (JTAG)
inline constexpr int8_t kI2sMicData = 7;   // INMP441 SD
inline constexpr int8_t kI2cSda = 8;
inline constexpr int8_t kI2cScl = 9;
inline constexpr int8_t kDisplayCs = 10;
inline constexpr int8_t kSpiMosi = 11;
inline constexpr int8_t kSpiSck = 12;
inline constexpr int8_t kSpiMiso = 13;
inline constexpr int8_t kCc1101Cs = 14;
inline constexpr int8_t kCc1101Gdo0 = 15;
inline constexpr int8_t kDisplayBacklight = 16;
inline constexpr int8_t kDisplayDc = 17;
inline constexpr int8_t kDisplayReset = 18;
inline constexpr int8_t kUsbDm = 19;
inline constexpr int8_t kUsbDp = 20;
inline constexpr int8_t kSdCs = 21;
inline constexpr int8_t kIrRx = 39;
inline constexpr int8_t kI2sBclk = 40;
inline constexpr int8_t kI2sLrclk = 41;
inline constexpr int8_t kI2sSpeakerData = 42;  // MAX98357A DIN
inline constexpr int8_t kIrTx = 47;
inline constexpr int8_t kPowerHold = 48;  // power-latch, na placa de energia

inline constexpr PinAssignment kPins[] = {
    {"POWER_BUTTON", kPowerButton, PinStatus::Proposed},
    {"VBAT_SENSE", kVbatSense, PinStatus::Proposed},
    {"CHARGER_STATUS", kChargerStatus, PinStatus::Proposed},
    {"AMP_SHUTDOWN", kAmpShutdown, PinStatus::Proposed},
    {"I2S_MIC_DATA", kI2sMicData, PinStatus::BenchValidated},
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
    {"USB_DM", kUsbDm, PinStatus::ChipFixed},
    {"USB_DP", kUsbDp, PinStatus::ChipFixed},
    {"SD_CS", kSdCs, PinStatus::BenchValidated},
    {"IR_RX", kIrRx, PinStatus::Proposed},
    {"I2S_BCLK", kI2sBclk, PinStatus::BenchValidated},
    {"I2S_LRCLK", kI2sLrclk, PinStatus::BenchValidated},
    {"I2S_SPEAKER_DATA", kI2sSpeakerData, PinStatus::Proposed},
    {"IR_TX", kIrTx, PinStatus::Proposed},
    {"POWER_HOLD", kPowerHold, PinStatus::Proposed},
};

// Livres no perfil target. 4 e 5 (ADC1) eram dos analógicos, retirados do v1 em 2026-11-25 (o 2
// virou o botão de energia e o 6 o status do carregador). 38, 43 e 44 eram do cartucho, também
// retirado em 2026-11-25; 43 e 44 são a UART0, ligada ao conversor USB-serial da DevKitC. 0, 45 e
// 46 são de strapping: o 0 fica só como BOOT (pad de teste ou botão interno), e 45 e 46 não podem
// ser puxados no reset.
inline constexpr int8_t kFreePins[] = {0, 4, 5, 38, 43, 44, 45, 46};

}  // namespace target

// ---------------------------------------------------------------------------
// Perfil bench: a protoboard de hoje (igual ao firmware/bruce/bmo.ini).
// Tela e barramento SPI iguais ao target; 3 botões ligados direto em GPIO.
// ---------------------------------------------------------------------------
namespace bench {

inline constexpr int8_t kButtonUp = 39;
inline constexpr int8_t kButtonOk = 2;  // GPIO RTC: acorda do deep sleep
inline constexpr int8_t kButtonDown = 47;
// Botão de energia da bancada: o BOOT da DevKitC. O GPIO 2, que é o do target, aqui é o OK.
inline constexpr int8_t kPowerButton = 0;
// IR no Bruce de bancada (ainda sem o módulo ligado).
inline constexpr int8_t kIrRx = 4;
inline constexpr int8_t kIrTx = 5;

inline constexpr PinAssignment kPins[] = {
    {"BUTTON_OK", kButtonOk, PinStatus::BenchValidated},
    {"POWER_BUTTON", kPowerButton, PinStatus::Proposed},
    {"IR_RX", kIrRx, PinStatus::BenchValidated},
    {"IR_TX", kIrTx, PinStatus::Proposed},
    {"I2C_SDA", target::kI2cSda, PinStatus::Proposed},
    {"I2C_SCL", target::kI2cScl, PinStatus::Proposed},
    {"DISPLAY_CS", target::kDisplayCs, PinStatus::BenchValidated},
    {"SPI_MOSI", target::kSpiMosi, PinStatus::BenchValidated},
    {"SPI_SCK", target::kSpiSck, PinStatus::BenchValidated},
    {"SPI_MISO", target::kSpiMiso, PinStatus::BenchValidated},
    {"CC1101_CS", target::kCc1101Cs, PinStatus::BenchValidated},
    {"CC1101_GDO0", target::kCc1101Gdo0, PinStatus::Proposed},
    {"DISPLAY_BACKLIGHT", target::kDisplayBacklight, PinStatus::Proposed},
    {"DISPLAY_DC", target::kDisplayDc, PinStatus::BenchValidated},
    {"DISPLAY_RESET", target::kDisplayReset, PinStatus::BenchValidated},
    {"USB_DM", target::kUsbDm, PinStatus::ChipFixed},
    {"USB_DP", target::kUsbDp, PinStatus::ChipFixed},
    {"SD_CS", target::kSdCs, PinStatus::BenchValidated},
    {"BUTTON_UP", kButtonUp, PinStatus::BenchValidated},
    {"BUTTON_DOWN", kButtonDown, PinStatus::BenchValidated},
    // Microfone INMP441 e amplificador MAX98357A nos mesmos pinos I2S do target. O SD_MODE do
    // amplificador não vai a um GPIO na bancada: a placa dele o mantém ligado.
    {"I2S_BCLK", target::kI2sBclk, PinStatus::BenchValidated},
    {"I2S_LRCLK", target::kI2sLrclk, PinStatus::BenchValidated},
    {"I2S_MIC_DATA", target::kI2sMicData, PinStatus::BenchValidated},
    {"I2S_SPEAKER_DATA", target::kI2sSpeakerData, PinStatus::Proposed},
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
// São 10 botões; os bits 11 a 14 ficaram livres quando o v1 perdeu os gatilhos de ombro
// (2026-11-25).
namespace mcp_buttons {
inline constexpr uint8_t kDpadUp = 0;
inline constexpr uint8_t kDpadDown = 1;
inline constexpr uint8_t kDpadLeft = 2;
inline constexpr uint8_t kDpadRight = 3;
inline constexpr uint8_t kActionA = 4;
inline constexpr uint8_t kActionB = 5;
inline constexpr uint8_t kActionX = 6;
inline constexpr uint8_t kActionY = 8;
inline constexpr uint8_t kSelect = 9;
inline constexpr uint8_t kStart = 10;
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
