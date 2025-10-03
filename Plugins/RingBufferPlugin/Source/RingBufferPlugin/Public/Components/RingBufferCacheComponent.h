#pragma once

#include "CoreMinimal.h"
#include "Components/RingBufferBaseComponent.h"
#include "ThirdParty/CacheRingBuffer.h"

#include "RingBufferCacheComponent.generated.h"

/**
 * Wrapper for CacheRingBuffer<N, char, uint32>.
 * - Optimized cache-friendly ring buffer.
 * - Single Producer / Single Consumer, lock-free.
 * - Exposed to Blueprints via URingBufferBaseComponent.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RINGBUFFERPLUGIN_API URingBufferCacheComponent
    : public URingBufferBaseComponent {
  GENERATED_BODY()

public:
  URingBufferCacheComponent();

protected:
  // URingBufferBaseComponent implementation
  virtual bool BufferPutByte(uint8 Byte) override;
  virtual bool BufferGetByte(uint8 &OutByte) override;
  virtual bool BufferIsEmpty() const override;
  virtual bool BufferIsFull() const override;
  virtual int32 BufferAvailableToRead() const override;
  virtual int32 BufferAvailableToWrite() const override;
  virtual void BufferClear() override;

private:
  static constexpr size_t N = 1024;
  CacheRingBuffer<N, char, uint32> Buffer;
};
