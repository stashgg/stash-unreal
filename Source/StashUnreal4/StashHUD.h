#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "StashHUD.generated.h"

/**
 * StashHUD
 * 
 * A simple HUD that displays a checkout button.
 * When the button is clicked, it opens the Stash Pay checkout dialog.
 */
UCLASS()
class STASHUNREAL4_API AStashHUD : public AHUD
{
    GENERATED_BODY()

public:
    AStashHUD();

protected:
    virtual void BeginPlay() override;

    /** The checkout URL to open when the button is clicked */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "StashPay")
    FString CheckoutURL;

    /** Status text displayed below the button */
    UPROPERTY(BlueprintReadWrite, Category = "StashPay")
    FString StatusText;

private:
    /** The widget class to use for the checkout UI (set in Blueprint) */
    UPROPERTY(EditDefaultsOnly, Category = "StashPay")
    TSubclassOf<class UUserWidget> CheckoutWidgetClass;

    /** The instantiated widget */
    UPROPERTY()
    class UUserWidget* CheckoutWidget;
};
