// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZombieGame/Interfaces/InteractableInterface.h"
#include "ZombieGame/Gameplay/PowerUpBase.h"
#include "WallBuyStation.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class APlayerCharacter;

/**
 * Supported purchase offerings at wall buy locations.
 */
UENUM(BlueprintType)
enum class EWallBuyType : uint8
{
	AmmoRestock          UMETA(DisplayName="Ammo Restock"),
	PowerUpMaxAmmo       UMETA(DisplayName="Max Ammo Purchase"),
	PowerUpInstaKill     UMETA(DisplayName="Insta-Kill Purchase"),
	PowerUpDoublePoints  UMETA(DisplayName="Double Points Purchase"),
	PowerUpNuke          UMETA(DisplayName="Nuke Purchase"),
	CustomPowerUpClass   UMETA(DisplayName="Custom Power-Up Class")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnWallBuyPurchasedSignature,
	AWallBuyStation*, Station,
	APlayerCharacter*, Buyer
);

/**
 * Wall-mounted vending station allowing player purchase of ammo or power-up items with points.
 */
UCLASS()
class ISPPV1_API AWallBuyStation : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	AWallBuyStation();

protected:
	virtual void BeginPlay() override;

	// ----------------------------------------------------------------------------------
	// Components
	// ----------------------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Wall-mounted plaque, frame, or chalk outline mesh. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> StationMesh;

	/** Floating 3D holographic or physical preview of the offered item. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> ItemPreviewMesh;

	/** Trigger box facilitating interaction traces. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> InteractionTrigger;

	// ----------------------------------------------------------------------------------
	// Configuration
	// ----------------------------------------------------------------------------------

	/** What commodity this station sells. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WallBuy|Config")
	EWallBuyType BuyType = EWallBuyType::AmmoRestock;

	/** Point cost required to purchase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WallBuy|Config", meta=(ClampMin="0"))
	int32 Cost = 500;

	/** Localized prompt shown on HUD (e.g. "Buy Ammo", "Buy Max Ammo"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WallBuy|Config")
	FText PromptText;

	/** Optional power-up actor class to spawn when purchasing power-up items. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WallBuy|Config")
	TSubclassOf<APowerUpBase> PowerUpClass;

	/** If true, activates the power-up effect immediately on purchase. If false, spawns a physical 3D pickup in front of the station. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WallBuy|Config")
	bool bActivateImmediately = true;

protected:
	/** Helper to instantiate a physical 3D pickup actor in front of the station. */
	void SpawnPowerUpPickup(EPowerUpType InType);

public:
	// ----------------------------------------------------------------------------------
	// Observer Events
	// ----------------------------------------------------------------------------------

	/** Broadcast when a player successfully completes a purchase. */
	UPROPERTY(BlueprintAssignable, Category="WallBuy|Events")
	FOnWallBuyPurchasedSignature OnWallBuyPurchased;

	// ----------------------------------------------------------------------------------
	// IInteractableInterface Implementation
	// ----------------------------------------------------------------------------------

	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual int32 GetInteractionCost_Implementation() const override;
	virtual bool CanInteract_Implementation(const APlayerCharacter* InstigatorPlayer) const override;
	virtual bool Interact_Implementation(APlayerCharacter* InstigatorPlayer) override;

	// ----------------------------------------------------------------------------------
	// Getters
	// ----------------------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category="WallBuy|Queries")
	FORCEINLINE EWallBuyType GetBuyType() const { return BuyType; }
};
