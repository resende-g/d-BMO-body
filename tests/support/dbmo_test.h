// Apoio aos testes nativos: Unity + comparações de Status com mensagem legível.
#pragma once

#include "dbmo/core/status.h"
#include "unity.h"

#define TEST_ASSERT_STATUS(expected, actual) \
  TEST_ASSERT_EQUAL_STRING(::dbmo::toString(expected), ::dbmo::toString(actual))

#define TEST_ASSERT_OK(actual) TEST_ASSERT_STATUS(::dbmo::Status::Ok, (actual))
