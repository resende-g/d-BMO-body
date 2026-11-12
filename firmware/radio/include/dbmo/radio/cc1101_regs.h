// Endereços do CC1101 (datasheet TI SWRS061I). O formato do cabeçalho SPI, os
// registradores de identificação e a regra do bit de rajada estão conferidos em
// docs/hardware/sources/PERIPHERALS.md (CC-05, CC-13, CC-14). Os demais endereços e
// campos desta lista vêm das tabelas de registradores do mesmo datasheet e NÃO foram
// reconferidos nesta sessão: a primeira validação de bancada é ler PARTNUM/VERSION e
// reler os registradores escritos.
#pragma once

#include <cstdint>

namespace dbmo::radio::cc1101 {

// Cabeçalho SPI.
inline constexpr uint8_t kRead = 0x80;
inline constexpr uint8_t kBurst = 0x40;

// Registradores de configuração.
inline constexpr uint8_t kIocfg2 = 0x00;
inline constexpr uint8_t kIocfg1 = 0x01;
inline constexpr uint8_t kIocfg0 = 0x02;
inline constexpr uint8_t kFifothr = 0x03;
inline constexpr uint8_t kSync1 = 0x04;
inline constexpr uint8_t kSync0 = 0x05;
inline constexpr uint8_t kPktlen = 0x06;
inline constexpr uint8_t kPktctrl1 = 0x07;
inline constexpr uint8_t kPktctrl0 = 0x08;
inline constexpr uint8_t kChannr = 0x0A;
inline constexpr uint8_t kFsctrl0 = 0x0C;
inline constexpr uint8_t kFreq2 = 0x0D;
inline constexpr uint8_t kFreq1 = 0x0E;
inline constexpr uint8_t kFreq0 = 0x0F;
inline constexpr uint8_t kMdmcfg4 = 0x10;
inline constexpr uint8_t kMdmcfg3 = 0x11;
inline constexpr uint8_t kMdmcfg2 = 0x12;
inline constexpr uint8_t kMdmcfg1 = 0x13;
inline constexpr uint8_t kMdmcfg0 = 0x14;
inline constexpr uint8_t kDeviatn = 0x15;
inline constexpr uint8_t kMcsm1 = 0x17;
inline constexpr uint8_t kMcsm0 = 0x18;
inline constexpr uint8_t kFrend0 = 0x22;
inline constexpr uint8_t kLastConfigRegister = 0x2E;

// Registradores que definem o que vai ao ar: frequência, banda ocupada, formato e tamanho
// do pacote, o que o rádio faz depois de transmitir e a seleção de potência. Só
// Cc1101::configure() os escreve, a partir do RadioConfig que a política de transmissão
// avaliou; nem ajuste fino nem acesso de baixo nível podem alterá-los. PKTCTRL0 inclui o
// modo de dados aleatórios e o modo assíncrono; MCSM1 inclui "permanecer transmitindo"
// depois do pacote. (PATABLE e FIFO não são registradores de configuração: a escrita de
// baixo nível os recusa por outro motivo.)
constexpr bool isPolicyRegister(uint8_t address) {
  // CHANNR, FSCTRL0 e o espaçamento de canal (MDMCFG1/0) deslocam a portadora; MDMCFG1
  // também muda o preâmbulo e o FEC, isto é, o tempo no ar.
  return address == kChannr || address == kFsctrl0 || address == kMdmcfg1 || address == kMdmcfg0 ||
         address == kFreq2 || address == kFreq1 || address == kFreq0 || address == kPktlen ||
         address == kPktctrl1 || address == kPktctrl0 || address == kMdmcfg4 ||
         address == kMdmcfg3 || address == kMdmcfg2 || address == kDeviatn || address == kMcsm1 ||
         address == kFrend0;
}

// Comandos (strobes).
inline constexpr uint8_t kSres = 0x30;
inline constexpr uint8_t kSrx = 0x34;
inline constexpr uint8_t kStx = 0x35;
inline constexpr uint8_t kSidle = 0x36;
inline constexpr uint8_t kSpwd = 0x39;
inline constexpr uint8_t kSfrx = 0x3A;
inline constexpr uint8_t kSftx = 0x3B;
inline constexpr uint8_t kSnop = 0x3D;

// Registradores de status (lidos com o bit de rajada).
inline constexpr uint8_t kPartnum = 0x30;
inline constexpr uint8_t kVersion = 0x31;
inline constexpr uint8_t kLqi = 0x33;
inline constexpr uint8_t kRssi = 0x34;
inline constexpr uint8_t kMarcstate = 0x35;
inline constexpr uint8_t kTxbytes = 0x3A;
inline constexpr uint8_t kRxbytes = 0x3B;

inline constexpr uint8_t kPatable = 0x3E;
inline constexpr uint8_t kFifo = 0x3F;

// Estados da máquina principal (MARCSTATE).
inline constexpr uint8_t kStateSleep = 0x00;
inline constexpr uint8_t kStateIdle = 0x01;
inline constexpr uint8_t kStateRx = 0x0D;
inline constexpr uint8_t kStateRxOverflow = 0x11;
inline constexpr uint8_t kStateTx = 0x13;
inline constexpr uint8_t kStateTxUnderflow = 0x16;

inline constexpr uint8_t kFifoSize = 64;
// PARTNUM e VERSION documentados (CC-13). VERSION "pode mudar sem aviso".
inline constexpr uint8_t kExpectedPartnum = 0x00;
inline constexpr uint8_t kVersionCurrent = 0x14;
inline constexpr uint8_t kVersionOlder = 0x04;

}  // namespace dbmo::radio::cc1101
