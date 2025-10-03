#include "Components/RingBufferLockedComponent.h"

URingBufferLockedComponent::URingBufferLockedComponent() {
  PrimaryComponentTick.bCanEverTick = false;
  CapacityBytes = static_cast<int32>(N);
}

bool URingBufferLockedComponent::BufferPutByte(uint8 Byte) {
  return Buffer.Put(static_cast<char>(Byte));
}

bool URingBufferLockedComponent::BufferGetByte(uint8 &OutByte) {
  char C;
  if (!Buffer.Get(C))
    return false;

  OutByte = static_cast<uint8>(C);
  return true;
}

bool URingBufferLockedComponent::BufferIsEmpty() const {
  return Buffer.IsEmpty();
}

bool URingBufferLockedComponent::BufferIsFull() const {
  return Buffer.IsFull();
}

int32 URingBufferLockedComponent::BufferAvailableToRead() const {
  return static_cast<int32>(Buffer.AvailableToRead());
}

int32 URingBufferLockedComponent::BufferAvailableToWrite() const {
  return static_cast<int32>(Buffer.AvailableToWrite());
}

void URingBufferLockedComponent::BufferClear() {
  // Not copy-assignable (uses atomics) → drain until empty.
  char Dummy;
  while (Buffer.Get(Dummy)) {
    // discard data
  }
}
