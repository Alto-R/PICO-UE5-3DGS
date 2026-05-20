// Copyright® 2015-2023 PICO Technology Co., Ltd. All rights reserved.
// This plugin incorporates portions of the Unreal® Engine. Unreal® is a trademark or registered trademark of Epic Games, Inc. in the United States of America and elsewhere.
// Unreal® Engine, Copyright 1998 – 2023, Epic Games, Inc. All rights reserved. 

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "PICO_MRTypes.h"
#include "PICO_SpatialMeshComponent.generated.h"

UCLASS(hidecategories = (Object, LOD), meta = (BlueprintSpawnableComponent), ClassGroup = Rendering)
class USpatialMeshComponentPICO : public UProceduralMeshComponent
{
	GENERATED_BODY()
public:

	USpatialMeshComponentPICO(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintPure, Category = "PICO|MR")
	ESemanticLabelPICO GetSemanticByIndex(int32 Index);

	void AddIndexToSemanticLabel(int32 Index,ESemanticLabelPICO SceneLabel);

	void SetUpdateTime(uint64 Time);

	int64 GetUpdateTime() const;
	
	TArray<int32> GetCachedIndices() { return CachedIndices; };

	bool IsEqualWithCached(TArray<int32>& Indices);

	void SetCachedIndices(const TArray<int32>& Indices) {CachedIndices = Indices; };
	
protected:
	TMap<int32,ESemanticLabelPICO> IndexToAnchorSceneLabelMap; //Index ->SceneLabel
	uint64 LastUpdateTime = 0;
	
	TArray<int32> CachedIndices;
};