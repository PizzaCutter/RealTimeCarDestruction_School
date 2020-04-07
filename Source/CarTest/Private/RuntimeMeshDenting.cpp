// Fill out your copyright notice in the Description page of Project Settings.

#include "RuntimeMeshDenting.h"
#include "RuntimeMeshComponent.h"
#include "RuntimeMeshShapeGenerator.h"
#include "RuntimeMeshBuilder.h"
#include "RuntimeMeshLibrary.h"
#include "RuntimeMeshData.h"
#include "RuntimeMesh.h"
#include "ProceduralMeshDenting.h"
#include "Runtime/Engine/Classes/GameFramework/Actor.h"

ARuntimeMeshDenting::ARuntimeMeshDenting()
	: BoxSize(100.f, 100.f, 100.f)
{
	PrimaryActorTick.bCanEverTick = true;

	staticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh Component To Copy"));
	staticMeshComp->SetCollisionProfileName(TEXT("NoCollision"));
	staticMeshComp->SetVisibility(false);
	GetRuntimeMeshComponent()->SetMobility(EComponentMobility::Movable);
}

void ARuntimeMeshDenting::GenerateMeshes_Implementation()
{
	Super::GenerateMeshes_Implementation();
	CopyStaticMesh();
	GetRuntimeMeshComponent()->SetMobility(EComponentMobility::Movable);
}

void ARuntimeMeshDenting::BeginPlay()
{
	Super::BeginPlay();
	CopyStaticMesh();

	GetRuntimeMeshComponent()->SetCollisionUseComplexAsSimple(UseComplexCollision);
	GetRuntimeMeshComponent()->SetSimulatePhysics(!UseComplexCollision);

	GetRuntimeMeshComponent()->OnComponentHit.AddDynamic(this, &ARuntimeMeshDenting::AddDent);
	GetRuntimeMeshComponent()->SetMobility(EComponentMobility::Movable);
}

void ARuntimeMeshDenting::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ARuntimeMeshDenting::CopyStaticMesh()
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

	FRuntimeMeshDataPtr Data = GetRuntimeMeshComponent()->GetOrCreateRuntimeMesh()->GetRuntimeMeshData();
	Data->EnterSerializedMode();

	Data->CreateMeshSection(0, false, false, 1, false, UseComplexCollision, EUpdateFrequency::Frequent);
	auto Section = Data->BeginSectionUpdate(0);

	Data->CreateMeshSection(0, CurrentVertices, Triangles, UseComplexCollision, EUpdateFrequency::Frequent);
	if (!UseComplexCollision)
	{
		Data->AddConvexCollisionSection(ConvexVertices);
	}

	Section->Commit();

	GetRuntimeMeshComponent()->SetMaterial(0, staticMeshComp->GetMaterial(0));
	staticMeshComp->SetActive(false);
}

void ARuntimeMeshDenting::AddDent(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	//DISABLE DENTING
	if (DisableDenting)
	{
		UE_LOG(LogTemp, Warning, TEXT("Denting disabled"));
		return;
	}

	//MAP IMPACT FORCE BETWEEN 0-1 BASED ON MIN/MAX IMPACT FORCE
	float normalizedImpactForce = NormalImpulse.Size();
	normalizedImpactForce = FMath::Clamp(normalizedImpactForce, 0.f, MaxImpactForce - MinImpactForce);
	normalizedImpactForce = normalizedImpactForce / MaxImpactForce;
	FVector normalizedImpactImpulse = NormalImpulse.GetSafeNormal();

	//APPLY IMPULSE ON STATIC MESH PARENT
	if (staticMeshParent != nullptr)
	{
		auto primitiveComp = Cast<UPrimitiveComponent>(staticMeshParent);
		if (primitiveComp != nullptr)
		{
			auto impulseVelocity = normalizedImpactImpulse * (normalizedImpactForce * PhysicsForce);

			UE_LOG(LogTemp, Log, TEXT("ImpulseVelocity: %s"), *impulseVelocity.ToString());
			impulseVelocity *= -1.f;
			UE_LOG(LogTemp, Log, TEXT("InvertedImpulseVelocity: %s"), *impulseVelocity.ToString());
			primitiveComp->AddImpulse(impulseVelocity, NAME_None, true);
		}
	}

	//MOVE VERTICES BASED ON IMPACT
	//calculate a rotation matrix
	const FRotator RuntimeMeshComponentRotation = FRotator(GetRuntimeMeshComponent()->GetComponentRotation().Euler().Y, GetRuntimeMeshComponent()->GetComponentRotation().Euler().Z, GetRuntimeMeshComponent()->GetComponentRotation().Euler().X);
	const FRotationTranslationMatrix RotationMatrix = FRotationTranslationMatrix(RuntimeMeshComponentRotation, FVector(0.f, 0.f, 0.f));

	for (int32 i = 0; i < CurrentVertices.Num(); i++)
	{
		//check the total vertex displacment
		const float totalVertexMoveDistance = FVector::Dist(OriginalVertices[i], CurrentVertices[i].Position);
		if (totalVertexMoveDistance >= MaximumVertexDistance)
			continue;

		//rotate the local vertex
		FVector currentRotatedVertex = RotationMatrix.TransformFVector4(CurrentVertices[i].Position);
		//convert the rotated local vertex to world position
		const FVector currentRotatedWorldVertexPosition = currentRotatedVertex + GetRuntimeMeshComponent()->GetComponentLocation();

		//calculate distance between impact location and the current vertex in world position
		const float DistanceBetweenImpactAndVertex = FVector::Dist(currentRotatedWorldVertexPosition, Hit.ImpactPoint);
		if (DistanceBetweenImpactAndVertex <= DentRadius)
		{
			const float interpolation = FMath::Abs((DistanceBetweenImpactAndVertex / DentRadius) - 1.f);

			//CurrentVertices[i].Position -= ((interpolation * VertexMoveDistance) * normalizedImpactForce) * normalizedImpactImpulse;
			CurrentVertices[i].Position -= ((interpolation * VertexMoveDistance) * normalizedImpactForce) * normalizedImpactImpulse;
			ConvexVertices[i] = CurrentVertices[i].Position;
		}
	}

	//UPDATE THE RUNTIME MESH
	FRuntimeMeshDataPtr Data = GetRuntimeMeshComponent()->GetOrCreateRuntimeMesh()->GetRuntimeMeshData();
	Data->EnterSerializedMode();
	auto Section = Data->BeginSectionUpdate(0);

	Data->UpdateMeshSection(0, CurrentVertices);

	if (!UseComplexCollision)
	{
		Data->ClearConvexCollisionSections();
		Data->AddConvexCollisionSection(ConvexVertices);
	}

	Section->Commit();
}