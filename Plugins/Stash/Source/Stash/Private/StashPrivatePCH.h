// Copyright Stash. All Rights Reserved.
// Private precompiled header for the Stash module.
//
// The iOS build enables Objective-C ARC for this module (see Stash.Build.cs). ARC is not part of
// UBT's shared-PCH signature, so reusing an engine SharedPCH built ARC-off fails with
// "Objective-C automated reference counting was disabled in precompiled file ... but is currently
// enabled". Using an explicit private PCH makes the module compile its own PCH in its own (ARC-on)
// environment, avoiding the mismatch while retaining PCH speed.

#pragma once

#include "CoreMinimal.h"
