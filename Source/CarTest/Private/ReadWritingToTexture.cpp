// Fill out your copyright notice in the Description page of Project Settings.
#include "ReadWritingToTexture.h"
#include "PackageName.h"

#include "Engine/Texture2D.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"


// Sets default values
AReadWritingToTexture::AReadWritingToTexture()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	staticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh Component"));
	staticMeshComp->BodyInstance.SetCollisionProfileName("BlockAllDynamic");
	staticMeshComp->SetSimulatePhysics(false);
	staticMeshComp->SetEnableGravity(false);
	staticMeshComp->SetNotifyRigidBodyCollision(true);
	staticMeshComp->OnComponentHit.AddDynamic(this, &AReadWritingToTexture::AddDent);

}

// Called when the game starts or when spawned
void AReadWritingToTexture::BeginPlay()
{
	Super::BeginPlay();

	LocalBoundingRadius = staticMeshComp->Bounds.SphereRadius;
	LocalMinExtends = staticMeshComp->Bounds.GetBoxExtrema(1) - staticMeshComp->GetComponentLocation();
	LocalMaxExtends = staticMeshComp->Bounds.GetBoxExtrema(0) - staticMeshComp->GetComponentLocation();

	if(LocalMinExtends.X > LocalMaxExtends.X)
	{
		float temp = LocalMaxExtends.X;
		LocalMaxExtends.X = LocalMinExtends.X;
		LocalMinExtends.X = temp;
	}
	if (LocalMinExtends.Y > LocalMaxExtends.Y)
	{
		float temp = LocalMaxExtends.Y;
		LocalMaxExtends.Y = LocalMinExtends.Y;
		LocalMinExtends.Y = temp;
	}
	if (LocalMinExtends.Z > LocalMaxExtends.Z)
	{
		float temp = LocalMaxExtends.Z;
		LocalMaxExtends.Z = LocalMinExtends.Z;
		LocalMinExtends.Z = temp;
	}

	xDivider = 255.f / (FMath::Abs(LocalMinExtends.X) + FMath::Abs(LocalMaxExtends.X));
	yDivider = 255.f / (FMath::Abs(LocalMinExtends.Y) + FMath::Abs(LocalMaxExtends.Y));
	zDivider = 255.f / (FMath::Abs(LocalMinExtends.Z) + FMath::Abs(LocalMaxExtends.Z));

	DynMaterial = UMaterialInstanceDynamic::Create(staticMeshComp->GetMaterial(0), this);
	staticMeshComp->SetMaterial(0, DynMaterial);

	DynMaterial->SetScalarParameterValue("TextureWidth", static_cast<float>(width));
	DynMaterial->SetScalarParameterValue("TextureHeight", static_cast<float>(height));
	DynMaterial->SetScalarParameterValue("XDivider", xDivider);
	DynMaterial->SetScalarParameterValue("YDivider", yDivider);
	DynMaterial->SetScalarParameterValue("ZDivider", zDivider);
	DynMaterial->SetVectorParameterValue("AbsMinExtends", FLinearColor(FMath::Abs(LocalMinExtends.X), FMath::Abs(LocalMinExtends.Y), FMath::Abs(LocalMinExtends.Z)));

	ClearTexture();
}

// Called every frame
void AReadWritingToTexture::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AReadWritingToTexture::ClearTexture(FColor clearColor)
{
	CurPixelColors.Empty();
	for (int32 i = 0; i < width * height; i++)
	{
		CurPixelColors.Add(clearColor);
	}

	texture = UTexture2D::CreateTransient(width, height);
	FTexture2DMipMap& mip = texture->PlatformData->Mips[0];
	texture->Filter = TF_Nearest;
	//texture->PlatformData->PixelFormat = PF_FloatRGB;
	void* data = mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(data, CurPixelColors.GetData(), (width * height * channels));
	mip.BulkData.Unlock();
	texture->UpdateResource();

	DynMaterial->SetTextureParameterValue(TEXT("Texture"), texture);
}


//TO-DO: FIX READING OF TEXTURE
void AReadWritingToTexture::ReadTexture()
{
	CurPixelColors.Empty();
	uint8* raw = NULL;
	FTexture2DMipMap& Mip = texture->PlatformData->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	raw = (uint8*)Data;
	TArray<FColor> rawData;

	for (int32 i = 0; i < width * height; i+=4)
	{
		CurPixelColors.Add(FColor(raw[i + 0], raw[i + 1], raw[i + 2], raw[i + 3]));
	}

	Mip.BulkData.Unlock();
	texture->UpdateResource();
}

void AReadWritingToTexture::WriteCurPixelsToTexture()
{
	FTexture2DMipMap& mip = texture->PlatformData->Mips[0];
	void* data = mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(data, CurPixelColors.GetData(), (width * height * channels));
	mip.BulkData.Unlock();
	texture->UpdateResource();

	//DynMaterial->SetTextureParameterValue(TEXT("Texture"), texture);
}

void AReadWritingToTexture::AddDent(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	FVector localizedImpactPosition = Hit.ImpactPoint - staticMeshComp->GetComponentLocation();
	FVector normalizedImpactPositions;
	normalizedImpactPositions.X = (localizedImpactPosition.X + FMath::Abs(LocalMinExtends.X)) * xDivider;
	normalizedImpactPositions.Y = (localizedImpactPosition.Y + FMath::Abs(LocalMinExtends.Y)) * yDivider;
	normalizedImpactPositions.Z = (localizedImpactPosition.Z + FMath::Abs(LocalMinExtends.Z)) * zDivider;

	CurPixelColors[curIndex].R = normalizedImpactPositions.X;
	CurPixelColors[curIndex].G = normalizedImpactPositions.Y;
	CurPixelColors[curIndex].B = normalizedImpactPositions.Z;
	CurPixelColors[curIndex].A = 255;

	curIndex++;
	curIndex %= (width * height);

	WriteCurPixelsToTexture();
}
