#include "dbmo/audio/audio_subsystem.h"

#include "dbmo/core/log.h"

namespace dbmo::audio {

namespace {
constexpr char kTag[] = "audio";
}

AudioSubsystem::AudioSubsystem(EventBus &bus, ConfigStore &config, IClock &clock)
    : bus_(bus), config_(config), clock_(clock) {}

Status AudioSubsystem::registerConfig(ConfigStore &config) {
  DBMO_TRY(config.define(ConfigSpec::integer(config_key::kVolume, 60, 0, 100)));
  DBMO_TRY(config.define(ConfigSpec::boolean(config_key::kMuted, false)));
  DBMO_TRY(
      config.define(ConfigSpec::integer(config_key::kSampleRate, kDefaultSampleRate, 8000, 48000)));
  DBMO_TRY(config.define(ConfigSpec::boolean(config_key::kMicEnabled, false)));
  return Status::Ok;
}

void AudioSubsystem::applyConfig() {
  volume_.setPercent(static_cast<uint8_t>(config_.getInt(config_key::kVolume).valueOr(60)));
  volume_.setMuted(config_.getBool(config_key::kMuted).valueOr(false));
  micEnabled_ = config_.getBool(config_key::kMicEnabled).valueOr(false);
  // Desabilitar o microfone no meio de uma gravação a encerra.
  if (!micEnabled_ && recording_) finishRecording();
}

Status AudioSubsystem::init() {
  initialized_ = false;
  playing_ = recording_ = amplifierOn_ = amplifierHold_ = false;
  pendingCount_ = pendingOffset_ = 0;
  outputErrors_ = inputErrors_ = 0;
  applyConfig();
  if (output_ == nullptr && input_ == nullptr) return Status::HardwareAbsent;

  const uint32_t requested =
      static_cast<uint32_t>(config_.getInt(config_key::kSampleRate).valueOr(kDefaultSampleRate));
  if (isSupportedSampleRate(requested)) {
    sampleRateHz_ = requested;
  } else {
    DBMO_LOGW(kTag, "taxa %lu Hz nao e aceita pelo amplificador; usando %lu",
              static_cast<unsigned long>(requested),
              static_cast<unsigned long>(kDefaultSampleRate));
    sampleRateHz_ = kDefaultSampleRate;
  }

  if (output_ != nullptr) {
    DBMO_TRY(output_->begin(sampleRateHz_));
    DBMO_TRY(output_->setAmplifier(false));  // amplificador desligado em repouso
  }
  initialized_ = true;
  DBMO_LOGI(kTag, "%lu Hz; saida: %s; microfone: %s", static_cast<unsigned long>(sampleRateHz_),
            output_ != nullptr ? "sim" : "nao",
            input_ == nullptr ? "nao"
            : micEnabled_     ? "habilitado"
                              : "desabilitado");
  return Status::Ok;
}

Status AudioSubsystem::shutdown() {
  if (recording_) finishRecording();
  playing_ = false;
  source_ = nullptr;
  Status status = Status::Ok;
  if (initialized_ && output_ != nullptr) {
    status = output_->setAmplifier(false);
    const Status ended = output_->end();
    if (isOk(status)) status = ended;
  }
  amplifierOn_ = amplifierHold_ = false;
  initialized_ = false;
  return status;
}

void AudioSubsystem::fault(Status status) {
  DBMO_LOGE(kTag, "falha de audio: %s", toString(status));
  (void)bus_.publish(audio_event::kFault, static_cast<uint32_t>(status));
}

// ---- Reprodução ----

Status AudioSubsystem::play(IAudioSource &source) {
  if (!initialized_) return Status::NotInitialized;
  if (output_ == nullptr) return Status::Unsupported;
  if (playing_) return Status::Busy;
  // Mudo ou volume zero: a reprodução corre (os eventos saem), mas o amplificador fica
  // desligado.
  const bool audible = !volume_.silent();
  if (audible != amplifierOn_) {
    DBMO_TRY(output_->setAmplifier(audible));
    amplifierOn_ = audible;
  }
  source_ = &source;
  playing_ = true;
  amplifierHold_ = false;
  pendingCount_ = pendingOffset_ = 0;
  outputErrors_ = 0;
  (void)bus_.publish(audio_event::kPlaybackStarted);
  return Status::Ok;
}

Status AudioSubsystem::playTone(uint32_t frequencyHz, uint32_t durationMs) {
  if (!initialized_) return Status::NotInitialized;
  if (output_ == nullptr) return Status::Unsupported;
  if (playing_) return Status::Busy;
  DBMO_TRY(tone_.start(frequencyHz, durationMs, sampleRateHz_));
  return play(tone_);
}

void AudioSubsystem::stopPlayback() {
  if (playing_) finishPlayback(clock_.nowMs());
}

void AudioSubsystem::finishPlayback(uint32_t nowMs) {
  playing_ = false;
  source_ = nullptr;
  pendingCount_ = pendingOffset_ = 0;
  amplifierHold_ = amplifierOn_;
  holdStartMs_ = nowMs;
  (void)bus_.publish(audio_event::kPlaybackFinished);
}

void AudioSubsystem::servicePlayback(uint32_t nowMs) {
  for (uint8_t chunk = 0; chunk < kChunksPerTick; ++chunk) {
    if (pendingOffset_ == pendingCount_) {
      pendingCount_ = source_->read(pending_, kChunk);
      pendingOffset_ = 0;
      if (pendingCount_ == 0) {
        finishPlayback(nowMs);
        return;
      }
      if (pendingCount_ > kChunk) pendingCount_ = kChunk;  // origem mal-comportada
      volume_.apply(pending_, pendingCount_);
    }
    const Result<size_t> written =
        output_->write(pending_ + pendingOffset_, pendingCount_ - pendingOffset_);
    if (!written.ok()) {
      if (++outputErrors_ >= kFaultThreshold) {
        fault(written.status());
        finishPlayback(nowMs);
      }
      return;
    }
    outputErrors_ = 0;
    const size_t accepted = written.value() > pendingCount_ - pendingOffset_
                                ? pendingCount_ - pendingOffset_
                                : written.value();
    pendingOffset_ += accepted;
    if (pendingOffset_ < pendingCount_) return;  // a saída está cheia: continua no próximo tick
  }
}

// ---- Gravação ----

Status AudioSubsystem::startRecording(int16_t *storage, size_t capacity, uint32_t maxMs) {
  if (!initialized_) return Status::NotInitialized;
  if (input_ == nullptr) return Status::Unsupported;
  if (!micEnabled_) return Status::NotPermitted;
  if (recording_) return Status::Busy;
  if (storage == nullptr || capacity == 0 || maxMs == 0) return Status::InvalidArgument;

  DBMO_TRY(input_->begin(sampleRateHz_));
  storage_ = storage;
  capacity_ = capacity;
  recorded_ = 0;
  recordPeak_ = 0;
  recordStartMs_ = clock_.nowMs();
  recordMaxMs_ = maxMs > kMaxRecordMs ? kMaxRecordMs : maxMs;
  discardLeft_ = static_cast<size_t>(static_cast<uint64_t>(kMicStartupMs) * sampleRateHz_ / 1000);
  inputErrors_ = 0;
  recording_ = true;
  // O aviso sai antes de qualquer amostra ser guardada.
  (void)bus_.publish(audio_event::kRecordingStarted);
  DBMO_LOGI(kTag, "gravacao iniciada");
  return Status::Ok;
}

void AudioSubsystem::stopRecording() {
  if (recording_) finishRecording();
}

void AudioSubsystem::finishRecording() {
  recording_ = false;
  if (input_ != nullptr) (void)input_->end();  // microfone só fica ligado gravando
  (void)bus_.publish(audio_event::kRecordingFinished, static_cast<uint32_t>(recorded_),
                     recordPeak_);
  DBMO_LOGI(kTag, "gravacao encerrada: %lu amostras", static_cast<unsigned long>(recorded_));
}

void AudioSubsystem::serviceRecording(uint32_t nowMs) {
  int16_t chunk[kChunk];
  for (uint8_t round = 0; round < kChunksPerTick; ++round) {
    const Result<size_t> got = input_->read(chunk, kChunk);
    if (!got.ok()) {
      if (++inputErrors_ >= kFaultThreshold) {
        fault(got.status());
        finishRecording();
      }
      return;
    }
    inputErrors_ = 0;
    size_t count = got.value() > kChunk ? kChunk : got.value();
    if (count == 0) break;

    const int16_t *samples = chunk;
    if (discardLeft_ > 0) {
      const size_t skip = count < discardLeft_ ? count : discardLeft_;
      discardLeft_ -= skip;
      samples += skip;
      count -= skip;
    }
    const size_t room = capacity_ - recorded_;
    if (count > room) count = room;
    for (size_t i = 0; i < count; ++i) storage_[recorded_ + i] = samples[i];
    const uint16_t peak = peakLevel(samples, count);
    if (peak > recordPeak_) recordPeak_ = peak;
    recorded_ += count;
    if (recorded_ == capacity_) {
      finishRecording();
      return;
    }
  }
  if (elapsedMs(nowMs, recordStartMs_) >= recordMaxMs_) finishRecording();
}

void AudioSubsystem::tick(uint32_t nowMs) {
  if (!initialized_) return;
  if (playing_) servicePlayback(nowMs);
  if (!playing_ && amplifierHold_ && elapsedMs(nowMs, holdStartMs_) >= kAmplifierHoldMs) {
    amplifierHold_ = false;
    if (isOk(output_->setAmplifier(false))) amplifierOn_ = false;
  }
  if (recording_) serviceRecording(nowMs);
}

}  // namespace dbmo::audio
