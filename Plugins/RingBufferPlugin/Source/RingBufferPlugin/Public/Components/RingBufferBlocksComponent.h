#pragma once

#include "CoreMinimal.h"
#include "Components/RingBufferBaseComponent.h"
#include "ThirdParty/BlocksRingBuffer.h"

#include "RingBufferBlocksComponent.generated.h"

/**
 * Wrapper for BlocksRingBuffer<N, char, uint32>.
 * - Block-oriented lock-free ring buffer.
 * - Single Producer / Single Consumer.
 * - Exposed to Blueprints via URingBufferBaseComponent.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RINGBUFFERPLUGIN_API URingBufferBlocksComponent
    : public URingBufferBaseComponent {
  GENERATED_BODY()

public:
  URingBufferBlocksComponent();

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
  BlocksRingBuffer<N, char, uint32> Buffer;
};
