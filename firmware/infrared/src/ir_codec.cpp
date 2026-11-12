#include "dbmo/infrared/ir_codec.h"

namespace dbmo::infrared {

namespace {

bool near(uint16_t value, uint16_t nominal) {
  const uint32_t tolerance = nominal / 4u + 100u;
  const uint32_t low = nominal > tolerance ? nominal - tolerance : 0;
  return value >= low && value <= nominal + tolerance;
}

Status decodeNec(const RawTiming &timing, IrCode *out) {
  // Líder (marca + espaço), 32 × (marca + espaço) e a marca final.
  if (timing.count < 67) return Status::NotFound;
  const uint16_t *d = timing.durationsUs;
  if (!near(d[0], nec::kLeaderMarkUs) || !near(d[1], nec::kLeaderSpaceUs)) return Status::NotFound;

  uint32_t data = 0;
  for (int bit = 0; bit < 32; ++bit) {
    const uint16_t mark = d[2 + 2 * bit];
    const uint16_t space = d[3 + 2 * bit];
    if (!near(mark, nec::kBitMarkUs)) return Status::NotFound;
    if (near(space, nec::kOneSpaceUs)) {
      data |= 1u << bit;  // bit menos significativo primeiro
    } else if (!near(space, nec::kZeroSpaceUs)) {
      return Status::NotFound;
    }
  }
  if (!near(d[66], nec::kBitMarkUs)) return Status::NotFound;

  const uint8_t address = static_cast<uint8_t>(data);
  const uint8_t addressInverse = static_cast<uint8_t>(data >> 8);
  const uint8_t command = static_cast<uint8_t>(data >> 16);
  const uint8_t commandInverse = static_cast<uint8_t>(data >> 24);
  if (static_cast<uint8_t>(command ^ commandInverse) != 0xFF) return Status::CrcError;

  out->protocol = IrProtocol::Nec;
  out->bits = 32;
  out->command = command;
  // Endereço de 8 bits quando o segundo byte é o inverso; senão, estendido de 16 bits.
  out->extended = static_cast<uint8_t>(address ^ addressInverse) != 0xFF;
  out->address = out->extended ? static_cast<uint16_t>(address | (addressInverse << 8)) : address;
  return Status::Ok;
}

Status decodeSony(const RawTiming &timing, IrCode *out) {
  const uint16_t *d = timing.durationsUs;
  if (timing.count < 2 || !near(d[0], sony::kStartMarkUs) || !near(d[1], sony::kSpaceUs))
    return Status::NotFound;

  // Depois do início: uma marca por bit, separadas por espaços; o último espaço pode
  // não vir (a rajada termina na última marca).
  uint32_t data = 0;
  uint8_t bits = 0;
  for (uint16_t i = 2; i < timing.count; i += 2) {
    if (bits >= 20) return Status::NotFound;
    if (near(d[i], sony::kOneMarkUs)) {
      data |= 1u << bits;
    } else if (!near(d[i], sony::kZeroMarkUs)) {
      return Status::NotFound;
    }
    ++bits;
    if (i + 1 < timing.count && !near(d[i + 1], sony::kSpaceUs)) {
      // Espaço longo: fim do quadro (o que vem depois é a repetição).
      break;
    }
  }
  if (bits != 12 && bits != 15 && bits != 20) return Status::NotFound;

  out->protocol = IrProtocol::Sony;
  out->bits = bits;
  out->command = static_cast<uint16_t>(data & 0x7F);
  out->address = static_cast<uint16_t>(data >> 7);
  return Status::Ok;
}

Status encodeNec(const IrCode &code, RawTiming *out) {
  if (code.command > 0xFF) return Status::InvalidArgument;
  const uint8_t low = static_cast<uint8_t>(code.address);
  const bool extended = code.extended || code.address > 0xFF;
  const uint8_t high =
      extended ? static_cast<uint8_t>(code.address >> 8) : static_cast<uint8_t>(~low);
  const uint8_t command = static_cast<uint8_t>(code.command);
  const uint32_t data = static_cast<uint32_t>(low) | (static_cast<uint32_t>(high) << 8) |
                        (static_cast<uint32_t>(command) << 16) |
                        (static_cast<uint32_t>(static_cast<uint8_t>(~command)) << 24);

  uint16_t n = 0;
  out->durationsUs[n++] = nec::kLeaderMarkUs;
  out->durationsUs[n++] = nec::kLeaderSpaceUs;
  for (int bit = 0; bit < 32; ++bit) {
    out->durationsUs[n++] = nec::kBitMarkUs;
    out->durationsUs[n++] = (data >> bit) & 1u ? nec::kOneSpaceUs : nec::kZeroSpaceUs;
  }
  out->durationsUs[n++] = nec::kBitMarkUs;
  out->count = n;
  out->carrierHz = nec::kCarrierHz;
  return Status::Ok;
}

Status encodeSony(const IrCode &code, RawTiming *out, uint8_t frames) {
  if (code.bits != 12 && code.bits != 15 && code.bits != 20) return Status::InvalidArgument;
  if (code.command > 0x7F) return Status::InvalidArgument;
  const uint8_t addressBits = static_cast<uint8_t>(code.bits - 7);
  if (code.address >= (1u << addressBits)) return Status::InvalidArgument;
  if (frames == 0 || frames > 3) return Status::InvalidArgument;

  const uint32_t data =
      static_cast<uint32_t>(code.command) | (static_cast<uint32_t>(code.address) << 7);
  uint16_t n = 0;
  for (uint8_t frame = 0; frame < frames; ++frame) {
    uint32_t frameUs = sony::kStartMarkUs + sony::kSpaceUs;
    out->durationsUs[n++] = sony::kStartMarkUs;
    out->durationsUs[n++] = sony::kSpaceUs;
    for (uint8_t bit = 0; bit < code.bits; ++bit) {
      const uint16_t mark = (data >> bit) & 1u ? sony::kOneMarkUs : sony::kZeroMarkUs;
      out->durationsUs[n++] = mark;
      frameUs += mark;
      if (bit + 1 < code.bits) {
        out->durationsUs[n++] = sony::kSpaceUs;
        frameUs += sony::kSpaceUs;
      }
    }
    // Espaço até o começo do próximo quadro (período de 45 ms).
    if (frame + 1 < frames)
      out->durationsUs[n++] = static_cast<uint16_t>(sony::kFramePeriodUs - frameUs);
  }
  out->count = n;
  out->carrierHz = sony::kCarrierHz;
  return Status::Ok;
}

}  // namespace

const char *toString(IrProtocol protocol) {
  switch (protocol) {
    case IrProtocol::Unknown: return "unknown";
    case IrProtocol::Nec: return "nec";
    case IrProtocol::Sony: return "sony";
  }
  return "?";
}

bool isNecRepeat(const RawTiming &timing) {
  return timing.count >= 3 && timing.count <= 4 &&
         near(timing.durationsUs[0], nec::kLeaderMarkUs) &&
         near(timing.durationsUs[1], nec::kRepeatSpaceUs) &&
         near(timing.durationsUs[2], nec::kBitMarkUs);
}

Status decode(const RawTiming &timing, IrCode *out) {
  if (out == nullptr) return Status::InvalidArgument;
  if (timing.count > RawTiming::kMaxEntries) return Status::InvalidArgument;
  IrCode code;
  Status status = decodeNec(timing, &code);
  if (status == Status::NotFound) status = decodeSony(timing, &code);
  if (isOk(status)) *out = code;
  return status;
}

Status encode(const IrCode &code, RawTiming *out, uint8_t frames) {
  if (out == nullptr) return Status::InvalidArgument;
  switch (code.protocol) {
    case IrProtocol::Nec: return encodeNec(code, out);
    case IrProtocol::Sony: return encodeSony(code, out, frames);
    case IrProtocol::Unknown: break;
  }
  return Status::InvalidArgument;
}

}  // namespace dbmo::infrared
