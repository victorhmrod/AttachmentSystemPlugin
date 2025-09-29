#include "Components/RingBufferFullComponent.h"

URingBufferFullComponent::URingBufferFullComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CapacityBytes = static_cast<int32>(N);
}

bool URingBufferFullComponent::BufferPutByte(uint8 Byte)
{
	return Buffer.Put(static_cast<char>(Byte));
}

bool URingBufferFullComponent::BufferGetByte(uint8& OutByte)
{
	char C;
	if (!Buffer.Get(C))
		return false;

	OutByte = static_cast<uint8>(C);
	return true;
}

bool URingBufferFullComponent::BufferIsEmpty() const
{
	return Buffer.IsEmpty();
}

bool URingBufferFullComponent::BufferIsFull() const
{
	return Buffer.IsFull();
}

int32 URingBufferFullComponent::BufferAvailableToRead() const
{
	return static_cast<int32>(Buffer.AvailableToRead());
}

int32 URingBufferFullComponent::BufferAvailableToWrite() const
{
	return static_cast<int32>(Buffer.AvailableToWrite());
}

void URingBufferFullComponent::BufferClear()
{
	// FullRingBuffer uses atomic indices, not copy-assignable.
	// Safe way to clear: drain until empty.
	char Dummy;
	while (Buffer.Get(Dummy))
	{
		// discard data
	}
}
