#include "Components/RingBufferBaseComponent.h"
#include <cstring> // memcpy

URingBufferBaseComponent::URingBufferBaseComponent() {
  PrimaryComponentTick.bCanEverTick = false;
}

void URingBufferBaseComponent::BeginPlay() { Super::BeginPlay(); }

// ---- Typed API ----
bool URingBufferBaseComponent::PutInt(int32 Value) { return PutPOD(Value); }
bool URingBufferBaseComponent::GetInt(int32 &OutVal) { return GetPOD(OutVal); }

bool URingBufferBaseComponent::PutFloat(float Value) { return PutPOD(Value); }
bool URingBufferBaseComponent::GetFloat(float &OutVal) {
  return GetPOD(OutVal);
}

bool URingBufferBaseComponent::PutByte(uint8 Value) {
  return BufferPutByte(Value);
}
bool URingBufferBaseComponent::GetByte(uint8 &OutVal) {
  return BufferGetByte(OutVal);
}

// ---- State API ----
bool URingBufferBaseComponent::IsEmpty() const { return BufferIsEmpty(); }
bool URingBufferBaseComponent::IsFull() const { return BufferIsFull(); }
int32 URingBufferBaseComponent::AvailableToReadBytes() const {
  return BufferAvailableToRead();
}
int32 URingBufferBaseComponent::AvailableToWriteBytes() const {
  return BufferAvailableToWrite();
}
void URingBufferBaseComponent::Clear() { BufferClear(); }

// ---- Helpers ----
template <typename T> bool URingBufferBaseComponent::PutPOD(const T &Value) {
  static_assert(std::is_trivially_copyable<T>::value,
                "PutPOD requires trivially copyable type.");
  const uint8 *Bytes = reinterpret_cast<const uint8 *>(&Value);
  for (size_t i = 0; i < sizeof(T); ++i) {
    if (!BufferPutByte(Bytes[i]))
      return false;
  }
  return true;
}

template <typename T> bool URingBufferBaseComponent::GetPOD(T &OutValue) {
  static_assert(std::is_trivially_copyable<T>::value,
                "GetPOD requires trivially copyable type.");
  uint8 Bytes[sizeof(T)];
  for (size_t i = 0; i < sizeof(T); ++i) {
    if (!BufferGetByte(Bytes[i]))
      return false;
  }
  std::memcpy(&OutValue, Bytes, sizeof(T));
  return true;
}