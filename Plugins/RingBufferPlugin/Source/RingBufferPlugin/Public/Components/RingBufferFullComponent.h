#pragma once

#include "CoreMinimal.h"
#include "Components/RingBufferBaseComponent.h"
#include "ThirdParty/FullRingBuffer.h"

#include "RingBufferFullComponent.generated.h"

/**
 * Wrapper for FullRingBuffer<N, char, uint32>.
 * - Lock-free ring buffer with full capacity usage.
 * - Uses atomic indices for thread safety.
 * - Exposed to Blueprints via URingBufferBaseComponent.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RINGBUFFERPLUGIN_API URingBufferFullComponent : public URingBufferBaseComponent
{
	GENERATED_BODY()

public:
	URingBufferFullComponent();

protected:
	// URingBufferBaseComponent implementation
	virtual bool  BufferPutByte(uint8 Byte) override;
	virtual bool  BufferGetByte(uint8& OutByte) override;
	virtual bool  BufferIsEmpty() const override;
	virtual bool  BufferIsFull() const override;
	virtual int32 BufferAvailableToRead() const override;
	virtual int32 BufferAvailableToWrite() const override;
	virtual void  BufferClear() override;

private:
	static constexpr size_t N = 1024;
	FullRingBuffer<N, char, uint32> Buffer;
};
