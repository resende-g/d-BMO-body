#include "dbmo/core/resource_registry.h"
#include "dbmo_test.h"

using dbmo::ResourceKind;
using dbmo::ResourceRegistry;
using dbmo::Status;

void setUp() {}
void tearDown() {}

static void test_second_owner_of_the_same_gpio_is_refused() {
  ResourceRegistry registry;
  TEST_ASSERT_OK(registry.claim(ResourceKind::Gpio, 39, "input"));
  // Mesmo dono de novo: idempotente.
  TEST_ASSERT_OK(registry.claim(ResourceKind::Gpio, 39, "input"));
  // Outro dono no mesmo pino: conflito de pinagem.
  TEST_ASSERT_STATUS(Status::Busy, registry.claim(ResourceKind::Gpio, 39, "infrared"));
  TEST_ASSERT_EQUAL_STRING("input", registry.ownerOf(ResourceKind::Gpio, 39));
  TEST_ASSERT_EQUAL_size_t(1, registry.count());
}

static void test_kinds_are_independent_namespaces() {
  ResourceRegistry registry;
  TEST_ASSERT_OK(registry.claim(ResourceKind::Gpio, 0x20, "power"));
  TEST_ASSERT_OK(registry.claim(ResourceKind::I2cAddress, 0x20, "input"));
  TEST_ASSERT_EQUAL_STRING("power", registry.ownerOf(ResourceKind::Gpio, 0x20));
  TEST_ASSERT_EQUAL_STRING("input", registry.ownerOf(ResourceKind::I2cAddress, 0x20));
  TEST_ASSERT_NULL(registry.ownerOf(ResourceKind::SpiChipSelect, 0x20));
}

static void test_release_rules() {
  ResourceRegistry registry;
  TEST_ASSERT_OK(registry.claim(ResourceKind::Gpio, 14, "radio"));
  TEST_ASSERT_STATUS(Status::NotPermitted, registry.release(ResourceKind::Gpio, 14, "nfc"));
  TEST_ASSERT_STATUS(Status::NotFound, registry.release(ResourceKind::Gpio, 15, "radio"));
  TEST_ASSERT_OK(registry.release(ResourceKind::Gpio, 14, "radio"));
  TEST_ASSERT_NULL(registry.ownerOf(ResourceKind::Gpio, 14));
  // Liberado, outro dono pode reservar.
  TEST_ASSERT_OK(registry.claim(ResourceKind::Gpio, 14, "nfc"));
}

static void test_release_all_frees_only_that_owner() {
  ResourceRegistry registry;
  TEST_ASSERT_OK(registry.claim(ResourceKind::Gpio, 40, "audio"));
  TEST_ASSERT_OK(registry.claim(ResourceKind::Gpio, 41, "audio"));
  TEST_ASSERT_OK(registry.claim(ResourceKind::Gpio, 14, "radio"));
  TEST_ASSERT_OK(registry.claim(ResourceKind::Gpio, 42, "audio"));
  TEST_ASSERT_EQUAL_size_t(3, registry.releaseAll("audio"));
  TEST_ASSERT_EQUAL_size_t(1, registry.count());
  TEST_ASSERT_EQUAL_STRING("radio", registry.ownerOf(ResourceKind::Gpio, 14));
  TEST_ASSERT_EQUAL_size_t(0, registry.releaseAll("audio"));
}

static void test_invalid_owner_and_capacity() {
  ResourceRegistry registry;
  TEST_ASSERT_STATUS(Status::InvalidArgument, registry.claim(ResourceKind::Gpio, 1, nullptr));
  TEST_ASSERT_STATUS(Status::InvalidArgument, registry.claim(ResourceKind::Gpio, 1, ""));
  for (uint16_t i = 0; i < ResourceRegistry::kMaxClaims; ++i)
    TEST_ASSERT_OK(registry.claim(ResourceKind::Peripheral, i, "teste"));
  TEST_ASSERT_STATUS(Status::NoSpace, registry.claim(ResourceKind::Peripheral, 999, "teste"));
}

static void test_owner_is_compared_by_content() {
  ResourceRegistry registry;
  const char first[] = "display";
  const char second[] = "display";
  TEST_ASSERT_OK(registry.claim(ResourceKind::SpiChipSelect, 10, first));
  TEST_ASSERT_OK(registry.claim(ResourceKind::SpiChipSelect, 10, second));
  TEST_ASSERT_OK(registry.release(ResourceKind::SpiChipSelect, 10, second));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_second_owner_of_the_same_gpio_is_refused);
  RUN_TEST(test_kinds_are_independent_namespaces);
  RUN_TEST(test_release_rules);
  RUN_TEST(test_release_all_frees_only_that_owner);
  RUN_TEST(test_invalid_owner_and_capacity);
  RUN_TEST(test_owner_is_compared_by_content);
  return UNITY_END();
}
