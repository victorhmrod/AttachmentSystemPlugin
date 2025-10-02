#include "Components/RingBufferCoreComponent.h"

URingBufferCoreComponent::URingBufferCoreComponent() {
  PrimaryComponentTick.bCanEverTick = false;
  CapacityBytes = static_cast<int32>(N);
}

bool URingBufferCoreComponent::BufferPutByte(uint8 Byte) {
  return Buffer.Put(static_cast<char>(Byte));
}

bool URingBufferCoreComponent::BufferGetByte(uint8 &OutByte) {
  char C;
  if (!Buffer.Get(C))
    return false;
  OutByte = static_cast<uint8>(C);
  return true;
}

bool URingBufferCoreComponent::BufferIsEmpty() const {
  return Buffer.IsEmpty();
}

bool URingBufferCoreComponent::BufferIsFull() const { return Buffer.IsFull(); }

int32 URingBufferCoreComponent::BufferAvailableToRead() const {
  return static_cast<int32>(Buffer.AvailableToRead());
}

int32 URingBufferCoreComponent::BufferAvailableToWrite() const {
  return static_cast<int32>(Buffer.AvailableToWrite());
}

void URingBufferCoreComponent::BufferClear() {
  // Not copy-assignable (uses atomics) → drain until empty.
  char Dummy;
  while (Buffer.Get(Dummy)) {
    // discard data
  }
}