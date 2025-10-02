#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Misc/AttachmentSystemTypes.h"
#include "WeaponBuilderComponent.generated.h"

class AWeapon;
class AAttachment;
class ARailAttachment;

/**
 * Component responsible for mounting/dismounting weapons using an attachment
 * graph. Each edge of the graph contains the parent's socket information.
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponBuilt,
                                            const TArray<AAttachment *> &,
                                            SpawnedAttachments);

UCLASS(meta = (BlueprintSpawnableComponent))
class ATTACHMENTSYSTEMPLUGIN_API UWeaponBuilderComponent
    : public UActorComponent {
  GENERATED_BODY()

public:
  /** Default constructor */
  UWeaponBuilderComponent();

protected:
  /** Called when the game starts or when the component is spawned.
   *  Initializes state and prepares the builder for use.
   */
  virtual void BeginPlay() override;

  /** Called when the component ends play (destroyed, level transition, etc.).
   *  Cleans up any runtime state (e.g., attachments, components).
   */
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
  /** Called every frame if ticking is enabled.
   *  Useful for per-frame updates related to attachments (rarely needed).
   *
   * @param DeltaTime   Time since last frame
   * @param TickType    Tick type (Game, Editor, etc.)
   * @param ThisTickFunction Tick function context
   */
  virtual void
  TickComponent(float DeltaTime, enum ELevelTick TickType,
                FActorComponentTickFunction *ThisTickFunction) override;

  /**
   * Builds (assembles) a weapon from the defined BaseAttachments list.
   *
   * - BlueprintCallable → can be triggered in runtime (e.g., respawn weapon).
   * - CallInEditor → can be tested directly from the editor.
   *
   * Spawns all defined attachments, attaches them to the Weapon,
   * and applies their behaviors/stats.
   */
  UFUNCTION(BlueprintCallable, CallInEditor, Category = "Weapon|Builder")
  void BuildWeapon();

  UFUNCTION(Server, Reliable)
  void Server_BuildWeapon();

  /**
   * Checks if an attachment placed at the given transform
   * would collide with the current rail configuration.
   *
   * @param TestTransform  Transform to test the child attachment at.
   * @param ChildMesh      Mesh of the attachment being tested.
   * @param IgnoredActor   Actor to ignore during collision (usually the
   * parent).
   * @return true if the placement causes a collision, false if it's valid.
   */
  bool DoesCollideWithRail(const FTransform &TestTransform,
                           USkeletalMeshComponent *ChildMesh,
                           AActor *IgnoredActor) const;

  /**
   * Disassembles the weapon by detaching and destroying all child attachments.
   *
   * - BlueprintCallable: Can be called at runtime (e.g., when player strips
   * weapon).
   * - CallInEditor: Also callable directly from the editor for quick testing.
   *
   * Useful for resetting a weapon back to its bare state.
   */
  UFUNCTION(BlueprintCallable, CallInEditor, Category = "Weapon|Builder")
  void ClearWeapon();

  UFUNCTION(Server, Reliable)
  void Server_ClearWeapon();

  /**
   * Searches for an attachment currently mounted on the weapon by its category.
   *
   * @param Category   The attachment category to look for (e.g., Optic, Stock).
   * @return           Pointer to the found attachment, or nullptr if none
   * exists.
   */
  UFUNCTION(BlueprintPure, Category = "Weapon|Builder")
  AAttachment *GetAttachmentAtSocket(EAttachmentCategory Category);

  /**
   * Resolves which socket name corresponds to a given attachment category.
   *
   * @param Category   The attachment category to map (e.g., Barrel →
   * "BarrelSocket").
   * @return           The socket name associated with that category.
   */
  FName GetSocketFromCategory(EAttachmentCategory Category) const;

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weapon|Builder")
  FORCEINLINE TArray<AAttachment *> GetSpawnedAttachments() {
    return SpawnedAttachments;
  }

  // Called whenever the weapon is (re)built and attachments are spawned
  UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
  FOnWeaponBuilt OnWeaponBuilt;

  /**
   * Adds extra runtime functionality depending on the attachment type.
   *
   * @param Attachment   The attachment instance being processed.
   *
   * Example:
   *  - Optic   → adds zoom/ADS component
   *  - Foregrip → adds recoil reduction component
   *  - Laser   → adds targeting/beam component
   */
  void AddBehaviorComponent(AAttachment *Attachment);

protected:
  /** The main weapon this builder is associated with (replicated). */
  UPROPERTY(Replicated)
  TObjectPtr<AWeapon> Weapon;

  /** Default set of base attachments (classes only).
   *  Used as the root configuration when assembling the weapon.
   */
  UPROPERTY(EditDefaultsOnly, Category = "Weapon|Parts")
  TArray<TSubclassOf<AAttachment>> BaseAttachments;

  /** Runtime instances of attachments spawned during build.
   *  Not saved, regenerated each time BuildWeapon runs.
   */
  UPROPERTY(Transient)
  TArray<AAttachment *> SpawnedAttachments;

  /** Maps spawned attachments to their corresponding scene components.
   *  Example: OpticAttachment → SkeletalMeshComponent instance.
   */
  UPROPERTY()
  TMap<AAttachment *, USceneComponent *> SpawnedMeshes;

  /** Maps spawned attachments to their active behavior components.
   *  Example: LaserAttachment → ULaserComponent instance.
   */
  UPROPERTY()
  TMap<AAttachment *, UActorComponent *> SpawnedBehaviors;

private:
  /**
   * Recursive traversal of the attachment graph.
   * Spawns and attaches children to the given parent.
   *
   * @param ParentAttachment  The current node being expanded.
   * @param Visited           Tracks visited nodes to avoid infinite loops.
   */
  void BuildWeaponFromAttachmentGraph(AAttachment *ParentAttachment,
                                      TSet<AAttachment *> &Visited);

  /**
   * Recursive search through the attachment graph for a target category.
   *
   * @param Attachment        Current node in the graph.
   * @param TargetCategory    Category we are searching for.
   * @param Visited           Tracks visited nodes to avoid loops.
   * @return                  The first matching attachment, or nullptr if none
   * found.
   */
  AAttachment *FindAttachmentRecursive(AAttachment *Attachment,
                                       EAttachmentCategory TargetCategory,
                                       TSet<AAttachment *> &Visited);

  /**
   * Recursive cleanup of the attachment graph.
   * Detaches and destroys all child attachments under the given node.
   *
   * @param Attachment  Current node being cleaned.
   * @param Visited     Tracks visited nodes to avoid double free or loops.
   */
  void ClearAttachmentRecursive(AAttachment *Attachment,
                                TSet<AAttachment *> &Visited);

  /**
   * Registers replicated properties with the Unreal networking system.
   * Ensures Weapon (and any relevant state) is synchronized across clients.
   */
  virtual void GetLifetimeReplicatedProps(
      TArray<class FLifetimeProperty> &OutLifetimeProps) const override;
};
