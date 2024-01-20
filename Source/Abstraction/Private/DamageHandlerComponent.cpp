// Fill out your copyright notice in the Description page of Project Settings.


#include "DamageHandlerComponent.h"
#include "AbstractionPlayerCharacter.h"
#include "GameFramework/DamageType.h"
#include "Particles/ParticleSystemComponent.h"
#include "Misc/ScopeLock.h"

// Sets default values for this component's properties
UDamageHandlerComponent::UDamageHandlerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performafnce if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


void UDamageHandlerComponent::TakeFireDamage(float BaseDamage, float DamageTotalTime, float TakeDamageInterval)
{
	FScopeLock Lock(&CriticalSection);
	if (ActiveDamageInfo.IsSet())
	{
		ActiveDamageInfo.GetValue().BaseDamage = FMath::Max(ActiveDamageInfo.GetValue().BaseDamage, BaseDamage);
		
		if(ActiveDamageInfo.GetValue().LifeTime < DamageTotalTime)
		{
			ActiveDamageInfo.GetValue().LifeTime = DamageTotalTime;
			ActiveDamageInfo.GetValue().IntervalTime = ActiveDamageInfo.GetValue().IntervalTime;
		}
	}
	else
	{
		ActiveDamageInfo.Emplace();
		ActiveDamageInfo.GetValue().BaseDamage = BaseDamage;
		ActiveDamageInfo.GetValue().IntervalTime = TakeDamageInterval;
		ActiveDamageInfo.GetValue().LifeTime = DamageTotalTime;

		if (FireTemplate && PlayerCharacter->ParticleSystemComponent)
		{
			PlayerCharacter->ParticleSystemComponent->SetTemplate(FireTemplate);
			PlayerCharacter->ParticleSystemComponent->Activate(true);
		}
	}
}

// Called when the game starts
void UDamageHandlerComponent::BeginPlay()
{
	Super::BeginPlay();

	PlayerCharacter = Cast<AAbstractionPlayerCharacter>(GetOwner());
	
}


// Called every frame
void UDamageHandlerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (PlayerCharacter)
	{
		FScopeLock Lock(&CriticalSection);
		if (ActiveDamageInfo.IsSet()) // only when this is set, should we access it within this if statement
		{
			// the if/else you are looking for is based on the accumulated time vs life time, while we have not reached life time, accumulate and apply damage in the else.
			if (ActiveDamageInfo.GetValue().AccumulatedTime > ActiveDamageInfo.GetValue().LifeTime)
			{
				if (PlayerCharacter->ParticleSystemComponent)
				{
					PlayerCharacter->ParticleSystemComponent->Deactivate();
					PlayerCharacter->ParticleSystemComponent->SetTemplate(nullptr);
				}
				ActiveDamageInfo.Reset();
			}
			else
			{
				ActiveDamageInfo.GetValue().AccumulatedTime += DeltaTime;
				ActiveDamageInfo.GetValue().CurrentTimeInterval += DeltaTime;
				if (ActiveDamageInfo.GetValue().CurrentTimeInterval > ActiveDamageInfo.GetValue().IntervalTime)
				{
					float ModifiedDamage = ActiveDamageInfo.GetValue().BaseDamage / (ActiveDamageInfo.GetValue().LifeTime / ActiveDamageInfo.GetValue().IntervalTime);
					TSubclassOf<UDamageType> const ValidDamageTypeClass = TSubclassOf<UDamageType>(UDamageType::StaticClass());
					FDamageEvent DamageEvent(ValidDamageTypeClass);
					PlayerCharacter->TakeDamage(ModifiedDamage, DamageEvent, nullptr, GetOwner());
					ActiveDamageInfo.GetValue().CurrentTimeInterval = 0.0f;
				}
			}
		}
	}
}

