#pragma once

#include "CoreMinimal.h"
#include "Components/RingBufferBaseComponent.h"
#include "ThirdParty/ModulusRingBuffer.h"
#include "ThirdParty/RingBuffer.h"

#include "RingBufferModulusComponent.generated.h"

/**
 * Wrapper for ModulusRingBuffer<N, char, uint32>.
 * - Lock-free, modular arithmetic for index management.
 * - Single Producer / Single Consumer.
 * - Exposed to Blueprints via URingBufferBaseComponent.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RINGBUFFERPLUGIN_API URingBufferModulusComponent : public URingBufferBaseComponent
{
	GENERATED_BODY()

public:
	URingBufferModulusComponent();

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
	ModulusRingBuffer<N, char, uint32> Buffer;
};
