#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include <cstdint>

#include "RingBufferBaseComponent.generated.h"

/**
 * Base ring buffer component.
 * - Works with raw bytes internally.
 * - Exposes typed API for Blueprint: int32, float, byte.
 * - Subclasses must implement the byte-level ops (Put/Get/State).
 */
UCLASS(Abstract, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RINGBUFFERPLUGIN_API URingBufferBaseComponent : public UActorComponent {
  GENERATED_BODY()

public:
  URingBufferBaseComponent();

  /** Informational only when underlying buffer uses compile-time capacity. */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RingBuffer")
  int32 CapacityBytes = 1024;

  // -------- Typed API for Blueprint --------
  UFUNCTION(BlueprintCallable, Category = "RingBuffer|Typed")
  bool PutInt(int32 Value);

  UFUNCTION(BlueprintCallable, Category = "RingBuffer|Typed")
  bool GetInt(UPARAM(ref) int32 &OutValue);

  UFUNCTION(BlueprintCallable, Category = "RingBuffer|Typed")
  bool PutFloat(float Value);

  UFUNCTION(BlueprintCallable, Category = "RingBuffer|Typed")
  bool GetFloat(UPARAM(ref) float &OutValue);

  UFUNCTION(BlueprintCallable, Category = "RingBuffer|Typed")
  bool PutByte(uint8 Value);

  UFUNCTION(BlueprintCallable, Category = "RingBuffer|Typed")
  bool GetByte(UPARAM(ref) uint8 &OutValue);

  // -------- State API --------
  UFUNCTION(BlueprintPure, Category = "RingBuffer|State")
  bool IsEmpty() const;

  UFUNCTION(BlueprintPure, Category = "RingBuffer|State")
  bool IsFull() const;

  UFUNCTION(BlueprintPure, Category = "RingBuffer|State")
  int32 AvailableToReadBytes() const;

  UFUNCTION(BlueprintPure, Category = "RingBuffer|State")
  int32 AvailableToWriteBytes() const;

  UFUNCTION(BlueprintCallable, Category = "RingBuffer|State")
  void Clear();

protected:
  virtual void BeginPlay() override;

  // ---- To be implemented by subclasses ----
  virtual bool BufferPutByte(uint8 Byte) {
    checkNoEntry();
    return false;
  }
  virtual bool BufferGetByte(uint8 &OutByte) {
    checkNoEntry();
    return false;
  }
  virtual bool BufferIsEmpty() const {
    checkNoEntry();
    return true;
  }
  virtual bool BufferIsFull() const {
    checkNoEntry();
    return true;
  }
  virtual int32 BufferAvailableToRead() const {
    checkNoEntry();
    return 0;
  }
  virtual int32 BufferAvailableToWrite() const {
    checkNoEntry();
    return 0;
  }
  virtual void BufferClear() { checkNoEntry(); }

private:
  template <typename T> bool PutPOD(const T &Value);

  template <typename T> bool GetPOD(T &OutValue);
};
