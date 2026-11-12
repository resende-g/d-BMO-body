#include <cstring>

#include "dbmo/nfc/pn532_frame.h"
#include "dbmo_test.h"

using dbmo::Status;
using namespace dbmo::nfc;

void setUp() {}
void tearDown() {}

void test_encode_get_firmware_version() {
  const uint8_t payload[] = {0x02};
  uint8_t out[kMaxFrame];
  const dbmo::Result<size_t> size = encodeFrame(kTfiHostToPn532, payload, 1, out, sizeof(out));
  const uint8_t expected[] = {0x00, 0x00, 0xFF, 0x02, 0xFE, 0xD4, 0x02, 0x2A, 0x00};
  TEST_ASSERT_EQUAL_UINT(sizeof(expected), size.value());
  TEST_ASSERT_EQUAL_MEMORY(expected, out, sizeof(expected));
}

void test_encode_in_list_passive_target() {
  const uint8_t payload[] = {0x4A, 0x01, 0x00};
  uint8_t out[kMaxFrame];
  const dbmo::Result<size_t> size = encodeFrame(kTfiHostToPn532, payload, 3, out, sizeof(out));
  // Soma: D4 + 4A + 01 + 00 = 0x11F → 0x1F; complemento de dois = 0xE1.
  const uint8_t expected[] = {0x00, 0x00, 0xFF, 0x04, 0xFC, 0xD4, 0x4A, 0x01, 0x00, 0xE1, 0x00};
  TEST_ASSERT_EQUAL_UINT(sizeof(expected), size.value());
  TEST_ASSERT_EQUAL_MEMORY(expected, out, sizeof(expected));
}

void test_encode_rejects_bad_arguments() {
  uint8_t out[kMaxFrame];
  const uint8_t payload[kMaxPayload + 1] = {};
  TEST_ASSERT_STATUS(Status::InvalidArgument,
                     encodeFrame(kTfiHostToPn532, payload, 0, out, sizeof(out)).status());
  TEST_ASSERT_STATUS(
      Status::InvalidArgument,
      encodeFrame(kTfiHostToPn532, payload, kMaxPayload + 1, out, sizeof(out)).status());
  TEST_ASSERT_STATUS(Status::InvalidArgument,
                     encodeFrame(kTfiHostToPn532, nullptr, 1, out, sizeof(out)).status());
  TEST_ASSERT_STATUS(Status::NoSpace, encodeFrame(kTfiHostToPn532, payload, 4, out, 8).status());
}

void test_decode_ack_nack_and_error() {
  Frame frame{};
  TEST_ASSERT_OK(decodeFrame(kAckFrame, sizeof(kAckFrame), &frame));
  TEST_ASSERT_EQUAL(FrameKind::Ack, frame.kind);
  TEST_ASSERT_OK(decodeFrame(kNackFrame, sizeof(kNackFrame), &frame));
  TEST_ASSERT_EQUAL(FrameKind::Nack, frame.kind);
  const uint8_t error[] = {0x00, 0x00, 0xFF, 0x01, 0xFF, 0x7F, 0x81, 0x00};
  TEST_ASSERT_OK(decodeFrame(error, sizeof(error), &frame));
  TEST_ASSERT_EQUAL(FrameKind::Error, frame.kind);
}

void test_decode_firmware_response() {
  // D5 03 32 01 06 07: soma 0x118 → 0x18; complemento = 0xE8.
  const uint8_t response[] = {0x00, 0x00, 0xFF, 0x06, 0xFA, 0xD5, 0x03,
                              0x32, 0x01, 0x06, 0x07, 0xE8, 0x00};
  Frame frame{};
  TEST_ASSERT_OK(decodeFrame(response, sizeof(response), &frame));
  TEST_ASSERT_EQUAL(FrameKind::Data, frame.kind);
  TEST_ASSERT_EQUAL_HEX8(0xD5, frame.tfi);
  TEST_ASSERT_EQUAL_UINT(5, frame.length);
  const uint8_t payload[] = {0x03, 0x32, 0x01, 0x06, 0x07};
  TEST_ASSERT_EQUAL_MEMORY(payload, frame.payload, 5);
}

void test_decode_tolerates_extra_leading_zeros_and_trailing_bytes() {
  const uint8_t response[] = {0x00, 0x00, 0x00, 0x00, 0xFF, 0x02, 0xFE,
                              0xD5, 0x15, 0x16, 0x00, 0x00, 0x00, 0x00};
  Frame frame{};
  TEST_ASSERT_OK(decodeFrame(response, sizeof(response), &frame));
  TEST_ASSERT_EQUAL_UINT(1, frame.length);
  TEST_ASSERT_EQUAL_HEX8(0x15, frame.payload[0]);
}

void test_decode_detects_corruption() {
  uint8_t response[] = {0x00, 0x00, 0xFF, 0x06, 0xFA, 0xD5, 0x03,
                        0x32, 0x01, 0x06, 0x07, 0xE8, 0x00};
  Frame frame{};
  response[8] ^= 0x01;  // dado alterado
  TEST_ASSERT_STATUS(Status::CrcError, decodeFrame(response, sizeof(response), &frame));
  response[8] ^= 0x01;
  response[4] = 0xFB;  // complemento do tamanho errado
  TEST_ASSERT_STATUS(Status::CrcError, decodeFrame(response, sizeof(response), &frame));
  response[4] = 0xFA;
  TEST_ASSERT_STATUS(Status::ProtocolError, decodeFrame(response, 9, &frame));  // truncado
  TEST_ASSERT_OK(decodeFrame(response, sizeof(response), &frame));
}

void test_decode_rejects_garbage() {
  Frame frame{};
  const uint8_t zeros[16] = {};
  TEST_ASSERT_STATUS(Status::ProtocolError, decodeFrame(zeros, sizeof(zeros), &frame));
  const uint8_t ones[16] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  TEST_ASSERT_STATUS(Status::ProtocolError, decodeFrame(ones, sizeof(ones), &frame));
  const uint8_t noise[] = {0x12, 0x00, 0xFF, 0x00, 0xFF, 0x00};
  TEST_ASSERT_STATUS(Status::ProtocolError, decodeFrame(noise, sizeof(noise), &frame));
  TEST_ASSERT_STATUS(Status::ProtocolError, decodeFrame(zeros, 0, &frame));
  TEST_ASSERT_STATUS(Status::InvalidArgument, decodeFrame(nullptr, 4, &frame));
  // Tamanho zero com complemento válido (00 00) não é um quadro de dados.
  const uint8_t empty[] = {0x00, 0x00, 0xFF, 0x00, 0x00, 0x00};
  TEST_ASSERT_STATUS(Status::ProtocolError, decodeFrame(empty, sizeof(empty), &frame));
}

void test_round_trip_all_lengths() {
  uint8_t payload[kMaxPayload];
  for (size_t i = 0; i < kMaxPayload; ++i) payload[i] = static_cast<uint8_t>(i * 7 + 3);
  for (size_t length = 1; length <= kMaxPayload; ++length) {
    uint8_t out[kMaxFrame];
    const dbmo::Result<size_t> size =
        encodeFrame(kTfiPn532ToHost, payload, length, out, sizeof(out));
    TEST_ASSERT_TRUE(size.ok());
    Frame frame{};
    TEST_ASSERT_OK(decodeFrame(out, size.value(), &frame));
    TEST_ASSERT_EQUAL_UINT(length, frame.length);
    TEST_ASSERT_EQUAL_MEMORY(payload, frame.payload, length);
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_encode_get_firmware_version);
  RUN_TEST(test_encode_in_list_passive_target);
  RUN_TEST(test_encode_rejects_bad_arguments);
  RUN_TEST(test_decode_ack_nack_and_error);
  RUN_TEST(test_decode_firmware_response);
  RUN_TEST(test_decode_tolerates_extra_leading_zeros_and_trailing_bytes);
  RUN_TEST(test_decode_detects_corruption);
  RUN_TEST(test_decode_rejects_garbage);
  RUN_TEST(test_round_trip_all_lengths);
  return UNITY_END();
}
