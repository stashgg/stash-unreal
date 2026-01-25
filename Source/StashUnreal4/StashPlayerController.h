#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "StashPlayerController.generated.h"

/**
 * StashPlayerController
 * 
 * A PlayerController that handles Stash Pay checkout functionality.
 * It binds to the payment callbacks and provides Blueprint-callable functions.
 */
UCLASS()
class STASHUNREAL4_API AStashPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AStashPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    /**
     * Opens the Stash Pay checkout with the specified URL.
     * Automatically uses the correct platform implementation (iOS or Android).
     * 
     * @param CheckoutURL The URL to load in the checkout dialog
     */
    UFUNCTION(BlueprintCallable, Category = "StashPay")
    void OpenCheckout(const FString& CheckoutURL);

    /**
     * Checks if the checkout dialog is currently open.
     * 
     * @return true if the checkout is displayed
     */
    UFUNCTION(BlueprintCallable, Category = "StashPay")
    bool IsCheckoutOpen();

    /**
     * Dismisses the currently open checkout dialog.
     */
    UFUNCTION(BlueprintCallable, Category = "StashPay")
    void DismissCheckout();

    // Blueprint-implementable events for handling payment callbacks
    
    /**
     * Called when a payment completes successfully.
     * Override this in Blueprint to handle successful payments.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "StashPay")
    void OnPaymentSucceeded();

    /**
     * Called when a payment fails.
     * Override this in Blueprint to handle failed payments.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "StashPay")
    void OnPaymentFailed();

    /**
     * Called when the checkout dialog is dismissed by the user.
     * Override this in Blueprint to handle dialog dismissal.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "StashPay")
    void OnCheckoutDismissed();

    /**
     * Called when the checkout page finishes loading.
     * Override this in Blueprint to track load performance.
     * 
     * @param LoadTimeMs The time in milliseconds it took to load the page
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "StashPay")
    void OnCheckoutPageLoaded(float LoadTimeMs);

private:
    // Internal callback handlers bound to the MobileNativeCodeBlueprint delegates
    UFUNCTION()
    void HandlePaymentSuccess();

    UFUNCTION()
    void HandlePaymentFailure();

    UFUNCTION()
    void HandleDialogDismissed();

    UFUNCTION()
    void HandlePageLoaded(float LoadTimeMs);
};
