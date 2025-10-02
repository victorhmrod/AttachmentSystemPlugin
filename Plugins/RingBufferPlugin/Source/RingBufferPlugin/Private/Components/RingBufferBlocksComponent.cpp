#include "Components/RingBufferBlocksComponent.h"

URingBufferBlocksComponent::URingBufferBlocksComponent() {
  PrimaryComponentTick.bCanEverTick = false;
  CapacityBytes = static_cast<int32>(N);
}

bool URingBufferBlocksComponent::BufferPutByte(uint8 Byte) {
  return Buffer.Put(static_cast<char>(Byte));
}

bool URingBufferBlocksComponent::BufferGetByte(uint8 &OutByte) {
  char C;
  if (!Buffer.Get(C))
    return false;

  OutByte = static_cast<uint8>(C);
  return true;
}

bool URingBufferBlocksComponent::BufferIsEmpty() const {
  return Buffer.IsEmpty();
}

bool URingBufferBlocksComponent::BufferIsFull() const {
  return Buffer.IsFull();
}

int32 URingBufferBlocksComponent::BufferAvailableToRead() const {
  return static_cast<int32>(Buffer.AvailableToRead());
}

int32 URingBufferBlocksComponent::BufferAvailableToWrite() const {
  return static_cast<int32>(Buffer.AvailableToWrite());
}

void URingBufferBlocksComponent::BufferClear() {
  // BlocksRingBuffer is not copy-assignable (uses atomics).
  // Safe way to clear: drain until empty.
  char Dummy;
  while (Buffer.Get(Dummy)) {
    // discard data
  }
}