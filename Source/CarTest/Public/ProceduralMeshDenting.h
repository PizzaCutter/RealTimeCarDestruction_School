// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RuntimeMeshComponent.h"
//#include "RuntimeMeshActor.h"
#include "ProceduralMeshDenting.generated.h"

class UStaticMeshComponent;

UCLASS()
class CARTEST_API AProceduralMeshDenting : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AProceduralMeshDenting();

protected:
	void CopyStaticMesh();
public:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& transform) override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
		void AddDent(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);
private:
	TArray<FVector> OriginalVertices;
	TArray<FVector> ConvexVertices;
	TArray<FRuntimeMeshVertexSimple> CurrentVertices;
	TArray<int32> Triangles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Denting", meta = (AllowPrivateAccess = "true"))
		float DentRadius = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact", meta = (AllowPrivateAccess = "true"))
		float MinImpactForce = 50000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact", meta = (AllowPrivateAccess = "true"))
		float ImpulseDiminisher = 5000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact", meta = (AllowPrivateAccess = "true"))
		float MaximumVertexDistance = 50.f;

public:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Mesh")
	class URuntimeMeshComponent* runtimeMeshComp = nullptr;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Mesh")
	UStaticMeshComponent* staticMeshComp = nullptr;
};
