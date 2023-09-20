using System.Collections;
using System.Linq;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class Player : MonoBehaviour
{
    public Shuttle shuttle;
    public SurfaceMesh surface;
    public Slider progress;

    [HideInInspector]
    internal TelemetryOutput telemetry;
    [HideInInspector]
    internal IOutcomeListener outcomeListener;

    private float _time;
    private float _duration;
    private Coroutine _playback;
    private ShuttleInterpolation _ipol;
    private ReplayData _replay;

    public bool IsPlaying => _playback != null;

    public void Play()
    {
        if (_playback == null)
            _playback = StartCoroutine(Playback());
    }

    public void Stop()
    {
        if (_playback != null)
        {
            _ipol = null;

            StopCoroutine(_playback);
            _playback = null;
        }
    }

    internal void SetReplay(ReplayData replay)
    {
        Stop();

        _replay = replay;
        surface.GenerateMesh(ShuttleInterpolation.fieldSize,
            _replay.Surface.Points.Select(p => (Vector2)p));

        _duration = _replay.Turns.Length - 1;

        progress.minValue = 0;
        progress.maxValue = _duration;
        UpdateUiState();

        shuttle.transform.SetLocalPositionAndRotation(
            new Vector3(20, 20, 0), Quaternion.identity);
    }

    private void ApplyTurn(ReplayData.GameTurn current)
    {
        /*current =*/ _ipol.ApplyTurn(current);
        if (telemetry) telemetry.WriteTelemetry(current);
    }

    private void ApplyTurn(ReplayData.GameTurn current, ReplayData.GameTurn next, float t)
    {
        /*current =*/ _ipol.ApplyTurn(current, next, t);
        if (telemetry) telemetry.WriteTelemetry(current);
    }

    private void OnDestroy()
    {
        Stop();
        StopAllCoroutines();
    }

    private void OnEnable()
    {
        UpdateUiState();
        progress.onValueChanged.AddListener((value) =>
        {
            _time = value;

            if (IsPlaying) UpdateTimeStep();
            else Play();
        });
    }

    private IEnumerator Playback()
    {
        _ipol = new ShuttleInterpolation {
            shuttle = shuttle,
            surface = surface
        };

        outcomeListener?.OnSimulationReset();

        _time = 0;
        while (UpdateTimeStep())
        {
            yield return null;

            _time += Time.deltaTime;
            progress.SetValueWithoutNotify(_time);
        }
    }

    private bool UpdateTimeStep()
    {
        var i = Mathf.FloorToInt(_time);
        var j = i + 1;

        if (j >= _replay.Turns.Length)
        {
            ApplyTurn(_replay.Turns[i]);
            Stop();

            outcomeListener?.OnSimulationComplete(_replay.Outcome);
            return false;
        }

        ApplyTurn(_replay.Turns[i], _replay.Turns[j], _time - i);
        return true;
    }

    private void UpdateUiState()
    {
        progress.interactable = _replay != null;
    }
}
