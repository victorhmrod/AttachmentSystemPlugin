#pragma once

#include "CoreMinimal.h"
#include "Components/RingBufferBaseComponent.h"
#include "ThirdParty/GenericRingBuffer.h"

#include "RingBufferGenericComponent.generated.h"

/**
 * Wrapper for GenericRingBuffer<N, char, uint32>.
 * - General-purpose ring buffer with template-based size and type.
 * - Copy-assignable, so clearing is done by reinitialization.
 * - Exposed to Blueprints via URingBufferBaseComponent.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RINGBUFFERPLUGIN_API URingBufferGenericComponent
    : public URingBufferBaseComponent {
  GENERATED_BODY()

public:
  URingBufferGenericComponent();

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
  GenericRingBuffer<N, char, uint32> Buffer;
};
