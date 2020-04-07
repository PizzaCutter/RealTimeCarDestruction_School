// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReadWritingToTexture.generated.h"

UCLASS()
class CARTEST_API AReadWritingToTexture : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AReadWritingToTexture();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* staticMeshComp = nullptr;

	UPROPERTY(EditAnywhere)
		UTexture2D* texture = nullptr;
	UPROPERTY(EditAnywhere)
		int width = 16;
	UPROPERTY(EditAnywhere)
		int height = 16;
	UPROPERTY(EditAnywhere)
		int channels = 4;

	int curIndex = 0;

	float LocalBoundingRadius;
	FVector LocalMinExtends, LocalMaxExtends;
	float xDivider;
	float yDivider;
	float zDivider;


	UFUNCTION()
		void AddDent(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

	TArray<FColor> CurPixelColors;
private:
	void ClearTexture(FColor clearColor = FColor(255, 0, 0));
	void ReadTexture();
	void WriteCurPixelsToTexture();

	UMaterialInstanceDynamic* DynMaterial;
};
