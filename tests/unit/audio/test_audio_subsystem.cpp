#include <cstring>

#include "dbmo/audio/audio_subsystem.h"
#include "dbmo/mock/mock_hal.h"
#include "dbmo_test.h"

using dbmo::ConfigStore;
using dbmo::Event;
using dbmo::EventBus;
using dbmo::Status;
using dbmo::mock::MockClock;
using namespace dbmo::audio;

namespace {

class FakeOutput final : public IAudioOutput {
 public:
  Status begin(uint32_t sampleRateHz) override {
    rate = sampleRateHz;
    ++beginCalls;
    return beginStatus;
  }
  Status setAmplifier(bool on) override {
    amplifier = on;
    ++amplifierChanges;
    return Status::Ok;
  }
  dbmo::Result<size_t> write(const int16_t *samples, size_t n) override {
    if (!dbmo::isOk(writeStatus)) return writeStatus;
    if (!amplifier) writtenWhileAmplifierOff += n;
    size_t accepted = n < acceptPerWrite ? n : acceptPerWrite;
    if (received + accepted > sizeof(data) / sizeof(data[0]))
      accepted = sizeof(data) / sizeof(data[0]) - received;
    memcpy(data + received, samples, accepted * sizeof(int16_t));
    received += accepted;
    return accepted;
  }
  Status end() override {
    ++endCalls;
    return Status::Ok;
  }

  uint32_t rate = 0;
  bool amplifier = true;  // estado desconhecido antes do init: o subsistema precisa desligar
  int amplifierChanges = 0;
  int beginCalls = 0;
  int endCalls = 0;
  size_t acceptPerWrite = 100000;
  int16_t data[8000] = {};
  size_t received = 0;
  size_t writtenWhileAmplifierOff = 0;
  Status beginStatus = Status::Ok;
  Status writeStatus = Status::Ok;
};

class FakeInput final : public IAudioInput {
 public:
  Status begin(uint32_t sampleRateHz) override {
    rate = sampleRateHz;
    running = true;
    ++beginCalls;
    return beginStatus;
  }
  dbmo::Result<size_t> read(int16_t *samples, size_t n) override {
    if (!dbmo::isOk(readStatus)) return readStatus;
    if (!running) return static_cast<size_t>(0);
    const size_t count = n < perRead ? n : perRead;
    // Rampa: a amostra de índice i vale i (módulo 20000), para conferir o que foi guardado.
    for (size_t i = 0; i < count; ++i) samples[i] = static_cast<int16_t>(produced++ % 20000);
    return count;
  }
  Status end() override {
    running = false;
    ++endCalls;
    return Status::Ok;
  }

  uint32_t rate = 0;
  bool running = false;
  int beginCalls = 0;
  int endCalls = 0;
  size_t perRead = 16;  // 16 amostras por leitura ≈ 1 ms a 16 kHz
  uint32_t produced = 0;
  Status beginStatus = Status::Ok;
  Status readStatus = Status::Ok;
};

struct Fixture {
  EventBus bus;
  ConfigStore config;
  MockClock clock;
  FakeOutput output;
  FakeInput input;
  AudioSubsystem audio{bus, config, clock};
  Event events[64];
  int count = 0;

  Fixture() {
    TEST_ASSERT_OK(AudioSubsystem::registerConfig(config));
    TEST_ASSERT_OK(bus.subscribeAll(&Fixture::collect, this));
    audio.setOutput(output);
    audio.setInput(input);
  }
  static void collect(const Event &event, void *context) {
    auto *self = static_cast<Fixture *>(context);
    if (self->count < 64) self->events[self->count++] = event;
  }
  void run(uint32_t durationMs) {
    for (uint32_t i = 0; i < durationMs; ++i) {
      clock.advanceMs(1);
      audio.tick(clock.nowMs());
      bus.dispatch();
    }
  }
  void enableMic() {
    TEST_ASSERT_OK(config.setBool(config_key::kMicEnabled, true));
    audio.applyConfig();
  }
  int countOf(dbmo::EventId id) const {
    int matches = 0;
    for (int i = 0; i < count; ++i)
      if (events[i].id == id) ++matches;
    return matches;
  }
  int indexOf(dbmo::EventId id) const {
    for (int i = 0; i < count; ++i)
      if (events[i].id == id) return i;
    return -1;
  }
  const Event *last(dbmo::EventId id) const {
    for (int i = count - 1; i >= 0; --i)
      if (events[i].id == id) return &events[i];
    return nullptr;
  }
};

}  // namespace

void setUp() {}
void tearDown() {}

void test_init_starts_output_with_amplifier_off() {
  Fixture f;
  TEST_ASSERT_OK(f.audio.init());
  TEST_ASSERT_EQUAL_UINT32(16000, f.output.rate);
  TEST_ASSERT_FALSE(f.output.amplifier);
  TEST_ASSERT_EQUAL_INT(0, f.input.beginCalls);  // o microfone não é ligado no boot
}

void test_init_requires_some_hardware() {
  EventBus bus;
  ConfigStore config;
  MockClock clock;
  TEST_ASSERT_OK(AudioSubsystem::registerConfig(config));
  AudioSubsystem audio(bus, config, clock);
  TEST_ASSERT_STATUS(Status::HardwareAbsent, audio.init());

  Fixture f;
  f.output.beginStatus = Status::IoError;
  TEST_ASSERT_STATUS(Status::IoError, f.audio.init());
}

void test_unsupported_sample_rate_falls_back() {
  Fixture f;
  TEST_ASSERT_OK(f.config.setInt(config_key::kSampleRate, 22050));  // o amplificador não aceita
  TEST_ASSERT_OK(f.audio.init());
  TEST_ASSERT_EQUAL_UINT32(16000, f.audio.sampleRateHz());
  TEST_ASSERT_EQUAL_UINT32(16000, f.output.rate);

  Fixture g;
  TEST_ASSERT_OK(g.config.setInt(config_key::kSampleRate, 44100));
  TEST_ASSERT_OK(g.audio.init());
  TEST_ASSERT_EQUAL_UINT32(44100, g.output.rate);
}

void test_play_tone() {
  Fixture f;
  TEST_ASSERT_OK(f.audio.init());
  TEST_ASSERT_OK(f.audio.playTone(1000, 50));
  TEST_ASSERT_TRUE(f.audio.playing());
  TEST_ASSERT_TRUE(f.output.amplifier);
  TEST_ASSERT_STATUS(Status::Busy, f.audio.playTone(500, 50));
  f.run(20);
  TEST_ASSERT_EQUAL_UINT(800, f.output.received);  // 50 ms a 16 kHz
  TEST_ASSERT_FALSE(f.audio.playing());
  TEST_ASSERT_EQUAL_INT(1, f.countOf(audio_event::kPlaybackStarted));
  TEST_ASSERT_EQUAL_INT(1, f.countOf(audio_event::kPlaybackFinished));
  TEST_ASSERT_EQUAL_UINT(0, f.output.writtenWhileAmplifierOff);
}

void test_amplifier_turns_off_after_the_hold_time() {
  Fixture f;
  TEST_ASSERT_OK(f.audio.init());
  TEST_ASSERT_OK(f.audio.playTone(1000, 10));
  f.run(10);
  TEST_ASSERT_FALSE(f.audio.playing());
  TEST_ASSERT_TRUE(f.output.amplifier);  // ainda esvaziando a fila
  f.run(AudioSubsystem::kAmplifierHoldMs + 5);
  TEST_ASSERT_FALSE(f.output.amplifier);
}

void test_back_to_back_tones_keep_the_amplifier_on() {
  Fixture f;
  TEST_ASSERT_OK(f.audio.init());
  TEST_ASSERT_OK(f.audio.playTone(1000, 10));
  f.run(10);
  const int changes = f.output.amplifierChanges;
  TEST_ASSERT_OK(f.audio.playTone(1200, 10));
  f.run(10);
  TEST_ASSERT_EQUAL_INT(changes, f.output.amplifierChanges);  // sem desligar e religar
  f.run(100);
  TEST_ASSERT_FALSE(f.output.amplifier);
}

void test_volume_scales_the_output() {
  Fixture f;
  TEST_ASSERT_OK(f.config.setInt(config_key::kVolume, 50));
  TEST_ASSERT_OK(f.audio.init());
  const int16_t pcm[] = {8000, -8000, 4000, -4000};
  BufferSource source;
  source.set(pcm, 4);
  TEST_ASSERT_OK(f.audio.play(source));
  f.run(5);
  TEST_ASSERT_EQUAL_UINT(4, f.output.received);
  TEST_ASSERT_EQUAL_INT16(2000, f.output.data[0]);
  TEST_ASSERT_EQUAL_INT16(-2000, f.output.data[1]);
  TEST_ASSERT_EQUAL_INT16(1000, f.output.data[2]);
}

void test_muted_playback_keeps_the_amplifier_off_but_reports_events() {
  Fixture f;
  TEST_ASSERT_OK(f.config.setBool(config_key::kMuted, true));
  TEST_ASSERT_OK(f.audio.init());
  TEST_ASSERT_OK(f.audio.playTone(1000, 20));
  f.run(30);
  TEST_ASSERT_FALSE(f.output.amplifier);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(audio_event::kPlaybackFinished));
  for (size_t i = 0; i < f.output.received; ++i) TEST_ASSERT_EQUAL_INT16(0, f.output.data[i]);
}

void test_slow_output_gets_every_sample_in_order() {
  Fixture f;
  TEST_ASSERT_OK(f.config.setInt(config_key::kVolume, 100));
  TEST_ASSERT_OK(f.audio.init());
  f.output.acceptPerWrite = 7;  // fila I2S quase cheia: aceita pouco por vez
  int16_t pcm[500];
  for (int i = 0; i < 500; ++i) pcm[i] = static_cast<int16_t>(i);
  BufferSource source;
  source.set(pcm, 500);
  TEST_ASSERT_OK(f.audio.play(source));
  f.run(200);
  TEST_ASSERT_EQUAL_UINT(500, f.output.received);
  for (int i = 0; i < 500; ++i) TEST_ASSERT_EQUAL_INT16(i, f.output.data[i]);
  TEST_ASSERT_FALSE(f.audio.playing());
}

void test_playback_is_bounded_per_tick() {
  Fixture f;
  TEST_ASSERT_OK(f.audio.init());
  TEST_ASSERT_OK(f.audio.playTone(1000, 1000));
  f.run(1);
  // Um tick processa no máximo kChunksPerTick blocos, mesmo com a saída aceitando tudo.
  TEST_ASSERT_EQUAL_UINT(AudioSubsystem::kChunk * AudioSubsystem::kChunksPerTick,
                         f.output.received);
}

void test_stop_playback() {
  Fixture f;
  TEST_ASSERT_OK(f.audio.init());
  TEST_ASSERT_OK(f.audio.playTone(1000, 400));
  f.run(2);
  f.audio.stopPlayback();
  f.run(2);
  TEST_ASSERT_FALSE(f.audio.playing());
  TEST_ASSERT_EQUAL_INT(1, f.countOf(audio_event::kPlaybackFinished));
  const size_t received = f.output.received;
  f.run(100);
  TEST_ASSERT_EQUAL_UINT(received, f.output.received);
  TEST_ASSERT_FALSE(f.output.amplifier);
}

void test_output_failure_stops_playback_and_reports() {
  Fixture f;
  TEST_ASSERT_OK(f.audio.init());
  TEST_ASSERT_OK(f.audio.playTone(1000, 200));
  f.run(2);
  f.output.writeStatus = Status::IoError;
  f.run(10);
  TEST_ASSERT_FALSE(f.audio.playing());
  TEST_ASSERT_EQUAL_INT(1, f.countOf(audio_event::kFault));
  TEST_ASSERT_EQUAL_INT(1, f.countOf(audio_event::kPlaybackFinished));
}

void test_recording_is_denied_while_the_microphone_is_disabled() {
  Fixture f;
  TEST_ASSERT_OK(f.audio.init());
  int16_t storage[64];
  TEST_ASSERT_STATUS(Status::NotPermitted, f.audio.startRecording(storage, 64));
  f.run(100);
  TEST_ASSERT_EQUAL_INT(0, f.input.beginCalls);  // o microfone nem é ligado
  TEST_ASSERT_EQUAL_INT(0, f.count);
}

void test_recording_announces_itself_before_any_sample() {
  Fixture f;
  TEST_ASSERT_OK(f.audio.init());
  f.enableMic();
  int16_t storage[400] = {};
  TEST_ASSERT_OK(f.audio.startRecording(storage, 400));
  TEST_ASSERT_EQUAL_UINT(0, f.audio.recordedSamples());
  f.bus.dispatch();
  TEST_ASSERT_EQUAL_INT(1, f.countOf(audio_event::kRecordingStarted));  // antes de qualquer tick

  f.run(1000);
  TEST_ASSERT_FALSE(f.audio.recording());
  TEST_ASSERT_EQUAL_UINT(400, f.audio.recordedSamples());
  const Event *finished = f.last(audio_event::kRecordingFinished);
  TEST_ASSERT_NOT_NULL(finished);
  TEST_ASSERT_EQUAL_UINT32(400, finished->a);
  TEST_ASSERT_TRUE(f.indexOf(audio_event::kRecordingStarted) <
                   f.indexOf(audio_event::kRecordingFinished));
}

void test_recording_discards_microphone_startup() {
  Fixture f;
  TEST_ASSERT_OK(f.audio.init());
  f.enableMic();
  int16_t storage[100] = {};
  TEST_ASSERT_OK(f.audio.startRecording(storage, 100));
  f.run(1000);
  // 100 ms a 16 kHz = 1600 amostras descartadas: a primeira guardada é a de índice 1600.
  TEST_ASSERT_EQUAL_INT16(1600, storage[0]);
  TEST_ASSERT_EQUAL_INT16(1699, storage[99]);
  TEST_ASSERT_EQUAL_UINT16(1699, f.audio.recordedPeak());
  TEST_ASSERT_EQUAL_UINT32(1699, f.last(audio_event::kRecordingFinished)->b);
}

void test_microphone_is_only_on_while_recording() {
  Fixture f;
  TEST_ASSERT_OK(f.audio.init());
  f.enableMic();
  TEST_ASSERT_FALSE(f.input.running);
  int16_t storage[100];
  TEST_ASSERT_OK(f.audio.startRecording(storage, 100));
  TEST_ASSERT_TRUE(f.input.running);
  f.run(1000);
  TEST_ASSERT_FALSE(f.input.running);
  TEST_ASSERT_EQUAL_INT(1, f.input.endCalls);
}

void test_recording_stops_at_max_duration() {
  Fixture f;
  TEST_ASSERT_OK(f.audio.init());
  f.enableMic();
  static int16_t storage[20000];
  TEST_ASSERT_OK(f.audio.startRecording(storage, 20000, 300));
  f.run(290);
  TEST_ASSERT_TRUE(f.audio.recording());
  f.run(20);
  TEST_ASSERT_FALSE(f.audio.recording());
  TEST_ASSERT_TRUE(f.audio.recordedSamples() < 20000);
  TEST_ASSERT_TRUE(f.audio.recordedSamples() > 0);
}

void test_stop_recording() {
  Fixture f;
  TEST_ASSERT_OK(f.audio.init());
  f.enableMic();
  static int16_t storage[20000];
  TEST_ASSERT_OK(f.audio.startRecording(storage, 20000));
  TEST_ASSERT_STATUS(Status::Busy, f.audio.startRecording(storage, 20000));
  f.run(200);
  f.audio.stopRecording();
  f.run(1);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(audio_event::kRecordingFinished));
  const size_t recorded = f.audio.recordedSamples();
  f.run(200);
  TEST_ASSERT_EQUAL_UINT(recorded, f.audio.recordedSamples());
}

void test_disabling_the_microphone_ends_the_recording() {
  Fixture f;
  TEST_ASSERT_OK(f.audio.init());
  f.enableMic();
  static int16_t storage[20000];
  TEST_ASSERT_OK(f.audio.startRecording(storage, 20000));
  f.run(200);
  TEST_ASSERT_OK(f.config.setBool(config_key::kMicEnabled, false));
  f.audio.applyConfig();
  f.run(1);
  TEST_ASSERT_FALSE(f.audio.recording());
  TEST_ASSERT_FALSE(f.input.running);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(audio_event::kRecordingFinished));
}

void test_recording_arguments() {
  Fixture f;
  TEST_ASSERT_OK(f.audio.init());
  f.enableMic();
  int16_t storage[8];
  TEST_ASSERT_STATUS(Status::InvalidArgument, f.audio.startRecording(nullptr, 8));
  TEST_ASSERT_STATUS(Status::InvalidArgument, f.audio.startRecording(storage, 0));
  TEST_ASSERT_STATUS(Status::InvalidArgument, f.audio.startRecording(storage, 8, 0));
  f.input.beginStatus = Status::IoError;
  TEST_ASSERT_STATUS(Status::IoError, f.audio.startRecording(storage, 8));
  TEST_ASSERT_FALSE(f.audio.recording());
}

void test_input_failure_ends_the_recording() {
  Fixture f;
  TEST_ASSERT_OK(f.audio.init());
  f.enableMic();
  static int16_t storage[20000];
  TEST_ASSERT_OK(f.audio.startRecording(storage, 20000));
  f.run(150);
  f.input.readStatus = Status::IoError;
  f.run(10);
  TEST_ASSERT_FALSE(f.audio.recording());
  TEST_ASSERT_EQUAL_INT(1, f.countOf(audio_event::kFault));
  TEST_ASSERT_EQUAL_INT(1, f.countOf(audio_event::kRecordingFinished));
}

void test_play_and_record_at_the_same_time() {
  Fixture f;
  TEST_ASSERT_OK(f.audio.init());
  f.enableMic();
  int16_t storage[200];
  TEST_ASSERT_OK(f.audio.startRecording(storage, 200));
  TEST_ASSERT_OK(f.audio.playTone(1000, 50));
  f.run(1000);
  TEST_ASSERT_EQUAL_UINT(200, f.audio.recordedSamples());
  TEST_ASSERT_EQUAL_UINT(800, f.output.received);
}

void test_shutdown_stops_everything() {
  Fixture f;
  TEST_ASSERT_OK(f.audio.init());
  f.enableMic();
  int16_t storage[4000];
  TEST_ASSERT_OK(f.audio.startRecording(storage, 4000));
  TEST_ASSERT_OK(f.audio.playTone(1000, 500));
  f.run(5);
  TEST_ASSERT_OK(f.audio.shutdown());
  f.bus.dispatch();
  TEST_ASSERT_FALSE(f.output.amplifier);
  TEST_ASSERT_FALSE(f.input.running);
  TEST_ASSERT_EQUAL_INT(1, f.output.endCalls);
  TEST_ASSERT_EQUAL_INT(1, f.countOf(audio_event::kRecordingFinished));
  TEST_ASSERT_STATUS(Status::NotInitialized, f.audio.playTone(1000, 10));
}

void test_output_only_and_input_only() {
  EventBus bus;
  ConfigStore config;
  MockClock clock;
  FakeOutput output;
  TEST_ASSERT_OK(AudioSubsystem::registerConfig(config));
  TEST_ASSERT_OK(config.setBool(config_key::kMicEnabled, true));
  AudioSubsystem audio(bus, config, clock);
  audio.setOutput(output);
  TEST_ASSERT_OK(audio.init());
  int16_t storage[8];
  TEST_ASSERT_STATUS(Status::Unsupported, audio.startRecording(storage, 8));

  FakeInput input;
  AudioSubsystem micOnly(bus, config, clock);
  micOnly.setInput(input);
  TEST_ASSERT_OK(micOnly.init());
  TEST_ASSERT_STATUS(Status::Unsupported, micOnly.playTone(1000, 10));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_init_starts_output_with_amplifier_off);
  RUN_TEST(test_init_requires_some_hardware);
  RUN_TEST(test_unsupported_sample_rate_falls_back);
  RUN_TEST(test_play_tone);
  RUN_TEST(test_amplifier_turns_off_after_the_hold_time);
  RUN_TEST(test_back_to_back_tones_keep_the_amplifier_on);
  RUN_TEST(test_volume_scales_the_output);
  RUN_TEST(test_muted_playback_keeps_the_amplifier_off_but_reports_events);
  RUN_TEST(test_slow_output_gets_every_sample_in_order);
  RUN_TEST(test_playback_is_bounded_per_tick);
  RUN_TEST(test_stop_playback);
  RUN_TEST(test_output_failure_stops_playback_and_reports);
  RUN_TEST(test_recording_is_denied_while_the_microphone_is_disabled);
  RUN_TEST(test_recording_announces_itself_before_any_sample);
  RUN_TEST(test_recording_discards_microphone_startup);
  RUN_TEST(test_microphone_is_only_on_while_recording);
  RUN_TEST(test_recording_stops_at_max_duration);
  RUN_TEST(test_stop_recording);
  RUN_TEST(test_disabling_the_microphone_ends_the_recording);
  RUN_TEST(test_recording_arguments);
  RUN_TEST(test_input_failure_ends_the_recording);
  RUN_TEST(test_play_and_record_at_the_same_time);
  RUN_TEST(test_shutdown_stops_everything);
  RUN_TEST(test_output_only_and_input_only);
  return UNITY_END();
}
