#include <cstring>

#include "dbmo/core/subsystem.h"
#include "dbmo/core/system.h"
#include "dbmo_test.h"

using dbmo::Result;
using dbmo::Status;

void setUp() {}
void tearDown() {}

static void test_every_status_has_a_distinct_name() {
  const Status all[] = {
      Status::Ok,           Status::InvalidArgument, Status::OutOfRange,
      Status::NotInitialized, Status::AlreadyInitialized, Status::NotFound,
      Status::Timeout,      Status::Busy,            Status::IoError,
      Status::CrcError,     Status::ProtocolError,   Status::Corrupt,
      Status::NoSpace,      Status::Unsupported,     Status::NotPermitted,
      Status::Incompatible, Status::HardwareAbsent,  Status::InternalError,
  };
  const size_t count = sizeof(all) / sizeof(all[0]);
  for (size_t i = 0; i < count; ++i) {
    TEST_ASSERT_NOT_EQUAL(0, strcmp("Unknown", dbmo::toString(all[i])));
    for (size_t j = i + 1; j < count; ++j)
      TEST_ASSERT_NOT_EQUAL(0, strcmp(dbmo::toString(all[i]), dbmo::toString(all[j])));
  }
}

static void test_result_carries_value_or_error() {
  const Result<int> good = 42;
  TEST_ASSERT_TRUE(good.ok());
  TEST_ASSERT_EQUAL_INT(42, good.value());
  TEST_ASSERT_EQUAL_INT(42, good.valueOr(7));

  const Result<int> bad = Status::Timeout;
  TEST_ASSERT_FALSE(bad.ok());
  TEST_ASSERT_STATUS(Status::Timeout, bad.status());
  TEST_ASSERT_EQUAL_INT(7, bad.valueOr(7));
}

static Status failsThenPropagates(Status inner, bool *reachedEnd) {
  DBMO_TRY(inner);
  *reachedEnd = true;
  return Status::Ok;
}

static void test_try_macro_propagates_only_errors() {
  bool reached = false;
  TEST_ASSERT_STATUS(Status::IoError, failsThenPropagates(Status::IoError, &reached));
  TEST_ASSERT_FALSE(reached);
  TEST_ASSERT_OK(failsThenPropagates(Status::Ok, &reached));
  TEST_ASSERT_TRUE(reached);
}

static void test_state_names() {
  TEST_ASSERT_EQUAL_STRING("Absent", dbmo::toString(dbmo::SubsystemState::Absent));
  TEST_ASSERT_EQUAL_STRING("Degraded", dbmo::toString(dbmo::SystemState::Degraded));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_every_status_has_a_distinct_name);
  RUN_TEST(test_result_carries_value_or_error);
  RUN_TEST(test_try_macro_propagates_only_errors);
  RUN_TEST(test_state_names);
  return UNITY_END();
}
