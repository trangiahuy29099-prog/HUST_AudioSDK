using UnityEngine;

namespace YueUltimateDronePhysics
{
    public class PCDroneEmulatorWASD : MonoBehaviour
    {
        [Header("References")]
        [SerializeField] private YueDronePhysics dronePhysics;
        [SerializeField] private YueInputModule inputModule;
        [SerializeField] private Rigidbody rb;

        [Header("Controls")]
        public KeyCode forwardKey = KeyCode.W;
        public KeyCode backwardKey = KeyCode.S;
        public KeyCode leftKey = KeyCode.A;
        public KeyCode rightKey = KeyCode.D;
        public KeyCode modifierKey = KeyCode.Space;
        public KeyCode resetKey = KeyCode.Backspace;

        [Header("Easy Assisted Movement")]
        public bool useAssistedMovement = true;
        public float forwardSpeed = 8.0f;
        public float strafeSpeed = 6.0f;
        public float verticalSpeed = 5.0f;
        public float yawSpeed = 90.0f;
        public float acceleration = 8.0f;

        [Header("Send Input To Yue Physics")]
        public bool alsoSendRawInput = true;
        public float rawMoveStrength = 0.6f;
        public float rawTurnStrength = 0.6f;

        [Header("Stabilization")]
        public bool keepUpright = true;
        public bool dampAngularVelocity = true;
        public float angularDamping = 0.85f;

        [Header("Debug")]
        public bool debugInput = false;

        private Vector3 startPos;
        private Quaternion startRot;

        private Vector3 targetVelocity;

        private void Awake()
        {
            AutoFindReferences();
        }

        private void Start()
        {
            AutoFindReferences();

            startPos = transform.position;
            startRot = transform.rotation;
        }

        private void AutoFindReferences()
        {
            if (dronePhysics == null)
            {
                dronePhysics = GetComponent<YueDronePhysics>();
            }

            if (inputModule == null)
            {
                inputModule = GetComponent<YueInputModule>();
            }

            if (rb == null)
            {
                rb = GetComponent<Rigidbody>();
            }
        }

        private void Update()
        {
            if (alsoSendRawInput)
            {
                SendRawInputToYueModule();
            }

            if (Input.GetKeyDown(resetKey))
            {
                ResetDrone();
            }
        }

        private void FixedUpdate()
        {
            if (!useAssistedMovement || rb == null)
            {
                return;
            }

            HandleAssistedMovement();
        }

        private void SendRawInputToYueModule()
        {
            if (inputModule == null)
            {
                return;
            }

            float rightHorizontal = 0.0f;
            float rightVertical = 0.0f;
            float leftHorizontal = 0.0f;
            float leftVertical = 0.0f;

            bool modifier = Input.GetKey(modifierKey);

            if (!modifier)
            {
                if (Input.GetKey(forwardKey))
                {
                    rightVertical += rawMoveStrength;
                }

                if (Input.GetKey(backwardKey))
                {
                    rightVertical -= rawMoveStrength;
                }

                if (Input.GetKey(leftKey))
                {
                    leftHorizontal -= rawTurnStrength;
                }

                if (Input.GetKey(rightKey))
                {
                    leftHorizontal += rawTurnStrength;
                }
            }
            else
            {
                if (Input.GetKey(forwardKey))
                {
                    leftVertical += rawMoveStrength;
                }

                if (Input.GetKey(backwardKey))
                {
                    leftVertical -= rawMoveStrength;
                }

                if (Input.GetKey(leftKey))
                {
                    rightHorizontal -= rawMoveStrength;
                }

                if (Input.GetKey(rightKey))
                {
                    rightHorizontal += rawMoveStrength;
                }
            }

            inputModule.rawRightHorizontal = rightHorizontal;
            inputModule.rawRightVertical = rightVertical;
            inputModule.rawLeftHorizontal = leftHorizontal;
            inputModule.rawLeftVertical = leftVertical;
        }

        private void HandleAssistedMovement()
        {
            bool modifier = Input.GetKey(modifierKey);

            Vector3 desiredVelocity = Vector3.zero;

            if (!modifier)
            {
                // W/S = tiến/lùi
                if (Input.GetKey(forwardKey))
                {
                    desiredVelocity += transform.forward * forwardSpeed;
                }

                if (Input.GetKey(backwardKey))
                {
                    desiredVelocity -= transform.forward * forwardSpeed;
                }

                // A/D = xoay trái/phải
                if (Input.GetKey(leftKey))
                {
                    rb.MoveRotation(
                        rb.rotation * Quaternion.Euler(0.0f, -yawSpeed * Time.fixedDeltaTime, 0.0f)
                    );
                }

                if (Input.GetKey(rightKey))
                {
                    rb.MoveRotation(
                        rb.rotation * Quaternion.Euler(0.0f, yawSpeed * Time.fixedDeltaTime, 0.0f)
                    );
                }
            }
            else
            {
                // Space + W/S = lên/xuống
                if (Input.GetKey(forwardKey))
                {
                    desiredVelocity += Vector3.up * verticalSpeed;
                }

                if (Input.GetKey(backwardKey))
                {
                    desiredVelocity += Vector3.down * verticalSpeed;
                }

                // Space + A/D = trượt trái/phải
                if (Input.GetKey(leftKey))
                {
                    desiredVelocity -= transform.right * strafeSpeed;
                }

                if (Input.GetKey(rightKey))
                {
                    desiredVelocity += transform.right * strafeSpeed;
                }
            }

            Vector3 currentVelocity = rb.linearVelocity;

            targetVelocity = Vector3.Lerp(
                currentVelocity,
                desiredVelocity,
                acceleration * Time.fixedDeltaTime
            );

            rb.linearVelocity = targetVelocity;

            if (keepUpright)
            {
                Vector3 euler = rb.rotation.eulerAngles;
                rb.MoveRotation(Quaternion.Euler(0.0f, euler.y, 0.0f));
            }

            if (dampAngularVelocity)
            {
                rb.angularVelocity *= angularDamping;
            }

            if (debugInput && desiredVelocity.sqrMagnitude > 0.01f)
            {
                Debug.Log("[PCDroneEmulatorWASD] Assisted velocity = " + desiredVelocity.ToString("F2"));
            }
        }

        private void ResetDrone()
        {
            transform.position = startPos;
            transform.rotation = startRot;

            if (rb != null)
            {
                rb.linearVelocity = Vector3.zero;
                rb.angularVelocity = Vector3.zero;
            }

            if (dronePhysics != null)
            {
                dronePhysics.ResetInternals();
            }
        }
    }
}