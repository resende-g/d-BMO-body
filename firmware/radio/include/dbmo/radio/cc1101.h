// Driver do CC1101 em modo de pacote (FIFO), sobre ISpiDevice. Não bloqueia além das
// transações SPI: recepção e transmissão são acompanhadas por consulta.
//
// O que este driver NÃO faz, por decisão de projeto: portadora contínua, transmissão
// assíncrona/serial sem pacote, modo de teste de PA e qualquer forma de interferência.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/core/hal/clock.h"
#include "dbmo/core/hal/spi.h"
#include "dbmo/core/status.h"
#include "dbmo/radio/radio_config.h"
#include "dbmo/radio/tx_policy.h"

namespace dbmo::radio {

struct ChipId {
  uint8_t partnum;
  uint8_t version;
};

struct RegisterSetting {
  uint8_t address;
  uint8_t value;
};

enum class RxPoll : uint8_t {
  Nothing,    // recebendo, nenhum pacote completo
  Packet,     // pacote válido em *out
  CrcError,   // pacote recebido com CRC errado (descartado)
  Overflow,   // a FIFO estourou (descartada)
  Malformed,  // tamanho incoerente (descartado)
  Stopped,    // o rádio saiu da recepção sem pacote
};

enum class TxPoll : uint8_t { Busy, Done, Underflow };

class Cc1101 {
 public:
  // Tempo máximo esperando o chip sinalizar "pronto" no MISO.
  static constexpr uint32_t kReadyTimeoutUs = 5000;

  Cc1101(ISpiDevice &spi, IClock &clock, uint32_t xtalHz = 26000000)
      : spi_(spi), clock_(clock), xtalHz_(xtalHz) {}

  void setXtalHz(uint32_t xtalHz) { xtalHz_ = xtalHz; }
  uint32_t xtalHz() const { return xtalHz_; }

  // Comando de reset e espera pelo cristal.
  Status reset();
  // Lê PARTNUM e VERSION. HardwareAbsent quando a resposta é a de um barramento sem
  // chip (0x00 ou 0xFF em VERSION) ou PARTNUM não é o do CC1101.
  Result<ChipId> identify();

  // Aplica a configuração e confere, relendo, a frequência gravada. paTable é o byte de
  // potência (PATABLE); a correspondência com dBm depende da faixa e está na tabela de
  // potência do datasheet.
  Status configure(const RadioConfig &config, uint8_t paTable);
  // Ajustes finos de bancada (ex.: exportados do SmartRF Studio), depois de configure().
  Status applyOverrides(const RegisterSetting *settings, size_t count);
  const RadioConfig &config() const { return config_; }

  Status idle();
  Status sleep();
  Status startReceive();

  Result<uint8_t> marcState();
  // RSSI instantâneo em dBm (só faz sentido em recepção).
  Result<int16_t> rssiDbm();

  // Consulta a recepção. Depois de qualquer resultado diferente de Nothing o rádio está
  // parado: chamar startReceive() para continuar.
  Result<RxPoll> pollReceive(RadioPacket *out);

  // Carrega a FIFO e inicia a transmissão de UM pacote. NotPermitted se a autorização é
  // inválida, já foi usada, foi emitida para outra frequência, ou não cobre a banda
  // ocupada e o tempo no ar deste pacote na configuração atual.
  Status startTransmit(const TxGrant &grant, const uint8_t *data, size_t length);
  Result<TxPoll> pollTransmit();

  // Acesso de baixo nível (testes de bancada e diagnóstico). A escrita recusa
  // (NotPermitted) os registradores que definem o que vai ao ar: ver isPolicyRegister().
  Status writeRegister(uint8_t address, uint8_t value);
  Result<uint8_t> readRegister(uint8_t address);
  Result<uint8_t> readStatus(uint8_t address);

 private:
  Status writePaTable();
  Status begin();  // abre a transação já com o chip pronto; quem chama fecha
  Status writeRegisterRaw(uint8_t address, uint8_t value);
  Status strobe(uint8_t command);
  Status writeBurst(uint8_t address, const uint8_t *data, size_t n);
  Status readBurst(uint8_t address, uint8_t *data, size_t n);
  Result<uint8_t> readStatusStable(uint8_t address);
  Status flushReceive();

  ISpiDevice &spi_;
  IClock &clock_;
  uint32_t xtalHz_;
  RadioConfig config_;
  bool configured_ = false;
  bool asleep_ = false;
  uint8_t paTable_ = 0;
  uint32_t lastGrantSerial_ = 0;
};

// RSSI bruto → dBm (deslocamento típico de 74 dB do datasheet).
int16_t rssiToDbm(uint8_t raw);

}  // namespace dbmo::radio
