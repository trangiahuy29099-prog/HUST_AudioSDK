using System.IO;
using UnityEngine;

public class ManualDroneAudioController : MonoBehaviour
{
    private enum MovementMode
    {
        DopplerFlyBy = 0,
        OrbitAroundListener,
        OverheadPass,
        WallOcclusionPass,
        CaveReverbPass,
        FullShowcase
    }

    [Header("Main References")]
    public Transform listenerTransform;       // Kéo AudioListenerPoint vào đây
    public Transform listenerVisualRoot;      // Kéo HumanVisualPivot vào đây
    public Transform droneRoot;
    public Transform emitterPoint;
    public Camera mainCamera;

    [Header("Drone Visual Model")]
    public Transform visualModel;             // Kéo FlyingSirenDrone/Model vào đây
    public bool enableVisualHover = false;
    public float hoverAmplitude = 0.05f;
    public float hoverSpeed = 4.0f;
    public float tiltAmount = 8.0f;
    public float droneYawOffset = 0.0f;

    [Header("Camera Follow")]
    public bool enableFollowCamera = true;
    public Vector3 cameraDirection = new Vector3(0.55f, 0.65f, -0.85f);
    public float minCameraDistance = 35.0f;
    public float maxCameraDistance = 85.0f;
    public float cameraDistanceMultiplier = 1.2f;
    public float cameraLookHeight = 3.5f;
    public float cameraSmooth = 5.0f;
    public float cameraFOV = 70.0f;

    [Header("Optional Visual References")]
    public GameObject occlusionWall;
    public Light redLight;
    public Light blueLight;
    public TrailRenderer droneTrail;
    public Transform[] propellers;

    [Header("Movement Settings")]
    public float orbitRadius = 18.0f;
    public float demoDuration = 8.0f;
    public float speedMultiplier = 1.0f;

    [Header("Audio / Occlusion")]
    public LayerMask occlusionMask = ~0;

    private MovementMode currentMode = MovementMode.DopplerFlyBy;

    private bool sdkReady = false;
    private bool runningDemo = false;
    private bool reverbEnabled = false;
    private bool occlusionEnabled = false;

    private float elapsedTime = 0.0f;

    private Vector3 lastEmitterPosition;
    private Vector3 currentVelocity;

    private Vector3 visualModelInitialLocalPosition;
    private Quaternion visualModelInitialLocalRotation;

    private GUIStyle labelStyle;
    private GUIStyle titleStyle;
    private GUIStyle buttonStyle;

    void Start()
    {
        if (listenerTransform == null)
        {
            Debug.LogError("[ManualDemo] Missing Listener Transform. Kéo PlayerListener/AudioListenerPoint vào đây.");
            return;
        }

        if (listenerVisualRoot == null)
        {
            Debug.LogWarning("[ManualDemo] Missing Listener Visual Root. Q/E sẽ xoay AudioListenerPoint thay vì model người.");
            listenerVisualRoot = listenerTransform;
        }

        if (droneRoot == null)
        {
            Debug.LogError("[ManualDemo] Missing Drone Root.");
            return;
        }

        if (emitterPoint == null)
        {
            Debug.LogError("[ManualDemo] Missing AudioEmitterPoint.");
            return;
        }

        if (mainCamera == null)
        {
            mainCamera = Camera.main;
        }

        if (mainCamera != null)
        {
            mainCamera.fieldOfView = cameraFOV;
        }

        if (droneTrail == null && droneRoot != null)
        {
            droneTrail = droneRoot.GetComponent<TrailRenderer>();
        }

        if (visualModel != null)
        {
            visualModelInitialLocalPosition = visualModel.localPosition;
            visualModelInitialLocalRotation = visualModel.localRotation;
        }

        if (occlusionWall != null)
        {
            occlusionWall.SetActive(false);
        }

        int initOk = 0;

        try
        {
            initOk = AudioSDKBridge.AudioSDK_Init();
        }
        catch (System.Exception ex)
        {
            Debug.LogError("[ManualDemo] Cannot load HUSTAudioSDK.dll or one of its dependencies.");
            Debug.LogError(ex.ToString());
            sdkReady = false;
            return;
        }

        if (initOk == 0)
        {
            Debug.LogError("[ManualDemo] Failed to initialize HUSTAudioSDK.");
            sdkReady = false;
            return;
        }

        sdkReady = true;

        string wavPath = Path.Combine(Application.streamingAssetsPath, "siren.wav");
        wavPath = wavPath.Replace("\\", "/");

        int loadOk = AudioSDKBridge.AudioSDK_LoadSound(wavPath);

        if (loadOk == 0)
        {
            Debug.LogError("[ManualDemo] Failed to load sound: " + wavPath);
            sdkReady = false;
            return;
        }

        AudioSDKBridge.AudioSDK_SetLooping(1);

        droneRoot.position = GetPositionForMode(currentMode, 0.0f);
        lastEmitterPosition = emitterPoint.position;
        currentVelocity = Vector3.zero;

        UpdateListenerToSDK();
        UpdateSourceToSDK(Vector3.zero);
        ApplyModeFeatureSetup(false);

        Debug.Log("[ManualDemo] HUSTAudioSDK initialized. HRTF: " + AudioSDKBridge.AudioSDK_IsHRTFEnabled());
    }

    void Update()
    {
        HandleInput();

        SpinPropellers();
        UpdateSirenLights();
        UpdateDroneVisualHoverAndTilt();

        if (!sdkReady)
        {
            return;
        }

        UpdateListenerToSDK();

        if (runningDemo)
        {
            elapsedTime += Time.deltaTime * speedMultiplier;

            float t = elapsedTime / demoDuration;

            if (t >= 1.0f)
            {
                t = 1.0f;
                runningDemo = false;
                AudioSDKBridge.AudioSDK_Stop();
            }

            Vector3 newPosition = GetPositionForMode(currentMode, t);
            droneRoot.position = newPosition;

            Vector3 velocity = (emitterPoint.position - lastEmitterPosition) / Mathf.Max(Time.deltaTime, 0.0001f);
            currentVelocity = velocity;

            RotateDroneVisualTowardVelocity();
            UpdateSourceToSDK(velocity);

            if (occlusionEnabled)
            {
                float amount = ComputeOcclusionAmount();
                AudioSDKBridge.AudioSDK_SetOcclusion(amount);
            }
            else
            {
                AudioSDKBridge.AudioSDK_SetOcclusion(0.0f);
            }

            lastEmitterPosition = emitterPoint.position;
        }
        else
        {
            currentVelocity = Vector3.zero;
            UpdateSourceToSDK(Vector3.zero);
        }
    }

    void LateUpdate()
    {
        UpdateCameraFollow();
    }

    private void HandleInput()
    {
        if (Input.GetKeyDown(KeyCode.Space))
        {
            StartCurrentMode();
        }

        if (Input.GetKeyDown(KeyCode.M))
        {
            ChangeMode();
        }

        if (Input.GetKeyDown(KeyCode.R))
        {
            ToggleReverb();
        }

        if (Input.GetKeyDown(KeyCode.O))
        {
            ToggleOcclusion();
        }

        if (Input.GetKey(KeyCode.Q))
        {
            RotateHumanListener(-60.0f * Time.deltaTime);
        }

        if (Input.GetKey(KeyCode.E))
        {
            RotateHumanListener(60.0f * Time.deltaTime);
        }
    }

    private void RotateHumanListener(float angle)
    {
        if (listenerVisualRoot == null)
        {
            return;
        }

        // Xoay chính HumanVisualPivot tại chỗ, không xoay quanh world point khác
        listenerVisualRoot.Rotate(Vector3.up, angle, Space.Self);
    }

    private void StartCurrentMode()
    {
        if (!sdkReady)
        {
            return;
        }

        elapsedTime = 0.0f;
        runningDemo = true;

        ApplyModeFeatureSetup(true);

        droneRoot.position = GetPositionForMode(currentMode, 0.0f);
        lastEmitterPosition = emitterPoint.position;
        currentVelocity = Vector3.zero;

        if (droneTrail != null)
        {
            droneTrail.Clear();
        }

        UpdateSourceToSDK(Vector3.zero);

        AudioSDKBridge.AudioSDK_Stop();
        AudioSDKBridge.AudioSDK_Play();

        Debug.Log("[ManualDemo] Started mode: " + currentMode);
    }

    private void ChangeMode()
    {
        int next = (int)currentMode + 1;

        if (next > (int)MovementMode.FullShowcase)
        {
            next = 0;
        }

        currentMode = (MovementMode)next;

        runningDemo = false;
        elapsedTime = 0.0f;

        if (sdkReady)
        {
            AudioSDKBridge.AudioSDK_Stop();
        }

        ApplyModeFeatureSetup(false);

        droneRoot.position = GetPositionForMode(currentMode, 0.0f);
        lastEmitterPosition = emitterPoint.position;
        currentVelocity = Vector3.zero;

        if (droneTrail != null)
        {
            droneTrail.Clear();
        }

        Debug.Log("[ManualDemo] Changed mode to: " + currentMode);
    }

    private void ToggleReverb()
    {
        if (!sdkReady)
        {
            return;
        }

        reverbEnabled = !reverbEnabled;
        AudioSDKBridge.AudioSDK_SetReverb(reverbEnabled ? 1 : 0);

        Debug.Log("[ManualDemo] Reverb: " + reverbEnabled);
    }

    private void ToggleOcclusion()
    {
        occlusionEnabled = !occlusionEnabled;

        if (occlusionWall != null)
        {
            occlusionWall.SetActive(occlusionEnabled);
        }

        if (!occlusionEnabled && sdkReady)
        {
            AudioSDKBridge.AudioSDK_SetOcclusion(0.0f);
        }

        Debug.Log("[ManualDemo] Occlusion: " + occlusionEnabled);
    }

    private void ApplyModeFeatureSetup(bool forRunningDemo)
    {
        if (currentMode == MovementMode.WallOcclusionPass)
        {
            occlusionEnabled = true;

            if (occlusionWall != null)
            {
                occlusionWall.SetActive(true);
            }
        }
        else
        {
            occlusionEnabled = false;

            if (occlusionWall != null)
            {
                occlusionWall.SetActive(false);
            }

            if (sdkReady)
            {
                AudioSDKBridge.AudioSDK_SetOcclusion(0.0f);
            }
        }

        if (currentMode == MovementMode.CaveReverbPass)
        {
            reverbEnabled = true;

            if (sdkReady)
            {
                AudioSDKBridge.AudioSDK_SetReverb(1);
            }
        }
        else
        {
            reverbEnabled = false;

            if (sdkReady)
            {
                AudioSDKBridge.AudioSDK_SetReverb(0);
            }
        }

        if (!forRunningDemo)
        {
            currentVelocity = Vector3.zero;
        }
    }

    private Vector3 GetPositionForMode(MovementMode mode, float t)
    {
        t = Mathf.Clamp01(t);

        switch (mode)
        {
            case MovementMode.DopplerFlyBy:
                return Vector3.Lerp(
                    new Vector3(-40.0f, 3.0f, -8.0f),
                    new Vector3(40.0f, 3.0f, -8.0f),
                    t
                );

            case MovementMode.OrbitAroundListener:
                {
                    float angle = t * Mathf.PI * 2.0f;
                    Vector3 center = listenerTransform != null ? listenerTransform.position : Vector3.zero;

                    return center + new Vector3(
                        Mathf.Cos(angle) * orbitRadius,
                        3.0f,
                        Mathf.Sin(angle) * orbitRadius
                    );
                }

            case MovementMode.OverheadPass:
                {
                    Vector3 start = new Vector3(0.0f, 2.0f, -35.0f);
                    Vector3 end = new Vector3(0.0f, 2.0f, 35.0f);

                    Vector3 pos = Vector3.Lerp(start, end, t);
                    pos.y += Mathf.Sin(t * Mathf.PI) * 16.0f;

                    return pos;
                }

            case MovementMode.WallOcclusionPass:
                return Vector3.Lerp(
                    new Vector3(-30.0f, 3.0f, -15.0f),
                    new Vector3(30.0f, 3.0f, -15.0f),
                    t
                );

            case MovementMode.CaveReverbPass:
                {
                    Vector3 start = new Vector3(-40.0f, 3.0f, 10.0f);
                    Vector3 cave = new Vector3(-24.0f, 4.0f, 24.0f);
                    Vector3 end = new Vector3(-10.0f, 3.0f, 28.0f);

                    if (t < 0.5f)
                    {
                        return Vector3.Lerp(start, cave, t / 0.5f);
                    }

                    return Vector3.Lerp(cave, end, (t - 0.5f) / 0.5f);
                }

            case MovementMode.FullShowcase:
                {
                    float angle = t * Mathf.PI * 4.0f;
                    float radius = Mathf.Lerp(30.0f, 10.0f, t);
                    float y = 6.0f + Mathf.Sin(t * Mathf.PI * 3.0f) * 5.0f;

                    return new Vector3(
                        Mathf.Cos(angle) * radius,
                        y,
                        Mathf.Sin(angle) * radius
                    );
                }

            default:
                return Vector3.zero;
        }
    }

    private void UpdateListenerToSDK()
    {
        if (listenerTransform == null || !sdkReady)
        {
            return;
        }

        // Vị trí nghe lấy từ AudioListenerPoint
        Vector3 sdkPos = AudioSDKBridge.UnityToSDKPosition(listenerTransform.position);

        // Hướng nghe lấy từ HumanVisualPivot
        Transform orientationSource = listenerVisualRoot != null ? listenerVisualRoot : listenerTransform;

        Vector3 sdkForward = AudioSDKBridge.UnityToSDKDirection(orientationSource.forward);
        Vector3 sdkUp = AudioSDKBridge.UnityToSDKDirection(orientationSource.up);

        AudioSDKBridge.AudioSDK_SetListenerPosition(
            sdkPos.x,
            sdkPos.y,
            sdkPos.z
        );

        AudioSDKBridge.AudioSDK_SetListenerVelocity(
            0.0f,
            0.0f,
            0.0f
        );

        AudioSDKBridge.AudioSDK_SetListenerOrientation(
            sdkForward.x,
            sdkForward.y,
            sdkForward.z,
            sdkUp.x,
            sdkUp.y,
            sdkUp.z
        );
    }

    private void UpdateSourceToSDK(Vector3 unityVelocity)
    {
        if (emitterPoint == null || !sdkReady)
        {
            return;
        }

        Vector3 sdkPos = AudioSDKBridge.UnityToSDKPosition(emitterPoint.position);
        Vector3 sdkVelocity = AudioSDKBridge.UnityToSDKDirection(unityVelocity);

        AudioSDKBridge.AudioSDK_SetSourcePosition(
            sdkPos.x,
            sdkPos.y,
            sdkPos.z
        );

        AudioSDKBridge.AudioSDK_SetSourceVelocity(
            sdkVelocity.x,
            sdkVelocity.y,
            sdkVelocity.z
        );
    }

    private float ComputeOcclusionAmount()
    {
        if (listenerTransform == null || emitterPoint == null || occlusionWall == null)
        {
            return 0.0f;
        }

        Vector3 from = emitterPoint.position;
        Vector3 to = listenerTransform.position;

        Vector3 direction = to - from;
        float distance = direction.magnitude;

        if (distance <= 0.01f)
        {
            return 0.0f;
        }

        Ray ray = new Ray(from, direction.normalized);
        RaycastHit[] hits = Physics.RaycastAll(ray, distance, occlusionMask);

        foreach (RaycastHit hit in hits)
        {
            if (hit.collider != null && hit.collider.gameObject == occlusionWall)
            {
                float baseAmount = 0.75f;

                float heightDifference = Mathf.Abs(emitterPoint.position.y - occlusionWall.transform.position.y);
                float heightReduction = Mathf.Clamp01(heightDifference / 20.0f);

                float amount = baseAmount * (1.0f - heightReduction * 0.5f);

                return Mathf.Clamp01(amount);
            }
        }

        return 0.0f;
    }

    private void RotateDroneVisualTowardVelocity()
    {
        if (visualModel == null || currentVelocity.sqrMagnitude < 0.01f)
        {
            return;
        }

        Vector3 flatVelocity = new Vector3(currentVelocity.x, 0.0f, currentVelocity.z);

        if (flatVelocity.sqrMagnitude < 0.01f)
        {
            return;
        }

        Quaternion targetRotation =
            Quaternion.LookRotation(flatVelocity.normalized, Vector3.up) *
            Quaternion.Euler(0.0f, droneYawOffset, 0.0f);

        visualModel.rotation = Quaternion.Slerp(
            visualModel.rotation,
            targetRotation,
            8.0f * Time.deltaTime
        );
    }

    private void UpdateDroneVisualHoverAndTilt()
    {
        if (visualModel == null)
        {
            return;
        }

        if (!enableVisualHover)
        {
            return;
        }

        float hover = Mathf.Sin(Time.time * hoverSpeed) * hoverAmplitude;
        visualModel.localPosition = visualModelInitialLocalPosition + new Vector3(0.0f, hover, 0.0f);

        float tiltX = 0.0f;
        float tiltZ = 0.0f;

        if (currentVelocity.sqrMagnitude > 0.1f)
        {
            Vector3 localVelocity = droneRoot.InverseTransformDirection(currentVelocity.normalized);

            tiltX = Mathf.Clamp(localVelocity.z * tiltAmount, -tiltAmount, tiltAmount);
            tiltZ = Mathf.Clamp(-localVelocity.x * tiltAmount, -tiltAmount, tiltAmount);
        }

        Quaternion targetTilt = Quaternion.Euler(tiltX, 0.0f, tiltZ);

        visualModel.localRotation = Quaternion.Slerp(
            visualModel.localRotation,
            visualModelInitialLocalRotation * targetTilt,
            5.0f * Time.deltaTime
        );
    }

    private void UpdateCameraFollow()
    {
        if (!enableFollowCamera || mainCamera == null || droneRoot == null || listenerTransform == null)
        {
            return;
        }

        Vector3 listenerPos = listenerTransform.position;
        Vector3 dronePos = droneRoot.position;

        Vector3 midpoint = (listenerPos + dronePos) * 0.5f;
        midpoint.y += cameraLookHeight;

        float distanceBetweenObjects = Vector3.Distance(listenerPos, dronePos);
        float cameraDistance = Mathf.Clamp(
            distanceBetweenObjects * cameraDistanceMultiplier,
            minCameraDistance,
            maxCameraDistance
        );

        Vector3 direction = cameraDirection.normalized;
        Vector3 desiredPosition = midpoint + direction * cameraDistance;

        mainCamera.transform.position = Vector3.Lerp(
            mainCamera.transform.position,
            desiredPosition,
            cameraSmooth * Time.deltaTime
        );

        mainCamera.transform.LookAt(midpoint);
    }

    private void UpdateSirenLights()
    {
        if (redLight == null || blueLight == null)
        {
            return;
        }

        float blink = Mathf.PingPong(Time.time * 8.0f, 1.0f);

        redLight.intensity = runningDemo ? Mathf.Lerp(0.7f, 5.0f, blink) : 0.7f;
        blueLight.intensity = runningDemo ? Mathf.Lerp(5.0f, 0.7f, blink) : 0.7f;
    }

    private void SpinPropellers()
    {
        if (propellers == null)
        {
            return;
        }

        float spinSpeed = runningDemo ? 1450.0f : 420.0f;

        foreach (Transform propeller in propellers)
        {
            if (propeller != null)
            {
                propeller.Rotate(Vector3.up, spinSpeed * Time.deltaTime, Space.Self);
            }
        }
    }

    void OnGUI()
    {
        if (labelStyle == null)
        {
            labelStyle = new GUIStyle(GUI.skin.label);
            labelStyle.fontSize = 16;
            labelStyle.normal.textColor = Color.white;
        }

        if (titleStyle == null)
        {
            titleStyle = new GUIStyle(GUI.skin.label);
            titleStyle.fontSize = 18;
            titleStyle.fontStyle = FontStyle.Bold;
            titleStyle.normal.textColor = Color.white;
        }

        if (buttonStyle == null)
        {
            buttonStyle = new GUIStyle(GUI.skin.button);
            buttonStyle.fontSize = 15;
        }

        GUILayout.BeginArea(new Rect(15, 15, 470, 450), GUI.skin.box);

        GUILayout.Label("HUST 3D Audio Game Engine SDK - Unity Visual Demo", titleStyle);
        GUILayout.Space(8);

        GUILayout.Label("Mode: " + currentMode, labelStyle);
        GUILayout.Label("Running: " + runningDemo, labelStyle);
        GUILayout.Label("Reverb: " + reverbEnabled, labelStyle);
        GUILayout.Label("Occlusion: " + occlusionEnabled, labelStyle);
        GUILayout.Label("HRTF: " + (sdkReady ? AudioSDKBridge.AudioSDK_IsHRTFEnabled().ToString() : "N/A"), labelStyle);

        GUILayout.Space(8);

        if (emitterPoint != null)
        {
            GUILayout.Label("Source Position: " + emitterPoint.position.ToString("F2"), labelStyle);
        }

        GUILayout.Label("Velocity: " + currentVelocity.ToString("F2"), labelStyle);

        GUILayout.Space(8);

        GUILayout.BeginHorizontal();

        if (GUILayout.Button("Start", buttonStyle))
        {
            StartCurrentMode();
        }

        if (GUILayout.Button("Mode", buttonStyle))
        {
            ChangeMode();
        }

        GUILayout.EndHorizontal();

        GUILayout.BeginHorizontal();

        if (GUILayout.Button("Reverb", buttonStyle))
        {
            ToggleReverb();
        }

        if (GUILayout.Button("Occlusion", buttonStyle))
        {
            ToggleOcclusion();
        }

        GUILayout.EndHorizontal();

        GUILayout.Space(8);

        GUILayout.Label("Scenario Modes:", labelStyle);
        GUILayout.Label("Doppler | Orbit | Overhead | Wall | Cave | Full Showcase", labelStyle);

        GUILayout.Space(6);

        GUILayout.Label("Controls:", labelStyle);
        GUILayout.Label("SPACE = Start | M = Mode | R = Reverb | O = Occlusion", labelStyle);
        GUILayout.Label("Q / E = Rotate Human Listener", labelStyle);
        GUILayout.Label("Unity = visualization only", labelStyle);
        GUILayout.Label("Audio = custom C++ OpenAL SDK", labelStyle);

        GUILayout.EndArea();
    }

    void OnApplicationQuit()
    {
        if (sdkReady)
        {
            AudioSDKBridge.AudioSDK_Stop();
            AudioSDKBridge.AudioSDK_Shutdown();
            sdkReady = false;
        }
    }
}