// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h" // Required for FInputActionValue
#include "InputMappingContext.h" // Required for UInputMappingContext
#include "GameFramework/Pawn.h"
#include "MVehicleBase.generated.h" // Always the last include for UCLASS()

// Forward declarations to avoid including full headers in the .h file
// This speeds up compile times. Full headers are included in the .cpp.
class UStaticMeshComponent;
class UArrowComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputComponent; // For SetupPlayerInputComponent
struct FInputActionValue; // For input action functions

UCLASS()
class MASHEDPOTATOES_API AMVehicleBase : public APawn
{
    GENERATED_BODY()

public:
    // Sets default values for this pawn's properties
    AMVehicleBase();

    // --- Components ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* BodyMeshC;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UArrowComponent* ArrowC_FR;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UArrowComponent* ArrowC_FL;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UArrowComponent* ArrowC_RR;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UArrowComponent* ArrowC_RL;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* WheelScene_FR;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* WheelScene_FL;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* WheelScene_RR;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* WheelScene_RL;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USpringArmComponent* SpringArmComponent;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UCameraComponent* CameraC;

    // --- Car Physics Parameters (Exposed in Editor) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Physics|Suspension")
    // Ideal length of the spring when no force is applied
    float RestLength = 100.0f; 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Physics|Suspension")
    // How much the spring can compress/extend from its rest length
    float SpringTravelLength = 150.0f; 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Physics|Suspension")
    // Radius of the wheel, used for line tracing
    float WheelRadius = 50.0f; 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Physics|Suspension")
    // Stiffness of the suspension spring
    float SpringForceConst = 7000.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Physics|Suspension")
    // Damping for the suspension (absorbs oscillations)
    float DamperForceConst = 2456.0f;

    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Physics|Engine")
    // Force applied for acceleration
    float ForwardForceConst = 300000.0f; 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Physics|Forces")
    // General friction coefficient for lateral slip
    float FrictionConst = 1000.0f; 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Physics|Forces")
    // Controls general drag opposing motion (e.g., rolling resistance, air resistance)
    float DragConst = 100.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Physics|Forces")
    // Force applied for braking
    float BrakeConst = 150.0f; 

    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Physics|Steering")
    // Maximum angle the front wheels can turn
    float MaxSteeringAngle = 30.0f; 
   
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Physics|Braking")
    // Constant for a strong brake hold when nearly stopped
    float BrakeStopForceConst = 13000.0f; 

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Physics|Braking")
    // Velocity threshold below which BrakeStopForceConst applies
    float BrakeStopThresholdVelocity = 50.0f;
   
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enhanced Input")
    UInputMappingContext* InputMapping;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enhanced Input")
    UInputAction* InputDrive;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enhanced Input")
    UInputAction* InputSteer;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enhanced Input")
    UInputAction* InputBrake;
    
protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;
    // Called after components are initialized (e.g., physics setup)
    virtual void PostInitializeComponents() override;

    // --- Internal State Variables ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Car State")
    bool bBrakeApplied; // True if brake input is active

    // Current interpolated values for input axes (used in Tick)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Car State")
    float ForwardAxisValue;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Car State")
    float SideAxisValue;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle Physics|Damping")
    // Constant for stronger drag when at rest/low speed
    float RestingDragConst = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle Physics|Damping")
    // Velocity threshold below which resting drag applies
    float StopThresholdVelocity = 50.0f; 
    // Raw input values (set by input actions)
    float ForwardAxisInput;
    float SideAxisInput;

    // Stores the current steering angle applied to front wheels
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Car State")
    float CurrentFrontWheelSteerAngle;

public: 
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // Called to bind functionality to input
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
    // --- Internal Physics Calculation Variables ---
    // Array to hold references to wheel arrow components
    TArray<UArrowComponent*> WheelArrowComponentHolder;
    // Array to hold references to wheel meshes
    TArray<USceneComponent*> WheelSceneComponentHolder;
    // Minimum compressed length of the spring
    float MinLength;
    // Maximum extended length of the spring
    float MaxLength;
    // Current length of each spring
    float SpringLength[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    // Parameters for the wheel line trace
    FCollisionQueryParams LineTraceCollisionQuery;

    // Calculates and applies forces for a single wheel
    void UpdateVehicleForce(int WheelArrowIndex, float DeltaTime); 
    
    // Input Action Callbacks
    void Accelerate(const FInputActionValue& Value);
    void Steer(const FInputActionValue& Value);
    void BrakeStarted(const FInputActionValue& Value);
    void BrakeEnded(const FInputActionValue& Value);
};