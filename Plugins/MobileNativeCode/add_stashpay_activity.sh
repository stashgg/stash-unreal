#!/bin/bash

# Script to add StashPayCardPortraitActivity to AndroidManifest.xml
# This runs after Unreal generates the manifest

MANIFEST_FILE="$1"

if [ -z "$MANIFEST_FILE" ]; then
    # Try to find the manifest file
    MANIFEST_FILE="Intermediate/Android/arm64/gradle/app/src/main/AndroidManifest.xml"
fi

if [ ! -f "$MANIFEST_FILE" ]; then
    echo "Manifest file not found: $MANIFEST_FILE"
    exit 1
fi

# Check if activity already exists
if grep -q "com.stash.popup.StashPayCardPortraitActivity" "$MANIFEST_FILE"; then
    echo "StashPayCardPortraitActivity already exists in manifest"
    exit 0
fi

# Add the activity before the closing </application> tag
sed -i.bak '/<\/application>/i\
    <activity android:name="com.stash.popup.StashPayCardPortraitActivity"\
              android:theme="@style/StashPayCardTheme"\
              android:screenOrientation="portrait"\
              android:configChanges="orientation|screenSize|keyboardHidden"\
              android:exported="false"/>
' "$MANIFEST_FILE"

echo "Added StashPayCardPortraitActivity to manifest"
