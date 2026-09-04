// Copyright (c) 2026 Academic Game Architecture. All Rights Reserved.

#include "ZombieGame/Character/ZombieHealthComponent.h"
#include "GameFramework/Actor.h"

UZombieHealthComponent::UZombieHealthComponent()
{
	// Disable ticking by default to optimize CPU performance.
	// We only process health on events (push model), not on every frame (polling).
	PrimaryComponentTick.bCanEverTick = false;
}

void UZombieHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize pools on game start
	CurrentHealth = MaxHealth;
	CurrentShield = MaxShield;
	bIsDead = false;
}

float UZombieHealthComponent::ProcessDamage(const FZombieDamageData& DamageData)
{
	// Guard clauses: Ignore damage if dead, invulnerable, or non-positive value
	if (bIsDead || bIsInvulnerable || DamageData.BaseDamage <= 0.0f)
	{
		return 0.0f;
	}

	// 1. Calculate effective damage factoring in critical hits (headshots)
	const float Multiplier = DamageData.bIsHeadshot ? FMath::Max(1.0f, HeadshotMultiplier) : 1.0f;
	float DamageToApply = DamageData.BaseDamage * Multiplier;
	const float TotalInitialDamage = DamageToApply;

	// 2. Shield Absorption Logic (Fortnite / CoD Armor Plate mechanic)
	if (CurrentShield > 0.0f)
	{
		const float PreviousShield = CurrentShield;
		if (CurrentShield >= DamageToApply)
		{
			CurrentShield -= DamageToApply;
			DamageToApply = 0.0f;
		}
		else
		{
			DamageToApply -= CurrentShield;
			CurrentShield = 0.0f;
		}

		const float ShieldDelta = CurrentShield - PreviousShield;
		OnShieldChanged.Broadcast(CurrentShield, MaxShield, ShieldDelta);
	}

	// 3. Health Depletion Logic
	if (DamageToApply > 0.0f)
	{
		const float PreviousHealth = CurrentHealth;
		CurrentHealth = FMath::Clamp(CurrentHealth - DamageToApply, 0.0f, MaxHealth);
		const float HealthDelta = CurrentHealth - PreviousHealth;

		// Notify observers (HUD, damage indicators, camera shake)
		OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, HealthDelta, DamageData);
	}

	// 4. Death Evaluation
	if (CurrentHealth <= 0.0f && !bIsDead)
	{
		bIsDead = true;

		AActor* Killer = DamageData.DamageCauser.IsValid() ? DamageData.DamageCauser.Get() : nullptr;
		OnDeath.Broadcast(GetOwner(), Killer);
	}

	return TotalInitialDamage;
}

void UZombieHealthComponent::Heal(float HealAmount)
{
	if (bIsDead || HealAmount <= 0.0f || CurrentHealth >= MaxHealth)
	{
		return;
	}

	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.0f, MaxHealth);
	const float HealthDelta = CurrentHealth - PreviousHealth;

	// Construct dummy damage data for event payload consistency
	FZombieDamageData HealData;
	HealData.BaseDamage = 0.0f;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, HealthDelta, HealData);
}

void UZombieHealthComponent::AddShield(float ShieldAmount)
{
	if (bIsDead || ShieldAmount <= 0.0f || CurrentShield >= MaxShield)
	{
		return;
	}

	const float PreviousShield = CurrentShield;
	CurrentShield = FMath::Clamp(CurrentShield + ShieldAmount, 0.0f, MaxShield);
	const float ShieldDelta = CurrentShield - PreviousShield;

	OnShieldChanged.Broadcast(CurrentShield, MaxShield, ShieldDelta);
}

void UZombieHealthComponent::SetMaxHealth(float NewMaxHealth, bool bAdjustCurrentHealth)
{
	if (NewMaxHealth <= 0.0f)
	{
		return;
	}

	const float OldMax = MaxHealth;
	MaxHealth = NewMaxHealth;

	if (bAdjustCurrentHealth)
	{
		// Scale proportionally to preserve existing damage ratio
		const float Ratio = (OldMax > 0.0f) ? (CurrentHealth / OldMax) : 1.0f;
		CurrentHealth = FMath::Clamp(MaxHealth * Ratio, 1.0f, MaxHealth);
	}
	else
	{
		CurrentHealth = FMath::Min(CurrentHealth, MaxHealth);
	}

	FZombieDamageData DummyData;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, 0.0f, DummyData);
}
