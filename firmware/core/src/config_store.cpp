#include "dbmo/core/config_store.h"

#include <climits>
#include <cstdio>
#include <cstring>

namespace dbmo {

namespace {

constexpr char kHeader[] = "# d-BMO config v1\n";

// Buffer de trabalho de load()/save(): estático para não ocupar a pilha da tarefa.
char gScratch[ConfigStore::kSerializedMax];

bool validKey(const char *key) {
  if (key == nullptr || *key == '\0') return false;
  size_t length = 0;
  for (const char *c = key; *c != '\0'; ++c, ++length) {
    const bool ok = (*c >= 'a' && *c <= 'z') || (*c >= '0' && *c <= '9') || *c == '_' || *c == '.';
    if (!ok) return false;
  }
  return length <= ConfigStore::kMaxKeyLength;
}

// Texto imprimível, sem espaço nas pontas (para a ida e volta pelo arquivo ser exata).
bool validString(const char *value) {
  if (value == nullptr) return false;
  const size_t length = strlen(value);
  if (length > ConfigStore::kMaxStringLength) return false;
  for (size_t i = 0; i < length; ++i)
    if (value[i] < 0x20 || value[i] > 0x7E) return false;
  return length == 0 || (value[0] != ' ' && value[length - 1] != ' ');
}

bool parseInt(const char *text, int32_t *out) {
  if (text == nullptr || *text == '\0') return false;
  const bool negative = *text == '-';
  const char *cursor = negative ? text + 1 : text;
  if (*cursor == '\0') return false;
  int64_t value = 0;
  for (; *cursor != '\0'; ++cursor) {
    if (*cursor < '0' || *cursor > '9') return false;
    value = value * 10 + (*cursor - '0');
    if (value > static_cast<int64_t>(INT32_MAX) + 1) return false;
  }
  if (negative) value = -value;
  if (value < INT32_MIN || value > INT32_MAX) return false;
  *out = static_cast<int32_t>(value);
  return true;
}

bool parseBool(const char *text, bool *out) {
  if (text == nullptr) return false;
  if (strcmp(text, "1") == 0 || strcmp(text, "true") == 0 || strcmp(text, "on") == 0) {
    *out = true;
    return true;
  }
  if (strcmp(text, "0") == 0 || strcmp(text, "false") == 0 || strcmp(text, "off") == 0) {
    *out = false;
    return true;
  }
  return false;
}

bool isBlank(char c) { return c == ' ' || c == '\t' || c == '\r'; }

}  // namespace

ConfigStore::Entry *ConfigStore::findEntry(const char *key) {
  return const_cast<Entry *>(static_cast<const ConfigStore *>(this)->findEntry(key));
}

const ConfigStore::Entry *ConfigStore::findEntry(const char *key) const {
  if (key == nullptr) return nullptr;
  for (size_t i = 0; i < count_; ++i)
    if (strcmp(entries_[i].spec.key, key) == 0) return &entries_[i];
  return nullptr;
}

void ConfigStore::changed(const Entry &entry) {
  dirty_ = true;
  if (observer_ != nullptr) observer_(entry.spec.key, observerContext_);
}

void ConfigStore::setChangeObserver(ChangeObserver observer, void *context) {
  observer_ = observer;
  observerContext_ = context;
}

Status ConfigStore::define(const ConfigSpec &spec) {
  if (!validKey(spec.key)) return Status::InvalidArgument;
  if (findEntry(spec.key) != nullptr) return Status::AlreadyInitialized;
  if (count_ == kMaxEntries) return Status::NoSpace;

  Entry entry{};
  entry.spec = spec;
  switch (spec.type) {
    case ConfigType::Bool:
    case ConfigType::Int:
      if (spec.minInt > spec.maxInt || spec.defaultInt < spec.minInt ||
          spec.defaultInt > spec.maxInt)
        return Status::InvalidArgument;
      entry.intValue = spec.defaultInt;
      break;
    case ConfigType::String:
      if (!validString(spec.defaultString)) return Status::InvalidArgument;
      snprintf(entry.stringValue, sizeof(entry.stringValue), "%s", spec.defaultString);
      break;
  }
  entries_[count_++] = entry;
  return Status::Ok;
}

bool ConfigStore::has(const char *key) const { return findEntry(key) != nullptr; }

Result<int32_t> ConfigStore::getInt(const char *key) const {
  const Entry *entry = findEntry(key);
  if (entry == nullptr) return Status::NotFound;
  if (entry->spec.type != ConfigType::Int) return Status::InvalidArgument;
  return entry->intValue;
}

Result<bool> ConfigStore::getBool(const char *key) const {
  const Entry *entry = findEntry(key);
  if (entry == nullptr) return Status::NotFound;
  if (entry->spec.type != ConfigType::Bool) return Status::InvalidArgument;
  return entry->intValue != 0;
}

Status ConfigStore::getString(const char *key, char *out, size_t capacity) const {
  const Entry *entry = findEntry(key);
  if (entry == nullptr) return Status::NotFound;
  if (entry->spec.type != ConfigType::String) return Status::InvalidArgument;
  if (out == nullptr || capacity <= strlen(entry->stringValue)) return Status::NoSpace;
  snprintf(out, capacity, "%s", entry->stringValue);
  return Status::Ok;
}

Status ConfigStore::setInt(const char *key, int32_t value) {
  Entry *entry = findEntry(key);
  if (entry == nullptr) return Status::NotFound;
  if (entry->spec.type != ConfigType::Int) return Status::InvalidArgument;
  if (value < entry->spec.minInt || value > entry->spec.maxInt) return Status::OutOfRange;
  if (entry->intValue != value) {
    entry->intValue = value;
    changed(*entry);
  }
  return Status::Ok;
}

Status ConfigStore::setBool(const char *key, bool value) {
  Entry *entry = findEntry(key);
  if (entry == nullptr) return Status::NotFound;
  if (entry->spec.type != ConfigType::Bool) return Status::InvalidArgument;
  const int32_t stored = value ? 1 : 0;
  if (entry->intValue != stored) {
    entry->intValue = stored;
    changed(*entry);
  }
  return Status::Ok;
}

Status ConfigStore::setString(const char *key, const char *value) {
  Entry *entry = findEntry(key);
  if (entry == nullptr) return Status::NotFound;
  if (entry->spec.type != ConfigType::String) return Status::InvalidArgument;
  if (!validString(value)) return Status::InvalidArgument;
  if (strcmp(entry->stringValue, value) != 0) {
    snprintf(entry->stringValue, sizeof(entry->stringValue), "%s", value);
    changed(*entry);
  }
  return Status::Ok;
}

Status ConfigStore::setFromText(const char *key, const char *value) {
  const Entry *entry = findEntry(key);
  if (entry == nullptr) return Status::NotFound;
  switch (entry->spec.type) {
    case ConfigType::Bool: {
      bool parsed = false;
      if (!parseBool(value, &parsed)) return Status::InvalidArgument;
      return setBool(key, parsed);
    }
    case ConfigType::Int: {
      int32_t parsed = 0;
      if (!parseInt(value, &parsed)) return Status::InvalidArgument;
      return setInt(key, parsed);
    }
    case ConfigType::String: return setString(key, value);
  }
  return Status::InternalError;
}

void ConfigStore::resetToDefaults() {
  for (size_t i = 0; i < count_; ++i) {
    Entry &entry = entries_[i];
    if (entry.spec.type == ConfigType::String) {
      if (strcmp(entry.stringValue, entry.spec.defaultString) != 0) {
        snprintf(entry.stringValue, sizeof(entry.stringValue), "%s", entry.spec.defaultString);
        changed(entry);
      }
    } else if (entry.intValue != entry.spec.defaultInt) {
      entry.intValue = entry.spec.defaultInt;
      changed(entry);
    }
  }
}

Result<size_t> ConfigStore::serialize(char *out, size_t capacity) const {
  if (out == nullptr) return Status::InvalidArgument;
  size_t used = 0;
  int written = snprintf(out, capacity, "%s", kHeader);
  if (written < 0 || static_cast<size_t>(written) >= capacity) return Status::NoSpace;
  used += static_cast<size_t>(written);

  for (size_t i = 0; i < count_; ++i) {
    const Entry &entry = entries_[i];
    if (entry.spec.type == ConfigType::String)
      written = snprintf(out + used, capacity - used, "%s=%s\n", entry.spec.key, entry.stringValue);
    else
      written = snprintf(out + used, capacity - used, "%s=%ld\n", entry.spec.key,
                         static_cast<long>(entry.intValue));
    if (written < 0 || static_cast<size_t>(written) >= capacity - used) return Status::NoSpace;
    used += static_cast<size_t>(written);
  }
  return used;
}

Status ConfigStore::deserialize(const char *text, size_t length, ConfigLoadReport *report) {
  if (text == nullptr) return Status::InvalidArgument;
  ConfigLoadReport local;

  size_t lineStart = 0;
  while (lineStart < length) {
    size_t lineEnd = lineStart;
    while (lineEnd < length && text[lineEnd] != '\n') ++lineEnd;

    size_t begin = lineStart;
    size_t end = lineEnd;
    while (begin < end && isBlank(text[begin])) ++begin;
    while (end > begin && isBlank(text[end - 1])) --end;
    lineStart = lineEnd + 1;

    if (begin == end || text[begin] == '#') continue;

    size_t equals = begin;
    while (equals < end && text[equals] != '=') ++equals;
    if (equals == end) {
      ++local.malformedLines;
      continue;
    }

    size_t keyEnd = equals;
    while (keyEnd > begin && isBlank(text[keyEnd - 1])) --keyEnd;
    size_t valueBegin = equals + 1;
    while (valueBegin < end && isBlank(text[valueBegin])) ++valueBegin;

    char key[kMaxKeyLength + 1];
    char value[kMaxStringLength + 1];
    const size_t keyLength = keyEnd - begin;
    const size_t valueLength = end - valueBegin;
    if (keyLength == 0 || keyLength > kMaxKeyLength) {
      ++local.malformedLines;
      continue;
    }
    memcpy(key, text + begin, keyLength);
    key[keyLength] = '\0';

    if (!has(key)) {
      ++local.unknownKeys;
      continue;
    }
    if (valueLength > kMaxStringLength) {
      ++local.invalidValues;
      continue;
    }
    memcpy(value, text + valueBegin, valueLength);
    value[valueLength] = '\0';

    if (isOk(setFromText(key, value)))
      ++local.applied;
    else
      ++local.invalidValues;
  }

  if (report != nullptr) *report = local;
  return (local.malformedLines == 0 && local.invalidValues == 0) ? Status::Ok : Status::Corrupt;
}

Status ConfigStore::load(IConfigBackend &backend, ConfigLoadReport *report) {
  size_t length = 0;
  DBMO_TRY(backend.load(gScratch, sizeof(gScratch), &length));
  if (length > sizeof(gScratch)) return Status::Corrupt;

  const Status status = deserialize(gScratch, length, report);
  // Carregado sem erro: memória e arquivo estão iguais. Com erro, fica "sujo" para que
  // o próximo save() regrave o arquivo já limpo.
  dirty_ = !isOk(status);
  return status;
}

Status ConfigStore::save(IConfigBackend &backend) {
  const Result<size_t> length = serialize(gScratch, sizeof(gScratch));
  if (!length.ok()) return length.status();
  DBMO_TRY(backend.save(gScratch, length.value()));
  dirty_ = false;
  return Status::Ok;
}

}  // namespace dbmo
