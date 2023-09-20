using System;
using System.Collections;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEngine.InputSystem;

public class Game : MonoBehaviour
{
    private readonly Vector2 _fieldSize = new(6999, 2999);

    #region Native API

    [StructLayout(LayoutKind.Sequential)]
    struct point<T>
    {
        public T x, y;
    }

    [StructLayout(LayoutKind.Sequential)]
    struct GameTurnInput
    {
        public int fuel, thrust, tilt;
        public point<int> position;
        public point<double> velocity;
    }

    [StructLayout(LayoutKind.Sequential)]
    struct GameTurnOutput
    {
        public int thrust, tilt;
    }

    [DllImport("marslander-plugin")]
    private extern static IntPtr BeginSimulation(
        [MarshalAs(UnmanagedType.LPStr)] string state);

    [DllImport("marslander-plugin")]
    private extern static IntPtr EndSimulation([In, Out] ref IntPtr ctx);

    [DllImport("marslander-plugin")]
    private extern static GameTurnInput GetTurnInput(IntPtr ctx);

    [DllImport("marslander-plugin")]
    private extern static ReplayData.GameOutcome StepSimulation(
        IntPtr ctx, ref GameTurnOutput output);

    #endregion Native API

    public Shuttle shuttle;
    public SurfaceMesh surface;
    public Camera eye;

    [HideInInspector]
    internal TelemetryOutput telemetry;
    [HideInInspector]
    internal IOutcomeListener outcomeListener;

    private Marslander _input;

    private IntPtr _gamePtr;
    private GameTurnOutput _gameOutput;

    private Action _cb;
    private Coroutine _playback;
    private ShuttleInterpolation _ipol;
    private ReplayData _replay;

    internal void Play(Action onStart)
    {
        Stop();

        _gamePtr = BeginSimulation(_replay.State);
        _cb = onStart;

        _ipol = new ShuttleInterpolation
        {
            shuttle = shuttle,
            surface = surface
        };

        outcomeListener?.OnSimulationReset();

        var current = GetTurnInput(_gamePtr);
        ApplyTurn(current);

        _input.Enable();
    }

    public void Stop()
    {
        if (_playback != null)
        {
            StopCoroutine(_playback);
            _playback = null;
        }

        _input.Disable();
        _ipol = null;

        _cb = null;
        EndSimulation(ref _gamePtr);
    }

    internal void SetReplay(ReplayData replay)
    {
        Stop();

        _replay = replay;
    }

    private void OnDestroy()
    {
        _cb = null;
        EndSimulation(ref _gamePtr);
    }

    void OnDisable()
    {
        Stop();
        StopAllCoroutines();
    }

    void OnEnable()
    {
        if (_input == null)
        {
            _input = new Marslander();
            _input.Player.Thrust.performed += OnThrust;
            _input.Player.Center.performed += OnTilt;
            _input.Player.Tilt.performed += OnTilt;
        }
    }

    private static ReplayData.GameTurn CvtTurn(GameTurnInput turn)
    {
        return new ReplayData.GameTurn(turn.fuel, turn.thrust, turn.tilt,
            new Vector2Int(turn.position.x, turn.position.y),
            new Vector2((float)turn.velocity.x, (float)turn.velocity.y));
    }

    private void OnThrust(InputAction.CallbackContext obj)
    {
        _gameOutput.thrust = int.Parse(obj.control.name);

        StartPlayback();
    }

    private void OnTilt(InputAction.CallbackContext obj)
    {
        if (obj.action == _input.Player.Center)
        {
            _gameOutput.tilt = 0;
        }
        else
        {
            var dir = obj.ReadValue<Vector2>()
                - (Vector2)eye.WorldToScreenPoint(
                    shuttle.transform.position);

            _gameOutput.tilt = Mathf.RoundToInt(
                180 * Mathf.Atan2(dir.y, dir.x) / Mathf.PI) - 90;
        }

        StartPlayback();
    }

    private void ApplyTurn(GameTurnInput currentInput, GameTurnInput nextInput, float t)
    {
        var current = CvtTurn(currentInput);
        current = _ipol.ApplyTurn(current, CvtTurn(nextInput), t);
        if (telemetry) telemetry.WriteTelemetry(current);
    }

    private void ApplyTurn(GameTurnInput currentInput)
    {
        var current = CvtTurn(currentInput);
        current = _ipol.ApplyTurn(current);
        if (telemetry) telemetry.WriteTelemetry(current);
    }

    private IEnumerator Playback()
    {
        var current = GetTurnInput(_gamePtr);
        ApplyTurn(current);

        yield return new WaitForSeconds(1);

        _cb?.Invoke();

        var o = StepSimulation(_gamePtr, ref _gameOutput);
        var next = GetTurnInput(_gamePtr);

        var time = .0f;
        while (true)
        {
            while (time < 1)
            {
                ApplyTurn(current, next, time);

                yield return null;
                time += Time.deltaTime;
            }

            ApplyTurn(next);
            current = next;

            while (time >= 1 && o == ReplayData.GameOutcome.Aerial)
            {
                o = StepSimulation(_gamePtr, ref _gameOutput);
                time -= 1;
            }

            if (o != ReplayData.GameOutcome.Aerial)
            {
                Stop();

                outcomeListener?.OnSimulationComplete(o);
                yield break;
            }

            next = GetTurnInput(_gamePtr);
        }
    }

    private void StartPlayback()
    {
        _playback ??= StartCoroutine(Playback());
    }
}
