# RingBufferPlugin

A collection of **Blueprint/C++ ready ring buffer components** for Unreal Engine.  
Each component wraps a different **third-party ring buffer implementation** with a unified API.

---

## Features
- 🔄 Multiple buffer strategies (lock-free, cache-friendly, mutex-based, etc.)
- 🎮 Fully usable in **Blueprints** and **C++**
- 📦 Consistent API across all implementations:
  - `PutInt`, `GetInt`
  - `PutFloat`, `GetFloat`
  - `PutByte`, `GetByte`
  - `IsEmpty`, `IsFull`
  - `AvailableToReadBytes`, `AvailableToWriteBytes`
  - `Clear`

---

## Installation
1. Copy the plugin into your project's `Plugins` folder:
   ```
   YourProject/Plugins/RingBufferPlugin/
   ```
2. Enable the plugin in **Edit → Plugins**.
3. Recompile your project.

---

## Components

| Component                        | Underlying Class                          | Clear Strategy |
|----------------------------------|-------------------------------------------|----------------|
| **URingBufferSimpleComponent**   | `SimpleRingBuffer<N>`                      | Reinit (`=`) |
| **URingBufferAtomicsComponent**  | `AtomicsRingBuffer<N, char, uint32>`       | Drain loop |
| **URingBufferRelaxedComponent**  | `RelaxedRingBuffer<N, char, uint32, FastRingMod>` | Drain loop |
| **URingBufferModulusComponent**  | `ModulusRingBuffer<N, char, uint32>`       | Drain loop |
| **URingBufferFullComponent**     | `FullRingBuffer<N, char, uint32>`          | Drain loop |
| **URingBufferCacheComponent**    | `CacheRingBuffer<N, char, uint32>`         | Drain loop |
| **URingBufferBlocksComponent**   | `BlocksRingBuffer<N, char, uint32>`        | Drain loop |
| **URingBufferGenericComponent**  | `GenericRingBuffer<N, char, uint32>`       | Drain loop |
| **URingBufferLockedComponent**   | `LockedRingBuffer<N, char, uint32>`        | Drain loop |
| **URingBufferCoreComponent**     | `Lomont::RingBuffer<N, char, uint32>`      | Drain loop |

---

## Usage in Blueprints
1. Add any RingBuffer component to an Actor.
2. Call the available functions:

- `PutInt`, `GetInt`
- `PutFloat`, `GetFloat`
- `PutByte`, `GetByte`
- `IsEmpty`, `IsFull`
- `AvailableToReadBytes`, `AvailableToWriteBytes`
- `Clear`

Example (Blueprint graph):
```
[BeginPlay] → [PutInt(42)]
             → [PutFloat(3.14)]
             → [PutByte(255)]
             → [AvailableToReadBytes]
```

---

## Usage in C++
```cpp
#include "Components/URingBufferSimpleComponent.h"

void AMyActor::BeginPlay()
{
    Super::BeginPlay();

    URingBufferSimpleComponent* Buffer = NewObject<URingBufferSimpleComponent>(this);
    Buffer->RegisterComponent();

    Buffer->PutInt(123);
    int32 Value;
    if (Buffer->GetInt(Value))
    {
        UE_LOG(LogTemp, Log, TEXT("Got value: %d"), Value);
    }
}
```

---

## Notes
- All components share the same interface (`URingBufferBaseComponent`).
- **Capacity** is currently fixed at `N = 1024` per component.  
  This can be parameterized later via templates or config if needed.
- For **Atomics/Relaxed/Modulus/Full/Cache/Blocks/Core/Locked/Generic**: `Clear()` drains the buffer.  
- For **Simple**: `Clear()` reinitializes via `=`.

---

## License
- Third-party ring buffer headers belong to their original authors.
- Unreal integration (components, base class, plugin structure) is MIT licensed.
