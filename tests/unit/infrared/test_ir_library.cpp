#include <cstring>

#include "dbmo/infrared/ir_codec.h"
#include "dbmo/infrared/ir_library.h"
#include "dbmo_test.h"

using dbmo::Status;
using namespace dbmo::infrared;

namespace {

// Guarda em memória, com o mesmo contrato da guarda em arquivo.
class MemoryStorage final : public IIrCodeStorage {
 public:
  Status save(const char *name, const char *text, size_t length) override {
    if (!dbmo::isOk(saveStatus)) return saveStatus;
    snprintf(storedName, sizeof(storedName), "%s", name);
    memcpy(stored, text, length);
    storedLength = length;
    exists = true;
    return Status::Ok;
  }
  dbmo::Result<size_t> load(const char *name, char *buffer, size_t capacity) override {
    if (!exists || strcmp(name, storedName) != 0) return Status::NotFound;
    if (storedLength > capacity) return Status::NoSpace;
    memcpy(buffer, stored, storedLength);
    return storedLength;
  }
  Status remove(const char *name) override {
    if (!exists || strcmp(name, storedName) != 0) return Status::NotFound;
    exists = false;
    return Status::Ok;
  }
  void put(const char *name, const char *text) {
    snprintf(storedName, sizeof(storedName), "%s", name);
    storedLength = strlen(text);
    memcpy(stored, text, storedLength);
    exists = true;
  }

  char storedName[32] = {};
  char stored[kMaxCodeText] = {};
  size_t storedLength = 0;
  bool exists = false;
  Status saveStatus = Status::Ok;
};

StoredCode necStored(uint16_t address, uint16_t command) {
  StoredCode code;
  code.code.protocol = IrProtocol::Nec;
  code.code.bits = 32;
  code.code.address = address;
  code.code.command = command;
  return code;
}

Status parse(const char *text, StoredCode *out) { return parseCode(text, strlen(text), out); }

}  // namespace

void setUp() {}
void tearDown() {}

void test_format_decoded_codes() {
  char text[64];
  dbmo::Result<size_t> length = formatCode(necStored(0x04, 0x08), text, sizeof(text));
  TEST_ASSERT_TRUE(length.ok());
  TEST_ASSERT_EQUAL_STRING("nec:0004:08", text);
  TEST_ASSERT_EQUAL_UINT(11, length.value());

  StoredCode sony;
  sony.code.protocol = IrProtocol::Sony;
  sony.code.bits = 12;
  sony.code.address = 0x01;
  sony.code.command = 0x15;
  TEST_ASSERT_TRUE(formatCode(sony, text, sizeof(text)).ok());
  TEST_ASSERT_EQUAL_STRING("sony:12:0001:15", text);
}

void test_format_raw_code() {
  StoredCode raw;
  raw.isRaw = true;
  raw.raw.carrierHz = 36000;
  raw.raw.durationsUs[0] = 889;
  raw.raw.durationsUs[1] = 889;
  raw.raw.durationsUs[2] = 1778;
  raw.raw.count = 3;
  char text[64];
  TEST_ASSERT_TRUE(formatCode(raw, text, sizeof(text)).ok());
  TEST_ASSERT_EQUAL_STRING("raw:36000:889,889,1778", text);
}

void test_format_errors() {
  char text[8];
  TEST_ASSERT_STATUS(Status::NoSpace, formatCode(necStored(4, 8), text, sizeof(text)).status());
  StoredCode unknown;
  char big[64];
  TEST_ASSERT_STATUS(Status::InvalidArgument, formatCode(unknown, big, sizeof(big)).status());
  StoredCode emptyRaw;
  emptyRaw.isRaw = true;
  TEST_ASSERT_STATUS(Status::InvalidArgument, formatCode(emptyRaw, big, sizeof(big)).status());
  TEST_ASSERT_STATUS(Status::InvalidArgument, formatCode(necStored(4, 8), nullptr, 10).status());
}

void test_parse_decoded_codes() {
  StoredCode code;
  TEST_ASSERT_OK(parse("nec:0004:08", &code));
  TEST_ASSERT_FALSE(code.isRaw);
  TEST_ASSERT_EQUAL(IrProtocol::Nec, code.code.protocol);
  TEST_ASSERT_EQUAL_HEX16(0x04, code.code.address);
  TEST_ASSERT_EQUAL_HEX16(0x08, code.code.command);

  TEST_ASSERT_OK(parse("nec:abCD:ef\n", &code));  // minúsculas e fim de linha
  TEST_ASSERT_EQUAL_HEX16(0xABCD, code.code.address);
  TEST_ASSERT_EQUAL_HEX16(0xEF, code.code.command);

  TEST_ASSERT_OK(parse("sony:20:1ABC:7F\r\n", &code));
  TEST_ASSERT_EQUAL(IrProtocol::Sony, code.code.protocol);
  TEST_ASSERT_EQUAL_UINT8(20, code.code.bits);
  TEST_ASSERT_EQUAL_HEX16(0x1ABC, code.code.address);
  TEST_ASSERT_EQUAL_HEX16(0x7F, code.code.command);
}

void test_parse_raw_code() {
  StoredCode code;
  TEST_ASSERT_OK(parse("raw:38000:9000,4500,562", &code));
  TEST_ASSERT_TRUE(code.isRaw);
  TEST_ASSERT_EQUAL_UINT32(38000, code.raw.carrierHz);
  TEST_ASSERT_EQUAL_UINT16(3, code.raw.count);
  TEST_ASSERT_EQUAL_UINT16(4500, code.raw.durationsUs[1]);
}

void test_parse_rejects_malformed_text() {
  StoredCode code;
  const char *const bad[] = {
      "",
      "nec",
      "nec:",
      "nec:0004",
      "nec:0004:",
      "nec:0004:100",    // comando de 9 bits
      "nec:10000:08",    // endereço de 17 bits
      "nec:00G4:08",     // dígito inválido
      "nec:0004:08:99",  // sobra
      "nec:0004:08 extra",
      "NEC:0004:08",      // prefixo em maiúsculas
      "rc5:0004:08",      // protocolo desconhecido
      "sony:13:0001:15",  // 13 bits não existe
      "sony:12:0020:15",  // endereço não cabe em 5 bits
      "sony:12:0001:80",  // comando de 8 bits
      "sony:99:0001:15",
      "raw:38000:",
      "raw:38000",
      "raw:38000:100,",
      "raw:38000:100,,200",
      "raw:38000:100,0,200",  // duração zero
      "raw:38000:100,70000",  // não cabe em 16 bits
      "raw:1000:100,200",     // portadora fora de 30–60 kHz
      "raw:99999:100,200",
      "raw:38000:-100,200",
      "raw:38000:1e3,200",
  };
  for (const char *text : bad) {
    const Status status = parse(text, &code);
    if (status != Status::Corrupt) TEST_FAIL_MESSAGE(text);
  }
}

void test_parse_rejects_too_many_durations() {
  char text[kMaxCodeText + 64] = "raw:38000:";
  size_t used = strlen(text);
  for (size_t i = 0; i <= RawTiming::kMaxEntries; ++i)
    used += static_cast<size_t>(snprintf(text + used, sizeof(text) - used, i == 0 ? "5" : ",5"));
  StoredCode code;
  TEST_ASSERT_STATUS(Status::Corrupt, parseCode(text, used, &code));
}

void test_parse_does_not_read_past_length() {
  // Texto sem terminador: só os bytes indicados contam.
  const char text[] = {'n', 'e', 'c', ':', '0', '4', ':', '0', '8', '9', '9'};
  StoredCode code;
  TEST_ASSERT_OK(parseCode(text, 9, &code));
  TEST_ASSERT_EQUAL_HEX16(0x08, code.code.command);
}

void test_round_trip_maximum_raw() {
  StoredCode raw;
  raw.isRaw = true;
  raw.raw.carrierHz = 60000;
  raw.raw.count = RawTiming::kMaxEntries;
  for (uint16_t i = 0; i < raw.raw.count; ++i) raw.raw.durationsUs[i] = 65535;
  char text[kMaxCodeText];
  const dbmo::Result<size_t> length = formatCode(raw, text, sizeof(text));
  TEST_ASSERT_TRUE(length.ok());  // o maior código possível cabe no buffer do módulo
  StoredCode parsed;
  TEST_ASSERT_OK(parseCode(text, length.value(), &parsed));
  TEST_ASSERT_EQUAL_UINT16(RawTiming::kMaxEntries, parsed.raw.count);
  TEST_ASSERT_EQUAL_UINT16(65535, parsed.raw.durationsUs[RawTiming::kMaxEntries - 1]);
}

void test_code_names() {
  TEST_ASSERT_TRUE(isValidCodeName("tv_sala"));
  TEST_ASSERT_TRUE(isValidCodeName("ar-condicionado-2"));
  TEST_ASSERT_TRUE(isValidCodeName("a"));
  TEST_ASSERT_TRUE(isValidCodeName("abcdefghijklmnopqrstuvwx"));    // 24
  TEST_ASSERT_FALSE(isValidCodeName("abcdefghijklmnopqrstuvwxy"));  // 25
  TEST_ASSERT_FALSE(isValidCodeName(""));
  TEST_ASSERT_FALSE(isValidCodeName(nullptr));
  TEST_ASSERT_FALSE(isValidCodeName("TV"));         // maiúsculas
  TEST_ASSERT_FALSE(isValidCodeName("../config"));  // tentativa de sair da pasta
  TEST_ASSERT_FALSE(isValidCodeName("a/b"));
  TEST_ASSERT_FALSE(isValidCodeName("tv.ir"));
  TEST_ASSERT_FALSE(isValidCodeName("tv sala"));
}

void test_library_save_load_remove() {
  MemoryStorage storage;
  IrLibrary library(storage);
  TEST_ASSERT_OK(library.save("tv_power", necStored(0x04, 0x08)));
  TEST_ASSERT_EQUAL_STRING("tv_power", storage.storedName);

  StoredCode loaded;
  TEST_ASSERT_OK(library.load("tv_power", &loaded));
  TEST_ASSERT_TRUE(loaded.code == necStored(0x04, 0x08).code);
  TEST_ASSERT_STATUS(Status::NotFound, library.load("outro", &loaded));

  TEST_ASSERT_OK(library.remove("tv_power"));
  TEST_ASSERT_STATUS(Status::NotFound, library.load("tv_power", &loaded));
}

void test_library_validates_names_before_touching_storage() {
  MemoryStorage storage;
  IrLibrary library(storage);
  StoredCode code = necStored(1, 2);
  TEST_ASSERT_STATUS(Status::InvalidArgument, library.save("../config", code));
  TEST_ASSERT_STATUS(Status::InvalidArgument, library.load("a/b", &code));
  TEST_ASSERT_STATUS(Status::InvalidArgument, library.remove(""));
  TEST_ASSERT_FALSE(storage.exists);
}

void test_library_reports_corrupt_and_failed_storage() {
  MemoryStorage storage;
  IrLibrary library(storage);
  storage.put("quebrado", "nec:zz:zz");
  StoredCode code;
  TEST_ASSERT_STATUS(Status::Corrupt, library.load("quebrado", &code));
  storage.saveStatus = Status::IoError;
  TEST_ASSERT_STATUS(Status::IoError, library.save("tv", necStored(1, 2)));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_format_decoded_codes);
  RUN_TEST(test_format_raw_code);
  RUN_TEST(test_format_errors);
  RUN_TEST(test_parse_decoded_codes);
  RUN_TEST(test_parse_raw_code);
  RUN_TEST(test_parse_rejects_malformed_text);
  RUN_TEST(test_parse_rejects_too_many_durations);
  RUN_TEST(test_parse_does_not_read_past_length);
  RUN_TEST(test_round_trip_maximum_raw);
  RUN_TEST(test_code_names);
  RUN_TEST(test_library_save_load_remove);
  RUN_TEST(test_library_validates_names_before_touching_storage);
  RUN_TEST(test_library_reports_corrupt_and_failed_storage);
  return UNITY_END();
}
