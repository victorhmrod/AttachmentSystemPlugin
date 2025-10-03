#include "Components/RingBufferRelaxedComponent.h"

URingBufferRelaxedComponent::URingBufferRelaxedComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CapacityBytes = static_cast<int32>(N);
}

bool URingBufferRelaxedComponent::BufferPutByte(uint8 Byte)
{
	return Buffer.Put(static_cast<char>(Byte));
}

bool URingBufferRelaxedComponent::BufferGetByte(uint8& OutByte)
{
	char C;
	if (!Buffer.Get(C))
		return false;

	OutByte = static_cast<uint8>(C);
	return true;
}

bool URingBufferRelaxedComponent::BufferIsEmpty() const
{
	return Buffer.IsEmpty();
}

bool URingBufferRelaxedComponent::BufferIsFull() const
{
	return Buffer.IsFull();
}

int32 URingBufferRelaxedComponent::BufferAvailableToRead() const
{
	return static_cast<int32>(Buffer.AvailableToRead());
}

int32 URingBufferRelaxedComponent::BufferAvailableToWrite() const
{
	return static_cast<int32>(Buffer.AvailableToWrite());
}

void URingBufferRelaxedComponent::BufferClear()
{
	// RelaxedRingBuffer uses std::atomic internally,
	// so it is not copy-assignable. 
	// The only safe way to clear is to drain until empty.
	char Dummy;
	while (Buffer.Get(Dummy))
	{
		// discard data
	}
}
