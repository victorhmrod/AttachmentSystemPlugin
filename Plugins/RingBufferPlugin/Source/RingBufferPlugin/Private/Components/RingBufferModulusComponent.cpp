#include "Components/RingBufferModulusComponent.h"

URingBufferModulusComponent::URingBufferModulusComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CapacityBytes = static_cast<int32>(N);
}

bool URingBufferModulusComponent::BufferPutByte(uint8 Byte)
{
	return Buffer.Put(static_cast<char>(Byte));
}

bool URingBufferModulusComponent::BufferGetByte(uint8& OutByte)
{
	char C;
	if (!Buffer.Get(C))
		return false;

	OutByte = static_cast<uint8>(C);
	return true;
}

bool URingBufferModulusComponent::BufferIsEmpty() const
{
	return Buffer.IsEmpty();
}

bool URingBufferModulusComponent::BufferIsFull() const
{
	return Buffer.IsFull();
}

int32 URingBufferModulusComponent::BufferAvailableToRead() const
{
	return static_cast<int32>(Buffer.AvailableToRead());
}

int32 URingBufferModulusComponent::BufferAvailableToWrite() const
{
	return static_cast<int32>(Buffer.AvailableToWrite());
}

void URingBufferModulusComponent::BufferClear()
{
	// ModulusRingBuffer uses std::atomic internally,
	// so it cannot be copy-assigned.
	// The only safe way to clear is to drain until empty.
	char Dummy;
	while (Buffer.Get(Dummy))
	{
		// discard data
	}
}
