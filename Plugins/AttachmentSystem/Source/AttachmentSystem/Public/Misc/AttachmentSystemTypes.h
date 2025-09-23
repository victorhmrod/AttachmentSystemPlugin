#pragma once

#include "CoreMinimal.h"
#include "AttachmentSystemTypes.generated.h"

class AAttachment;
/**
 * @brief Defines atomic categories for all major firearm components and attachments.
 */
UENUM(BlueprintType, Category = "Attachments")
enum class EAttachmentCategory : uint8
{
    /*
     * Core Firearm Components
     * These are the primary structural components of a rifle.
     */
    // A single-piece receiver that integrates the upper receiver and handguard. Some guns like UZIs/MP7s use this part instead of upper/lower receivers.
    MonolithicReceiver     UMETA(DisplayName = "Monolithic Receiver"),
    // The core component of a rifle (e.g., AR-15) that houses the bolt carrier group and barrel assembly.
    UpperReceiver          UMETA(DisplayName = "Upper Receiver"),
    // The component that holds the fire control group, magazine well, and pistol grip.
    LowerReceiver          UMETA(DisplayName = "Lower Receiver"),

    /*
     * Pistol Core Components
     * These are the fundamental structural components of a handgun.
     */
    // The reciprocating part of a semi-automatic pistol that houses the barrel, firing pin, and extractor.
    PistolSlide            UMETA(DisplayName = "Pistol Slide"),
    // The main body of a pistol that holds the fire control group, grip, and magazine well.
    PistolFrame            UMETA(DisplayName = "Pistol Frame"),

    /*
     * Sights and Optics
     * Devices used to assist in aiming the firearm.
     */
    // Any magnified or non-magnified sight that uses a lens or electronic display for aiming. Examples include red dots, holographic sights, and scopes.
    Optic                  UMETA(DisplayName = "Optic"),
    // Simple, non-optical sights consisting of a front sight post and a rear sight notch or aperture.
    IronSights             UMETA(DisplayName = "Iron Sights"),

    /*
     * Barrels and Muzzle Devices
     * Components related to the projectile's path and the end of the barrel.
     */
    // The tube through which the projectile travels. The length and profile affect accuracy, velocity, and handling.
    Barrel                 UMETA(DisplayName = "Barrel"),
    // An attachment mounted to the end of the barrel to manage recoil, flash, or both. Examples include flash hiders, muzzle brakes, and compensators.
    MuzzleDevice           UMETA(DisplayName = "Muzzle Device"),
    // A device attached to the muzzle to reduce the sound and flash of a gunshot.
    Suppressor             UMETA(DisplayName = "Suppressor"),
    // A component on gas-operated firearms that siphons gas from the barrel to cycle the action.
    GasBlock               UMETA(DisplayName = "Gas Block"),

    /*
     * Underbarrel and Forward Attachments
     * Accessories mounted to the underside or front of the firearm.
     */
    // A general category for attachments that mount below the barrel, such as grenade launchers or shotgun attachments.
    Underbarrel            UMETA(DisplayName = "Underbarrel"),
    // A grip mounted to the underside of the handguard to improve handling, stability, and recoil control.
    Foregrip               UMETA(DisplayName = "Foregrip"),
    // A general category for devices that provide a tactical advantage, such as a combination laser/flashlight unit.
    TacticalDevice         UMETA(DisplayName = "Tactical Device"),
    // A device that projects a laser beam for quick target acquisition.
    Laser                  UMETA(DisplayName = "Laser"),
    // A light source mounted to the firearm for use in low-light conditions.
    Flashlight             UMETA(DisplayName = "Flashlight"),

    /*
     * Internal and Operational Components
     * Parts that control the internal mechanics and operation of the firearm.
     */
    // The lever that is pulled to initiate the firing sequence. Aftermarket triggers can improve pull weight and feel.
    Trigger                UMETA(DisplayName = "Trigger"),
    // The assembly of internal parts (trigger, hammer, sear) that controls the firing of the weapon.
    FiringControlGroup     UMETA(DisplayName = "Firing Control Group"),
    // The spring-and-rod system that returns the bolt or slide to the forward position after firing.
    ActionReturnSpringAssembly UMETA(DisplayName = "Action Return Spring Assembly"),
    // The tube on AR-style rifles that houses the action return spring and buffer, connecting the stock to the lower receiver.
    BufferTube             UMETA(DisplayName = "Buffer Tube"),
    // A component used to manually pull the bolt back to chamber a round or clear a malfunction.
    ChargingHandle         UMETA(DisplayName = "Charging Handle"),
    // The main moving part of a rifle that contains the bolt, firing pin, and extractor. It cycles new rounds and ejects spent casings.
    BoltCarrierGroup       UMETA(DisplayName = "Bolt Carrier Group"),

    /*
     * Stocks and Grips
     * Components that affect the ergonomics and handling of the firearm.
     */
    // The component that rests on the shooter's shoulder, providing stability and a point of contact for aiming.
    Stock                  UMETA(DisplayName = "Stock"),
    // The grip used to hold the firearm with the dominant hand, housing the fire control group.
    HandGrip               UMETA(DisplayName = "Hand Grip"),
    // A small, modular insert for pistol grips that can alter the ergonomics and storage capabilities.
    PistolGripInsert       UMETA(DisplayName = "Pistol Grip Insert"),
    // A soft pad at the end of the stock that absorbs some of the weapon's recoil.
    RecoilPad              UMETA(DisplayName = "Recoil Pad"),

    /*
     * Magazine
     * The component that holds and feeds ammunition.
     */
    // A detachable container that holds and feeds ammunition into the firearm.
    Magazine               UMETA(DisplayName = "Magazine"),

    /*
     * Rails and Mounting
     * Components used to attach other accessories to the firearm.
     */
    // A component with a standardized interface (e.g., Picatinny, KeyMod, M-LOK) used to attach accessories.
    Rail                   UMETA(DisplayName = "Rail"),
    // A protective panel that snaps onto a rail to improve ergonomics and protect the shooter's hands from sharp edges.
    RailCover              UMETA(DisplayName = "Rail Cover"),
    // A general term for a component used to attach an accessory to a rail or another part of the firearm.
    Mount                  UMETA(DisplayName = "Mount"),
    // A specialized mount used to attach an optic to the receiver or rail.
    OpticMount             UMETA(DisplayName = "Optic Mount"),

    /*
     * Small Parts
     * Minor, often cosmetic or functional, components.
     */
    // A hinged cover on the ejection port that protects the internals from debris when the weapon is not in use.
    EjectionPortCover      UMETA(DisplayName = "Ejection Port Cover"),
    // A small, cosmetic accessory that hangs from the firearm, similar to a keychain.
    Charm                  UMETA(DisplayName = "Charm")
};

USTRUCT(BlueprintType, Category = "Attachments")
struct FAttachmentInfo : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Display")
    FName Display_Name;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Display")
    FText Display_Description ;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Display")
    TSoftObjectPtr<USkeletalMesh> Mesh;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Inventory")
    EAttachmentCategory Category;

    /** How many rail slots ("bumps") this attachment occupies. Defaults to 1. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail")
    int32 Size = 1;

    /** Starting slot index when mounted on a rail. Defaults to 0 (leftmost). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail")
    int32 StartSlot = 0;
    /** New: if true, we run full Rail validation (UI + Spline + Bitmask).
       If false, we just use standard socket + collision checks */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attachment")
    bool bUseRail = false;
};

USTRUCT(BlueprintType)
struct FAttachmentLink
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    TArray<TSubclassOf<AAttachment>> ChildClasses;

    UPROPERTY(Transient)
    TArray<AAttachment*> ChildInstances;

    /** Positional offset relative to the socket */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attachment")
    FTransform Offset = FTransform::Identity;

    /** Desired start slots when attaching children to a rail parent */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rail")
    int32 StartSlot;
};