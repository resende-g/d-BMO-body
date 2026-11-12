// Codificação e decodificação dos protocolos NEC e Sony SIRC.
//
// Os tempos são os amplamente publicados para cada protocolo (não há norma aberta
// consultada nesta sessão; ver README). Tolerância na decodificação: 25 % + 100 µs,
// porque o receptor demodulado alonga marcas e encurta espaços.
#pragma once

#include "dbmo/core/status.h"
#include "dbmo/infrared/ir_types.h"

namespace dbmo::infrared {

namespace nec {
inline constexpr uint32_t kCarrierHz = 38000;
inline constexpr uint16_t kLeaderMarkUs = 9000;
inline constexpr uint16_t kLeaderSpaceUs = 4500;
inline constexpr uint16_t kRepeatSpaceUs = 2250;
inline constexpr uint16_t kBitMarkUs = 562;
inline constexpr uint16_t kZeroSpaceUs = 562;
inline constexpr uint16_t kOneSpaceUs = 1688;
}  // namespace nec

namespace sony {
inline constexpr uint32_t kCarrierHz = 40000;
inline constexpr uint16_t kStartMarkUs = 2400;
inline constexpr uint16_t kSpaceUs = 600;
inline constexpr uint16_t kZeroMarkUs = 600;
inline constexpr uint16_t kOneMarkUs = 1200;
inline constexpr uint32_t kFramePeriodUs = 45000;
// Os controles da Sony enviam cada comando pelo menos três vezes.
inline constexpr uint8_t kDefaultFrames = 3;
}  // namespace sony

// Identifica o protocolo e extrai endereço e comando.
// NotFound: não é NEC nem Sony (tratar como bruto). CrcError: quadro NEC cujo comando
// não confere com o seu inverso.
Status decode(const RawTiming &timing, IrCode *out);

// Código de repetição do NEC (tecla mantida apertada).
bool isNecRepeat(const RawTiming &timing);

// Monta o sinal de um código. Para Sony, frames é quantas vezes o quadro é enviado
// (1 a 3); para NEC é ignorado (um quadro, sem códigos de repetição).
// InvalidArgument se o código não é representável no protocolo.
Status encode(const IrCode &code, RawTiming *out, uint8_t frames = sony::kDefaultFrames);

}  // namespace dbmo::infrared
