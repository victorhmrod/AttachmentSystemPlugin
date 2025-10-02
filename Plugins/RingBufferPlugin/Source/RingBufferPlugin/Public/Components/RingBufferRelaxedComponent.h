// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/RingBufferBaseComponent.h"
#include "CoreMinimal.h"
#include "ThirdParty/RelaxedRingBuffer.h"

#include "RingBufferRelaxedComponent.generated.h"

/**
 * Wrapper for RelaxedRingBuffer<N, char, uint32, FastRingMod<N,uint32>>.
 * - Single Producer / Single Consumer, lock-free.
 * - Uses fast modular arithmetic (FastRingMod).
 * - Exposed to Blueprints through URingBufferBaseComponent.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RINGBUFFERPLUGIN_API URingBufferRelaxedComponent
    : public URingBufferBaseComponent {
  GENERATED_BODY()

public:
  URingBufferRelaxedComponent();

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
  RelaxedRingBuffer<N, char, uint32, Lomont::FastRingMod<N, uint32>> Buffer;
};