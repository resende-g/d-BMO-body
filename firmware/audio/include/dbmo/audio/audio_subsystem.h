// Subsistema de áudio: reprodução (tons e amostras em memória) e gravação em memória.
//
// Privacidade:
//   - o microfone vem DESABILITADO (audio.mic_enabled = false);
//   - toda gravação publica kRecordingStarted antes da primeira amostra e
//     kRecordingFinished ao terminar: não existe gravação silenciosa;
//   - a gravação vai para um buffer que quem chamou forneceu, tem duração máxima e não é
//     gravada em arquivo nem enviada a lugar nenhum por este módulo;
//   - a entrada I2S só fica ligada durante a gravação.
// Não há reconhecimento de voz neste firmware.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dbmo/audio/audio_types.h"
#include "dbmo/audio/tone.h"
#include "dbmo/core/config_store.h"
#include "dbmo/core/event_bus.h"
#include "dbmo/core/hal/clock.h"
#include "dbmo/core/subsystem.h"

namespace dbmo::audio {

namespace audio_event {
inline constexpr EventId kPlaybackStarted = makeEventId(EventDomain::Audio, 1);
inline constexpr EventId kPlaybackFinished = makeEventId(EventDomain::Audio, 2);
inline constexpr EventId kRecordingStarted = makeEventId(EventDomain::Audio, 3);
// a = amostras gravadas, b = pico (0 a 32768).
inline constexpr EventId kRecordingFinished = makeEventId(EventDomain::Audio, 4);
// a = Status.
inline constexpr EventId kFault = makeEventId(EventDomain::Audio, 5);
}  // namespace audio_event

namespace config_key {
inline constexpr char kVolume[] = "audio.volume";
inline constexpr char kMuted[] = "audio.muted";
inline constexpr char kSampleRate[] = "audio.sample_rate";
inline constexpr char kMicEnabled[] = "audio.mic_enabled";
}  // namespace config_key

class AudioSubsystem final : public ISubsystem {
 public:
  static constexpr size_t kChunk = 128;
  // Blocos processados por tick, para o áudio não segurar o laço principal.
  static constexpr uint8_t kChunksPerTick = 4;
  // Tempo com o amplificador ainda ligado depois da última amostra (esvazia a fila I2S).
  static constexpr uint32_t kAmplifierHoldMs = 50;
  // O INMP441 entrega zeros logo depois de ligado (MIC-08): esse trecho é descartado.
  static constexpr uint32_t kMicStartupMs = 100;
  static constexpr uint32_t kMaxRecordMs = 30000;
  static constexpr uint8_t kFaultThreshold = 3;
  static constexpr uint32_t kDefaultSampleRate = 16000;

  AudioSubsystem(EventBus &bus, ConfigStore &config, IClock &clock);

  static Status registerConfig(ConfigStore &config);

  // Ligações opcionais, antes de init().
  void setOutput(IAudioOutput &output) { output_ = &output; }
  void setInput(IAudioInput &input) { input_ = &input; }

  const char *name() const override { return "audio"; }
  // HardwareAbsent se não há saída nem entrada ligadas.
  Status init() override;
  void tick(uint32_t nowMs) override;
  Status shutdown() override;

  uint32_t sampleRateHz() const { return sampleRateHz_; }
  Volume &volume() { return volume_; }

  // ---- Reprodução ----
  // A origem precisa continuar válida até kPlaybackFinished. Busy se já há reprodução.
  Status play(IAudioSource &source);
  Status playTone(uint32_t frequencyHz, uint32_t durationMs);
  void stopPlayback();
  bool playing() const { return playing_; }

  // ---- Gravação ----
  // Grava em storage até enchê-lo, até maxMs ou até stopRecording(). NotPermitted com o
  // microfone desabilitado.
  Status startRecording(int16_t *storage, size_t capacity, uint32_t maxMs = kMaxRecordMs);
  void stopRecording();
  bool recording() const { return recording_; }
  size_t recordedSamples() const { return recorded_; }
  uint16_t recordedPeak() const { return recordPeak_; }

  // Relê volume, mudo e habilitação do microfone.
  void applyConfig();

 private:
  void servicePlayback(uint32_t nowMs);
  void serviceRecording(uint32_t nowMs);
  void finishPlayback(uint32_t nowMs);
  void finishRecording();
  void fault(Status status);

  EventBus &bus_;
  ConfigStore &config_;
  IClock &clock_;
  IAudioOutput *output_ = nullptr;
  IAudioInput *input_ = nullptr;
  Volume volume_;
  uint32_t sampleRateHz_ = kDefaultSampleRate;
  bool micEnabled_ = false;
  bool initialized_ = false;

  IAudioSource *source_ = nullptr;
  ToneSource tone_;
  bool playing_ = false;
  bool amplifierOn_ = false;
  bool amplifierHold_ = false;
  uint32_t holdStartMs_ = 0;
  int16_t pending_[kChunk] = {};
  size_t pendingCount_ = 0;
  size_t pendingOffset_ = 0;
  uint8_t outputErrors_ = 0;

  bool recording_ = false;
  int16_t *storage_ = nullptr;
  size_t capacity_ = 0;
  size_t recorded_ = 0;
  uint16_t recordPeak_ = 0;
  uint32_t recordStartMs_ = 0;
  uint32_t recordMaxMs_ = 0;
  size_t discardLeft_ = 0;
  uint8_t inputErrors_ = 0;
};

}  // namespace dbmo::audio
