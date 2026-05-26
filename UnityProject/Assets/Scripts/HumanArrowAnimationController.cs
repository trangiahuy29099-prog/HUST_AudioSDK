using UnityEngine;
using UnityEngine.Playables;
using UnityEngine.Animations;

public class HumanArrowAnimationController : MonoBehaviour
{
    private enum HumanMoveState
    {
        Idle,
        Forward,
        Backward,
        Left,
        Right,
        ForwardLeft,
        ForwardRight,
        BackwardLeft,
        BackwardRight
    }

    public enum HeadRotationAxis
    {
        X,
        Y,
        Z
    }

    [Header("References")]
    public Transform movementRoot;          // PlayerListener
    public Transform visualRoot;            // HumanVisualPivot
    public Transform headOrientationRoot;   // AudioListenerPoint/HeadOrientationPivot
    public Animator animator;               // Animator trên Human_BasicMotionsDummy_F

    [Header("Movement")]
    public float walkSpeed = 2.2f;
    public float runSpeed = 4.2f;

    [Header("Head / Listener Rotation")]
    public float headRotateSpeed = 120.0f;
    public float maxHeadYaw = 80.0f;
    public bool clampHeadYaw = true;

    [Header("Visual Head Bone")]
    public bool rotateHeadBoneVisual = true;
    public Transform headBone;              // Kéo B-head vào đây
    public HeadRotationAxis headRotationAxis = HeadRotationAxis.Y;
    public bool invertHeadRotation = false;
    public float visualHeadYawMultiplier = 1.0f;

    [Header("Input")]
    public KeyCode forwardKey = KeyCode.UpArrow;
    public KeyCode backwardKey = KeyCode.DownArrow;
    public KeyCode leftKey = KeyCode.LeftArrow;
    public KeyCode rightKey = KeyCode.RightArrow;
    public KeyCode runKey = KeyCode.LeftShift;

    [Header("Rotate Head / Listener")]
    public KeyCode rotateHeadLeftKey = KeyCode.Q;
    public KeyCode rotateHeadRightKey = KeyCode.E;

    [Header("Reset")]
    public KeyCode resetKey = KeyCode.Backspace;

    [Header("Walk Animation Clips - 8 Directions")]
    public AnimationClip idleClip;

    public AnimationClip walkForwardClip;
    public AnimationClip walkBackwardClip;
    public AnimationClip walkLeftClip;
    public AnimationClip walkRightClip;

    public AnimationClip walkForwardLeftClip;
    public AnimationClip walkForwardRightClip;
    public AnimationClip walkBackwardLeftClip;
    public AnimationClip walkBackwardRightClip;

    [Header("Run Animation Clips - Optional")]
    public AnimationClip runForwardClip;
    public AnimationClip runBackwardClip;
    public AnimationClip runLeftClip;
    public AnimationClip runRightClip;

    public AnimationClip runForwardLeftClip;
    public AnimationClip runForwardRightClip;
    public AnimationClip runBackwardLeftClip;
    public AnimationClip runBackwardRightClip;

    [Header("Animation Settings")]
    public bool disableRootMotion = true;

    private Vector3 startPosition;
    private Quaternion startRootRotation;
    private Quaternion startVisualRotation;
    private Quaternion startHeadOrientationLocalRotation;
    private Quaternion startHeadBoneLocalRotation;

    private float headYaw = 0.0f;

    private HumanMoveState currentState = HumanMoveState.Idle;
    private bool currentRunning = false;

    private PlayableGraph graph;
    private AnimationClip currentClip;

    public float HeadYaw
    {
        get { return headYaw; }
    }

    private void Start()
    {
        if (movementRoot == null)
        {
            movementRoot = transform;
        }

        if (visualRoot == null)
        {
            visualRoot = movementRoot;
        }

        if (headOrientationRoot == null)
        {
            headOrientationRoot = visualRoot;
        }

        if (animator == null)
        {
            animator = GetComponentInChildren<Animator>();
        }

        if (animator != null && disableRootMotion)
        {
            animator.applyRootMotion = false;
        }

        startPosition = movementRoot.position;
        startRootRotation = movementRoot.rotation;
        startVisualRotation = visualRoot.rotation;
        startHeadOrientationLocalRotation = headOrientationRoot.localRotation;

        if (headBone != null)
        {
            startHeadBoneLocalRotation = headBone.localRotation;
        }

        PlayAnimation(idleClip);
    }

    private void Update()
    {
        HandleHeadRotationInput();
        HandleMovementInput();

        if (Input.GetKeyDown(resetKey))
        {
            ResetHuman();
        }
    }

    private void LateUpdate()
    {
        ApplyVisualHeadRotation();
    }

    private void HandleHeadRotationInput()
    {
        if (headOrientationRoot == null)
        {
            return;
        }

        float rotateInput = 0.0f;

        if (Input.GetKey(rotateHeadLeftKey))
        {
            rotateInput -= 1.0f;
        }

        if (Input.GetKey(rotateHeadRightKey))
        {
            rotateInput += 1.0f;
        }

        if (Mathf.Abs(rotateInput) <= 0.01f)
        {
            return;
        }

        headYaw += rotateInput * headRotateSpeed * Time.deltaTime;

        if (clampHeadYaw)
        {
            headYaw = Mathf.Clamp(headYaw, -maxHeadYaw, maxHeadYaw);
        }

        headOrientationRoot.localRotation =
            startHeadOrientationLocalRotation *
            Quaternion.Euler(0.0f, headYaw, 0.0f);
    }

    private void ApplyVisualHeadRotation()
    {
        if (!rotateHeadBoneVisual || headBone == null)
        {
            return;
        }

        float visualYaw = headYaw * visualHeadYawMultiplier;

        if (invertHeadRotation)
        {
            visualYaw = -visualYaw;
        }

        Quaternion offsetRotation = Quaternion.identity;

        switch (headRotationAxis)
        {
            case HeadRotationAxis.X:
                offsetRotation = Quaternion.Euler(visualYaw, 0.0f, 0.0f);
                break;

            case HeadRotationAxis.Y:
                offsetRotation = Quaternion.Euler(0.0f, visualYaw, 0.0f);
                break;

            case HeadRotationAxis.Z:
                offsetRotation = Quaternion.Euler(0.0f, 0.0f, visualYaw);
                break;
        }

        headBone.localRotation = startHeadBoneLocalRotation * offsetRotation;
    }

    private void HandleMovementInput()
    {
        if (movementRoot == null || visualRoot == null)
        {
            return;
        }

        float x = 0.0f;
        float z = 0.0f;

        if (Input.GetKey(forwardKey))
        {
            z += 1.0f;
        }

        if (Input.GetKey(backwardKey))
        {
            z -= 1.0f;
        }

        if (Input.GetKey(leftKey))
        {
            x -= 1.0f;
        }

        if (Input.GetKey(rightKey))
        {
            x += 1.0f;
        }

        Vector2 input = new Vector2(x, z);

        bool isMoving = input.sqrMagnitude > 0.01f;
        bool isRunning = Input.GetKey(runKey);

        if (!isMoving)
        {
            SetState(HumanMoveState.Idle, false);
            return;
        }

        input = input.normalized;

        Vector3 moveDirection =
            visualRoot.right * input.x +
            visualRoot.forward * input.y;

        moveDirection.y = 0.0f;

        float speed = isRunning ? runSpeed : walkSpeed;

        movementRoot.position += moveDirection.normalized * speed * Time.deltaTime;

        HumanMoveState nextState = GetStateFromInput(input);

        SetState(nextState, isRunning);
    }

    private HumanMoveState GetStateFromInput(Vector2 input)
    {
        float x = input.x;
        float z = input.y;

        bool forward = z > 0.35f;
        bool backward = z < -0.35f;
        bool left = x < -0.35f;
        bool right = x > 0.35f;

        if (forward && left)
        {
            return HumanMoveState.ForwardLeft;
        }

        if (forward && right)
        {
            return HumanMoveState.ForwardRight;
        }

        if (backward && left)
        {
            return HumanMoveState.BackwardLeft;
        }

        if (backward && right)
        {
            return HumanMoveState.BackwardRight;
        }

        if (forward)
        {
            return HumanMoveState.Forward;
        }

        if (backward)
        {
            return HumanMoveState.Backward;
        }

        if (left)
        {
            return HumanMoveState.Left;
        }

        if (right)
        {
            return HumanMoveState.Right;
        }

        return HumanMoveState.Idle;
    }

    private void SetState(HumanMoveState newState, bool isRunning)
    {
        if (currentState == newState && currentRunning == isRunning)
        {
            return;
        }

        currentState = newState;
        currentRunning = isRunning;

        AnimationClip clip = GetClipForState(newState, isRunning);

        if (clip == null)
        {
            clip = idleClip;
        }

        PlayAnimation(clip);
    }

    private AnimationClip GetClipForState(HumanMoveState state, bool isRunning)
    {
        if (state == HumanMoveState.Idle)
        {
            return idleClip;
        }

        if (isRunning)
        {
            AnimationClip runClip = GetRunClip(state);

            if (runClip != null)
            {
                return runClip;
            }
        }

        return GetWalkClip(state);
    }

    private AnimationClip GetWalkClip(HumanMoveState state)
    {
        switch (state)
        {
            case HumanMoveState.Forward:
                return walkForwardClip;

            case HumanMoveState.Backward:
                return walkBackwardClip != null ? walkBackwardClip : walkForwardClip;

            case HumanMoveState.Left:
                return walkLeftClip != null ? walkLeftClip : walkForwardClip;

            case HumanMoveState.Right:
                return walkRightClip != null ? walkRightClip : walkForwardClip;

            case HumanMoveState.ForwardLeft:
                return walkForwardLeftClip != null ? walkForwardLeftClip : walkLeftClip;

            case HumanMoveState.ForwardRight:
                return walkForwardRightClip != null ? walkForwardRightClip : walkRightClip;

            case HumanMoveState.BackwardLeft:
                return walkBackwardLeftClip != null ? walkBackwardLeftClip : walkBackwardClip;

            case HumanMoveState.BackwardRight:
                return walkBackwardRightClip != null ? walkBackwardRightClip : walkBackwardClip;

            case HumanMoveState.Idle:
            default:
                return idleClip;
        }
    }

    private AnimationClip GetRunClip(HumanMoveState state)
    {
        switch (state)
        {
            case HumanMoveState.Forward:
                return runForwardClip;

            case HumanMoveState.Backward:
                return runBackwardClip;

            case HumanMoveState.Left:
                return runLeftClip;

            case HumanMoveState.Right:
                return runRightClip;

            case HumanMoveState.ForwardLeft:
                return runForwardLeftClip;

            case HumanMoveState.ForwardRight:
                return runForwardRightClip;

            case HumanMoveState.BackwardLeft:
                return runBackwardLeftClip;

            case HumanMoveState.BackwardRight:
                return runBackwardRightClip;

            case HumanMoveState.Idle:
            default:
                return null;
        }
    }

    private void PlayAnimation(AnimationClip clip)
    {
        if (animator == null || clip == null)
        {
            return;
        }

        if (currentClip == clip)
        {
            return;
        }

        currentClip = clip;

        if (graph.IsValid())
        {
            graph.Destroy();
        }

        graph = PlayableGraph.Create("Human8DirectionAnimationGraph");
        graph.SetTimeUpdateMode(DirectorUpdateMode.GameTime);

        AnimationPlayableOutput output = AnimationPlayableOutput.Create(
            graph,
            "HumanAnimationOutput",
            animator
        );

        AnimationClipPlayable playable = AnimationClipPlayable.Create(graph, clip);
        playable.SetApplyFootIK(true);

        output.SetSourcePlayable(playable);
        graph.Play();
    }

    private void ResetHuman()
    {
        movementRoot.position = startPosition;
        movementRoot.rotation = startRootRotation;

        visualRoot.rotation = startVisualRotation;

        headYaw = 0.0f;

        if (headOrientationRoot != null)
        {
            headOrientationRoot.localRotation = startHeadOrientationLocalRotation;
        }

        if (headBone != null)
        {
            headBone.localRotation = startHeadBoneLocalRotation;
        }

        SetState(HumanMoveState.Idle, false);
    }

    private void OnDisable()
    {
        if (graph.IsValid())
        {
            graph.Destroy();
        }
    }

    private void OnDestroy()
    {
        if (graph.IsValid())
        {
            graph.Destroy();
        }
    }
}