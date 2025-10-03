#pragma once

#include "CoreMinimal.h"
#include "Components/RingBufferBaseComponent.h"
#include "ThirdParty/RingBuffer.h"

#include "RingBufferCoreComponent.generated.h"

/**
 * Wrapper for Lomont::RingBuffer<N, char, uint32>.
 * - SPSC, lock-free (uses atomics).
 * - Not copy-assignable.
 * - Exposed to Blueprints via URingBufferBaseComponent.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RINGBUFFERPLUGIN_API URingBufferCoreComponent
    : public URingBufferBaseComponent {
  GENERATED_BODY()

public:
  URingBufferCoreComponent();

protected:
  // URingBufferBaseComponent impl
  virtual bool BufferPutByte(uint8 Byte) override;
  virtual bool BufferGetByte(uint8 &OutByte) override;
  virtual bool BufferIsEmpty() const override;
  virtual bool BufferIsFull() const override;
  virtual int32 BufferAvailableToRead() const override;
  virtual int32 BufferAvailableToWrite() const override;
  virtual void BufferClear() override;

private:
  static constexpr size_t N = 1024;
  Lomont::RingBuffer<N, char, uint32> Buffer;
};
