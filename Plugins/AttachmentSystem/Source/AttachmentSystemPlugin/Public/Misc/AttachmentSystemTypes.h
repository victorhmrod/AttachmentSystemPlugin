#pragma once

#include "AttachmentSystemTypes.generated.h"
#include "CoreMinimal.h"

class AAttachment;

// Clean UE log category for this class
DECLARE_LOG_CATEGORY_EXTERN(LogAttachmentSystem, Log, All);

/**
 * @brief Defines how weapon durability should be calculated.
 */
UENUM(BlueprintType)
enum class EWeaponDurabilityMode : uint8 {
  Average UMETA(DisplayName = "Average"), // Average durability
  Minimum UMETA(DisplayName =
                    "Minimum"), // The lowest durability (weakest part)
  Maximum UMETA(DisplayName =
                    "Maximum") // The greatest durability (strongest part)
};

/**
 * @brief Defines atomic categories for all major firearm components and
 * attachments.
 */
UENUM(BlueprintType, Category = "Attachments")
enum class EAttachmentCategory : uint8 {
  /*
   * Core Firearm Components
   * These are the primary structural components of a rifle.
   */
  // A single-piece receiver that integrates the upper receiver and handguard.
  // Some guns like UZIs/MP7s use this part instead of upper/lower receivers.
  MonolithicReceiver UMETA(DisplayName = "Monolithic Receiver"),
  // The core component of a rifle (e.g., AR-15) that houses the bolt carrier
  // group and barrel assembly.
  UpperReceiver UMETA(DisplayName = "Upper Receiver"),
  // The component that holds the fire control group, magazine well, and pistol
  // grip.
  LowerReceiver UMETA(DisplayName = "Lower Receiver"),

  /*
   * Pistol Core Components
   * These are the fundamental structural components of a handgun.
   */
  // The reciprocating part of a semi-automatic pistol that houses the barrel,
  // firing pin, and extractor.
  PistolSlide UMETA(DisplayName = "Pistol Slide"),
  // The main body of a pistol that holds the fire control group, grip, and
  // magazine well.
  PistolFrame UMETA(DisplayName = "Pistol Frame"),

  /*
   * Sights and Optics
   * Devices used to assist in aiming the firearm.
   */
  // Any magnified or non-magnified sight that uses a lens or electronic display
  // for aiming. Examples include red dots, holographic sights, and scopes.
  Optic UMETA(DisplayName = "Optic"),
  // Simple, non-optical sights consisting of a front sight post and a rear
  // sight notch or aperture.
  IronSights UMETA(DisplayName = "Iron Sights"),

  /*
   * Barrels and Muzzle Devices
   * Components related to the projectile's path and the end of the barrel.
   */
  // The tube through which the projectile travels. The length and profile
  // affect accuracy, velocity, and handling.
  Barrel UMETA(DisplayName = "Barrel"),
  // An attachment mounted to the end of the barrel to manage recoil, flash, or
  // both. Examples include flash hiders, muzzle brakes, and compensators.
  MuzzleDevice UMETA(DisplayName = "Muzzle Device"),
  // A device attached to the muzzle to reduce the sound and flash of a gunshot.
  Suppressor UMETA(DisplayName = "Suppressor"),
  // A component on gas-operated firearms that siphons gas from the barrel to
  // cycle the action.
  GasBlock UMETA(DisplayName = "Gas Block"),

  /*
   * Underbarrel and Forward Attachments
   * Accessories mounted to the underside or front of the firearm.
   */
  // A general category for attachments that mount below the barrel, such as
  // grenade launchers or shotgun attachments.
  Underbarrel UMETA(DisplayName = "Underbarrel"),
  // A grip mounted to the underside of the handguard to improve handling,
  // stability, and recoil control.
  Foregrip UMETA(DisplayName = "Foregrip"),
  // A general category for devices that provide a tactical advantage, such as a
  // combination laser/flashlight unit.
  TacticalDevice UMETA(DisplayName = "Tactical Device"),
  // A device that projects a laser beam for quick target acquisition.
  Laser UMETA(DisplayName = "Laser"),
  // A light source mounted to the firearm for use in low-light conditions.
  Flashlight UMETA(DisplayName = "Flashlight"),

  /*
   * Internal and Operational Components
   * Parts that control the internal mechanics and operation of the firearm.
   */
  // The lever that is pulled to initiate the firing sequence. Aftermarket
  // triggers can improve pull weight and feel.
  Trigger UMETA(DisplayName = "Trigger"),
  // The assembly of internal parts (trigger, hammer, sear) that controls the
  // firing of the weapon.
  FiringControlGroup UMETA(DisplayName = "Firing Control Group"),
  // The spring-and-rod system that returns the bolt or slide to the forward
  // position after firing.
  ActionReturnSpringAssembly UMETA(DisplayName =
                                       "Action Return Spring Assembly"),
  // The tube on AR-style rifles that houses the action return spring and
  // buffer, connecting the stock to the lower receiver.
  BufferTube UMETA(DisplayName = "Buffer Tube"),
  // A component used to manually pull the bolt back to chamber a round or clear
  // a malfunction.
  ChargingHandle UMETA(DisplayName = "Charging Handle"),
  // The main moving part of a rifle that contains the bolt, firing pin, and
  // extractor. It cycles new rounds and ejects spent casings.
  BoltCarrierGroup UMETA(DisplayName = "Bolt Carrier Group"),

  /*
   * Stocks and Grips
   * Components that affect the ergonomics and handling of the firearm.
   */
  // The component that rests on the shooter's shoulder, providing stability and
  // a point of contact for aiming.
  Stock UMETA(DisplayName = "Stock"),
  // The grip used to hold the firearm with the dominant hand, housing the fire
  // control group.
  HandGrip UMETA(DisplayName = "Hand Grip"),
  // A small, modular insert for pistol grips that can alter the ergonomics and
  // storage capabilities.
  PistolGripInsert UMETA(DisplayName = "Pistol Grip Insert"),
  // A soft pad at the end of the stock that absorbs some of the weapon's
  // recoil.
  RecoilPad UMETA(DisplayName = "Recoil Pad"),

  /*
   * Magazine
   * The component that holds and feeds ammunition.
   */
  // A detachable container that holds and feeds ammunition into the firearm.
  Magazine UMETA(DisplayName = "Magazine"),

  /*
   * Rails and Mounting
   * Components used to attach other accessories to the firearm.
   */
  // A component with a standardized interface (e.g., Picatinny, KeyMod, M-LOK)
  // used to attach accessories.
  Rail UMETA(DisplayName = "Rail"),
  // A protective panel that snaps onto a rail to improve ergonomics and protect
  // the shooter's hands from sharp edges.
  RailCover UMETA(DisplayName = "Rail Cover"),
  // A general term for a component used to attach an accessory to a rail or
  // another part of the firearm.
  Mount UMETA(DisplayName = "Mount"),
  // A specialized mount used to attach an optic to the receiver or rail.
  OpticMount UMETA(DisplayName = "Optic Mount"),

  /*
   * Small Parts
   * Minor, often cosmetic or functional, components.
   */
  // A hinged cover on the ejection port that protects the internals from debris
  // when the weapon is not in use.
  EjectionPortCover UMETA(DisplayName = "Ejection Port Cover"),
  // A small, cosmetic accessory that hangs from the firearm, similar to a
  // keychain.
  Charm UMETA(DisplayName = "Charm")
};

/**
 * @brief Defines the method used to modify a weapon stat.
 * This is intended to be used by attachments and other modifiers.
 */

UENUM(BlueprintType)
enum class EStatModType : uint8 {
  /** Adds or subtracts a flat value from the stat (e.g., +10 Ergonomics). */
  SMT_Flat UMETA(DisplayName = "Flat Additive"),

  /** Multiplies the stat by a percentage (e.g., -15% Recoil, which is a x0.85
     multiplier). */
  SMT_Percentage UMETA(DisplayName = "Percentage Multiplier"),

  /** Completely replaces the base stat with a new value. */
  SMT_Override UMETA(DisplayName = "Override"),

  SMT_MAX UMETA(DisplayName = "MAX_NONE")
};

/**
 * @brief Defines the different types of weapon stats
 */
UENUM(BlueprintType)
enum class EWeaponStat : uint8 {
  /* =============================
   * Recoil Control
   * ============================= */
  /** Vertical kick when firing. Lower = easier control. */
  EWS_VerticalRecoil UMETA(DisplayName = "Vertical Recoil"),

  /** Horizontal sway when firing. Lower = more stable aim. */
  EWS_HorizontalRecoil UMETA(DisplayName = "Horizontal Recoil"),

  /** Camera shake effect applied on shot. */
  EWS_CameraRecoil UMETA(DisplayName = "Camera Recoil"),

  /* =============================
   * Handling / Ergonomics
   * ============================= */
  /** General weapon handling, ADS speed, equip speed, etc. */
  EWS_Ergonomics UMETA(DisplayName = "Ergonomics"),

  /** Weapon weight affecting handling, stamina, and sway. */
  EWS_Weight UMETA(DisplayName = "Weight"),

  /* =============================
   * Ballistics / Performance
   * ============================= */
  /** Precision of shots relative to crosshair. */
  EWS_Accuracy UMETA(DisplayName = "Accuracy"),

  /** Bullet velocity (m/s). Higher = less drop and faster hit. */
  EWS_MuzzleVelocity UMETA(DisplayName = "Muzzle Velocity"),

  /** Effective range before damage dropoff. */
  EWS_Range UMETA(DisplayName = "Range"),

  /** Rate of fire (Rounds Per Minute). */
  EWS_RoundsPerMinute UMETA(DisplayName = "Rounds Per Minute"),

  /* =============================
   * Advanced Spread Behavior
   * ============================= */
  /** How quickly bullets align after continuous fire. */
  EWS_Convergence UMETA(DisplayName = "Convergence"),

  /** Random spread / inaccuracy added to shots. */
  EWS_Dispersion UMETA(DisplayName = "Dispersion"),

  /* =============================
   * Utility
   * ============================= */
  /** Placeholder / invalid value. */
  EWS_MAX UMETA(DisplayName = "MAX_NONE")
};

USTRUCT(BlueprintType)
struct FStatModifier {
  GENERATED_BODY()

  /* =============================
   * Target Stat
   * ============================= */
  /** Which weapon stat this modifier affects (e.g., Vertical Recoil, Accuracy).
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
  EWeaponStat StatToModify;

  /* =============================
   * Modification Type
   * ============================= */
  /** How the value is applied:
   *  - Flat        → Adds/subtracts a constant (e.g., +10 Ergonomics).
   *  - Percentage  → Multiplies base value (e.g., -15% Recoil).
   *  - Override    → Replaces base value completely.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
  EStatModType ModificationType;

  /* =============================
   * Value
   * ============================= */
  /** Numerical strength of the modifier.
   *  - Flat: exact units (e.g., +5 Ergonomics).
   *  - Percentage: expressed as fraction (e.g., 0.85 = -15%).
   *  - Override: absolute value to force.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
  float Value = 0.f;
};

USTRUCT(BlueprintType)
struct FAttachmentLink {
  GENERATED_BODY()

  /* =============================
   * Blueprint Setup
   * ============================= */

  /** Classes of attachments that can be spawned as children.
   *  Defined in DataTables or per-weapon setup.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
  TArray<TSubclassOf<AAttachment>> ChildClasses;

  /** Local transform offset relative to the parent socket.
   *  Used for precise alignment when attaching.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
  FTransform Offset = FTransform::Identity;

  /** Desired starting slot index when mounted on a rail parent.
   *  Example: if StartSlot = 2, this attachment begins at the 3rd bump.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail")
  int32 StartSlot = 0;

  /* =============================
   * Runtime State
   * ============================= */

  /** Live attachment instances spawned from ChildClasses.
   *  Populated/managed at runtime (not serialized).
   */
  UPROPERTY(Transient)
  TArray<AAttachment *> ChildInstances;
};

USTRUCT(BlueprintType, Category = "Attachments")
struct FAttachmentInfo : public FTableRowBase {
  GENERATED_BODY()

  /* =============================
   * Display
   * ============================= */

  /** Short internal name / ID of the attachment.
   *  - Used in tooltips, logs, or debug.
   */
  UPROPERTY(EditDefaultsOnly, BlueprintReadWrite,
            Category = "Attachment|Display")
  FName Display_Name;

  /** Full localized description shown in UI/tooltip. */
  UPROPERTY(EditDefaultsOnly, BlueprintReadWrite,
            Category = "Attachment|Display")
  FText Display_Description;

  /** Skeletal mesh to represent this attachment in-game. */
  UPROPERTY(EditDefaultsOnly, BlueprintReadWrite,
            Category = "Attachment|Display")
  TSoftObjectPtr<USkeletalMesh> Mesh;

  /* =============================
   * Classification
   * ============================= */

  /** High-level category (e.g., Optic, Barrel, Stock, etc.). */
  UPROPERTY(EditDefaultsOnly, BlueprintReadWrite,
            Category = "Attachment|Classification")
  EAttachmentCategory Category;

  /* =============================
   * Stats
   * ============================= */

  /** Stat modifiers applied to the weapon when attached. */
  UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attachment|Stats")
  TArray<FStatModifier> Modifiers;

  /* =============================
   * Rail Configuration
   * ============================= */

  /** If true → validates against rail rules (UI, spline, bitmask).
   *  If false → attaches via simple socket/collision logic.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment|Rail")
  bool bUseRail = false;

  /** Number of rail slots this attachment occupies. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment|Rail")
  int32 Size = 1;

  /** Starting slot index when mounted on a rail (0 = leftmost). */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment|Rail")
  int32 StartSlot = 0;

  /* =============================
   * Durability
   * ============================= */

  /** Base durability of this attachment.
   *  - Static value defined in DataTable.
   *  - Copied to CurrentState at runtime.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment|Stats")
  float Durability = 100.f;
};

USTRUCT(BlueprintType, Category = "Attachments")
struct FAttachmentCurrentState {
  GENERATED_BODY()

  /** Current durability at runtime.
   *  - Starts from FAttachmentInfo::Durability.
   *  - Decreases with use / damage.
   *  - Can be repaired or modified in-game.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
  float Durability = 100.f;
};

USTRUCT(BlueprintType, Category = "Attachments")
struct FWeaponInfo : public FTableRowBase {
  GENERATED_BODY()

  /* =============================
   * Config (static defaults)
   * ============================= */

  /** Default durability calculation mode for this weapon. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Stats")
  EWeaponDurabilityMode DefaultDurabilityMode{EWeaponDurabilityMode::Average};

  /** Attachments linked by default (ex: spawn with optic, stock, etc.). */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Attachments")
  TArray<TSubclassOf<AAttachment>> LinkedAttachments;
};

USTRUCT(BlueprintType, Category = "Attachments")
struct FWeaponCurrentState {
  GENERATED_BODY()

  /* =============================
   * Runtime values
   * ============================= */

  /** Current durability at runtime.
   *  - Starts from base or recalculated from attachments.
   *  - Decreases with use/damage.
   *  - Can be repaired or reset.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Stats")
  float Durability = 100.f;

  /** Live attachment instances currently spawned on the weapon. */
  UPROPERTY(EditAnywhere, Transient, BlueprintReadWrite,
            Category = "Weapon|Attachments")
  TArray<AAttachment *> ActiveAttachments;

  /** Live attachment instances currently spawned on the weapon. */
  UPROPERTY(EditAnywhere, Transient, BlueprintReadWrite,
            Category = "Weapon|Attachments")
  TArray<USkeletalMeshComponent *> ActiveAttachmentMeshes;
};

/**
 * @brief Defines ammo types.
 */
UENUM(BlueprintType)
enum class EBulletType : uint8 {
  /** Invalid / no bullet present. */
  None UMETA(DisplayName = "None"),

  /** M43/M67 Standard Full Metal Jacket (Ball). Default round with balanced
     stats. */
  Standard_FMJ UMETA(DisplayName = "Standard FMJ (Ball)"),

  /** Armor-Piercing (e.g., Russian BP). High penetration, moderate damage.
     (Black Tip) */
  ArmorPiercing UMETA(DisplayName = "Armor Piercing (Black Tip)"),

  /** Soft Point or Hollow Point (HP/SP). High damage to flesh, low penetration.
   */
  HollowPoint_SP UMETA(DisplayName = "Hollow/Soft Point"),

  /** Tracer Round (e.g., Russian T45). Leaves a visible trail, lower damage.
     (Green Tip) */
  Tracer UMETA(DisplayName = "Tracer (Green Tip)"),

  /** Subsonic Round. Used with suppressors. Very low velocity/damage, no sound
     warning. */
  Subsonic UMETA(DisplayName = "Subsonic"),

  /** High-Powered Jacketed Soft Point (JSP). */
  Hunting_JSP UMETA(DisplayName = "Jacketed Soft Point")
};

/**
 * Defines magazine types and their specific capacities.
 */
UENUM(BlueprintType)
enum class EMagazineType : uint8 {
  // ---- 7.62x39mm ----
  // --- Polymer and Steel-Reinforced Magazines (30-Round Variants) ---
  // Magpul polymer magazines
  Magpul_MOE_30 UMETA(DisplayName = "Magpul PMAG MOE (30)"),
  Magpul_M3_30_Reinforced UMETA(DisplayName =
                                    "Magpul PMAG M3 (30, Steel-Reinforced)"),
  // Arsenal and XTech polymer magazines
  Arsenal_Waffle_30 UMETA(DisplayName = "Arsenal Circle 10 (30, Waffle)"),
  XTech_MAG47_30 UMETA(DisplayName = "XTech MAG47 (30)"),
  USPalm_AK30R_30 UMETA(DisplayName = "US Palm AK30R (30)"),
  // ProMag standard polymer magazine
  ProMag_Standard_30 UMETA(DisplayName = "ProMag (30)"),

  // --- Reduced Capacity Magazines ---
  // Reduced capacity Magpul MOE
  Magpul_MOE_10 UMETA(DisplayName = "Magpul PMAG MOE (10)"),
  Magpul_MOE_20 UMETA(DisplayName = "Magpul PMAG MOE (20)"),
  // Reduced capacity ProMag
  ProMag_Standard_20 UMETA(DisplayName = "ProMag (20)"),

  // --- High Capacity Box Magazines (Standard Box & Quad-Stack) ---
  // 40-round box magazines
  Arsenal_Waffle_40 UMETA(DisplayName = "Arsenal Circle 10 (40, Waffle)"),
  ProMag_Standard_40 UMETA(DisplayName = "ProMag (40)"),
  // 60-round quad-stack
  ATI_Schmeisser_S60 UMETA(DisplayName = "ATI Schmeisser S60 (60)"),

  // --- Drum Magazines ---
  // Drum magazines of various capacities
  ProMag_Drum_50 UMETA(DisplayName = "ProMag Drum (50)"),
  KCI_Drum_75 UMETA(DisplayName = "KCI Drum (75)"),
  Chinese_Drum_75 UMETA(DisplayName = "Chinese-Pattern Drum (75)"),

  // --- Steel Surplus Magazines ---
  // Classic steel box magazine variants
  Surplus_Steel_30 UMETA(DisplayName = "Steel Surplus (30)"),
  Surplus_Steel_40_RPK UMETA(DisplayName = "Steel Surplus (40, RPK-Style)")
};