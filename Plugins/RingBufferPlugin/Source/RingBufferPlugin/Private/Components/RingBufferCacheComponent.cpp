#include "Components/RingBufferCacheComponent.h"

URingBufferCacheComponent::URingBufferCacheComponent() {
  PrimaryComponentTick.bCanEverTick = false;
  CapacityBytes = static_cast<int32>(N);
}

bool URingBufferCacheComponent::BufferPutByte(uint8 Byte) {
  return Buffer.Put(static_cast<char>(Byte));
}

bool URingBufferCacheComponent::BufferGetByte(uint8 &OutByte) {
  char C;
  if (!Buffer.Get(C))
    return false;

  OutByte = static_cast<uint8>(C);
  return true;
}

bool URingBufferCacheComponent::BufferIsEmpty() const {
  return Buffer.IsEmpty();
}

bool URingBufferCacheComponent::BufferIsFull() const { return Buffer.IsFull(); }

int32 URingBufferCacheComponent::BufferAvailableToRead() const {
  return static_cast<int32>(Buffer.AvailableToRead());
}

int32 URingBufferCacheComponent::BufferAvailableToWrite() const {
  return static_cast<int32>(Buffer.AvailableToWrite());
}

void URingBufferCacheComponent::BufferClear() {
  // CacheRingBuffer is not copy-assignable (uses atomics).
  // Safe way to clear: drain until empty.
  char Dummy;
  while (Buffer.Get(Dummy)) {
    // discard data
  }
}
