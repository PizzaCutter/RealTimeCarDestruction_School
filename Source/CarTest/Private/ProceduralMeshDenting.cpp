// Fill out your copyright notice in the Description page of Project Settings.

#include "ProceduralMeshDenting.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

#include "RuntimeMeshComponent.h"
#include "RuntimeMeshShapeGenerator.h"
#include "RuntimeMeshBuilder.h"
#include "RuntimeMeshLibrary.h"
#include "RuntimeMeshData.h"
#include "RuntimeMesh.h"

// Sets default values
AProceduralMeshDenting::AProceduralMeshDenting()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	runtimeMeshComp = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("Runtime Mesh Component"));
	staticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh Component To Copy"));

	RootComponent = runtimeMeshComp;
	//runtimeMeshComp->AttachTo(RootComponent);
	//staticMeshComp->AttachTo(RootComponent);

	runtimeMeshComp->SetCollisionUseComplexAsSimple(false);
}

// Called when the game starts or when spawned
void AProceduralMeshDenting::BeginPlay()
{
	Super::BeginPlay();
	//CopyStaticMesh();
	runtimeMeshComp->OnComponentHit.AddDynamic(this, &AProceduralMeshDenting::AddDent);
	runtimeMeshComp->SetSimulatePhysics(true);
}

void AProceduralMeshDenting::OnConstruction(const FTransform& transform)
{
	Super::OnConstruction(transform);
	CopyStaticMesh();
}

// Called every frame
void AProceduralMeshDenting::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProceduralMeshDenting::CopyStaticMesh()
{
	if (staticMeshComp == nullptr)
		return;

	if (staticMeshComp->GetStaticMesh() == nullptr)
		return;

	if (staticMeshComp->GetStaticMesh()->RenderData == nullptr)
		return;

	auto staticMesh = staticMeshComp->GetStaticMesh();

	auto& VertexBuffer = staticMesh->RenderData->LODResources[0].VertexBuffers.PositionVertexBuffer;
	auto& IndexBuffer = staticMesh->RenderData->LODResources[0].IndexBuffer;
	uint32 VertexCount = VertexBuffer.GetNumVertices();
	uint32 IndicesCount = IndexBuffer.GetNumIndices();

	for (uint32 vertexIndex = 0; vertexIndex < VertexCount; vertexIndex++)
	{
		auto vertexPosition = VertexBuffer.VertexPosition(vertexIndex);
		auto normal = staticMesh->RenderData->LODResources[0].VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(vertexIndex);
		normal.Normalize();
		auto tangent = staticMesh->RenderData->LODResources[0].VertexBuffers.StaticMeshVertexBuffer.VertexTangentY(vertexIndex);
		auto texCoord = staticMesh->RenderData->LODResources[0].VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(vertexIndex, 0);

		OriginalVertices.Add(vertexPosition);
		ConvexVertices.Add(vertexPosition);
		CurrentVertices.Add(FRuntimeMeshVertexSimple(vertexPosition, normal, FRuntimeMeshTangent(tangent), FColor::White, texCoord));
	}
	for (uint32 indiceIndex = 0; indiceIndex < IndicesCount; indiceIndex++)
	{
		Triangles.Add(IndexBuffer.GetIndex(indiceIndex));
	}

	FRuntimeMeshDataPtr Data = runtimeMeshComp->GetOrCreateRuntimeMesh()->GetRuntimeMeshData();
	Data->EnterSerializedMode();
	Data->CreateMeshSection(0, false, false, 1, false, false, EUpdateFrequency::Frequent);

	auto Section = Data->BeginSectionUpdate(0);

	Data->CreateMeshSection(0, CurrentVertices, Triangles, false, EUpdateFrequency::Frequent);
	//runtimeMeshComp->GetOrCreateRuntimeMesh()->CreateMeshSection(0, CurrentVertices, Triangles, true, EUpdateFrequency::Frequent);
	runtimeMeshComp->SetMaterial(0, staticMeshComp->GetMaterial(0));
	Data->AddConvexCollisionSection(ConvexVertices);

	Section->Commit();

	staticMeshComp->SetActive(false);
	staticMeshComp->SetCollisionProfileName(TEXT("NoCollision"));
	staticMeshComp->SetVisibility(false);
}

void AProceduralMeshDenting::AddDent(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{

	//Take the length of the impulse vector
	float impulseStrength = NormalImpulse.Size();
	if (impulseStrength <= MinImpactForce)
		return;
	impulseStrength /= ImpulseDiminisher;

	//Normalize the impulse vector
	FVector normalizedImpactForce = NormalImpulse;
	normalizedImpactForce.Normalize();

	FRotator runtimeMeshCompRotation = FRotator(runtimeMeshComp->GetComponentRotation().Euler().X, runtimeMeshComp->GetComponentRotation().Euler().Y, runtimeMeshComp->GetComponentRotation().Euler().Z);

	for (int32 i = 0; i < CurrentVertices.Num(); i++)
	{
		FVector currentVertex = CurrentVertices[i].Position;
		FVector currentRotatedVertex = runtimeMeshCompRotation.RotateVector(currentVertex);

		const FVector WorldVertexPosition = currentRotatedVertex + runtimeMeshComp->GetComponentLocation();
		const float Distance = FVector::Dist(WorldVertexPosition, Hit.ImpactPoint);
		if (Distance <= DentRadius)
		{
			const float interpolation = FMath::Abs((Distance / DentRadius) - 1.f);
			const float interpolatedImpulseStrength = impulseStrength * interpolation;

			float VertexMovedDistance = FVector::Dist(OriginalVertices[i], CurrentVertices[i].Position);
			if(VertexMovedDistance <= MaximumVertexDistance)
			{
				CurrentVertices[i].Position -= interpolatedImpulseStrength * normalizedImpactForce;
				ConvexVertices[i] = CurrentVertices[i].Position;
			}
		}
	}

	FRuntimeMeshDataPtr Data = runtimeMeshComp->GetOrCreateRuntimeMesh()->GetRuntimeMeshData();
	Data->EnterSerializedMode();
	auto Section = Data->BeginSectionUpdate(0);

	Data->UpdateMeshSection(0, CurrentVertices);

	Data->ClearConvexCollisionSections();
	Data->AddConvexCollisionSection(ConvexVertices);

	Section->Commit();
}