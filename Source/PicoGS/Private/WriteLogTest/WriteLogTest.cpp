// Fill out your copyright notice in the Description page of Project Settings.


#include "WriteLogTest/WriteLogTest.h"


void UWriteLogTest::WriteLog(const FString& LogText, const FString& FilePath)
{
	if (!FPaths::FileExists(FilePath))
	{
		FFileHelper::SaveStringToFile(TEXT(""), *FilePath);
	}
	FFileHelper::SaveStringToFile(LogText + LINE_TERMINATOR, *FilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), EFileWrite::FILEWRITE_Append);
}

void UWriteLogTest::HttpPost(const FString& LogText)
{
    // 创建HTTP请求
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();

    // 设置请求URL
    HttpRequest->SetURL("http://219.223.206.227:5000/api/UploadResult");

    // 设置请求方法为POST
    HttpRequest->SetVerb("POST");

    // 设置请求头
    HttpRequest->SetHeader("Content-Type", "application/json");

    // 构建JSON数据
    FString JsonContent = FString::Printf(TEXT("{\"log\":\"%s\"}"), *LogText);
    HttpRequest->SetContentAsString(JsonContent);

    // 设置完成回调
    //HttpRequest->OnProcessRequestComplete().BindUObject(this, &UWriteLogTest::OnResponseReceived);

    // 发送请求
    HttpRequest->ProcessRequest();

    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue,
        FString::Printf(TEXT("Send_HttpPost")));
}

TArray<FVector> UWriteLogTest::GetStaticMeshVertexCoordinates(UStaticMeshComponent* MeshComponent)
{
    TArray<FVector> Vertices;

    if (!MeshComponent || !MeshComponent->GetStaticMesh())
        return Vertices;

    UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh();
    FTransform ComponentTransform = MeshComponent->GetComponentTransform();

    // 使用StaticMesh直接访问LOD资源
    // 注意：这是较低级别的API访问，可能在未来版本中变化
    if (StaticMesh->GetRenderData() &&
        StaticMesh->GetRenderData()->LODResources.Num() > 0)
    {
        const FStaticMeshLODResources& LODModel = StaticMesh->GetRenderData()->LODResources[0];
        const FPositionVertexBuffer& PositionBuffer = LODModel.VertexBuffers.PositionVertexBuffer;

        const int32 VertexCount = PositionBuffer.GetNumVertices();
        Vertices.Reserve(VertexCount);

        for (int32 i = 0; i < VertexCount; i++)
        {
            FVector3f Pos3f = PositionBuffer.VertexPosition(i);
            FVector Pos(Pos3f.X, Pos3f.Y, Pos3f.Z); // 显式转换
            Vertices.Add(ComponentTransform.TransformPosition(Pos));
        }
    }

    return Vertices;
}

void UWriteLogTest::GetAllStaticMeshesVertices(UWorld* World, TArray<FMeshData>& AllMeshesData)
{
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor);
        if (MeshActor && MeshActor->GetStaticMeshComponent())
        {
            UStaticMeshComponent* MeshComp = MeshActor->GetStaticMeshComponent();

            FMeshData MeshData;
            MeshData.ActorName = MeshActor->GetName();
            //MeshData.ActorLabel = MeshActor->GetActorLabel();
            MeshData.ActorLabel = MeshActor->GetActorNameOrLabel();
            MeshData.ActorLocation = MeshActor->GetActorLocation();
            MeshData.ActorRotation = MeshActor->GetActorRotation();
            MeshData.ActorScale = MeshActor->GetActorScale();

            // 获取静态网格体资源名称
            if (MeshComp->GetStaticMesh())
            {
                MeshData.MeshName = MeshComp->GetStaticMesh()->GetName();
                MeshData.BoundingBox = MeshComp->Bounds.GetBox();
            }

            // 获取顶点
            MeshData.Vertices = GetStaticMeshVertexCoordinates(MeshComp);

            AllMeshesData.Add(MeshData);
        }
    }
}

static FAutoConsoleCommand CmdCaptureMeshData(
    TEXT("CaptureMeshData.MeshData"),    // 命令名称
    TEXT("Capture level all Meshdata"), // 命令描述
    FConsoleCommandDelegate::CreateLambda([]()
        {
            // 获取当前世界
            UWorld* World = GEngine->GetWorldFromContextObject(
                GEngine->GetCurrentPlayWorld(),
                EGetWorldErrorMode::LogAndReturnNull);

            if (!World)
            {
                UE_LOG(LogTemp, Error, TEXT("Can't load The World"));
                return;
            }

            // 创建WriteLogTest对象
            UWriteLogTest* LogTest = NewObject<UWriteLogTest>();
            if (!LogTest) return;

            // 获取所有网格顶点数据
            TArray<FMeshData> AllMeshesData;
            LogTest->GetAllStaticMeshesVertices(World, AllMeshesData);

            // 生成输出文件名(带时间戳)
            FDateTime Now = FDateTime::Now();
            FString Timestamp = Now.ToString(TEXT("%Y%m%d_%H%M%S"));
            /*FString FilePath = FPaths::ProjectSavedDir() / FString::Printf(
                TEXT("MeshData_%s.txt"), *Timestamp);*/
            FString FilePath = FPaths::Combine(
                TEXT("C:/Users/a/Desktop/"),
                FString::Printf(TEXT("MeshData_%s.txt"), *Timestamp)
            );

            // 构建输出文本
            FString ResultText;
            int32 TotalVertices = 0;

            // 添加标题
            ResultText += FString::Printf(TEXT("MeshData Out - %s\n\n"),
                *FDateTime::Now().ToString());

            // 处理每个网格体数据
            for (int32 Index = 0; Index < AllMeshesData.Num(); Index++)
            {
                const FMeshData& Data = AllMeshesData[Index];

                ResultText += FString::Printf(TEXT("==== Meshs #%d ====\n"), Index + 1);
                ResultText += FString::Printf(TEXT("Name: %s\n"), *Data.ActorLabel);
                ResultText += FString::Printf(TEXT("Actor Name: %s\n"), *Data.ActorName);
                ResultText += FString::Printf(TEXT("MeshResources: %s\n"), *Data.MeshName);
                ResultText += FString::Printf(TEXT("3DLocation: X=%.2f, Y=%.2f, Z=%.2f\n"),
                    Data.ActorLocation.X, Data.ActorLocation.Y, Data.ActorLocation.Z);
                ResultText += FString::Printf(TEXT("Rotation: Pitch=%.2f, Yaw=%.2f, Roll=%.2f\n"),
                    Data.ActorRotation.Pitch, Data.ActorRotation.Yaw, Data.ActorRotation.Roll);
                ResultText += FString::Printf(TEXT("Zoom: X=%.2f, Y=%.2f, Z=%.2f\n"),
                    Data.ActorScale.X, Data.ActorScale.Y, Data.ActorScale.Z);
                ResultText += FString::Printf(TEXT("Bounding Box: Min(%.2f,%.2f,%.2f) Max(%.2f,%.2f,%.2f)\n"),
                    Data.BoundingBox.Min.X, Data.BoundingBox.Min.Y, Data.BoundingBox.Min.Z,
                    Data.BoundingBox.Max.X, Data.BoundingBox.Max.Y, Data.BoundingBox.Max.Z);
                ResultText += FString::Printf(TEXT("Vertices: %d\n\n"), Data.Vertices.Num());

                TotalVertices += Data.Vertices.Num();

                // 输出部分顶点
                for (int32 i = 0; i < FMath::Min(10, Data.Vertices.Num()); i++)
                {
                    ResultText += FString::Printf(TEXT("  Vertices[%d]: X=%.2f, Y=%.2f, Z=%.2f\n"),
                        i, Data.Vertices[i].X, Data.Vertices[i].Y, Data.Vertices[i].Z);
                }

                if (Data.Vertices.Num() > 10)
                {
                    ResultText += TEXT("  ...(More)...\n");
                }

                ResultText += TEXT("\n");
            }

            // 添加摘要信息
            ResultText += FString::Printf(TEXT("Total: %d Mesh, %d Vertex"),
                AllMeshesData.Num(), TotalVertices);

            // 保存到文件
            if (FFileHelper::SaveStringToFile(ResultText, *FilePath))
            {
                UE_LOG(LogTemp, Log, TEXT("Save in: %s"), *FilePath);

                // 在屏幕上显示确认信息
                if (GEngine)
                {
                    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green,
                        FString::Printf(TEXT("Saved %d Meshs"),
                            AllMeshesData.Num()));
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Can't Save The File: %s"), *FilePath);
            }
        })
);

void UWriteLogTest::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConncetedSucessfully)
{
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,
        FString::Printf(TEXT("Http_Success")));
}