using UnityEngine;

public class DroneWaypointAutopilot : MonoBehaviour
{
    public enum AutopilotState
    {
        Idle,
        Stage1DemoFlight,
        MovingToStage1Exit,
        HoldingAtStage1Exit,
        HoldingAtStage2Target
    }

    [Header("References")]
    public Transform droneRoot;
    public Rigidbody droneRigidbody;

    [Header("Stage 1 Waypoints")]
    public Transform[] stage1Waypoints;
    public Transform[] stage1ExitPoints;

    [Header("Timing")]
    public float stage1DemoDuration = 20.0f;
    public float stage1ExitTravelDuration = 5.0f;

    [Header("Movement")]
    public float rotationSpeed = 8.0f;
    public bool keepOriginalWaypointY = true;
    public float fixedFlightHeight = 3.0f;

    [Header("Control")]
    public bool makeRigidbodyKinematicDuringAutopilot = true;
    public Behaviour[] scriptsToDisableDuringAutopilot;

    [Header("Debug")]
    public bool debugLog = true;
    public bool drawDebugLines = true;

    private AutopilotState state = AutopilotState.Idle;

    private Transform currentTarget;
    private Transform selectedExitPoint;
    private Transform selectedStage2Target;

    private int currentWaypointIndex = 0;

    private float segmentTimer = 0.0f;
    private float segmentDuration = 1.0f;

    private float exitTimer = 0.0f;

    private Vector3 segmentStartPosition;
    private Vector3 segmentTargetPosition;

    private Vector3 exitStartPosition;
    private Vector3 exitTargetPosition;

    private bool originalIsKinematic = false;
    private bool originalUseGravity = false;
    private bool hasStoredRigidbodyState = false;

    public AutopilotState State
    {
        get { return state; }
    }

    public bool IsWaitingAtStage1Exit
    {
        get { return state == AutopilotState.HoldingAtStage1Exit; }
    }

    public bool IsHoldingAtStage2Target
    {
        get { return state == AutopilotState.HoldingAtStage2Target; }
    }

    public Transform SelectedExitPoint
    {
        get { return selectedExitPoint; }
    }

    public Transform SelectedStage2Target
    {
        get { return selectedStage2Target; }
    }

    public string CurrentTargetName
    {
        get
        {
            if (currentTarget == null)
            {
                return "None";
            }

            return currentTarget.name;
        }
    }

    private void Awake()
    {
        AutoFindReferences();
    }

    private void Start()
    {
        AutoFindReferences();
    }

    private void Update()
    {
        switch (state)
        {
            case AutopilotState.Stage1DemoFlight:
                UpdateStage1DemoFlight();
                break;

            case AutopilotState.MovingToStage1Exit:
                UpdateMoveToExit();
                break;

            case AutopilotState.HoldingAtStage1Exit:
            case AutopilotState.HoldingAtStage2Target:
                HoldPosition();
                break;

            case AutopilotState.Idle:
            default:
                break;
        }
    }

    private void AutoFindReferences()
    {
        if (droneRoot == null)
        {
            droneRoot = transform;
        }

        if (droneRigidbody == null)
        {
            droneRigidbody = GetComponent<Rigidbody>();
        }
    }

    public void BeginStage1(
        Transform droneStartPoint,
        Transform[] waypoints,
        Transform[] exitPoints,
        float demoDuration
    )
    {
        AutoFindReferences();

        stage1Waypoints = waypoints;
        stage1ExitPoints = exitPoints;
        stage1DemoDuration = Mathf.Max(0.5f, demoDuration);

        selectedExitPoint = null;
        selectedStage2Target = null;
        currentTarget = null;

        currentWaypointIndex = 0;
        segmentTimer = 0.0f;
        exitTimer = 0.0f;

        PrepareAutopilotControl();

        if (droneStartPoint != null)
        {
            TeleportDrone(droneStartPoint.position, droneStartPoint.rotation);
        }

        if (stage1Waypoints == null || stage1Waypoints.Length == 0)
        {
            Debug.LogWarning("[DroneWaypointAutopilot] No Stage 1 waypoints. Moving directly to exit.");
            StartMoveToExit();
            return;
        }

        segmentDuration = stage1DemoDuration / stage1Waypoints.Length;
        segmentDuration = Mathf.Max(0.1f, segmentDuration);

        StartWaypointSegment(0);

        state = AutopilotState.Stage1DemoFlight;

        if (debugLog)
        {
            Debug.Log("[DroneWaypointAutopilot] Stage 1 started.");
            Debug.Log("[DroneWaypointAutopilot] Waypoint count = " + stage1Waypoints.Length);
            Debug.Log("[DroneWaypointAutopilot] Segment duration = " + segmentDuration.ToString("F2") + " seconds.");
        }
    }

    public void BeginStage2Hold(Transform targetPoint)
    {
        AutoFindReferences();

        PrepareAutopilotControl();

        selectedExitPoint = null;
        selectedStage2Target = targetPoint;
        currentTarget = targetPoint;

        if (targetPoint != null)
        {
            TeleportDrone(targetPoint.position, targetPoint.rotation);
        }

        state = AutopilotState.HoldingAtStage2Target;

        if (debugLog)
        {
            Debug.Log("[DroneWaypointAutopilot] Stage 2 hold at: " + GetSafeName(targetPoint));
        }
    }

    public void StopAutopilot(bool restoreManualControl)
    {
        state = AutopilotState.Idle;
        currentTarget = null;

        if (restoreManualControl)
        {
            RestoreManualControl();
        }

        StopRigidbodyMotion();
    }

    private void StartWaypointSegment(int index)
    {
        if (stage1Waypoints == null || index < 0 || index >= stage1Waypoints.Length)
        {
            StartMoveToExit();
            return;
        }

        currentWaypointIndex = index;
        currentTarget = stage1Waypoints[index];

        segmentTimer = 0.0f;
        segmentStartPosition = droneRoot.position;
        segmentTargetPosition = GetTargetPosition(currentTarget);

        if (debugLog)
        {
            Debug.Log(
                "[DroneWaypointAutopilot] Moving to waypoint " +
                index +
                ": " +
                GetSafeName(currentTarget) +
                " from " +
                segmentStartPosition.ToString("F2") +
                " to " +
                segmentTargetPosition.ToString("F2")
            );
        }
    }

    private void UpdateStage1DemoFlight()
    {
        if (currentTarget == null)
        {
            StartMoveToExit();
            return;
        }

        segmentTimer += Time.deltaTime;

        float t = Mathf.Clamp01(segmentTimer / segmentDuration);
        float smoothT = Mathf.SmoothStep(0.0f, 1.0f, t);

        Vector3 newPosition = Vector3.Lerp(
            segmentStartPosition,
            segmentTargetPosition,
            smoothT
        );

        SetDronePosition(newPosition);

        Vector3 moveDirection = segmentTargetPosition - segmentStartPosition;
        RotateTowardsMovementDirection(moveDirection);

        if (drawDebugLines)
        {
            Debug.DrawLine(segmentStartPosition, segmentTargetPosition, Color.cyan);
        }

        if (segmentTimer >= segmentDuration)
        {
            int nextIndex = currentWaypointIndex + 1;

            if (nextIndex >= stage1Waypoints.Length)
            {
                StartMoveToExit();
            }
            else
            {
                StartWaypointSegment(nextIndex);
            }
        }
    }

    private void StartMoveToExit()
    {
        Transform exit = PickRandomTransform(stage1ExitPoints);

        if (exit == null)
        {
            Debug.LogWarning("[DroneWaypointAutopilot] No exit point assigned.");
            state = AutopilotState.HoldingAtStage1Exit;
            return;
        }

        selectedExitPoint = exit;
        currentTarget = exit;

        exitTimer = 0.0f;
        exitStartPosition = droneRoot.position;
        exitTargetPosition = GetTargetPosition(exit);

        state = AutopilotState.MovingToStage1Exit;

        if (debugLog)
        {
            Debug.Log(
                "[DroneWaypointAutopilot] Moving to exit: " +
                exit.name +
                " in " +
                stage1ExitTravelDuration.ToString("F2") +
                " seconds."
            );
        }
    }

    private void UpdateMoveToExit()
    {
        exitTimer += Time.deltaTime;

        float duration = Mathf.Max(0.1f, stage1ExitTravelDuration);
        float t = Mathf.Clamp01(exitTimer / duration);
        float smoothT = Mathf.SmoothStep(0.0f, 1.0f, t);

        Vector3 newPosition = Vector3.Lerp(
            exitStartPosition,
            exitTargetPosition,
            smoothT
        );

        SetDronePosition(newPosition);

        Vector3 moveDirection = exitTargetPosition - exitStartPosition;
        RotateTowardsMovementDirection(moveDirection);

        if (drawDebugLines)
        {
            Debug.DrawLine(exitStartPosition, exitTargetPosition, Color.yellow);
        }

        if (exitTimer >= duration)
        {
            SetDronePosition(exitTargetPosition);
            state = AutopilotState.HoldingAtStage1Exit;

            if (debugLog)
            {
                Debug.Log("[DroneWaypointAutopilot] Reached exit: " + selectedExitPoint.name);
            }
        }
    }

    private Vector3 GetTargetPosition(Transform target)
    {
        Vector3 pos = target.position;

        if (!keepOriginalWaypointY)
        {
            pos.y = fixedFlightHeight;
        }

        return pos;
    }

    private void SetDronePosition(Vector3 position)
    {
        if (droneRoot == null)
        {
            return;
        }

        droneRoot.position = position;

        if (droneRigidbody != null)
        {
            droneRigidbody.position = position;

            // Không set velocity nếu Rigidbody đang kinematic.
            // Nếu set sẽ gây warning:
            // "Setting linear velocity of a kinematic body is not supported."
            if (!droneRigidbody.isKinematic)
            {
                droneRigidbody.linearVelocity = Vector3.zero;
                droneRigidbody.angularVelocity = Vector3.zero;
            }
        }
    }

    private void RotateTowardsMovementDirection(Vector3 direction)
    {
        if (droneRoot == null)
        {
            return;
        }

        direction.y = 0.0f;

        if (direction.sqrMagnitude <= 0.001f)
        {
            return;
        }

        Quaternion targetRotation = Quaternion.LookRotation(direction.normalized, Vector3.up);

        droneRoot.rotation = Quaternion.Slerp(
            droneRoot.rotation,
            targetRotation,
            rotationSpeed * Time.deltaTime
        );

        if (droneRigidbody != null)
        {
            droneRigidbody.rotation = droneRoot.rotation;
        }
    }

    private void PrepareAutopilotControl()
    {
        DisableManualControlScripts();

        if (droneRigidbody != null)
        {
            if (!hasStoredRigidbodyState)
            {
                originalIsKinematic = droneRigidbody.isKinematic;
                originalUseGravity = droneRigidbody.useGravity;
                hasStoredRigidbodyState = true;
            }

            StopRigidbodyMotion();

            if (makeRigidbodyKinematicDuringAutopilot)
            {
                droneRigidbody.isKinematic = true;
            }

            droneRigidbody.useGravity = false;
        }
    }

    private void RestoreManualControl()
    {
        EnableManualControlScripts();

        if (droneRigidbody != null && hasStoredRigidbodyState)
        {
            droneRigidbody.isKinematic = originalIsKinematic;
            droneRigidbody.useGravity = originalUseGravity;

            StopRigidbodyMotion();
        }
    }

    private void DisableManualControlScripts()
    {
        if (scriptsToDisableDuringAutopilot == null)
        {
            return;
        }

        for (int i = 0; i < scriptsToDisableDuringAutopilot.Length; i++)
        {
            if (scriptsToDisableDuringAutopilot[i] != null)
            {
                scriptsToDisableDuringAutopilot[i].enabled = false;
            }
        }
    }

    private void EnableManualControlScripts()
    {
        if (scriptsToDisableDuringAutopilot == null)
        {
            return;
        }

        for (int i = 0; i < scriptsToDisableDuringAutopilot.Length; i++)
        {
            if (scriptsToDisableDuringAutopilot[i] != null)
            {
                scriptsToDisableDuringAutopilot[i].enabled = true;
            }
        }
    }

    private Transform PickRandomTransform(Transform[] list)
    {
        if (list == null || list.Length == 0)
        {
            return null;
        }

        return list[Random.Range(0, list.Length)];
    }

    private void HoldPosition()
    {
        // Không set linearVelocity / angularVelocity ở đây.
        // Vì lúc Autopilot chạy, Rigidbody thường đang kinematic.
        // Nếu set velocity mỗi frame sẽ spam warning trong Console.
    }

    private void StopRigidbodyMotion()
    {
        if (droneRigidbody == null)
        {
            return;
        }

        // Unity không cho set velocity khi Rigidbody đang kinematic.
        if (droneRigidbody.isKinematic)
        {
            return;
        }

        droneRigidbody.linearVelocity = Vector3.zero;
        droneRigidbody.angularVelocity = Vector3.zero;
    }

    private void TeleportDrone(Vector3 position, Quaternion rotation)
    {
        if (droneRoot == null)
        {
            return;
        }

        droneRoot.position = position;
        droneRoot.rotation = rotation;

        if (droneRigidbody != null)
        {
            droneRigidbody.position = position;
            droneRigidbody.rotation = rotation;

            // Không set velocity nếu Rigidbody đang kinematic.
            if (!droneRigidbody.isKinematic)
            {
                droneRigidbody.linearVelocity = Vector3.zero;
                droneRigidbody.angularVelocity = Vector3.zero;
            }
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

    private void OnDisable()
    {
        StopRigidbodyMotion();
    }
}