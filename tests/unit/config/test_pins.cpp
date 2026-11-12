// Integridade do mapa de pinos. As regras estruturais (GPIO utilizável, sem duplicata)
// já são static_assert em pins.h; aqui ficam as conferências que cruzam tabelas e as
// que registram riscos conhecidos, para que uma mudança de pinagem os revise.
#include <cstring>

#include "dbmo/config/pins.h"
#include "dbmo/core/resource_registry.h"
#include "dbmo_test.h"

using namespace dbmo::config;

namespace {

template <size_t N>
const PinAssignment *findSignal(const PinAssignment (&pins)[N], const char *signal) {
  for (size_t i = 0; i < N; ++i)
    if (strcmp(pins[i].signal, signal) == 0) return &pins[i];
  return nullptr;
}

template <size_t N>
bool usesGpio(const PinAssignment (&pins)[N], int gpio) {
  for (size_t i = 0; i < N; ++i)
    if (pins[i].gpio == gpio) return true;
  return false;
}

}  // namespace

void setUp() {}
void tearDown() {}

static void test_gpio_classification_helpers() {
  TEST_ASSERT_TRUE(isUsableGpio(0));
  TEST_ASSERT_TRUE(isUsableGpio(21));
  TEST_ASSERT_FALSE(isUsableGpio(22));  // não existe no ESP32-S3
  TEST_ASSERT_FALSE(isUsableGpio(26));  // flash
  TEST_ASSERT_FALSE(isUsableGpio(35));  // PSRAM octal do N16R8
  TEST_ASSERT_FALSE(isUsableGpio(37));
  TEST_ASSERT_TRUE(isUsableGpio(38));
  TEST_ASSERT_TRUE(isUsableGpio(48));
  TEST_ASSERT_FALSE(isUsableGpio(49));
  TEST_ASSERT_FALSE(isUsableGpio(-1));

  TEST_ASSERT_TRUE(isStrappingGpio(0) && isStrappingGpio(3) && isStrappingGpio(45) &&
                   isStrappingGpio(46));
  TEST_ASSERT_FALSE(isStrappingGpio(1));
  TEST_ASSERT_TRUE(isAdc1Gpio(1) && isAdc1Gpio(10));
  TEST_ASSERT_FALSE(isAdc1Gpio(0) || isAdc1Gpio(11));
  TEST_ASSERT_TRUE(isRtcGpio(21));
  TEST_ASSERT_FALSE(isRtcGpio(38));
}

static void test_target_profile_never_drives_the_free_strapping_pins() {
  for (int8_t gpio : target::kFreePins) {
    TEST_ASSERT_TRUE(isStrappingGpio(gpio));
    TEST_ASSERT_FALSE(usesGpio(target::kPins, gpio));
  }
}

static void test_every_usable_gpio_is_assigned_or_listed_as_free() {
  size_t usable = 0;
  for (int gpio = 0; gpio <= 48; ++gpio)
    if (isUsableGpio(gpio)) ++usable;
  const size_t assigned = sizeof(target::kPins) / sizeof(target::kPins[0]);
  const size_t free = sizeof(target::kFreePins) / sizeof(target::kFreePins[0]);
  // 33 GPIOs utilizáveis no módulo: 31 atribuídos + 2 livres. Se a conta mudar, a
  // documentação (GPIO_MAP.md) precisa mudar junto.
  TEST_ASSERT_EQUAL_size_t(33, usable);
  TEST_ASSERT_EQUAL_size_t(usable, assigned + free);
}

static void test_strapping_pins_in_use_are_flagged() {
  // GPIO0 (botão de energia) é strapping de boot: o risco precisa estar marcado.
  const PinAssignment *power = findSignal(target::kPins, "POWER_BUTTON");
  TEST_ASSERT_NOT_NULL(power);
  TEST_ASSERT_TRUE(isStrappingGpio(power->gpio));
  TEST_ASSERT_TRUE(power->status == PinStatus::ProposedAtRisk);
}

static void test_shared_bus_pins_are_identical_in_both_profiles() {
  const char *shared[] = {"SPI_MOSI",   "SPI_SCK",    "SPI_MISO",      "DISPLAY_CS",
                          "DISPLAY_DC", "DISPLAY_RESET", "CC1101_CS", "SD_CS",
                          "I2C_SDA",    "I2C_SCL"};
  for (const char *signal : shared) {
    const PinAssignment *t = findSignal(target::kPins, signal);
    const PinAssignment *b = findSignal(bench::kPins, signal);
    TEST_ASSERT_NOT_NULL_MESSAGE(t, signal);
    TEST_ASSERT_NOT_NULL_MESSAGE(b, signal);
    TEST_ASSERT_EQUAL_INT8_MESSAGE(t->gpio, b->gpio, signal);
  }
}

static void test_bench_and_target_disagree_exactly_where_documented() {
  // Os botões diretos da bancada ocupam pinos que o perfil target dá a outra função.
  // A migração precisa mover os botões para o MCP23017 antes de ligar essas funções.
  TEST_ASSERT_EQUAL_INT8(target::kIrRx, bench::kButtonUp);
  TEST_ASSERT_EQUAL_INT8(target::kIrTx, bench::kButtonDown);
  TEST_ASSERT_EQUAL_INT8(target::kStickLeftX, bench::kButtonOk);
  TEST_ASSERT_EQUAL_INT8(target::kStickLeftY, bench::kIrRx);
  TEST_ASSERT_EQUAL_INT8(target::kStickRightX, bench::kIrTx);
}

static void test_i2c_addresses_do_not_collide() {
  const uint8_t addresses[] = {i2c::kMcp23017Buttons, i2c::kMcp23017Aux, i2c::kPn532,
                               i2c::kCartridgeEeprom};
  for (size_t i = 0; i < 4; ++i) {
    TEST_ASSERT_TRUE(addresses[i] >= 0x08 && addresses[i] <= 0x77);  // faixa válida de 7 bits
    for (size_t j = i + 1; j < 4; ++j) TEST_ASSERT_NOT_EQUAL(addresses[i], addresses[j]);
  }
}

static void test_mcp_button_bits_are_unique_and_avoid_output_only_pins() {
  const uint8_t bits[] = {
      mcp_buttons::kDpadUp,  mcp_buttons::kDpadDown, mcp_buttons::kDpadLeft,
      mcp_buttons::kDpadRight, mcp_buttons::kActionA, mcp_buttons::kActionB,
      mcp_buttons::kActionX, mcp_buttons::kActionY,  mcp_buttons::kSelect,
      mcp_buttons::kStart,   mcp_buttons::kL1,       mcp_buttons::kR1,
      mcp_buttons::kL2,      mcp_buttons::kR2};
  uint16_t seen = 0;
  for (uint8_t bit : bits) {
    TEST_ASSERT_TRUE(bit < 16);
    TEST_ASSERT_EQUAL_UINT16(0, seen & (1u << bit));
    seen = static_cast<uint16_t>(seen | (1u << bit));
  }
  // GPA7 e GPB7 são somente saída no MCP23017 (datasheet rev. D): nenhum botão neles,
  // e os 14 botões ocupam exatamente as 14 entradas restantes.
  TEST_ASSERT_EQUAL_HEX16(0x8080, mcp_buttons::kOutputOnlyMask);
  TEST_ASSERT_EQUAL_HEX16(0, seen & mcp_buttons::kOutputOnlyMask);
  TEST_ASSERT_EQUAL_HEX16(0x7F7F, seen);
}

static void test_stick_clicks_on_the_second_expander_avoid_output_only_pins() {
  TEST_ASSERT_NOT_EQUAL(mcp_aux::kStickLeftClick, mcp_aux::kStickRightClick);
  TEST_ASSERT_EQUAL_HEX16(0, (1u << mcp_aux::kStickLeftClick) & mcp_buttons::kOutputOnlyMask);
  TEST_ASSERT_EQUAL_HEX16(0, (1u << mcp_aux::kStickRightClick) & mcp_buttons::kOutputOnlyMask);
}

static void test_claiming_the_whole_target_profile_at_runtime_has_no_conflict() {
  // O mesmo mapa passa pelo registro de recursos usado na inicialização.
  dbmo::ResourceRegistry registry;
  for (const PinAssignment &pin : target::kPins)
    TEST_ASSERT_OK(registry.claim(dbmo::ResourceKind::Gpio, static_cast<uint16_t>(pin.gpio),
                                  pin.signal));
  // E um segundo dono no pino de um botão da bancada é detectado.
  TEST_ASSERT_STATUS(dbmo::Status::Busy,
                     registry.claim(dbmo::ResourceKind::Gpio,
                                    static_cast<uint16_t>(bench::kButtonUp), "BUTTON_UP"));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_gpio_classification_helpers);
  RUN_TEST(test_target_profile_never_drives_the_free_strapping_pins);
  RUN_TEST(test_every_usable_gpio_is_assigned_or_listed_as_free);
  RUN_TEST(test_strapping_pins_in_use_are_flagged);
  RUN_TEST(test_shared_bus_pins_are_identical_in_both_profiles);
  RUN_TEST(test_bench_and_target_disagree_exactly_where_documented);
  RUN_TEST(test_i2c_addresses_do_not_collide);
  RUN_TEST(test_mcp_button_bits_are_unique_and_avoid_output_only_pins);
  RUN_TEST(test_stick_clicks_on_the_second_expander_avoid_output_only_pins);
  RUN_TEST(test_claiming_the_whole_target_profile_at_runtime_has_no_conflict);
  return UNITY_END();
}
