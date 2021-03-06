﻿#if UNITY_IOS
using UnityEditor.Callbacks;
using UnityEditor;
using UnityEditor.iOS.Xcode;
using System.IO;

public class BluetoothPostProcessBuild
{
	[PostProcessBuild]
	public static void ChangeXcodePlist(BuildTarget buildTarget, string pathToBuiltProject)
	{
		if (buildTarget == BuildTarget.iOS)
		{
			// Get plist
			string plistPath = pathToBuiltProject + "/Info.plist";
			PlistDocument plist = new PlistDocument();
			plist.ReadFromString(File.ReadAllText(plistPath));

			// Get root
			PlistElementDict rootDict = plist.root;

			rootDict.SetString("NSBluetoothPeripheralUsageDescription", "Uses BLE to communicate with devices.");
			rootDict.SetString("NSCameraUsageDescription", "Uses the camera to spy on your neighbors.");
			rootDict.SetString("NSMicrophoneUsageDescription", "Uses the microphone to spy on your neighbors.");
			rootDict.SetString("NSPhotoLibraryAddUsageDescription", "Drops malware into your documents.");

			// Write to file
			File.WriteAllText(plistPath, plist.WriteToString());
		}
	}
}
#endif
