using UnityEngine;

public class AudioSearchGameManager : MonoBehaviour
{
    public enum GameStage
    {
        NotStarted,
        Stage1_DemoFlight,
        Stage1_SearchingExit,
        Stage1_Complete,
        Stage2_SearchingCave,
        MissionComplete
    }

    [Header("Main References")]
    public Transform playerRoot;
    public Transform droneRoot;
    public Rigidbody playerRigidbody;
    public Rigidbody droneRigidbody;

    [Header("Audio System")]
    public DroneAudioSyncController audioSyncController;
    public DroneWaypointAutopilot droneAutopilot;

    [Header("Stage 1 - Maze")]
    public Transform stage1PlayerStart;
    public Transform stage1DroneStart;
    public Transform[] stage1DroneWaypoints;
    public Transform[] stage1ExitPoints;
    public float stage1DemoFlightDuration = 7.0f;
    public float stage1FindDistance = 4.0f;

    [Header("Stage 2 - Cave")]
    public Transform stage2PlayerStart;
    public Transform[] stage2DroneTargetPoints;
    public float stage2FindDistance = 4.0f;

    [Header("Optional Parent Auto Collect")]
    public Transform stage1WaypointsParent;
    public Transform stage1ExitPointsParent;
    public Transform stage2DroneTargetsParent;

    [Header("Stage Timing")]
    public float stageCompleteDelay = 2.0f;

    [Header("Input")]
    public bool startGameOnPlay = true;
    public KeyCode restartKey = KeyCode.F5;
    public KeyCode forceStage2Key = KeyCode.F6;

    [Header("UI")]
    public bool showObjectiveUI = true;
    public int uiFontSize = 18;

    private GameStage currentStage = GameStage.NotStarted;

    private float transitionTimer = 0.0f;
    private bool transitionActive = false;

    private Transform selectedStage2Target;

    private GUIStyle labelStyle;
    private GUIStyle titleStyle;
    private GUIStyle boxStyle;

    public GameStage CurrentStage
    {
        get { return currentStage; }
    }

    public Transform SelectedStage2Target
    {
        get { return selectedStage2Target; }
    }

    private void Awake()
    {
        AutoFindReferences();
        AutoCollectPoints();
    }

    private void Start()
    {
        if (startGameOnPlay)
        {
            StartStage1();
        }
    }

    private void Update()
    {
        if (Input.GetKeyDown(restartKey))
        {
            StartStage1();
        }

        if (Input.GetKeyDown(forceStage2Key))
        {
            StartStage2();
        }

        UpdateStageLogic();
    }

    private void AutoFindReferences()
    {
        if (playerRoot == null)
        {
            GameObject playerObj = GameObject.Find("PlayerListener");

            if (playerObj != null)
            {
                playerRoot = playerObj.transform;
            }
        }

        if (droneRoot == null)
        {
            GameObject droneObj = GameObject.Find("FlyingSirenDrone");

            if (droneObj != null)
            {
                droneRoot = droneObj.transform;
            }
        }

        if (playerRigidbody == null && playerRoot != null)
        {
            playerRigidbody = playerRoot.GetComponent<Rigidbody>();
        }

        if (droneRigidbody == null && droneRoot != null)
        {
            droneRigidbody = droneRoot.GetComponent<Rigidbody>();
        }

        if (droneAutopilot == null && droneRoot != null)
        {
            droneAutopilot = droneRoot.GetComponent<DroneWaypointAutopilot>();
        }

        if (audioSyncController == null)
        {
            audioSyncController = FindFirstObjectByType<DroneAudioSyncController>();
        }
    }

    private void AutoCollectPoints()
    {
        if ((stage1DroneWaypoints == null || stage1DroneWaypoints.Length == 0) && stage1WaypointsParent != null)
        {
            stage1DroneWaypoints = CollectDirectChildren(stage1WaypointsParent);
        }

        if ((stage1ExitPoints == null || stage1ExitPoints.Length == 0) && stage1ExitPointsParent != null)
        {
            stage1ExitPoints = CollectDirectChildren(stage1ExitPointsParent);
        }

        if ((stage2DroneTargetPoints == null || stage2DroneTargetPoints.Length == 0) && stage2DroneTargetsParent != null)
        {
            stage2DroneTargetPoints = CollectDirectChildren(stage2DroneTargetsParent);
        }
    }

    private Transform[] CollectDirectChildren(Transform parent)
    {
        if (parent == null)
        {
            return new Transform[0];
        }

        Transform[] result = new Transform[parent.childCount];

        for (int i = 0; i < parent.childCount; i++)
        {
            result[i] = parent.GetChild(i);
        }

        return result;
    }

    public void StartStage1()
    {
        AutoFindReferences();
        AutoCollectPoints();

        transitionActive = false;
        transitionTimer = 0.0f;
        selectedStage2Target = null;

        if (playerRoot != null && stage1PlayerStart != null)
        {
            TeleportObject(playerRoot, playerRigidbody, stage1PlayerStart.position, stage1PlayerStart.rotation);
        }

        if (droneAutopilot != null)
        {
            droneAutopilot.BeginStage1(
                stage1DroneStart,
                stage1DroneWaypoints,
                stage1ExitPoints,
                stage1DemoFlightDuration
            );
        }
        else if (droneRoot != null && stage1DroneStart != null)
        {
            TeleportObject(droneRoot, droneRigidbody, stage1DroneStart.position, stage1DroneStart.rotation);
        }

        currentStage = GameStage.Stage1_DemoFlight;

        Debug.Log("[AudioSearchGameManager] Stage 1 started.");
    }

    public void StartStage2()
    {
        AutoFindReferences();
        AutoCollectPoints();

        transitionActive = false;
        transitionTimer = 0.0f;

        if (playerRoot != null && stage2PlayerStart != null)
        {
            TeleportObject(playerRoot, playerRigidbody, stage2PlayerStart.position, stage2PlayerStart.rotation);
        }

        selectedStage2Target = PickRandomTransform(stage2DroneTargetPoints);

        if (droneAutopilot != null)
        {
            droneAutopilot.BeginStage2Hold(selectedStage2Target);
        }
        else if (droneRoot != null && selectedStage2Target != null)
        {
            TeleportObject(droneRoot, droneRigidbody, selectedStage2Target.position, selectedStage2Target.rotation);
        }

        currentStage = GameStage.Stage2_SearchingCave;

        Debug.Log("[AudioSearchGameManager] Stage 2 started. Target: " + GetSafeName(selectedStage2Target));
    }

    private void UpdateStageLogic()
    {
        switch (currentStage)
        {
            case GameStage.Stage1_DemoFlight:
                UpdateStage1DemoFlight();
                break;

            case GameStage.Stage1_SearchingExit:
                UpdateStage1SearchingExit();
                break;

            case GameStage.Stage1_Complete:
                UpdateStage1CompleteTransition();
                break;

            case GameStage.Stage2_SearchingCave:
                UpdateStage2SearchingCave();
                break;

            case GameStage.MissionComplete:
                break;

            case GameStage.NotStarted:
            default:
                break;
        }
    }

    private void UpdateStage1DemoFlight()
    {
        if (droneAutopilot == null)
        {
            currentStage = GameStage.Stage1_SearchingExit;
            return;
        }

        if (droneAutopilot.IsWaitingAtStage1Exit)
        {
            currentStage = GameStage.Stage1_SearchingExit;

            Debug.Log("[AudioSearchGameManager] Stage 1 search phase started. Drone exit: " + GetSafeName(droneAutopilot.SelectedExitPoint));
        }
    }

    private void UpdateStage1SearchingExit()
    {
        float distance = GetPlayerDroneDistance();

        if (distance <= stage1FindDistance)
        {
            currentStage = GameStage.Stage1_Complete;
            transitionActive = true;
            transitionTimer = stageCompleteDelay;

            Debug.Log("[AudioSearchGameManager] Stage 1 complete.");
        }
    }

    private void UpdateStage1CompleteTransition()
    {
        if (!transitionActive)
        {
            return;
        }

        transitionTimer -= Time.deltaTime;

        if (transitionTimer <= 0.0f)
        {
            transitionActive = false;
            StartStage2();
        }
    }

    private void UpdateStage2SearchingCave()
    {
        float distance = GetPlayerDroneDistance();

        if (distance <= stage2FindDistance)
        {
            currentStage = GameStage.MissionComplete;

            if (droneAutopilot != null)
            {
                droneAutopilot.StopAutopilot(false);
            }

            Debug.Log("[AudioSearchGameManager] Mission complete.");
        }
    }

    private float GetPlayerDroneDistance()
    {
        if (playerRoot == null || droneRoot == null)
        {
            return 9999.0f;
        }

        return Vector3.Distance(playerRoot.position, droneRoot.position);
    }

    private Transform PickRandomTransform(Transform[] list)
    {
        if (list == null || list.Length == 0)
        {
            return null;
        }

        int index = Random.Range(0, list.Length);

        return list[index];
    }

    private void TeleportObject(Transform target, Rigidbody rb, Vector3 position, Quaternion rotation)
    {
        if (target == null)
        {
            return;
        }

        if (rb != null)
        {
            rb.linearVelocity = Vector3.zero;
            rb.angularVelocity = Vector3.zero;

            if (rb.isKinematic)
            {
                rb.position = position;
                rb.rotation = rotation;
            }
            else
            {
                target.position = position;
                target.rotation = rotation;
            }
        }
        else
        {
            target.position = position;
            target.rotation = rotation;
        }
    }

    private string GetStageTitle()
    {
        switch (currentStage)
        {
            case GameStage.Stage1_DemoFlight:
                return "Stage 1: Maze Audio Scan";

            case GameStage.Stage1_SearchingExit:
                return "Stage 1: Find the Drone Exit";

            case GameStage.Stage1_Complete:
                return "Stage 1 Complete";

            case GameStage.Stage2_SearchingCave:
                return "Stage 2: Cave Reverb Search";

            case GameStage.MissionComplete:
                return "Mission Complete";

            case GameStage.NotStarted:
            default:
                return "Not Started";
        }
    }

    private string GetObjectiveText()
    {
        switch (currentStage)
        {
            case GameStage.Stage1_DemoFlight:
                return "Listen to the drone flying around the maze. HRTF, Doppler, and material occlusion are being demonstrated.";

            case GameStage.Stage1_SearchingExit:
                return "The drone stopped at a random exit. Follow the 3D siren sound and find it.";

            case GameStage.Stage1_Complete:
                return "Drone found. Moving to the cave stage...";

            case GameStage.Stage2_SearchingCave:
                return "The drone is hidden at the end of a tunnel. Use reverb and HRTF cues to find it.";

            case GameStage.MissionComplete:
                return "You found the drone using spatial audio.";

            case GameStage.NotStarted:
            default:
                return "Press Play or F5 to start.";
        }
    }

    private string GetAudioFocusText()
    {
        switch (currentStage)
        {
            case GameStage.Stage1_DemoFlight:
                return "Audio Focus: HRTF 360 + Doppler + Wood/Stone Occlusion";

            case GameStage.Stage1_SearchingExit:
                return "Audio Focus: Direction finding + Occlusion cues";

            case GameStage.Stage2_SearchingCave:
                return "Audio Focus: Cave Reverb + HRTF";

            case GameStage.MissionComplete:
                return "Audio Focus: Full spatial audio pipeline completed";

            default:
                return "Audio Focus: Waiting";
        }
    }

    private string GetSafeName(Transform target)
    {
        if (target == null)
        {
            return "None";
        }

        return target.name;
    }

    private void SetupGUIStyles()
    {
        titleStyle = new GUIStyle(GUI.skin.label);
        titleStyle.fontSize = uiFontSize + 4;
        titleStyle.fontStyle = FontStyle.Bold;
        titleStyle.normal.textColor = Color.white;

        labelStyle = new GUIStyle(GUI.skin.label);
        labelStyle.fontSize = uiFontSize;
        labelStyle.normal.textColor = Color.white;
        labelStyle.wordWrap = true;

        boxStyle = new GUIStyle(GUI.skin.box);
        boxStyle.fontSize = uiFontSize;
    }

    private void OnGUI()
    {
        if (!showObjectiveUI)
        {
            return;
        }

        if (labelStyle == null || titleStyle == null || boxStyle == null)
        {
            SetupGUIStyles();
        }

        GUILayout.BeginArea(new Rect(20, 460, 650, 260), boxStyle);

        GUILayout.Label("3D Audio Search Game", titleStyle);
        GUILayout.Space(6);

        GUILayout.Label(GetStageTitle(), labelStyle);
        GUILayout.Label(GetObjectiveText(), labelStyle);

        GUILayout.Space(6);

        GUILayout.Label(GetAudioFocusText(), labelStyle);
        GUILayout.Label("Distance to Drone: " + GetPlayerDroneDistance().ToString("F2") + " m", labelStyle);

        if (droneAutopilot != null)
        {
            GUILayout.Label("Drone State: " + droneAutopilot.State, labelStyle);
            GUILayout.Label("Drone Target: " + droneAutopilot.CurrentTargetName, labelStyle);
        }

        if (currentStage == GameStage.Stage1_SearchingExit && droneAutopilot != null)
        {
            GUILayout.Label("Selected Exit: " + GetSafeName(droneAutopilot.SelectedExitPoint), labelStyle);
        }

        if (currentStage == GameStage.Stage2_SearchingCave)
        {
            GUILayout.Label("Selected Cave Target: " + GetSafeName(selectedStage2Target), labelStyle);
        }

        GUILayout.Space(6);

        GUILayout.Label("Controls: Human = Arrow Keys | Head = Q/E | Restart = F5 | Force Stage 2 = F6", labelStyle);

        GUILayout.EndArea();
    }
}