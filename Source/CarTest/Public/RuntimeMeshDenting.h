// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshActor.h"
#include "RuntimeMeshComponent.h"
#include "RuntimeMeshDenting.generated.h"

/**
 *
 */
UCLASS()
class CARTEST_API ARuntimeMeshDenting : public ARuntimeMeshActor
{
	GENERATED_BODY()

public:
	ARuntimeMeshDenting();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variables", meta = (AllowPrivateAccess = "true"))
		FVector BoxSize;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variables", meta = (AllowPrivateAccess = "true"))
		bool UseComplexCollision = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variables", meta = (AllowPrivateAccess = "true"))
		bool DisableDenting = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variables", meta = (AllowPrivateAccess = "true"))
		bool EnableDebugging = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables", meta = (AllowPrivateAccess = "true"))
		float DentRadius = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables", meta = (AllowPrivateAccess = "true"))
		float MinImpactForce = 50000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables", meta = (AllowPrivateAccess = "true"))
		float MaxImpactForce = 5000000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables", meta = (AllowPrivateAccess = "true"))
		float VertexMoveDistance = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables", meta = (AllowPrivateAccess = "true"))
		float PhysicsForce = 10000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables", meta = (AllowPrivateAccess = "true"))
		float MaximumVertexDistance = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables", meta = (AllowPrivateAccess = "true"))
		UStaticMeshComponent* staticMeshComp = nullptr;

	UStaticMeshComponent* staticMeshParent = nullptr;

	TArray<FVector> OriginalVertices;
	TArray<FVector> ConvexVertices;
	TArray<FRuntimeMeshVertexSimple> CurrentVertices;
	TArray<int32> Triangles;

	UFUNCTION()
		void CopyStaticMesh();
	UFUNCTION()
		void AddDent(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);
	UFUNCTION(BlueprintCallable)
		void SetStaticMeshParent(UStaticMeshComponent* newStaticMeshParent)
	{
		staticMeshParent = newStaticMeshParent;
	}

protected:
	virtual void GenerateMeshes_Implementation() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
};
