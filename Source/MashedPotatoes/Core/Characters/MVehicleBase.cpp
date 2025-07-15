// Fill out your copyright notice in the Description page of Project Settings.


#include "MVehicleBase.h"

#include <string>

#include "Components/ArrowComponent.h"
#include "EnhancedInput/Public/EnhancedInputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "EnhancedInput/Public/EnhancedInputSubsystems.h"

// Sets default values
AMVehicleBase::AMVehicleBase()
{
	PrimaryActorTick.bCanEverTick = true;
	
	BodyMeshC = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMeshC"));
	
	if (UStaticMesh* CarMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("/Game/Assets/Car/SM_Car_Body.SM_Car_Body"))))
	{
		BodyMeshC->SetStaticMesh(CarMesh);
	}
	ArrowC_FL = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowC_FL"));
	ArrowC_FL->SetupAttachment(BodyMeshC, FName("WheelFL"));
	ArrowC_FL->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	ArrowC_FR = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowC_FR"));
	ArrowC_FR->SetupAttachment(BodyMeshC, FName("WheelFR"));
	ArrowC_FR->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	ArrowC_RL = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowC_RR"));
	ArrowC_RL->SetupAttachment(BodyMeshC, FName("WheelRR"));
	ArrowC_RL->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	ArrowC_RR = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowC_RL"));
	ArrowC_RR->SetupAttachment(BodyMeshC, FName("WheelRL"));
	ArrowC_RR->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(BodyMeshC);
	SpringArmComponent->TargetArmLength = 600.0f;
	SpringArmComponent->SetRelativeRotation(FRotator(-10, 0, 0));
	SpringArmComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
	CameraC = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraC"));
	CameraC->SetupAttachment(SpringArmComponent);

}

void AMVehicleBase::BeginPlay()
{
	Super::BeginPlay();
	WheelArrowComponentHolder = {ArrowC_FL, ArrowC_FR, ArrowC_RL, ArrowC_RR};
	MinLength = RestLength - SpringTravelLength;
	MaxLength = RestLength + SpringTravelLength;

	const FName TraceTag("MyTraceTag");
	LineTraceCollisionQuery.TraceTag = TraceTag;
	LineTraceCollisionQuery.bDebugQuery = true;
	LineTraceCollisionQuery.AddIgnoredActor(this);
}

void AMVehicleBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	BodyMeshC->SetSimulatePhysics(true);
	BodyMeshC->SetMassOverrideInKg(NAME_None,1100);
}

// Called every frame
void AMVehicleBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	for (int wheelArrowIndex = 0; wheelArrowIndex < 4; wheelArrowIndex++)
	{
		UpdateVehicleForce(wheelArrowIndex, DeltaTime);
	}
	
}

void AMVehicleBase::UpdateVehicleForce(int WheelArrowIndex, float DeltaTime)
{
	if (!WheelArrowComponentHolder.IsValidIndex(WheelArrowIndex)) return;
	auto wheelArrowC = WheelArrowComponentHolder[WheelArrowIndex];
	FHitResult outHit;
	FVector startTraceLoc = wheelArrowC->GetComponentLocation();
	FVector endTraceLoc = wheelArrowC->GetForwardVector() * (MaxLength + WheelRadius) + startTraceLoc;
	GetWorld()->LineTraceSingleByChannel(outHit, startTraceLoc, endTraceLoc, ECC_Camera, LineTraceCollisionQuery, FCollisionResponseParams());
	float previousSpringLength = SpringLength[WheelArrowIndex];
	FVector worldLinearVelocity = BodyMeshC->GetPhysicsLinearVelocity();
	FVector localLinearVelocity = UKismetMathLibrary::InverseTransformDirection(BodyMeshC->GetComponentTransform(), worldLinearVelocity);
	if (outHit.bBlockingHit)
	{
		float currentSpringLength = outHit.Distance - WheelRadius;
		SpringLength[WheelArrowIndex] = UKismetMathLibrary::FClamp(currentSpringLength, MinLength, MaxLength);
		float springVelocity = (previousSpringLength - SpringLength[WheelArrowIndex]) / DeltaTime;
		float springForce = (RestLength - SpringLength[WheelArrowIndex]) * SpringForceConst; // The bigger the difference, the more it is compressed/expanded
		float damperForce = springVelocity * DamperForceConst;
		FVector upwardForce = GetActorUpVector() * (springForce + damperForce);
		BodyMeshC->AddForceAtLocation(upwardForce, wheelArrowC->GetComponentLocation());
		
	}
	else
	{
		SpringLength[WheelArrowIndex] = MaxLength;
	}

	float currentSteeringAngle = UKismetMathLibrary::MapRangeClamped(SideAxisValue, -1, 1, -MaxSteeringAngle, MaxSteeringAngle);
	if (worldLinearVelocity.SizeSquared() > 1)
	{
		BodyMeshC->AddTorqueInDegrees(FVector(0,0,currentSteeringAngle), NAME_None, true);
	}

	FVector vehicleForwardForce = GetActorForwardVector() * ForwardAxisValue * ForwardForceConst;
	BodyMeshC->AddForceAtLocation(vehicleForwardForce, wheelArrowC->GetComponentLocation());
}

// Called to bind functionality to input
void AMVehicleBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 150.0f, FColor::Yellow, FString::Printf(TEXT("Setting up Input.")));

    // Ensure the PlayerController and Enhanced Input Subsystem are valid
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
        if (InputSubsystem)
        {
            if (InputMapping) // Assuming InputMapping is your UInputMappingContext* property
            {
                InputSubsystem->AddMappingContext(InputMapping, 0);
            }
            else
            {
                if (GEngine)
                    GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("InputMapping is not set in MVehicleBase!"));
            }
        }
        else
        {
            if (GEngine)
                GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("EnhancedInputLocalPlayerSubsystem is null!"));
        }
    }
    else
    {
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("PlayerController is null!"));
    }

    // IMPORTANT: Check if the cast to EnhancedInputComponent is successful
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (InputDrive)
        {
            EnhancedInputComponent->BindAction(InputDrive, ETriggerEvent::Triggered, this, &AMVehicleBase::Accelerate);
        	EnhancedInputComponent->BindAction(InputDrive, ETriggerEvent::Completed, this, &AMVehicleBase::Accelerate);
        }
        else
        {
            if (GEngine)
                GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("InputDrive Action is not set in MVehicleBase!"));
        }
        if (InputSteer) 
        {
            EnhancedInputComponent->BindAction(InputSteer, ETriggerEvent::Triggered, this, &AMVehicleBase::Steer);
        	EnhancedInputComponent->BindAction(InputSteer, ETriggerEvent::Completed, this, &AMVehicleBase::Steer);

        }
        else
        {
            if (GEngine)
                GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("InputSteer Action is not set in MVehicleBase!"));
        }
    }
    else
    {
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("PlayerInputComponent is not an EnhancedInputComponent!"));
    }
}

void AMVehicleBase::Accelerate(const FInputActionValue& Value)
{
	ForwardAxisValue = Value.Get<float>();
	if(GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Acc: %f"), ForwardAxisValue));
}
void AMVehicleBase::Steer(const FInputActionValue& Value)
{
	SideAxisValue = Value.Get<float>();
	if(GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Steer: %f"), SideAxisValue));
	return;
}
