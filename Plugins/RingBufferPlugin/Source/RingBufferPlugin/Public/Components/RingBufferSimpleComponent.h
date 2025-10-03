// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/RingBufferBaseComponent.h"
#include "ThirdParty/SimpleRingBuffer.h"
#include "RingBufferSimpleComponent.generated.h"

/**
 * Wrapper for third-party SimpleRingBuffer<N>.
 * Works byte-wise, higher-level Put/Get handled by URingBufferBaseComponent.
 */
UCLASS()
class RINGBUFFERPLUGIN_API URingBufferSimpleComponent
    : public URingBufferBaseComponent {
  GENERATED_BODY()
public:
  URingBufferSimpleComponent();

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
  static constexpr size_t N = 1024; // capacity
  SimpleRingBuffer<N> Buffer;
};
