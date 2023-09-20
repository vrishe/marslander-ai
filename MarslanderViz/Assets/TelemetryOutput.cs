using TMPro;
using UnityEngine;
using UnityEngine.Animations;

[RequireComponent(typeof(PositionConstraint))]
public sealed class TelemetryOutput : MonoBehaviour
{
    public TMP_Text fuelValue;
    public TMP_Text thrustValue;
    public TMP_Text tiltValue;
    public TMP_Text velocityXValue;
    public TMP_Text velocityYValue;

    private PositionConstraint _contraint;

    public void AttachToTransform(Transform transform)
    {
        _contraint.SetSource(0, new ConstraintSource { sourceTransform = transform, weight = 1 });
    }

    internal void WriteTelemetry(ReplayData.GameTurn current)
    {
        fuelValue.text = current.Fuel.ToString();
        thrustValue.text = current.Thrust.ToString();
        tiltValue.text = current.Tilt.ToString();
        velocityXValue.text = current.Velocity.x.ToString("F2");
        velocityYValue.text = current.Velocity.y.ToString("F2");
    }

    void Start()
    {
        _contraint = GetComponent<PositionConstraint>();
    }
}
