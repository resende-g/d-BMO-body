#include "dbmo/radio/tx_policy.h"

#include "dbmo/core/hal/clock.h"
#include "dbmo/radio/radio_config.h"

namespace dbmo::radio {

const char *toString(TxDenial denial) {
  switch (denial) {
    case TxDenial::None: return "None";
    case TxDenial::Disabled: return "Disabled";
    case TxDenial::NoBandConfigured: return "NoBandConfigured";
    case TxDenial::OutsideChipBand: return "OutsideChipBand";
    case TxDenial::OutOfBand: return "OutOfBand";
    case TxDenial::AirtimeExceeded: return "AirtimeExceeded";
    case TxDenial::PacketTooLong: return "PacketTooLong";
  }
  return "?";
}

void TxPolicy::advance(uint32_t nowMs) {
  if (!started_) {
    started_ = true;
    bucketStartMs_ = nowMs;
    return;
  }
  uint32_t elapsed = elapsedMs(nowMs, bucketStartMs_);
  if (elapsed >= kBuckets * kBucketMs) {
    // Mais de uma janela inteira sem atividade: tudo expirou.
    for (uint32_t &bucket : buckets_) bucket = 0;
    bucketStartMs_ = nowMs;
    return;
  }
  while (elapsed >= kBucketMs) {
    current_ = (current_ + 1) % kBuckets;
    buckets_[current_] = 0;
    bucketStartMs_ += kBucketMs;
    elapsed -= kBucketMs;
  }
}

uint32_t TxPolicy::airtimeUsedMs(uint32_t nowMs) {
  advance(nowMs);
  uint32_t total = 0;
  for (uint32_t bucket : buckets_) total += bucket;
  return total;
}

TxDenial TxPolicy::check(uint32_t frequencyHz, uint32_t occupiedHz, uint32_t airtimeMs,
                         uint32_t nowMs) {
  if (!limits_.enabled) return TxDenial::Disabled;
  if (limits_.minHz == 0 || limits_.maxHz == 0 || limits_.minHz >= limits_.maxHz)
    return TxDenial::NoBandConfigured;
  if (!inChipBand(frequencyHz)) return TxDenial::OutsideChipBand;
  const uint32_t half = occupiedHz / 2;
  if (frequencyHz < half || frequencyHz - half < limits_.minHz || frequencyHz > UINT32_MAX - half ||
      frequencyHz + half > limits_.maxHz)
    return TxDenial::OutOfBand;
  if (airtimeMs > limits_.maxAirtimeMsPerMinute) return TxDenial::PacketTooLong;
  if (airtimeUsedMs(nowMs) + airtimeMs > limits_.maxAirtimeMsPerMinute)
    return TxDenial::AirtimeExceeded;
  return TxDenial::None;
}

TxDecision TxPolicy::authorize(uint32_t frequencyHz, uint32_t occupiedHz, uint32_t airtimeMs,
                               uint32_t nowMs) {
  const TxDenial denial = check(frequencyHz, occupiedHz, airtimeMs, nowMs);
  const bool allowed = denial == TxDenial::None;
  if (allowed) buckets_[current_] += airtimeMs;
  return TxDecision{
      denial, TxGrant(allowed, frequencyHz, occupiedHz, airtimeMs, allowed ? nextSerial_++ : 0)};
}

}  // namespace dbmo::radio
