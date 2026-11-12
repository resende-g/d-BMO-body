#include "dbmo/core/resource_registry.h"

#include <cstring>

namespace dbmo {

namespace {
bool sameOwner(const char *a, const char *b) { return a == b || strcmp(a, b) == 0; }
}  // namespace

int ResourceRegistry::find(ResourceKind kind, uint16_t id) const {
  for (size_t i = 0; i < count_; ++i)
    if (claims_[i].kind == kind && claims_[i].id == id) return static_cast<int>(i);
  return -1;
}

Status ResourceRegistry::claim(ResourceKind kind, uint16_t id, const char *owner) {
  if (owner == nullptr || *owner == '\0') return Status::InvalidArgument;
  const int index = find(kind, id);
  if (index >= 0) return sameOwner(claims_[index].owner, owner) ? Status::Ok : Status::Busy;
  if (count_ == kMaxClaims) return Status::NoSpace;
  claims_[count_++] = Claim{kind, id, owner};
  return Status::Ok;
}

Status ResourceRegistry::release(ResourceKind kind, uint16_t id, const char *owner) {
  if (owner == nullptr) return Status::InvalidArgument;
  const int index = find(kind, id);
  if (index < 0) return Status::NotFound;
  if (!sameOwner(claims_[index].owner, owner)) return Status::NotPermitted;
  claims_[index] = claims_[--count_];
  return Status::Ok;
}

size_t ResourceRegistry::releaseAll(const char *owner) {
  if (owner == nullptr) return 0;
  size_t released = 0;
  for (size_t i = 0; i < count_;) {
    if (sameOwner(claims_[i].owner, owner)) {
      claims_[i] = claims_[--count_];
      ++released;
    } else {
      ++i;
    }
  }
  return released;
}

const char *ResourceRegistry::ownerOf(ResourceKind kind, uint16_t id) const {
  const int index = find(kind, id);
  return index < 0 ? nullptr : claims_[index].owner;
}

}  // namespace dbmo
