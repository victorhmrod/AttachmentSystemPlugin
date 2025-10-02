#include "Components/RingBufferGenericComponent.h"

URingBufferGenericComponent::URingBufferGenericComponent() {
  PrimaryComponentTick.bCanEverTick = false;
  CapacityBytes = static_cast<int32>(N);
}

bool URingBufferGenericComponent::BufferPutByte(uint8 Byte) {
  return Buffer.Put(static_cast<char>(Byte));
}

bool URingBufferGenericComponent::BufferGetByte(uint8 &OutByte) {
  char C;
  if (!Buffer.Get(C))
    return false;

  OutByte = static_cast<uint8>(C);
  return true;
}

bool URingBufferGenericComponent::BufferIsEmpty() const {
  return Buffer.IsEmpty();
}

bool URingBufferGenericComponent::BufferIsFull() const {
  return Buffer.IsFull();
}

int32 URingBufferGenericComponent::BufferAvailableToRead() const {
  return static_cast<int32>(Buffer.AvailableToRead());
}

int32 URingBufferGenericComponent::BufferAvailableToWrite() const {
  return static_cast<int32>(Buffer.AvailableToWrite());
}

void URingBufferGenericComponent::BufferClear() {
  // GenericRingBuffer is copy-assignable, safe to reset via reinitialization.
  Buffer = GenericRingBuffer<N, char, uint32>();
}
