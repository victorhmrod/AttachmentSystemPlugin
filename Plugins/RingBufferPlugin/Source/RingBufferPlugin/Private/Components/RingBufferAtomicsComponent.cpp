#include "Components/RingBufferAtomicsComponent.h"

URingBufferAtomicsComponent::URingBufferAtomicsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CapacityBytes = static_cast<int32>(N);
}

bool URingBufferAtomicsComponent::BufferPutByte(uint8 Byte)
{
	return Buffer.Put(static_cast<char>(Byte));
}

bool URingBufferAtomicsComponent::BufferGetByte(uint8& OutByte)
{
	char C;
	if (!Buffer.Get(C)) return false;
	OutByte = static_cast<uint8>(C);
	return true;
}

bool URingBufferAtomicsComponent::BufferIsEmpty() const
{
	return Buffer.IsEmpty();
}

bool URingBufferAtomicsComponent::BufferIsFull() const
{
	return Buffer.IsFull();
}

int32 URingBufferAtomicsComponent::BufferAvailableToRead() const
{
	return static_cast<int32>(Buffer.AvailableToRead());
}

int32 URingBufferAtomicsComponent::BufferAvailableToWrite() const
{
	return static_cast<int32>(Buffer.AvailableToWrite());
}

void URingBufferAtomicsComponent::BufferClear()
{
	char Dummy;
	while (Buffer.Get(Dummy)) {}
}
