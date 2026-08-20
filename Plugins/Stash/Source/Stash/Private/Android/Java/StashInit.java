// Copyright Stash. All Rights Reserved.
// Stash Unreal Engine SDK - Android Platform Initialization

package com.Plugins.Stash;

import androidx.annotation.Keep;

/**
 * StashInit - Platform initialization helper
 * 
 * Provides initialization callback for the Stash plugin
 * to verify that Java/JNI communication is working correctly.
 */
@Keep
public class StashInit {

	/**
	 * Platform initialization check.
	 * Called during plugin startup to verify JNI connectivity.
	 * 
	 * @return 1 if initialization successful
	 */
	@Keep
	public static int initialize() {
		return 1;
	}
}
