#include "dbmo/infrared/ir_codec.h"
#include "dbmo_test.h"

using dbmo::Status;
using namespace dbmo::infrared;

namespace {

IrCode necCode(uint16_t address, uint16_t command) {
  IrCode code;
  code.protocol = IrProtocol::Nec;
  code.bits = 32;
  code.address = address;
  code.command = command;
  return code;
}

IrCode sonyCode(uint8_t bits, uint16_t address, uint16_t command) {
  IrCode code;
  code.protocol = IrProtocol::Sony;
  code.bits = bits;
  code.address = address;
  code.command = command;
  return code;
}

// Distorção típica de um receptor demodulado: marcas mais longas, espaços mais curtos.
RawTiming distort(RawTiming timing, int deltaUs) {
  for (uint16_t i = 0; i < timing.count; ++i) {
    const int value = timing.durationsUs[i] + (i % 2 == 0 ? deltaUs : -deltaUs);
    timing.durationsUs[i] = static_cast<uint16_t>(value);
  }
  return timing;
}

}  // namespace

void setUp() {}
void tearDown() {}

void test_nec_frame_layout() {
  RawTiming timing;
  TEST_ASSERT_OK(encode(necCode(0x04, 0x08), &timing));
  TEST_ASSERT_EQUAL_UINT16(67, timing.count);
  TEST_ASSERT_EQUAL_UINT32(38000, timing.carrierHz);
  TEST_ASSERT_EQUAL_UINT16(9000, timing.durationsUs[0]);
  TEST_ASSERT_EQUAL_UINT16(4500, timing.durationsUs[1]);
  // Endereço 0x04, bit menos significativo primeiro: 0, 0, 1, 0...
  TEST_ASSERT_EQUAL_UINT16(562, timing.durationsUs[3]);
  TEST_ASSERT_EQUAL_UINT16(562, timing.durationsUs[5]);
  TEST_ASSERT_EQUAL_UINT16(1688, timing.durationsUs[7]);
  // Segundo byte = inverso do endereço: começa com 1, 1, 0.
  TEST_ASSERT_EQUAL_UINT16(1688, timing.durationsUs[3 + 16]);
  TEST_ASSERT_EQUAL_UINT16(1688, timing.durationsUs[5 + 16]);
  TEST_ASSERT_EQUAL_UINT16(562, timing.durationsUs[7 + 16]);
  // Cada par byte/inverso tem 8 uns e 8 zeros, então todo quadro dura o mesmo:
  // 13500 + 32 × 562 + 16 × 562 + 16 × 1688 + 562 = 68046 µs.
  TEST_ASSERT_EQUAL_UINT32(68046, timing.totalUs());
}

void test_nec_round_trip() {
  const uint16_t addresses[] = {0x00, 0x04, 0x80, 0xFF};
  const uint16_t commands[] = {0x00, 0x08, 0x5A, 0xFF};
  for (uint16_t address : addresses) {
    for (uint16_t command : commands) {
      RawTiming timing;
      TEST_ASSERT_OK(encode(necCode(address, command), &timing));
      IrCode decoded;
      TEST_ASSERT_OK(decode(timing, &decoded));
      TEST_ASSERT_EQUAL(IrProtocol::Nec, decoded.protocol);
      TEST_ASSERT_EQUAL_UINT8(32, decoded.bits);
      TEST_ASSERT_EQUAL_HEX16(address, decoded.address);
      TEST_ASSERT_EQUAL_HEX16(command, decoded.command);
    }
  }
}

void test_nec_extended_address() {
  RawTiming timing;
  TEST_ASSERT_OK(encode(necCode(0x1234, 0x56), &timing));
  IrCode decoded;
  TEST_ASSERT_OK(decode(timing, &decoded));
  TEST_ASSERT_EQUAL_HEX16(0x1234, decoded.address);
  TEST_ASSERT_EQUAL_HEX16(0x56, decoded.command);
}

void test_nec_extended_address_with_zero_high_byte_survives_round_trip() {
  // Quadro 04 00 08 F7: endereço estendido 0x0004, diferente do endereço de 8 bits 0x04.
  IrCode extended = necCode(0x0004, 0x08);
  extended.extended = true;
  RawTiming timing;
  TEST_ASSERT_OK(encode(extended, &timing));
  IrCode decoded;
  TEST_ASSERT_OK(decode(timing, &decoded));
  TEST_ASSERT_TRUE(decoded.extended);
  TEST_ASSERT_EQUAL_HEX16(0x0004, decoded.address);
  RawTiming again;
  TEST_ASSERT_OK(encode(decoded, &again));
  TEST_ASSERT_EQUAL_UINT16_ARRAY(timing.durationsUs, again.durationsUs, timing.count);

  RawTiming standard;
  TEST_ASSERT_OK(encode(necCode(0x04, 0x08), &standard));
  TEST_ASSERT_FALSE(standard.durationsUs[3 + 16] ==
                    timing.durationsUs[3 + 16]);  // sinais diferentes
}

void test_nec_tolerates_receiver_distortion() {
  RawTiming timing;
  TEST_ASSERT_OK(encode(necCode(0x20, 0xDF), &timing));
  IrCode decoded;
  TEST_ASSERT_OK(decode(distort(timing, 150), &decoded));
  TEST_ASSERT_EQUAL_HEX16(0x20, decoded.address);
  TEST_ASSERT_EQUAL_HEX16(0xDF, decoded.command);
  TEST_ASSERT_OK(decode(distort(timing, -100), &decoded));
  TEST_ASSERT_EQUAL_HEX16(0xDF, decoded.command);
}

void test_nec_corrupted_command_is_rejected() {
  RawTiming timing;
  TEST_ASSERT_OK(encode(necCode(0x20, 0x10), &timing));
  // Vira um bit do comando sem virar o do inverso.
  timing.durationsUs[3 + 2 * 16] = nec::kOneSpaceUs;
  IrCode decoded;
  TEST_ASSERT_STATUS(Status::CrcError, decode(timing, &decoded));
}

void test_nec_truncated_frame_is_not_nec() {
  RawTiming timing;
  TEST_ASSERT_OK(encode(necCode(0x20, 0x10), &timing));
  timing.count = 40;
  IrCode decoded;
  TEST_ASSERT_STATUS(Status::NotFound, decode(timing, &decoded));
}

void test_nec_repeat_code() {
  RawTiming repeat;
  repeat.durationsUs[0] = 9000;
  repeat.durationsUs[1] = 2250;
  repeat.durationsUs[2] = 562;
  repeat.count = 3;
  TEST_ASSERT_TRUE(isNecRepeat(repeat));
  repeat.durationsUs[1] = 4500;  // isso é o líder de um quadro, não repetição
  TEST_ASSERT_FALSE(isNecRepeat(repeat));
  RawTiming frame;
  TEST_ASSERT_OK(encode(necCode(1, 2), &frame));
  TEST_ASSERT_FALSE(isNecRepeat(frame));
  IrCode decoded;
  repeat.durationsUs[1] = 2250;
  TEST_ASSERT_STATUS(Status::NotFound, decode(repeat, &decoded));
}

void test_nec_rejects_unrepresentable_command() {
  RawTiming timing;
  TEST_ASSERT_STATUS(Status::InvalidArgument, encode(necCode(0x10, 0x100), &timing));
}

void test_sony_frame_layout() {
  RawTiming timing;
  // 12 bits: comando 0x15 (0b0010101), endereço 0x01.
  TEST_ASSERT_OK(encode(sonyCode(12, 0x01, 0x15), &timing, 1));
  TEST_ASSERT_EQUAL_UINT32(40000, timing.carrierHz);
  TEST_ASSERT_EQUAL_UINT16(2 + 12 + 11, timing.count);  // início + 12 marcas + 11 espaços
  TEST_ASSERT_EQUAL_UINT16(2400, timing.durationsUs[0]);
  TEST_ASSERT_EQUAL_UINT16(600, timing.durationsUs[1]);
  TEST_ASSERT_EQUAL_UINT16(1200, timing.durationsUs[2]);  // bit 0 do comando = 1
  TEST_ASSERT_EQUAL_UINT16(600, timing.durationsUs[4]);   // bit 1 = 0
  TEST_ASSERT_EQUAL_UINT16(1200, timing.durationsUs[6]);  // bit 2 = 1
}

void test_sony_round_trip_all_sizes() {
  const IrCode codes[] = {sonyCode(12, 0x01, 0x15), sonyCode(12, 0x1F, 0x7F),
                          sonyCode(15, 0xA5, 0x2E), sonyCode(20, 0x1ABC, 0x00),
                          sonyCode(12, 0x00, 0x00)};
  for (const IrCode &code : codes) {
    RawTiming timing;
    TEST_ASSERT_OK(encode(code, &timing, 1));
    IrCode decoded;
    TEST_ASSERT_OK(decode(timing, &decoded));
    TEST_ASSERT_TRUE(decoded == code);
    TEST_ASSERT_OK(decode(distort(timing, 120), &decoded));
    TEST_ASSERT_TRUE(decoded == code);
  }
}

void test_sony_three_frames_at_45_ms() {
  RawTiming timing;
  TEST_ASSERT_OK(encode(sonyCode(12, 0x01, 0x15), &timing));
  TEST_ASSERT_EQUAL_UINT16(3 * 25 + 2, timing.count);
  // Do começo de um quadro ao começo do próximo: 45 ms.
  uint32_t firstFrame = 0;
  for (int i = 0; i < 26; ++i) firstFrame += timing.durationsUs[i];
  TEST_ASSERT_EQUAL_UINT32(45000, firstFrame);
  // Decodifica o primeiro quadro mesmo com as repetições na sequência.
  IrCode decoded;
  TEST_ASSERT_OK(decode(timing, &decoded));
  TEST_ASSERT_TRUE(decoded == sonyCode(12, 0x01, 0x15));
}

void test_sony_rejects_unrepresentable_codes() {
  RawTiming timing;
  TEST_ASSERT_STATUS(Status::InvalidArgument, encode(sonyCode(13, 0, 0), &timing));
  TEST_ASSERT_STATUS(Status::InvalidArgument, encode(sonyCode(12, 0x20, 0), &timing));  // 5 bits
  TEST_ASSERT_STATUS(Status::InvalidArgument, encode(sonyCode(12, 0, 0x80), &timing));
  TEST_ASSERT_STATUS(Status::InvalidArgument, encode(sonyCode(12, 0, 0), &timing, 0));
  TEST_ASSERT_STATUS(Status::InvalidArgument, encode(sonyCode(12, 0, 0), &timing, 4));
}

void test_sony_wrong_bit_count_is_not_sony() {
  RawTiming timing;
  TEST_ASSERT_OK(encode(sonyCode(12, 0x01, 0x15), &timing, 1));
  timing.count -= 4;  // 10 bits
  IrCode decoded;
  TEST_ASSERT_STATUS(Status::NotFound, decode(timing, &decoded));
}

void test_unknown_signals_are_not_decoded() {
  IrCode decoded;
  RawTiming empty;
  TEST_ASSERT_STATUS(Status::NotFound, decode(empty, &decoded));

  RawTiming noise;
  for (uint16_t i = 0; i < 40; ++i) noise.durationsUs[i] = static_cast<uint16_t>(300 + i * 37);
  noise.count = 40;
  TEST_ASSERT_STATUS(Status::NotFound, decode(noise, &decoded));

  // RC-5 (marcas e espaços de 889 µs) não é NEC nem Sony.
  RawTiming rc5;
  for (uint16_t i = 0; i < 26; ++i) rc5.durationsUs[i] = 889;
  rc5.count = 26;
  TEST_ASSERT_STATUS(Status::NotFound, decode(rc5, &decoded));

  RawTiming bad;
  bad.count = RawTiming::kMaxEntries + 1;
  TEST_ASSERT_STATUS(Status::InvalidArgument, decode(bad, &decoded));
  TEST_ASSERT_STATUS(Status::InvalidArgument, decode(empty, nullptr));
}

void test_unknown_protocol_cannot_be_encoded() {
  RawTiming timing;
  IrCode code;
  TEST_ASSERT_STATUS(Status::InvalidArgument, encode(code, &timing));
  TEST_ASSERT_STATUS(Status::InvalidArgument, encode(necCode(1, 2), nullptr));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_nec_frame_layout);
  RUN_TEST(test_nec_round_trip);
  RUN_TEST(test_nec_extended_address);
  RUN_TEST(test_nec_extended_address_with_zero_high_byte_survives_round_trip);
  RUN_TEST(test_nec_tolerates_receiver_distortion);
  RUN_TEST(test_nec_corrupted_command_is_rejected);
  RUN_TEST(test_nec_truncated_frame_is_not_nec);
  RUN_TEST(test_nec_repeat_code);
  RUN_TEST(test_nec_rejects_unrepresentable_command);
  RUN_TEST(test_sony_frame_layout);
  RUN_TEST(test_sony_round_trip_all_sizes);
  RUN_TEST(test_sony_three_frames_at_45_ms);
  RUN_TEST(test_sony_rejects_unrepresentable_codes);
  RUN_TEST(test_sony_wrong_bit_count_is_not_sony);
  RUN_TEST(test_unknown_signals_are_not_decoded);
  RUN_TEST(test_unknown_protocol_cannot_be_encoded);
  return UNITY_END();
}
