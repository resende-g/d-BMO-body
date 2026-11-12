#include <cstdio>
#include <cstring>

#include "dbmo/app/console.h"
#include "dbmo_test.h"

using dbmo::Status;
using dbmo::app::Console;

namespace {

class CapturedOutput final : public dbmo::app::IConsoleOutput {
 public:
  void write(const char *text) override {
    const size_t length = strlen(text);
    if (used + length < sizeof(buffer)) {
      memcpy(buffer + used, text, length + 1);
      used += length;
    }
  }
  bool contains(const char *fragment) const { return strstr(buffer, fragment) != nullptr; }
  void clear() {
    used = 0;
    buffer[0] = '\0';
  }

  char buffer[2048] = {};
  size_t used = 0;
};

struct Call {
  int count = 0;
  int argc = 0;
  char argv[Console::kMaxArgs][48] = {};
};

void record(Console &, int argc, const char *const *argv, void *context) {
  auto *call = static_cast<Call *>(context);
  ++call->count;
  call->argc = argc;
  for (int i = 0; i < argc; ++i) snprintf(call->argv[i], sizeof(call->argv[i]), "%s", argv[i]);
}

}  // namespace

void setUp() {}
void tearDown() {}

static void test_line_is_split_into_arguments() {
  CapturedOutput out;
  Console console(out);
  Call call;
  TEST_ASSERT_OK(console.registerCommand({"face", "mostra uma expressao", record, &call}));

  console.feed("face   happy 1500\n");
  TEST_ASSERT_EQUAL_INT(1, call.count);
  TEST_ASSERT_EQUAL_INT(3, call.argc);
  TEST_ASSERT_EQUAL_STRING("face", call.argv[0]);
  TEST_ASSERT_EQUAL_STRING("happy", call.argv[1]);
  TEST_ASSERT_EQUAL_STRING("1500", call.argv[2]);
}

static void test_quoted_argument_keeps_spaces() {
  CapturedOutput out;
  Console console(out);
  Call call;
  TEST_ASSERT_OK(console.registerCommand({"set", "", record, &call}));

  TEST_ASSERT_OK(console.execute("set device.name \"bancada 1\""));
  TEST_ASSERT_EQUAL_INT(3, call.argc);
  TEST_ASSERT_EQUAL_STRING("bancada 1", call.argv[2]);

  TEST_ASSERT_STATUS(Status::InvalidArgument, console.execute("set x \"sem fechar"));
  TEST_ASSERT_EQUAL_INT(1, call.count);
}

static void test_line_endings_cr_lf_and_crlf_run_the_command_once() {
  CapturedOutput out;
  Console console(out);
  Call call;
  TEST_ASSERT_OK(console.registerCommand({"ping", "", record, &call}));

  console.feed("ping\r\n");
  TEST_ASSERT_EQUAL_INT(1, call.count);
  console.feed("ping\r");
  TEST_ASSERT_EQUAL_INT(2, call.count);
  console.feed("ping\n");
  TEST_ASSERT_EQUAL_INT(3, call.count);
  // Linhas vazias não fazem nada nem geram mensagem de erro.
  out.clear();
  console.feed("\n\r\n\n");
  TEST_ASSERT_EQUAL_INT(3, call.count);
  TEST_ASSERT_EQUAL_size_t(0, out.used);
}

static void test_backspace_edits_the_line() {
  CapturedOutput out;
  Console console(out);
  Call call;
  TEST_ASSERT_OK(console.registerCommand({"ping", "", record, &call}));
  console.feed("pinx\bg\n");
  TEST_ASSERT_EQUAL_INT(1, call.count);
  // Backspace em linha vazia não quebra nada.
  console.feed("\b\b\x7Fping\n");
  TEST_ASSERT_EQUAL_INT(2, call.count);
}

static void test_unknown_command_and_too_many_arguments_are_reported() {
  CapturedOutput out;
  Console console(out);
  Call call;
  TEST_ASSERT_OK(console.registerCommand({"ping", "", record, &call}));

  console.feed("pong\n");
  TEST_ASSERT_TRUE(out.contains("comando desconhecido"));
  TEST_ASSERT_STATUS(Status::NotFound, console.execute("pong"));

  out.clear();
  console.feed("ping 1 2 3 4 5 6\n");  // 7 argumentos, o limite é 6
  TEST_ASSERT_TRUE(out.contains("argumentos invalidos"));
  TEST_ASSERT_EQUAL_INT(0, call.count);
}

static void test_overlong_line_is_discarded_whole() {
  CapturedOutput out;
  Console console(out);
  Call call;
  TEST_ASSERT_OK(console.registerCommand({"ping", "", record, &call}));

  console.feed("ping ");
  for (size_t i = 0; i < Console::kLineMax + 10; ++i) console.feed('a');
  console.feed('\n');
  TEST_ASSERT_TRUE(out.contains("linha longa demais"));
  TEST_ASSERT_EQUAL_INT(0, call.count);  // nada é executado com a linha truncada

  // A linha seguinte funciona normalmente.
  console.feed("ping\n");
  TEST_ASSERT_EQUAL_INT(1, call.count);
}

static void test_non_printable_bytes_are_ignored() {
  CapturedOutput out;
  Console console(out);
  Call call;
  TEST_ASSERT_OK(console.registerCommand({"ping", "", record, &call}));
  console.feed("pi\x01\x1Bng\xC3\xA9\n");
  TEST_ASSERT_EQUAL_INT(1, call.count);
}

static void test_registration_rules_and_help() {
  CapturedOutput out;
  Console console(out);
  Call call;
  TEST_ASSERT_EQUAL_size_t(1, console.commandCount());  // help já vem
  TEST_ASSERT_STATUS(Status::InvalidArgument, console.registerCommand({"", "", record, &call}));
  TEST_ASSERT_STATUS(Status::InvalidArgument, console.registerCommand({"x", "", nullptr, &call}));
  TEST_ASSERT_OK(console.registerCommand({"status", "estado dos subsistemas", record, &call}));
  TEST_ASSERT_STATUS(Status::AlreadyInitialized,
                     console.registerCommand({"status", "", record, &call}));
  TEST_ASSERT_STATUS(Status::AlreadyInitialized, console.registerCommand({"help", "", record, &call}));

  console.feed("help\n");
  TEST_ASSERT_TRUE(out.contains("status"));
  TEST_ASSERT_TRUE(out.contains("estado dos subsistemas"));

  static char names[Console::kMaxCommands][8];
  for (size_t i = console.commandCount(); i < Console::kMaxCommands; ++i) {
    snprintf(names[i], sizeof(names[i]), "c%02u", static_cast<unsigned>(i));
    TEST_ASSERT_OK(console.registerCommand({names[i], "", record, &call}));
  }
  TEST_ASSERT_STATUS(Status::NoSpace, console.registerCommand({"extra", "", record, &call}));
}

static void test_echo_repeats_typed_characters() {
  CapturedOutput out;
  Console console(out);
  Call call;
  TEST_ASSERT_OK(console.registerCommand({"ping", "", record, &call}));
  console.setEcho(true);
  console.feed("ping\n");
  TEST_ASSERT_TRUE(out.contains("ping\r\n"));
}

static void test_printf_truncates_instead_of_overflowing() {
  CapturedOutput out;
  Console console(out);
  char big[400];
  memset(big, 'z', sizeof(big) - 1);
  big[sizeof(big) - 1] = '\0';
  console.printf("%s", big);
  TEST_ASSERT_EQUAL_size_t(Console::kPrintMax - 1, out.used);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_line_is_split_into_arguments);
  RUN_TEST(test_quoted_argument_keeps_spaces);
  RUN_TEST(test_line_endings_cr_lf_and_crlf_run_the_command_once);
  RUN_TEST(test_backspace_edits_the_line);
  RUN_TEST(test_unknown_command_and_too_many_arguments_are_reported);
  RUN_TEST(test_overlong_line_is_discarded_whole);
  RUN_TEST(test_non_printable_bytes_are_ignored);
  RUN_TEST(test_registration_rules_and_help);
  RUN_TEST(test_echo_repeats_typed_characters);
  RUN_TEST(test_printf_truncates_instead_of_overflowing);
  return UNITY_END();
}
