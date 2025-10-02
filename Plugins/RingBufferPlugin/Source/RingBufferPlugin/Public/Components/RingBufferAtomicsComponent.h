#pragma once

#include "CoreMinimal.h"
#include "RingBufferAtomicsComponent.generated.h"
#include "RingBufferBaseComponent.h"
#include "ThirdParty/AtomicsRingBuffer.h"

/**
 * Wrapper for third-party AtomicsRingBuffer<N, char, uint32>.
 * - Single Producer / Single Consumer.
 * - Lock-free, uses std::atomic.
 * - Works byte-wise, high-level Put/Get handled by URingBufferBaseComponent.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RINGBUFFERPLUGIN_API URingBufferAtomicsComponent
    : public URingBufferBaseComponent {
  GENERATED_BODY()

public:
  // Sets default values for this component's properties
  URingBufferAtomicsComponent();

protected:
  // Implementação da interface base
  virtual bool BufferPutByte(uint8 Byte) override;
  virtual bool BufferGetByte(uint8 &OutByte) override;
  virtual bool BufferIsEmpty() const override;
  virtual bool BufferIsFull() const override;
  virtual int32 BufferAvailableToRead() const override;
  virtual int32 BufferAvailableToWrite() const override;
  virtual void BufferClear() override;

private:
  static constexpr size_t N = 1024;
  AtomicsRingBuffer<N, char, uint32_t> Buffer;
};
