// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Gameplay/InteractionComponent.h"
#include "ZombieGame/Character/PlayerCharacter.h"
#include "ZombieGame/Interfaces/InteractableInterface.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "CollisionShape.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.05f; // 20 Hz throttled raycasts for optimal CPU efficiency
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	OwningPlayer = Cast<APlayerCharacter>(GetOwner());
}

void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	PerformInteractionTrace();
}

void UInteractionComponent::PerformInteractionTrace()
{
	UWorld* World = GetWorld();
	if (!World || !OwningPlayer.IsValid())
	{
		return;
	}

	FVector TraceStart = FVector::ZeroVector;
	FVector TraceDirection = FVector::ForwardVector;

	// Query player camera manager or character camera
	if (APlayerController* PC = Cast<APlayerController>(OwningPlayer->GetController()))
	{
		if (PC->PlayerCameraManager)
		{
			TraceStart = PC->PlayerCameraManager->GetCameraLocation();
			TraceDirection = PC->PlayerCameraManager->GetCameraRotation().Vector();
		}
	}

	if (TraceStart.IsZero())
	{
		TraceStart = OwningPlayer->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);
		TraceDirection = OwningPlayer->GetActorForwardVector();
	}

	const FVector TraceEnd = TraceStart + (TraceDirection * InteractionDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwningPlayer.Get());
	QueryParams.bTraceComplex = true;

	FHitResult HitResult;
	bool bHit = false;

	if (InteractionTraceRadius > 0.0f)
	{
		FCollisionShape SphereShape = FCollisionShape::MakeSphere(InteractionTraceRadius);
		bHit = World->SweepSingleByChannel(HitResult, TraceStart, TraceEnd, FQuat::Identity, InteractionChannel, SphereShape, QueryParams);
	}
	else
	{
		bHit = World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, InteractionChannel, QueryParams);
	}

	AActor* HitActor = bHit ? HitResult.GetActor() : nullptr;
	TScriptInterface<IInteractableInterface> NewInteractable = nullptr;

	if (HitActor && HitActor->Implements<UInteractableInterface>())
	{
		NewInteractable.SetObject(HitActor);
		NewInteractable.SetInterface(Cast<IInteractableInterface>(HitActor));
	}

	// State transition handling
	if (NewInteractable != CurrentInteractable)
	{
		CurrentInteractable = NewInteractable;

		if (CurrentInteractable.GetObject())
		{
			const FText Prompt = IInteractableInterface::Execute_GetInteractionPrompt(CurrentInteractable.GetObject());
			const int32 Cost = IInteractableInterface::Execute_GetInteractionCost(CurrentInteractable.GetObject());
			OnInteractableFound.Broadcast(CurrentInteractable, Prompt, Cost);
		}
		else
		{
			OnInteractableLost.Broadcast();
		}
	}
}

bool UInteractionComponent::TryInteract()
{
	if (!CurrentInteractable.GetObject() || !OwningPlayer.IsValid())
	{
		return false;
	}

	if (IInteractableInterface::Execute_CanInteract(CurrentInteractable.GetObject(), OwningPlayer.Get()))
	{
		const bool bSuccess = IInteractableInterface::Execute_Interact(CurrentInteractable.GetObject(), OwningPlayer.Get());
		if (bSuccess)
		{
			// Re-evaluate interactable status immediately (cost or state might have changed)
			PerformInteractionTrace();
		}
		return bSuccess;
	}

	return false;
}
