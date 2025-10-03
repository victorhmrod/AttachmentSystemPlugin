#include "Components/RingBufferSimpleComponent.h"

URingBufferSimpleComponent::URingBufferSimpleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CapacityBytes = static_cast<int32>(N);
}

bool URingBufferSimpleComponent::BufferPutByte(uint8 Byte)
{
	return Buffer.Put(static_cast<char>(Byte));
}

bool URingBufferSimpleComponent::BufferGetByte(uint8& OutByte)
{
	char C;
	if (!Buffer.Get(C)) return false;
	OutByte = static_cast<uint8>(C);
	return true;
}

bool URingBufferSimpleComponent::BufferIsEmpty() const
{
	return Buffer.IsEmpty();
}

bool URingBufferSimpleComponent::BufferIsFull() const
{
	return Buffer.IsFull();
}

int32 URingBufferSimpleComponent::BufferAvailableToRead() const
{
	return static_cast<int32>(Buffer.AvailableToRead());
}

int32 URingBufferSimpleComponent::BufferAvailableToWrite() const
{
	return static_cast<int32>(Buffer.AvailableToWrite());
}

void URingBufferSimpleComponent::BufferClear()
{
	// reset state by reinitializing
	Buffer = SimpleRingBuffer<N>();
}
