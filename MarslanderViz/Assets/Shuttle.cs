using UnityEngine;
using static UnityEngine.RuleTile.TilingRuleOutput;

public class Shuttle : MonoBehaviour
{
    private int _thrust;

    public SpriteRenderer body;
    public SpriteRenderer jet;
    public Sprite[] jetFlames;

    public float JetOpacity
    {
        get => jet.color.a;
        set => jet.color = WithAlpha(jet.color, value);
    }

    public int Thrust
    {
        get => _thrust;
        set
        {
            _thrust = value;
            jet.sprite = _thrust > 0 ? jetFlames[_thrust - 1] : null;
        }
    }

    public void SetSilhouette(bool enabled)
    {
        var tintColor = enabled ? Color.black : Color.white;
        body.color = tintColor;
        jet.color = tintColor;
    }

    private static Color WithAlpha(Color color, float alpha)
    {
        color.a = alpha;
        return color;
    }
}

internal sealed class ShuttleInterpolation
{
    public static readonly Vector2 fieldSize = new(6999, 2999);

    public Shuttle shuttle;
    public SurfaceMesh surface;

    public ReplayData.GameTurn ApplyTurn(ReplayData.GameTurn current)
    {
        shuttle.Thrust = current.Thrust;
        shuttle.JetOpacity = 1;
        shuttle.transform.SetPositionAndRotation(
            (Vector3)surface.GetSurfacePosition(fieldSize, current.Position),
            Quaternion.Euler(0, 0, current.Tilt));

        return current;
    }

    public ReplayData.GameTurn ApplyTurn(ReplayData.GameTurn current, ReplayData.GameTurn next, float t)
    {
        var shuttlePos = Vector2.Lerp(current.Position, next.Position, t);
        Vector3 position = surface.GetSurfacePosition(fieldSize, shuttlePos);

        var shuttleTilt = next.Tilt != current.Tilt
            ? Mathf.Lerp(current.Tilt, next.Tilt,
                t * 15.0f / Mathf.Abs(next.Tilt - current.Tilt))
            : next.Tilt;
        var rotation = Quaternion.Euler(0, 0, shuttleTilt);

        shuttle.Thrust = current.Thrust;
        shuttle.JetOpacity = Mathf.Lerp(current.Thrust, next.Thrust, t) / 4;
        shuttle.transform.SetPositionAndRotation(position, rotation);

        return new ReplayData.GameTurn(
            Mathf.RoundToInt(Mathf.Lerp(current.Fuel, next.Fuel, t)),
            Mathf.RoundToInt(Mathf.Lerp(current.Thrust, next.Thrust, t)),
            Mathf.RoundToInt(shuttleTilt),
            new Vector2Int(
                Mathf.RoundToInt(shuttlePos.x),
                Mathf.RoundToInt(shuttlePos.y)),
            Vector2.Lerp(current.Velocity, next.Velocity, t));
    }
}
