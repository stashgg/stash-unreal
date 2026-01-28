// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine SDK - Android Platform Initialization

package com.Plugins.MobileNativeCode;

import androidx.annotation.Keep;

/**
 * DeviceInfo - Platform initialization helper
 * 
 * Provides initialization callback for the MobileNativeCode plugin
 * to verify that Java/JNI communication is working correctly.
 */
@Keep
public class DeviceInfo {

	/**
	 * Platform initialization check.
	 * Called during plugin startup to verify JNI connectivity.
	 * 
	 * @return 1 if initialization successful
	 */
	@Keep
	public static int Initialization() {
		return 1;
	}
}
