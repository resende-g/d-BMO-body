// Comandos do console serial: diagnóstico e operação de bancada.
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "dbmo/app/application.h"
#include "dbmo/core/version.h"

namespace dbmo::app {

namespace {

Application &appOf(void *context) { return *static_cast<Application *>(context); }

bool isWord(const char *text, const char *word) { return strcmp(text, word) == 0; }

// Número decimal sem sinal; false se o texto não é só dígitos ou passa de limit.
bool parseNumber(const char *text, uint32_t limit, uint32_t *out) {
  if (text == nullptr || *text == '\0') return false;
  char *end = nullptr;
  const unsigned long value = strtoul(text, &end, 10);
  if (*end != '\0' || value > limit) return false;
  *out = static_cast<uint32_t>(value);
  return true;
}

void printValue(Console &console, ConfigStore &config, const char *key) {
  const Result<int32_t> number = config.getInt(key);
  if (number.ok()) {
    console.printf("%s=%ld\r\n", key, static_cast<long>(number.value()));
    return;
  }
  const Result<bool> flag = config.getBool(key);
  if (flag.ok()) {
    console.printf("%s=%d\r\n", key, flag.value() ? 1 : 0);
    return;
  }
  char text[ConfigStore::kMaxStringLength + 1];
  if (isOk(config.getString(key, text, sizeof(text)))) console.printf("%s=%s\r\n", key, text);
}

void statusCommand(Console &console, int, const char *const *, void *context) {
  Application &app = appOf(context);
  System &system = app.system();
  console.printf("aplicacao: %s; sistema: %s\r\n", toString(app.runState()),
                 toString(system.state()));
  for (size_t i = 0; i < system.subsystemCount(); ++i) {
    const SubsystemInfo &info = system.info(i);
    console.printf("  %-12s %-13s %s\r\n", info.subsystem->name(), toString(info.state),
                   toString(info.lastStatus));
  }
  const Result<uint16_t> battery = app.power().battery().millivolts();
  if (battery.ok()) {
    console.printf("bateria: %u mV (%s)\r\n", static_cast<unsigned>(battery.value()),
                   power::toString(app.power().battery().level()));
  } else {
    // Sem leitura válida não se mostra número nenhum.
    console.printf("bateria: sem leitura (%s)\r\n", power::toString(app.power().battery().fault()));
  }
  console.printf("carga: %s; cartao: %s; nfc: %s; radio: %s\r\n",
                 power::toString(app.power().chargeState()),
                 app.storage().available() ? "disponivel" : "indisponivel",
                 nfc::toString(app.nfc().state()), radio::toString(app.radio().state()));
  console.printf("eventos descartados: %lu\r\n",
                 static_cast<unsigned long>(app.bus().droppedCount()));
}

void configCommand(Console &console, int argc, const char *const *argv, void *context) {
  ConfigStore &config = appOf(context).config();
  if (argc == 1) {
    for (size_t i = 0; i < config.size(); ++i) printValue(console, config, config.keyAt(i));
    return;
  }
  if (!config.has(argv[1])) {
    console.printf("chave desconhecida: %s\r\n", argv[1]);
    return;
  }
  if (argc >= 3) {
    const Status status = config.setFromText(argv[1], argv[2]);
    if (!isOk(status)) {
      console.printf("valor recusado: %s\r\n", toString(status));
      return;
    }
  }
  printValue(console, config, argv[1]);
}

void saveCommand(Console &console, int, const char *const *, void *context) {
  console.printf("gravar configuracao: %s\r\n", toString(appOf(context).persistence().saveNow()));
}

void faceCommand(Console &console, int argc, const char *const *argv, void *context) {
  if (argc < 2) {
    for (uint8_t i = 0; i < rosto::kExpressionCount; ++i)
      console.printf("%u %s\r\n", static_cast<unsigned>(i),
                     rosto::name(static_cast<rosto::Expression>(i)));
    return;
  }
  uint32_t index = 0;
  Result<rosto::Expression> expression = parseNumber(argv[1], 255, &index)
                                             ? rosto::expressionFromIndex(index)
                                             : rosto::expressionFromName(argv[1]);
  if (!expression.ok()) {
    console.print("expressao desconhecida\r\n");
    return;
  }
  // Publica o pedido como qualquer outro módulo faria.
  (void)appOf(context).bus().publish(rosto::face_event::kRequest,
                                     static_cast<uint32_t>(expression.value()));
  console.printf("rosto: %s\r\n", rosto::name(expression.value()));
}

void toneCommand(Console &console, int argc, const char *const *argv, void *context) {
  uint32_t frequency = 1000;
  uint32_t duration = 200;
  if ((argc >= 2 && !parseNumber(argv[1], 20000, &frequency)) ||
      (argc >= 3 && !parseNumber(argv[2], 5000, &duration))) {
    console.print("uso: tone [hz] [ms]\r\n");
    return;
  }
  console.printf("tom: %s\r\n", toString(appOf(context).audio().playTone(frequency, duration)));
}

// Teste do microfone: grava em memória e mostra o pico no log ("gravacao encerrada").
// Nada vai para arquivo. Reserva de 16000 amostras = 1 s a 16 kHz (32 KB de RAM), usada quando
// a plataforma não dá um buffer maior (Application::setRecordBuffer).
int16_t gMicBuffer[16000];

struct MicBuffer {
  int16_t *samples;
  size_t capacity;
};

MicBuffer micBufferOf(const Application &app) {
  if (app.recordBuffer() != nullptr && app.recordCapacity() > 0)
    return {app.recordBuffer(), app.recordCapacity()};
  return {gMicBuffer, sizeof(gMicBuffer) / sizeof(gMicBuffer[0])};
}

// Média, extremos e RMS da última gravação, e o pico a cada 100 ms descontada a média: separa
// som de desvio de DC e mostra em que trecho o pico aconteceu.
void micLast(Console &console, audio::AudioSubsystem &audio, const int16_t *samples) {
  if (audio.recording()) {
    console.print("mic: gravando\r\n");
    return;
  }
  const size_t n = audio.recordedSamples();
  if (n == 0) {
    console.print("mic: nenhuma gravacao\r\n");
    return;
  }
  int64_t sum = 0;
  uint64_t squares = 0;
  int16_t low = samples[0];
  int16_t high = samples[0];
  for (size_t i = 0; i < n; ++i) {
    const int32_t sample = samples[i];
    sum += sample;
    squares += static_cast<uint64_t>(sample * sample);
    if (sample < low) low = static_cast<int16_t>(sample);
    if (sample > high) high = static_cast<int16_t>(sample);
  }
  const int32_t mean = static_cast<int32_t>(sum / static_cast<int64_t>(n));
  console.printf("mic: %lu amostras; media %ld; min %d; max %d; rms %lu\r\n",
                 static_cast<unsigned long>(n), static_cast<long>(mean), low, high,
                 static_cast<unsigned long>(sqrt(static_cast<double>(squares) / n) + 0.5));
  // Por trecho (100 ms até 1 s de gravação, 1 s acima disso): a média (desvio lento) e o RMS e o
  // pico em volta da média do próprio trecho (som e ruído), para separar um do outro.
  const uint32_t blockMs = n <= audio.sampleRateHz() ? 100 : 1000;
  const size_t perBlock = static_cast<size_t>(audio.sampleRateHz()) * blockMs / 1000;
  const size_t block = perBlock > 0 ? perBlock : n;
  const char *const labels[] = {"media", "rms", "pico"};
  for (int line = 0; line < 3; ++line) {
    console.printf("%s por %lu ms:", labels[line], static_cast<unsigned long>(blockMs));
    for (size_t start = 0; start < n; start += block) {
      const size_t end = start + block < n ? start + block : n;
      int64_t blockSum = 0;
      for (size_t i = start; i < end; ++i) blockSum += samples[i];
      const int32_t blockMean = static_cast<int32_t>(blockSum / static_cast<int64_t>(end - start));
      uint64_t blockSquares = 0;
      int32_t peak = 0;
      for (size_t i = start; i < end; ++i) {
        const int32_t deviation = samples[i] - blockMean;
        blockSquares += static_cast<uint64_t>(deviation * deviation);
        if (abs(deviation) > peak) peak = abs(deviation);
      }
      const long value =
          line == 0 ? blockMean
          : line == 1
              ? static_cast<long>(sqrt(static_cast<double>(blockSquares) / (end - start)) + 0.5)
              : peak;
      console.printf(" %ld", value);
    }
    console.print("\r\n");
  }
}

void micCommand(Console &console, int argc, const char *const *argv, void *context) {
  Application &app = appOf(context);
  const MicBuffer buffer = micBufferOf(app);
  if (argc > 1 && isWord(argv[1], "last")) {
    micLast(console, app.audio(), buffer.samples);
    return;
  }
  // Duração que cabe no buffer; os 100 ms descartados na partida do microfone não ocupam espaço.
  const uint32_t rate = app.audio().sampleRateHz() > 0 ? app.audio().sampleRateHz() : 16000;
  const uint32_t limitMs = static_cast<uint32_t>(buffer.capacity * 1000 / rate);
  uint32_t ms = 1000 < limitMs ? 1000 : limitMs;
  if (argc > 1 && !parseNumber(argv[1], limitMs, &ms)) {
    console.printf("uso: mic [ms], ate %lu; mic last\r\n", static_cast<unsigned long>(limitMs));
    return;
  }
  const Status status = app.audio().startRecording(buffer.samples, buffer.capacity, ms);
  if (status == Status::NotPermitted) {
    console.print("mic: desabilitado; use config audio.mic_enabled 1\r\n");
  } else if (isOk(status)) {
    console.printf("mic: gravando %lu ms\r\n", static_cast<unsigned long>(ms));
  } else {
    console.printf("mic: %s\r\n", toString(status));
  }
}

void nfcCommand(Console &console, int argc, const char *const *argv, void *context) {
  nfc::NfcSubsystem &nfc = appOf(context).nfc();
  if (argc >= 2 && isWord(argv[1], "on")) {
    console.printf("busca: %s\r\n", toString(nfc.startScan()));
  } else if (argc >= 2 && isWord(argv[1], "off")) {
    console.printf("busca: %s\r\n", toString(nfc.stopScan()));
  } else {
    nfc::TagInfo tag{};
    // O UID só aparece aqui, a pedido de quem está com o aparelho na mão.
    if (nfc.currentTag(&tag)) {
      console.print("etiqueta:");
      for (uint8_t i = 0; i < tag.uidLength; ++i)
        console.printf(" %02X", static_cast<unsigned>(tag.uid[i]));
      console.print("\r\n");
    } else {
      console.printf("nfc: %s, sem etiqueta (uso: nfc on|off)\r\n", nfc::toString(nfc.state()));
    }
  }
}

void irCommand(Console &console, int argc, const char *const *argv, void *context) {
  Application &app = appOf(context);
  infrared::InfraredSubsystem &ir = app.infrared();
  if (argc >= 2 && isWord(argv[1], "learn")) {
    console.printf("aprender: %s\r\n", toString(ir.startLearning()));
  } else if (argc >= 3 && isWord(argv[1], "save")) {
    infrared::StoredCode code;
    if (!ir.learned(&code)) {
      console.print("nenhum codigo aprendido\r\n");
      return;
    }
    console.printf("guardar %s: %s\r\n", argv[2], toString(app.irLibrary().save(argv[2], code)));
  } else if (argc >= 3 && isWord(argv[1], "send")) {
    infrared::StoredCode code;
    Status status = app.irLibrary().load(argv[2], &code);
    if (isOk(status)) status = ir.send(code);
    console.printf("enviar %s: %s\r\n", argv[2], toString(status));
  } else if (argc >= 2 && isWord(argv[1], "last")) {
    infrared::StoredCode code;
    char text[infrared::kMaxCodeText];
    if (ir.lastReceived(&code) && infrared::formatCode(code, text, sizeof(text)).ok()) {
      console.print(text);
      console.print("\r\n");
    } else {
      console.print("nada recebido\r\n");
    }
  } else {
    console.print("uso: ir learn | ir save <nome> | ir send <nome> | ir last\r\n");
  }
}

void radioCommand(Console &console, int argc, const char *const *argv, void *context) {
  radio::RadioSubsystem &radio = appOf(context).radio();
  if (argc >= 3 && isWord(argv[1], "rx") && isWord(argv[2], "on")) {
    console.printf("recepcao: %s\r\n", toString(radio.startReceive()));
  } else if (argc >= 3 && isWord(argv[1], "rx") && isWord(argv[2], "off")) {
    console.printf("recepcao: %s\r\n", toString(radio.stop()));
  } else {
    const radio::RadioCounters &counters = radio.counters();
    console.printf(
        "radio: %s, %lu Hz; pacotes %lu, crc %lu, negados %lu (uso: radio rx on|off)\r\n",
        radio::toString(radio.state()), static_cast<unsigned long>(radio.radioConfig().frequencyHz),
        static_cast<unsigned long>(counters.packets),
        static_cast<unsigned long>(counters.crcErrors),
        static_cast<unsigned long>(counters.denied));
  }
}

void cartridgeCommand(Console &console, int, const char *const *, void *context) {
  cartridges::CartridgeSubsystem &slot = appOf(context).cartridges();
  const cartridges::CartridgeHeader *header = slot.header();
  if (header != nullptr) {
    console.printf("cartucho: %s (%04X:%04X), %u mA\r\n", header->name,
                   static_cast<unsigned>(header->vendorId),
                   static_cast<unsigned>(header->productId),
                   static_cast<unsigned>(header->maxCurrentMa));
  } else {
    console.printf("cartucho: nenhum identificado (%s)\r\n",
                   cartridges::toString(slot.lastHeaderCheck()));
  }
}

void sdCommand(Console &console, int, const char *const *, void *context) {
  console.printf("cartao: %s\r\n", toString(appOf(context).retryStorage()));
}

void offCommand(Console &console, int, const char *const *, void *context) {
  console.print("desligando\r\n");
  appOf(context).power().requestShutdown(power::ShutdownReason::Command);
}

}  // namespace

Status Application::registerCommands(Console &console) {
  const Console::Command commands[] = {
      {"status", "estado do sistema e dos subsistemas", statusCommand, this},
      {"config", "config [chave [valor]]: lista, mostra ou altera", configCommand, this},
      {"save", "grava a configuracao agora", saveCommand, this},
      {"face", "face [nome|numero]: mostra uma expressao", faceCommand, this},
      {"tone", "tone [hz] [ms]: toca um tom", toneCommand, this},
      {"mic", "mic [ms] | mic last: grava em memoria; estatisticas da ultima gravacao", micCommand,
       this},
      {"nfc", "nfc [on|off]: busca de etiquetas", nfcCommand, this},
      {"ir", "ir learn|save|send|last: infravermelho", irCommand, this},
      {"radio", "radio [rx on|off]: radio sub-GHz", radioCommand, this},
      {"cart", "cartucho inserido", cartridgeCommand, this},
      {"sd", "tenta montar o cartao de novo", sdCommand, this},
      {"off", "desliga o aparelho", offCommand, this},
  };
  for (const Console::Command &command : commands) DBMO_TRY(console.registerCommand(command));
  return Status::Ok;
}

}  // namespace dbmo::app
