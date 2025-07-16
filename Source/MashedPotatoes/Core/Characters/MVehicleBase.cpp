// Fill out your copyright notice in the Description page of Project Settings.


#include "MVehicleBase.h"
#include "Components/ArrowComponent.h"
#include "EnhancedInput/Public/EnhancedInputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "DrawDebugHelpers.h" // For visualizing forces
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "EnhancedInput/Public/EnhancedInputSubsystems.h"

// Sets default values
AMVehicleBase::AMVehicleBase()
{
    PrimaryActorTick.bCanEverTick = true;
    
    BodyMeshC = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMeshC"));
    SetRootComponent(BodyMeshC); // Set the body mesh as the root component
    
    if (UStaticMesh* CarMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("/Game/Assets/Car/SM_Car_Body.SM_Car_Body"))))
    {
       BodyMeshC->SetStaticMesh(CarMesh);
    }
    
    // Attach arrow components to the body mesh
    ArrowC_FL = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowC_FL"));
    ArrowC_FL->SetupAttachment(BodyMeshC, FName("WheelFL"));
    ArrowC_FL->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f)); // Rotated to point downwards for suspension trace
    
    ArrowC_FR = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowC_FR"));
    ArrowC_FR->SetupAttachment(BodyMeshC, FName("WheelFR"));
    ArrowC_FR->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
    
    ArrowC_RL = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowC_RR")); 
    ArrowC_RL->SetupAttachment(BodyMeshC, FName("WheelRR"));
    ArrowC_RL->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
    
    ArrowC_RR = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowC_RL"));
    ArrowC_RR->SetupAttachment(BodyMeshC, FName("WheelRL"));
    ArrowC_RR->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));

    WheelScene_FL = CreateDefaultSubobject<USceneComponent>("WheelSceneFL");
    WheelScene_FL->SetupAttachment(ArrowC_FL);
    WheelScene_FR = CreateDefaultSubobject<USceneComponent>("WheelSceneFR");
    WheelScene_FR->SetupAttachment(ArrowC_FR);
    WheelScene_RL = CreateDefaultSubobject<USceneComponent>("WheelSceneRL");
    WheelScene_RL->SetupAttachment(ArrowC_RL);
    WheelScene_RR = CreateDefaultSubobject<USceneComponent>("WheelSceneRR");
    WheelScene_RR->SetupAttachment(ArrowC_RR);
    
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
    // Initialize the holder array for wheel arrow components
    WheelArrowComponentHolder = {ArrowC_FL, ArrowC_FR, ArrowC_RL, ArrowC_RR};
    WheelSceneComponentHolder = {WheelScene_FL, WheelScene_FR, WheelScene_RL, WheelScene_RR};

    // Setup for line trace collision query
    const FName TraceTag("MyTraceTag");
    LineTraceCollisionQuery.TraceTag = TraceTag;
    LineTraceCollisionQuery.bDebugQuery = true; // Enable debug drawing for the trace
    LineTraceCollisionQuery.AddIgnoredActor(this); // Ignore the car itself in the trace
}

void AMVehicleBase::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    // Ensure the body mesh simulates physics
    BodyMeshC->SetSimulatePhysics(true);
    BodyMeshC->SetMassOverrideInKg(NAME_None,1100); // Set car mass
}

// Called every frame
void AMVehicleBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Interpolate input values for smoother acceleration and steering
    SideAxisValue = UKismetMathLibrary::FInterpTo(SideAxisValue, SideAxisInput, GetWorld()->GetDeltaSeconds(), 5);
    ForwardAxisValue = UKismetMathLibrary::FInterpTo(ForwardAxisValue, ForwardAxisInput, GetWorld()->GetDeltaSeconds(), 5);

    // Update forces for each wheel
    for (int wheelArrowIndex = 0; wheelArrowIndex < 4; wheelArrowIndex++)
    {
       UpdateVehicleForce(wheelArrowIndex, DeltaTime);
    }
}

void AMVehicleBase::UpdateVehicleForce(int WheelArrowIndex, float DeltaTime)
{
    // Calculate min and max spring lengths based on rest length and travel
    MinLength = RestLength - SpringTravelLength;
    MaxLength = RestLength + SpringTravelLength;
    
    // Validate wheel arrow component index
    if (!WheelArrowComponentHolder.IsValidIndex(WheelArrowIndex)) return;
    auto wheelArrowC = WheelArrowComponentHolder[WheelArrowIndex];

    FHitResult outHit;
    FVector startTraceLoc = wheelArrowC->GetComponentLocation();
    // Trace downwards from the wheel's arrow component to detect ground
    FVector endTraceLoc = wheelArrowC->GetForwardVector() * (MaxLength + WheelRadius) + startTraceLoc;
    
    // Perform the line trace to detect ground contact
    GetWorld()->LineTraceSingleByChannel(outHit, startTraceLoc, endTraceLoc, ECC_Visibility, LineTraceCollisionQuery, FCollisionResponseParams());
    // Draw debug line for the suspension trace
    DrawDebugLine(GetWorld(), wheelArrowC->GetComponentLocation(), endTraceLoc , FColor::Green, false, -1, 0, 2.0f);

    // --- Suspension Force (Only apply if the wheel is on the ground) ---
    if (outHit.bBlockingHit)
    {
        float currentSpringLength = outHit.Distance - WheelRadius;
        
        // Store the current spring length and calculate velocity from the previous frame's length
        float prevSpringLength = SpringLength[WheelArrowIndex]; // Capture previous length before updating
        SpringLength[WheelArrowIndex] = UKismetMathLibrary::FClamp(currentSpringLength, MinLength, MaxLength);
        
        float springVelocity = (prevSpringLength - SpringLength[WheelArrowIndex]) / DeltaTime; // Velocity of compression/extension
        
        float springForce = (RestLength - SpringLength[WheelArrowIndex]) * SpringForceConst; // Force from spring compression/extension
        float damperForce = springVelocity * DamperForceConst; // Damping force opposes spring velocity

        // Apply upward force from suspension
        FVector upwardForce = GetActorUpVector() * (springForce + damperForce);
        BodyMeshC->AddForceAtLocation(upwardForce, wheelArrowC->GetComponentLocation());

        // --- Debug Drawing for Suspension Force ---
        DrawDebugLine(GetWorld(), wheelArrowC->GetComponentLocation(), wheelArrowC->GetComponentLocation() + upwardForce * 0.01f, FColor::Blue, false, -1, 0, 2.0f);
    }
    else
    {
        // If the wheel is not on the ground, reset its spring length and apply no other forces from it.
        SpringLength[WheelArrowIndex] = MaxLength; // Spring is fully extended when not touching ground
        return; // No further forces are applied if the wheel is airborne
    }

    // --- Calculate Ground-Aligned Wheel Vectors for Steering and Forces ---
    // Get the car's forward vector and flatten it to the ground plane (Z=0)
    FVector carForwardOnGround = GetActorForwardVector();
    carForwardOnGround.Z = 0.0f; 
    carForwardOnGround.Normalize(); // Ensure it's a unit vector

    FVector wheelForwardVectorGroundAligned;

    // Apply steering only to front wheels (assuming 0=FL, 1=FR)
    if (WheelArrowIndex == 0 || WheelArrowIndex == 1) 
    {
        // Interpolate the steering angle for smoother transitions
        CurrentFrontWheelSteerAngle = UKismetMathLibrary::FInterpTo(CurrentFrontWheelSteerAngle, MaxSteeringAngle * SideAxisInput, DeltaTime, 5.0f);
        // Rotate the car's ground-aligned forward vector by the steering angle around the world's Up vector
        wheelForwardVectorGroundAligned = carForwardOnGround.RotateAngleAxis(CurrentFrontWheelSteerAngle, FVector::UpVector);
        WheelSceneComponentHolder[WheelArrowIndex]->SetRelativeRotation(FRotator(0,0,CurrentFrontWheelSteerAngle));
        
    }
    else // Rear wheels (2=RL, 3=RR) do not steer. They follow the car's forward direction
    {
        wheelForwardVectorGroundAligned = carForwardOnGround;
    }
    wheelForwardVectorGroundAligned.Normalize(); // Ensure it's a unit vector after rotation

    // Calculate the wheel's "right" vector, perpendicular to its ground-aligned forward direction
    FVector wheelRightVectorGroundAligned = UKismetMathLibrary::Cross_VectorVector(FVector::UpVector, wheelForwardVectorGroundAligned);
    wheelRightVectorGroundAligned.Normalize(); // Ensure it's a unit vector

    // Get the velocity of the car at this wheel's contact point
    // This is the linear velocity of the rigid body + the tangential velocity due to angular rotation
    FVector wheelWorldVelocityAtContact = BodyMeshC->GetPhysicsLinearVelocity() +
                                         UKismetMathLibrary::Cross_VectorVector(BodyMeshC->GetPhysicsAngularVelocityInDegrees() * (PI / 180.0f), // Convert deg/s to rad/s
                                                                                wheelArrowC->GetComponentLocation() - BodyMeshC->GetCenterOfMass()); // Use CoM for accurate lever arm

    // --- Calculate Slip Velocities using the ground-aligned vectors ---
    // Longitudinal velocity: how fast the wheel is moving in its ground-aligned forward direction
    float longitudinalVelocity = FVector::DotProduct(wheelWorldVelocityAtContact, wheelForwardVectorGroundAligned);
    // Lateral velocity: how fast the wheel is slipping sideways relative to its ground-aligned orientation
    float lateralVelocity = FVector::DotProduct(wheelWorldVelocityAtContact, wheelRightVectorGroundAligned);

    // --- PROPULSION / BRAKING FORCE (Active Force) ---
    float activeLongitudinalForce = 0.0f;
    float BrakeHoldForceApplied = 0.0f; // For debugging
    if (bBrakeApplied)
    {
        // Apply strong braking force directly opposing the current longitudinal velocity
        activeLongitudinalForce = -longitudinalVelocity * BrakeConst;

        // NEW: Apply a strong "hold" force when braking and nearly stopped
        if (FMath::Abs(longitudinalVelocity) < BrakeStopThresholdVelocity)
        {
            // Apply a fixed force in the opposite direction of current velocity
            BrakeHoldForceApplied = -FMath::Sign(longitudinalVelocity) * BrakeStopForceConst;
            activeLongitudinalForce += BrakeHoldForceApplied; // Add this to the existing braking force
        }
    }
    else
    {
        // Apply propulsion force based on ForwardAxisValue, ONLY TO REAR WHEELS (indices 2 and 3)
        if (WheelArrowIndex == 2 || WheelArrowIndex == 3) // Assuming 2=RL, 3=RR are rear wheels
        {
            activeLongitudinalForce = ForwardAxisValue * ForwardForceConst;
        }
    }
    // CRITICAL FIX: Apply propulsion/braking force along the ground-aligned wheel forward vector
    FVector totalWheelForce = wheelForwardVectorGroundAligned * activeLongitudinalForce;

    // --- FRICTION FORCES (Passive, always oppose slip) ---
    // Lateral Friction: Opposes sideways movement (crucial for steering and preventing spin)
    float lateralFrictionMagnitude = -lateralVelocity * FrictionConst;
    totalWheelForce += wheelRightVectorGroundAligned * lateralFrictionMagnitude; // Use ground-aligned right vector for lateral friction

    // Longitudinal Friction (Rolling Resistance / Drag): Opposes forward/backward movement
    // This slows the car down when no active propulsion/braking is applied.
    float rollingResistanceMagnitude = -longitudinalVelocity * DragConst;
    totalWheelForce += wheelForwardVectorGroundAligned * rollingResistanceMagnitude; // Use ground-aligned forward vector for rolling resistance

    // --- Resting Drag / Low Velocity Damping ---
    float RestingDragForceMagnitude = 0.0f;
    // Only apply if no active drive/brake input and car is moving very slowly
    if (FMath::IsNearlyZero(ForwardAxisInput, KINDA_SMALL_NUMBER) && !bBrakeApplied && FMath::Abs(longitudinalVelocity) < StopThresholdVelocity)
    {
        // Apply a stronger drag to bring the car to a complete stop
        RestingDragForceMagnitude = -longitudinalVelocity * RestingDragConst;
        totalWheelForce += wheelForwardVectorGroundAligned * RestingDragForceMagnitude;
    }


    // --- Apply Final Combined Force for this Wheel ---
    BodyMeshC->AddForceAtLocation(totalWheelForce, wheelArrowC->GetComponentLocation());

    // Setting the wheel mesh position
    WheelSceneComponentHolder[WheelArrowIndex]->SetRelativeLocation(FVector(SpringLength[WheelArrowIndex], 0,0));
    // Spin wheel
        
    WheelSceneComponentHolder[WheelArrowIndex]->GetChildComponent(0)-> AddLocalRotation(FRotator( // directly in the statich mesh
    0,0,
    (-360*longitudinalVelocity*DeltaTime)/(2 * UKismetMathLibrary::GetPI() * WheelRadius)));
    
    // WheelSceneComponentHolder[WheelArrowIndex]->AddLocalRotation(FRotator(
    //     (-360*longitudinalVelocity*DeltaTime)/(2 * UKismetMathLibrary::GetPI() * WheelRadius),
    //     0,
    //     0));

    // --- Debug Drawing for Individual Force Components ---
    // Total Force (red)
    DrawDebugLine(GetWorld(), wheelArrowC->GetComponentLocation(), wheelArrowC->GetComponentLocation() + totalWheelForce * 0.001f, FColor::Red, false, -1, 0, 2.0f);
    
    // Active Longitudinal Force (green) - should be strong when accelerating/braking
    DrawDebugLine(GetWorld(), wheelArrowC->GetComponentLocation(), wheelArrowC->GetComponentLocation() + wheelForwardVectorGroundAligned * activeLongitudinalForce * 0.001f, FColor::Green, false, -1, 0, 1.0f);
    
    // Lateral Friction Force (yellow) - should be strong when turning or sliding sideways
    DrawDebugLine(GetWorld(), wheelArrowC->GetComponentLocation(), wheelArrowC->GetComponentLocation() + wheelRightVectorGroundAligned * lateralFrictionMagnitude * 0.001f, FColor::Yellow, false, -1, 0, 1.0f);
    
    // Rolling Resistance/Drag Force (magenta) - should always oppose motion
    DrawDebugLine(GetWorld(), wheelArrowC->GetComponentLocation(), wheelArrowC->GetComponentLocation() + wheelForwardVectorGroundAligned * rollingResistanceMagnitude * 0.001f, FColor::Magenta, false, -1, 0, 1.0f);

    // Optional: Visualize wheel's local ground-aligned forward and right vectors
    // DrawDebugLine(GetWorld(), wheelArrowC->GetComponentLocation(), wheelArrowC->GetComponentLocation() + wheelForwardVectorGroundAligned * 100.0f, FColor::Cyan, false, -1, 0, 0.5f);
    // DrawDebugLine(GetWorld(), wheelArrowC->GetComponentLocation(), wheelArrowC->GetComponentLocation() + wheelRightVectorGroundAligned * 100.0f, FColor::Orange, false, -1, 0, 0.5f);

    // --- Debug Messages for Steering Diagnosis (Front-Left Wheel only) ---
    if (WheelArrowIndex == 0) // Only for the front-left wheel
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(1, 0.1f, FColor::Orange, FString::Printf(TEXT("FL Wheel Lateral Vel: %.2f"), lateralVelocity));
            GEngine->AddOnScreenDebugMessage(2, 0.1f, FColor::Orange, FString::Printf(TEXT("FL Wheel Lat Friction Mag: %.2f"), lateralFrictionMagnitude));
            GEngine->AddOnScreenDebugMessage(3, 0.1f, FColor::Orange, FString::Printf(TEXT("FL Wheel Steer Angle: %.2f"), CurrentFrontWheelSteerAngle));
            GEngine->AddOnScreenDebugMessage(4, 0.1f, FColor::Cyan, FString::Printf(TEXT("Resting Drag Force: %.2f"), RestingDragForceMagnitude));
            GEngine->AddOnScreenDebugMessage(5, 0.1f, FColor::Red, FString::Printf(TEXT("Brake Hold Force: %.2f"), BrakeHoldForceApplied));
        }
    }
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
       if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
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
        if (InputBrake) 
        {
           EnhancedInputComponent->BindAction(InputBrake, ETriggerEvent::Started, this, &AMVehicleBase::BrakeStarted);
           EnhancedInputComponent->BindAction(InputBrake, ETriggerEvent::Completed, this, &AMVehicleBase::BrakeEnded);
        }
        else
        {
           if (GEngine)
              GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("Brake Action is not set in MVehicleBase!"));
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
    ForwardAxisInput = Value.Get<float>(); // Set here, interpolate in tick
}
void AMVehicleBase::Steer(const FInputActionValue& Value)
{
    SideAxisInput = Value.Get<float>(); // Set here, interpolate in tick
}
void AMVehicleBase::BrakeStarted(const FInputActionValue& Value)
{
    bBrakeApplied = true;
    if (GEngine)
       GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("Brake Started."));
}
void AMVehicleBase::BrakeEnded(const FInputActionValue& Value)
{
    bBrakeApplied = false;
    if (GEngine)
       GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("Brake Ended."));
}
