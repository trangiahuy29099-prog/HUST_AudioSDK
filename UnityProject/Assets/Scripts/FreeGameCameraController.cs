using UnityEngine;

public class FreeGameCameraController : MonoBehaviour
{
    [Header("Orbit Target")]
    public Transform orbitTarget;
    public Vector3 fallbackOrbitPoint = Vector3.zero;

    [Header("Orbit")]
    public float orbitDistance = 25.0f;
    public float minOrbitDistance = 2.0f;
    public float maxOrbitDistance = 120.0f;
    public float orbitSensitivity = 4.0f;

    [Header("Zoom")]
    public float zoomSpeed = 8.0f;

    [Header("Right Mouse Look")]
    public float lookSensitivity = 2.0f;

    [Header("Orbit Modifier")]
    public bool useAltForOrbit = true;
    public bool useCtrlForOrbit = true;

    private float yaw;
    private float pitch;
    private bool wasOrbitingLastFrame = false;

    private void Start()
    {
        SyncAnglesFromCurrentCamera();

        if (orbitTarget != null)
        {
            orbitDistance = Vector3.Distance(transform.position, orbitTarget.position);
        }
    }

    private void Update()
    {
        HandleOrbit();
        HandleScrollZoom();
        HandleRightMouseLookOnly();
    }

    private void HandleOrbit()
    {
        bool orbitModifier = IsOrbitModifierPressed();
        bool orbitingNow = orbitModifier && Input.GetMouseButton(0);

        if (!orbitingNow)
        {
            wasOrbitingLastFrame = false;
            return;
        }

        // Quan trọng:
        // Khi vừa bắt đầu giữ Ctrl/Alt + chuột trái,
        // lấy vị trí camera hiện tại làm điểm bắt đầu orbit.
        // Nhờ vậy camera không bị snap/reset về vị trí cũ.
        if (!wasOrbitingLastFrame)
        {
            SyncOrbitStateFromCurrentCamera();
            wasOrbitingLastFrame = true;
        }

        float mouseX = Input.GetAxis("Mouse X") * orbitSensitivity;
        float mouseY = Input.GetAxis("Mouse Y") * orbitSensitivity;

        yaw += mouseX;
        pitch -= mouseY;
        pitch = Mathf.Clamp(pitch, -85.0f, 85.0f);

        ApplyOrbitPosition();
    }

    private void HandleScrollZoom()
    {
        float scroll = Input.mouseScrollDelta.y;

        if (Mathf.Abs(scroll) < 0.01f)
        {
            return;
        }

        if (IsOrbitModifierPressed())
        {
            // Zoom kiểu Scene View quanh target.
            SyncOrbitDistanceFromCurrentCamera();

            orbitDistance -= scroll * zoomSpeed;
            orbitDistance = Mathf.Clamp(orbitDistance, minOrbitDistance, maxOrbitDistance);

            ApplyOrbitPosition();
        }
        else
        {
            // Zoom thường theo hướng camera hiện tại.
            transform.position += transform.forward * scroll * zoomSpeed;
        }
    }

    private void HandleRightMouseLookOnly()
    {
        if (!Input.GetMouseButton(1))
        {
            return;
        }

        float mouseX = Input.GetAxis("Mouse X") * lookSensitivity;
        float mouseY = Input.GetAxis("Mouse Y") * lookSensitivity;

        yaw += mouseX;
        pitch -= mouseY;
        pitch = Mathf.Clamp(pitch, -85.0f, 85.0f);

        transform.rotation = Quaternion.Euler(pitch, yaw, 0.0f);
    }

    private void ApplyOrbitPosition()
    {
        Vector3 pivot = GetOrbitPoint();

        Quaternion rotation = Quaternion.Euler(pitch, yaw, 0.0f);
        Vector3 offset = rotation * new Vector3(0.0f, 0.0f, -orbitDistance);

        transform.position = pivot + offset;
        transform.rotation = rotation;
    }

    private void SyncOrbitStateFromCurrentCamera()
    {
        Vector3 pivot = GetOrbitPoint();
        Vector3 toCamera = transform.position - pivot;

        if (toCamera.magnitude < 0.01f)
        {
            toCamera = -transform.forward * orbitDistance;
        }

        orbitDistance = Mathf.Clamp(
            toCamera.magnitude,
            minOrbitDistance,
            maxOrbitDistance
        );

        // Cho camera nhìn về pivot giống Scene View orbit.
        Vector3 lookDirection = pivot - transform.position;

        if (lookDirection.sqrMagnitude > 0.01f)
        {
            Quaternion lookRotation = Quaternion.LookRotation(lookDirection.normalized, Vector3.up);

            Vector3 euler = lookRotation.eulerAngles;
            yaw = euler.y;
            pitch = NormalizePitch(euler.x);

            transform.rotation = lookRotation;
        }
        else
        {
            SyncAnglesFromCurrentCamera();
        }
    }

    private void SyncOrbitDistanceFromCurrentCamera()
    {
        Vector3 pivot = GetOrbitPoint();
        orbitDistance = Mathf.Clamp(
            Vector3.Distance(transform.position, pivot),
            minOrbitDistance,
            maxOrbitDistance
        );
    }

    private void SyncAnglesFromCurrentCamera()
    {
        Vector3 euler = transform.rotation.eulerAngles;

        yaw = euler.y;
        pitch = NormalizePitch(euler.x);
    }

    private Vector3 GetOrbitPoint()
    {
        if (orbitTarget != null)
        {
            return orbitTarget.position;
        }

        return fallbackOrbitPoint;
    }

    private bool IsOrbitModifierPressed()
    {
        bool altPressed =
            Input.GetKey(KeyCode.LeftAlt) ||
            Input.GetKey(KeyCode.RightAlt);

        bool ctrlPressed =
            Input.GetKey(KeyCode.LeftControl) ||
            Input.GetKey(KeyCode.RightControl);

        return (useAltForOrbit && altPressed) || (useCtrlForOrbit && ctrlPressed);
    }

    private float NormalizePitch(float angle)
    {
        if (angle > 180.0f)
        {
            angle -= 360.0f;
        }

        return angle;
    }
}