using System.Runtime.InteropServices;
using UnityEngine;

public static class AudioSDKBridge
{
    private const string DLL_NAME = "HUSTAudioSDK";

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern int AudioSDK_Init();

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void AudioSDK_Shutdown();

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    public static extern int AudioSDK_LoadSound(
        [MarshalAs(UnmanagedType.LPStr)] string filepath
    );

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern int AudioSDK_LoadSoundFromMemory(
        [In] byte[] data,
        int dataSize
    );

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void AudioSDK_Play();

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void AudioSDK_Stop();

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void AudioSDK_SetLooping(int loop);

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void AudioSDK_SetSourcePosition(float x, float y, float z);

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void AudioSDK_SetSourceVelocity(float x, float y, float z);

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void AudioSDK_SetListenerPosition(float x, float y, float z);

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void AudioSDK_SetListenerVelocity(float x, float y, float z);

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void AudioSDK_SetListenerOrientation(
        float fx, float fy, float fz,
        float ux, float uy, float uz
    );

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void AudioSDK_SetReverb(int enabled);

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void AudioSDK_SetOcclusion(float amount);

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern int AudioSDK_IsHRTFEnabled();

    public static Vector3 UnityToSDKPosition(Vector3 unityPosition)
    {
        return new Vector3(
            unityPosition.x,
            unityPosition.y,
            -unityPosition.z
        );
    }

    public static Vector3 UnityToSDKDirection(Vector3 unityDirection)
    {
        return new Vector3(
            unityDirection.x,
            unityDirection.y,
            -unityDirection.z
        );
    }
}
