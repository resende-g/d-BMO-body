// Política de transmissão. Nenhum pacote sai do rádio sem uma autorização (TxGrant)
// emitida aqui, e só esta classe consegue criar uma.
//
// Padrão de fábrica: transmissão DESABILITADA e nenhuma faixa configurada. Para
// transmitir, o usuário precisa habilitar e informar a faixa em que está autorizado a
// operar (docs/hardware/RF_REGULATORY_NOTES.md: nenhuma norma foi verificada nesta
// etapa, então o firmware não assume nenhuma faixa como permitida).
//
// O limite de tempo no ar impede transmissão contínua. O firmware não oferece portadora
// contínua, ruído nem qualquer modo de interferência deliberada.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/status.h"

namespace dbmo::radio {

enum class TxDenial : uint8_t {
  None,
  Disabled,          // radio.tx_enabled = false
  NoBandConfigured,  // faixa de transmissão não informada
  OutsideChipBand,   // fora das faixas do CC1101
  OutOfBand,         // fora da faixa informada pelo usuário (contando a banda ocupada)
  AirtimeExceeded,   // estouraria o tempo no ar permitido por minuto
  PacketTooLong,     // um único pacote maior que o tempo no ar permitido
};

const char *toString(TxDenial denial);

struct TxLimits {
  bool enabled = false;
  uint32_t minHz = 0;
  uint32_t maxHz = 0;
  // Tempo máximo de transmissão em qualquer janela de 60 s. 600 ms = 1 %.
  uint32_t maxAirtimeMsPerMinute = 600;
};

class TxPolicy;

// Autorização para UM pacote. Só TxPolicy constrói uma autorização válida; a de uma
// decisão negada é inválida e o driver a recusa.
class TxGrant {
 public:
  bool valid() const { return valid_; }
  uint32_t frequencyHz() const { return frequencyHz_; }
  uint32_t occupiedHz() const { return occupiedHz_; }
  uint32_t airtimeMs() const { return airtimeMs_; }
  // Número de série crescente: o driver aceita cada autorização uma única vez.
  uint32_t serial() const { return serial_; }

 private:
  friend class TxPolicy;
  TxGrant(bool valid, uint32_t frequencyHz, uint32_t occupiedHz, uint32_t airtimeMs,
          uint32_t serial)
      : valid_(valid),
        frequencyHz_(frequencyHz),
        occupiedHz_(occupiedHz),
        airtimeMs_(airtimeMs),
        serial_(serial) {}
  bool valid_;
  uint32_t frequencyHz_;
  uint32_t occupiedHz_;
  uint32_t airtimeMs_;
  uint32_t serial_;
};

struct TxDecision {
  TxDenial denial;
  TxGrant grant;
  bool allowed() const { return denial == TxDenial::None; }
};

class TxPolicy {
 public:
  static constexpr uint32_t kWindowMs = 60000;
  static constexpr uint32_t kBucketMs = 10000;
  // Um intervalo a mais que a janela: o gasto só é esquecido quando o intervalo inteiro em
  // que ocorreu já saiu dos últimos 60 s (a soma cobre de 60 a 70 s, nunca menos).
  static constexpr size_t kBuckets = kWindowMs / kBucketMs + 1;

  void setLimits(const TxLimits &limits) { limits_ = limits; }
  const TxLimits &limits() const { return limits_; }

  // Só consulta; não consome tempo no ar.
  TxDenial check(uint32_t frequencyHz, uint32_t occupiedHz, uint32_t airtimeMs, uint32_t nowMs);
  // Consulta e, se permitido, debita o tempo no ar.
  TxDecision authorize(uint32_t frequencyHz, uint32_t occupiedHz, uint32_t airtimeMs,
                       uint32_t nowMs);

  // Tempo no ar gasto na janela atual.
  uint32_t airtimeUsedMs(uint32_t nowMs);

 private:
  void advance(uint32_t nowMs);

  TxLimits limits_;
  uint32_t buckets_[kBuckets] = {};
  size_t current_ = 0;
  uint32_t bucketStartMs_ = 0;
  bool started_ = false;
  uint32_t nextSerial_ = 1;
};

}  // namespace dbmo::radio
