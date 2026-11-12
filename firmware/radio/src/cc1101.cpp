#include "dbmo/radio/cc1101.h"

#include <cstring>

#include "dbmo/radio/cc1101_regs.h"

namespace dbmo::radio {

using namespace cc1101;

namespace {

// Fecha a transação ao sair do escopo.
class Deselect {
 public:
  explicit Deselect(ISpiDevice &spi) : spi_(spi) {}
  ~Deselect() { spi_.deselect(); }
  Deselect(const Deselect &) = delete;
  Deselect &operator=(const Deselect &) = delete;

 private:
  ISpiDevice &spi_;
};

}  // namespace

int16_t rssiToDbm(uint8_t raw) {
  const int16_t value = raw >= 128 ? static_cast<int16_t>(raw) - 256 : static_cast<int16_t>(raw);
  return static_cast<int16_t>(value / 2 - 74);
}

Status Cc1101::begin() {
  DBMO_TRY(spi_.select());
  // Com o CS baixo, o chip sinaliza "cristal estável" baixando o MISO (CC-09). Em modo
  // ativo isso é imediato; ao sair do sono leva cerca de 150 µs (CC-10).
  const Result<bool> miso = spi_.readMiso();
  if (miso.status() == Status::Unsupported) {
    if (asleep_) clock_.delayUs(300);
    asleep_ = false;
    return Status::Ok;
  }
  uint32_t waited = 0;
  for (;;) {
    const Result<bool> level = spi_.readMiso();
    if (!level.ok()) return level.status();
    if (!level.value()) break;
    if (waited >= kReadyTimeoutUs) return Status::Timeout;
    clock_.delayUs(50);
    waited += 50;
  }
  asleep_ = false;
  return Status::Ok;
}

Status Cc1101::strobe(uint8_t command) {
  Deselect guard(spi_);
  DBMO_TRY(begin());
  return spi_.transfer(&command, nullptr, 1);
}

Status Cc1101::writeRegister(uint8_t address, uint8_t value) {
  if (address > kLastConfigRegister) return Status::InvalidArgument;
  if (isPolicyRegister(address)) return Status::NotPermitted;
  return writeRegisterRaw(address, value);
}

Status Cc1101::writeRegisterRaw(uint8_t address, uint8_t value) {
  if (address > kLastConfigRegister) return Status::InvalidArgument;
  Deselect guard(spi_);
  DBMO_TRY(begin());
  const uint8_t frame[2] = {address, value};
  return spi_.transfer(frame, nullptr, 2);
}

Result<uint8_t> Cc1101::readRegister(uint8_t address) {
  if (address > kLastConfigRegister) return Status::InvalidArgument;
  Deselect guard(spi_);
  const Status ready = begin();
  if (!isOk(ready)) return ready;
  const uint8_t tx[2] = {static_cast<uint8_t>(address | kRead), 0};
  uint8_t rx[2] = {};
  const Status status = spi_.transfer(tx, rx, 2);
  if (!isOk(status)) return status;
  return rx[1];
}

Result<uint8_t> Cc1101::readStatus(uint8_t address) {
  if (address < kPartnum || address > 0x3D) return Status::InvalidArgument;
  Deselect guard(spi_);
  const Status ready = begin();
  if (!isOk(ready)) return ready;
  // Registradores de status exigem o bit de rajada e são lidos um por vez (CC-14).
  const uint8_t tx[2] = {static_cast<uint8_t>(address | kRead | kBurst), 0};
  uint8_t rx[2] = {};
  const Status status = spi_.transfer(tx, rx, 2);
  if (!isOk(status)) return status;
  return rx[1];
}

Result<uint8_t> Cc1101::readStatusStable(uint8_t address) {
  // Um registrador de status pode mudar durante a leitura; repete até duas leituras
  // seguidas iguais.
  Result<uint8_t> previous = readStatus(address);
  if (!previous.ok()) return previous;
  for (int attempt = 0; attempt < 4; ++attempt) {
    const Result<uint8_t> current = readStatus(address);
    if (!current.ok()) return current;
    if (current.value() == previous.value()) return current;
    previous = current;
  }
  return previous;
}

Status Cc1101::writeBurst(uint8_t address, const uint8_t *data, size_t n) {
  uint8_t frame[1 + kFifoSize];
  if (n > kFifoSize) return Status::InvalidArgument;
  frame[0] = static_cast<uint8_t>(address | kBurst);
  memcpy(frame + 1, data, n);
  Deselect guard(spi_);
  DBMO_TRY(begin());
  return spi_.transfer(frame, nullptr, n + 1);
}

Status Cc1101::readBurst(uint8_t address, uint8_t *data, size_t n) {
  uint8_t tx[1 + kFifoSize] = {};
  uint8_t rx[1 + kFifoSize] = {};
  if (n > kFifoSize) return Status::InvalidArgument;
  tx[0] = static_cast<uint8_t>(address | kRead | kBurst);
  Deselect guard(spi_);
  DBMO_TRY(begin());
  DBMO_TRY(spi_.transfer(tx, rx, n + 1));
  memcpy(data, rx + 1, n);
  return Status::Ok;
}

Status Cc1101::reset() {
  configured_ = false;
  DBMO_TRY(strobe(kSres));
  // Depois do reset o cristal reinicia; a próxima transação espera o "pronto".
  asleep_ = true;
  return strobe(kSnop);
}

Result<ChipId> Cc1101::identify() {
  const Result<uint8_t> partnum = readStatus(kPartnum);
  if (!partnum.ok())
    return partnum.status() == Status::Timeout ? Status::HardwareAbsent : partnum.status();
  const Result<uint8_t> version = readStatus(kVersion);
  if (!version.ok()) return version.status();
  // PARTNUM do CC1101 é 0x00, o mesmo que um MISO preso em nível baixo devolve; por isso
  // quem decide a presença é VERSION, que nunca é 0x00 nem 0xFF num chip real.
  if (version.value() == 0x00 || version.value() == 0xFF) return Status::HardwareAbsent;
  if (partnum.value() != kExpectedPartnum) return Status::HardwareAbsent;
  return ChipId{partnum.value(), version.value()};
}

Status Cc1101::configure(const RadioConfig &config, uint8_t paTable) {
  DBMO_TRY(validate(config, xtalHz_));
  const FieldPair rate = computeDataRate(xtalHz_, config.dataRateBaud).value();
  const FieldPair bandwidth = computeBandwidth(xtalHz_, config.bandwidthHz).value();
  const FieldPair deviation = computeDeviation(xtalHz_, config.deviationHz);
  const uint32_t word = frequencyWord(xtalHz_, config.frequencyHz);
  const bool ook = config.modulation == Modulation::Ook;

  // A partir daqui o chip pode ficar com registradores misturados: enquanto a nova
  // configuração não for gravada e conferida por inteiro, o driver não transmite nem recebe.
  configured_ = false;
  DBMO_TRY(strobe(kSidle));
  const RegisterSetting settings[] = {
      // Canal zero, sem deslocamento e com o espaçamento de reset: a portadora é só FREQ.
      {kChannr, 0x00},
      {kFsctrl0, 0x00},
      {kMdmcfg1, 0x22},  // sem FEC, 4 bytes de preâmbulo
      {kMdmcfg0, 0xF8},
      {kIocfg2, 0x2E},  // GDO2 em alta impedância
      {kIocfg0, 0x06},  // GDO0: ativo entre o sincronismo e o fim do pacote
      {kSync1, static_cast<uint8_t>(config.syncWord >> 8)},
      {kSync0, static_cast<uint8_t>(config.syncWord & 0xFF)},
      {kPktlen, config.maxPayload},
      {kPktctrl1, 0x04},  // anexa RSSI e LQI/CRC ao pacote; sem filtro de endereço
      // Tamanho variável, formato normal (FIFO); CRC e whitening conforme pedido.
      {kPktctrl0,
       static_cast<uint8_t>((config.whitening ? 0x40 : 0x00) | (config.crc ? 0x04 : 0x00) | 0x01)},
      {kFreq2, static_cast<uint8_t>(word >> 16)},
      {kFreq1, static_cast<uint8_t>(word >> 8)},
      {kFreq0, static_cast<uint8_t>(word)},
      {kMdmcfg4,
       static_cast<uint8_t>((bandwidth.exponent << 6) | (bandwidth.mantissa << 4) | rate.exponent)},
      {kMdmcfg3, rate.mantissa},
      // Modulação e sincronismo de 16 bits.
      {kMdmcfg2, static_cast<uint8_t>((static_cast<uint8_t>(config.modulation) << 4) | 0x02)},
      {kDeviatn, static_cast<uint8_t>((deviation.exponent << 4) | deviation.mantissa)},
      {kMcsm1, 0x30},  // depois de RX ou TX, volta a IDLE
      {kMcsm0, 0x18},  // calibra ao sair de IDLE
      // OOK usa PATABLE[0] para o bit 0 e PATABLE[1] para o bit 1.
      {kFrend0, static_cast<uint8_t>(ook ? 0x11 : 0x10)},
  };
  for (const RegisterSetting &setting : settings)
    DBMO_TRY(writeRegisterRaw(setting.address, setting.value));

  paTable_ = paTable;
  config_.modulation = config.modulation;  // usado por writePaTable()
  DBMO_TRY(writePaTable());

  // Confere a escrita: um barramento com defeito não pode deixar o rádio numa frequência
  // diferente da pedida.
  const Result<uint8_t> f2 = readRegister(kFreq2);
  const Result<uint8_t> f1 = readRegister(kFreq1);
  const Result<uint8_t> f0 = readRegister(kFreq0);
  if (!f2.ok() || !f1.ok() || !f0.ok()) return Status::IoError;
  const uint32_t readBack = (static_cast<uint32_t>(f2.value()) << 16) |
                            (static_cast<uint32_t>(f1.value()) << 8) | f0.value();
  if (readBack != word) return Status::IoError;

  config_ = config;
  configured_ = true;
  return Status::Ok;
}

Status Cc1101::writePaTable() {
  // OOK usa PATABLE[0] para o bit 0 e PATABLE[1] para o bit 1. Só a primeira posição
  // sobrevive ao modo de sono do chip, então a tabela é regravada antes de cada
  // transmissão.
  const bool ook = config_.modulation == Modulation::Ook;
  const uint8_t table[2] = {static_cast<uint8_t>(ook ? 0x00 : paTable_), paTable_};
  return writeBurst(kPatable, table, 2);
}

Status Cc1101::applyOverrides(const RegisterSetting *settings, size_t count) {
  if (!configured_) return Status::NotInitialized;
  if (settings == nullptr && count != 0) return Status::InvalidArgument;
  for (size_t i = 0; i < count; ++i) {
    // Frequência, banda, pacote e potência não são ajuste fino: mudam só por configure(),
    // que é o que a política de transmissão enxerga. O lote inteiro é recusado.
    const uint8_t address = settings[i].address;
    if (address > kLastConfigRegister || isPolicyRegister(address)) return Status::NotPermitted;
  }
  for (size_t i = 0; i < count; ++i)
    DBMO_TRY(writeRegisterRaw(settings[i].address, settings[i].value));
  return Status::Ok;
}

Status Cc1101::idle() { return strobe(kSidle); }

Status Cc1101::sleep() {
  DBMO_TRY(strobe(kSidle));
  DBMO_TRY(strobe(kSpwd));
  asleep_ = true;
  return Status::Ok;
}

Status Cc1101::flushReceive() {
  DBMO_TRY(strobe(kSidle));
  return strobe(kSfrx);
}

Status Cc1101::startReceive() {
  if (!configured_) return Status::NotInitialized;
  DBMO_TRY(flushReceive());
  return strobe(kSrx);
}

Result<uint8_t> Cc1101::marcState() {
  const Result<uint8_t> state = readStatusStable(kMarcstate);
  if (!state.ok()) return state;
  return static_cast<uint8_t>(state.value() & 0x1F);
}

Result<int16_t> Cc1101::rssiDbm() {
  const Result<uint8_t> raw = readStatus(kRssi);
  if (!raw.ok()) return raw.status();
  return rssiToDbm(raw.value());
}

Result<RxPoll> Cc1101::pollReceive(RadioPacket *out) {
  if (!configured_) return Status::NotInitialized;
  if (out == nullptr) return Status::InvalidArgument;

  const Result<uint8_t> state = marcState();
  if (!state.ok()) return state.status();
  if (state.value() == kStateRx) return RxPoll::Nothing;  // ainda recebendo

  const Result<uint8_t> bytes = readStatusStable(kRxbytes);
  if (!bytes.ok()) return bytes.status();
  if (state.value() == kStateRxOverflow || (bytes.value() & 0x80) != 0) {
    DBMO_TRY(flushReceive());
    return RxPoll::Overflow;
  }
  const uint8_t count = bytes.value() & 0x7F;
  if (count == 0) return RxPoll::Stopped;

  uint8_t fifo[kFifoSize];
  if (count > kFifoSize) {
    DBMO_TRY(flushReceive());
    return RxPoll::Malformed;
  }
  DBMO_TRY(readBurst(kFifo, fifo, count));
  const uint8_t length = fifo[0];
  if (length == 0 || length > config_.maxPayload || count != length + 3) {
    DBMO_TRY(flushReceive());
    return RxPoll::Malformed;
  }
  const uint8_t status = fifo[length + 2];
  if (config_.crc && (status & 0x80) == 0) return RxPoll::CrcError;

  memcpy(out->data, fifo + 1, length);
  out->length = length;
  out->rssiDbm = rssiToDbm(fifo[length + 1]);
  out->lqi = status & 0x7F;
  return RxPoll::Packet;
}

Status Cc1101::startTransmit(const TxGrant &grant, const uint8_t *data, size_t length) {
  if (!configured_) return Status::NotInitialized;
  if (data == nullptr || length == 0 || length > config_.maxPayload) return Status::InvalidArgument;
  // A autorização vale para uma transmissão, na frequência, banda e duração avaliadas.
  if (!grant.valid() || grant.serial() <= lastGrantSerial_ ||
      grant.frequencyHz() != config_.frequencyHz ||
      occupiedBandwidthHz(config_) > grant.occupiedHz() ||
      airtimeMs(config_, length) > grant.airtimeMs())
    return Status::NotPermitted;
  lastGrantSerial_ = grant.serial();

  DBMO_TRY(strobe(kSidle));
  DBMO_TRY(strobe(kSftx));
  uint8_t frame[1 + kMaxPayload];
  frame[0] = static_cast<uint8_t>(length);
  memcpy(frame + 1, data, length);
  DBMO_TRY(writePaTable());
  DBMO_TRY(writeBurst(kFifo, frame, length + 1));
  return strobe(kStx);
}

Result<TxPoll> Cc1101::pollTransmit() {
  const Result<uint8_t> state = marcState();
  if (!state.ok()) return state.status();
  if (state.value() == kStateTxUnderflow) {
    DBMO_TRY(strobe(kSftx));
    DBMO_TRY(strobe(kSidle));
    return TxPoll::Underflow;
  }
  if (state.value() == kStateIdle) return TxPoll::Done;
  return TxPoll::Busy;
}

}  // namespace dbmo::radio
