// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Http.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "StaticMeshResources.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"

#include "RenderingThread.h"
#include "StaticMeshResources.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"

#include "WriteLogTest.generated.h"

/**
 * 
 */

USTRUCT()
struct FMeshData
{
	GENERATED_BODY()

	UPROPERTY()
	FString MeshName;          // ��̬��������Դ����

	UPROPERTY()
	FString ActorName;         // Actor����

	UPROPERTY()
	FString ActorLabel;        // �༭������ʾ�ı�ǩ

	UPROPERTY()
	FVector ActorLocation;     // λ��

	UPROPERTY()
	FVector ActorScale;        // ����

	UPROPERTY()
	FRotator ActorRotation;    // ��ת

	UPROPERTY()
	FBox BoundingBox;          // �߽��

	UPROPERTY()
	TArray<FVector> Vertices;  // �����б�
};

UCLASS()
class PICOGS_API UWriteLogTest : public UObject
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Logging")
	static void WriteLog(const FString& LogText, const FString& FilePath);

	UFUNCTION(BlueprintCallable, Category = "Logging")
	static void HttpPost(const FString& LogText);

	//UFUNCTION(BluePrintCallable, Category = "Logging")
	//static void WriteDatas(const FString& Data1, const FString& Data2);

	//UFUNCTION(BluePrintCallable, Category = "Logging")
	//static void ReadDatas(const FString& address);

	UFUNCTION(BlueprintCallable, Category = "Mesh Analysis")
	static TArray<FVector> GetStaticMeshVertexCoordinates(UStaticMeshComponent* MeshComponent);

	void GetAllStaticMeshesVertices(UWorld* World, TArray<FMeshData>& AllMeshesData);

private:
	void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConncetedSucessfully);
};
