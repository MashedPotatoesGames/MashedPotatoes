// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "GameFramework/Pawn.h"
#include "MVehicleBase.generated.h"

class UArrowComponent;
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class MASHEDPOTATOES_API AMVehicleBase : public APawn
{

	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	UStaticMeshComponent* BodyMeshC;
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	UArrowComponent* ArrowC_FR;
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	UArrowComponent* ArrowC_FL;
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	UArrowComponent* ArrowC_RR;
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	UArrowComponent* ArrowC_RL;

	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	USpringArmComponent* SpringArmComponent;
	
	UPROPERTY(EditAnywhere)
	UCameraComponent* CameraC;

	//Exposed Vars
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	float RestLength = 140;
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	float SpringTravelLength = 75;
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	float WheelRadius = 34;
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	float SpringForceConst = 5000;
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	float DamperForceConst = 5000;
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	float ForwardForceConst = 100000;
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	float MaxSteeringAngle = 30;
	UPROPERTY(EditAnywhere, Category="Enhanced Input")
	UInputMappingContext* InputMapping;

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Enhanced Input")
	UInputAction* InputDrive;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Enhanced Input")
	UInputAction* InputSteer;
	
	// Sets default values for this pawn's properties
	AMVehicleBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	TArray<UArrowComponent*> WheelArrowComponentHolder;
	float MinLength;
	float MaxLength;
	float SpringLength[4] = {0,0,0,0};
	float ForwardAxisValue;
	float SideAxisValue;
	FCollisionQueryParams LineTraceCollisionQuery;
	void UpdateVehicleForce(int WheelArrowIndex, float DeltaTime);
	void Accelerate(const FInputActionValue& Value);
	void Steer(const FInputActionValue& Value);
};
