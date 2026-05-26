using UnityEngine;

public class AudioOcclusionMaterial : MonoBehaviour
{
    public enum OcclusionMaterialType
    {
        Generic,
        Wood,
        Stone,
        Metal,
        Glass
    }

    [Header("Material Type")]
    public OcclusionMaterialType materialType = OcclusionMaterialType.Generic;

    [Header("Occlusion Settings")]
    [Range(0.0f, 1.0f)]
    public float occlusionAmount = 0.65f;

    [Header("Debug Info")]
    public string materialLabel = "Generic";

    private void OnValidate()
    {
        switch (materialType)
        {
            case OcclusionMaterialType.Wood:
                materialLabel = "Wood";
                if (Mathf.Approximately(occlusionAmount, 0.65f))
                {
                    occlusionAmount = 0.55f;
                }
                break;

            case OcclusionMaterialType.Stone:
                materialLabel = "Stone";
                if (Mathf.Approximately(occlusionAmount, 0.65f))
                {
                    occlusionAmount = 0.85f;
                }
                break;

            case OcclusionMaterialType.Metal:
                materialLabel = "Metal";
                if (Mathf.Approximately(occlusionAmount, 0.65f))
                {
                    occlusionAmount = 0.75f;
                }
                break;

            case OcclusionMaterialType.Glass:
                materialLabel = "Glass";
                if (Mathf.Approximately(occlusionAmount, 0.65f))
                {
                    occlusionAmount = 0.35f;
                }
                break;

            case OcclusionMaterialType.Generic:
            default:
                materialLabel = "Generic";
                break;
        }

        occlusionAmount = Mathf.Clamp01(occlusionAmount);
    }

    public float GetOcclusionAmount()
    {
        return Mathf.Clamp01(occlusionAmount);
    }

    public string GetMaterialName()
    {
        return materialLabel;
    }
}