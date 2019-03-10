// Fill out your copyright notice in the Description page of Project Settings.

#include "GameManager.h"
#include "EngineUtils.h"

#include "PlayerRepresentation.h"
#include "ServerCommandManager/Server.h"
#include "PlayerInfo.h"
#include "HexCoord.h"
#include "EscapeFTAliens/ServerCommandManager/Request/GetRequest.h"
#include "EscapeFTAliens/ServerCommandManager/Request/PostRequest.h"

// Sets default values
AGameManager::AGameManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void AGameManager::MovePlayer(AHexBlock* block)
{
	PlayerRepresentation->updatePosition(block);
}

// Called when the game starts or when spawned
void AGameManager::BeginPlay()
{
	GameState_ = GameState::Connecting;
	RoundState_ = RoundState::NotConnected;

	Super::BeginPlay();
	OnPlayerMoveRequest.AddDynamic(this, &AGameManager::MovePlayer);

	for (TActorIterator<APlayerRepresentation> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		// Same as with the Object Iterator, access the subclass instance with the * or -> operators.
		APlayerRepresentation *player = *ActorItr;
		FString actorName = player->GetName();

		if (actorName.Compare("Player") == 0)
		{
			PlayerRepresentation = player;
		}
	}

	for (TActorIterator<AServerManager> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		// Same as with the Object Iterator, access the subclass instance with the * or -> operators.
		ServerManager = *ActorItr;
	}

	for (TActorIterator<AGridManager> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		// Same as with the Object Iterator, access the subclass instance with the * or -> operators.
		gridManager_ = *ActorItr;
	}

	if (!gridManager_)
	{
		gridManager_ = GetWorld()->SpawnActor<AGridManager>(FVector(0,0,0) , FRotator(0, 0, 0));

	}
}


// Called every frame
void AGameManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	switch (GameState_)
	{
		case GameState::Connecting:
		{
			UE_LOG(LogTemp, Warning, TEXT("Connecting..."));

			bool waitingGameRequest = false;

			for (TSharedPtr<HttpRequest> call : WaitCalls_)
			{
				if (call->getNameRequest() == HttpRequest::NameRequest::CreateGame)
				{
					waitingGameRequest = true;

					if (call->receivedResponse())
					{
						PostGameRequest* postgameRequest = (PostGameRequest*)(call.Get());
						if (postgameRequest)
						{
							postgameRequest->getMap();
							if (postgameRequest->getMap().IsValid())
							{
								gridManager_->GenerateMap(postgameRequest->getMap());

								call->setHandledResponse(true);

								GameState_ = GameState::Playing;
							}
							else
							{
								UE_LOG(LogTemp, Error, TEXT("Received response from server, but no map parsed... Retrying..."));

								call->updateIsExpired(true);
							}
						}
					}
					else
					{
						if ( FDateTime::Now() > call->getDateTimeExpiration() )
						{
							call->updateIsExpired(true);
						}
					}
				}
			}

			if (!waitingGameRequest)
			{
				TSharedPtr<PostGameRequest> postGame(new PostGameRequest());
				ServerManager->sendCall(postGame);
				WaitCalls_.Add(postGame);
			}

			break;
		}

		case GameState::Playing:
		{
			UE_LOG(LogTemp, Warning, TEXT("Playing!"));
			break;
		}

		case GameState::EndGame:
		{
			break;
		}

		default:
		{
			break;
		}
		
	}

	for (int i = WaitCalls_.Num() - 1; i >= 0; --i)
	{
		if (WaitCalls_[i]->isHandled() || WaitCalls_[i]->isExpired())
		{
			WaitCalls_.RemoveAt(i);
		}
	}
}

