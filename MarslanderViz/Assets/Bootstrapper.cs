using System;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using Newtonsoft.Json;
using OSBridge.Windows;
using TMPro;
using Unity.VisualScripting;
using UnityEngine;
using UnityEngine.UI;

internal interface IOutcomeListener
{
    void OnSimulationComplete(ReplayData.GameOutcome outcome);

    void OnSimulationReset();
}

public class Bootstrapper : MonoBehaviour, IOutcomeListener
{
    public Game game;
    public Player player;
    public TelemetryOutput output;
    public TMP_Text outcome;
    public Toggle allowUserControl;

    private Action _start;

    public async void OpenReplay()
    {
        outcome.gameObject.SetActive(false);

        var replay = await ReadReplayAsync();

        player.SetReplay(replay);

        if (!allowUserControl || !allowUserControl.isOn)
        {
            game.gameObject.SetActive(false);
            game.SetReplay(null);
            game.outcomeListener = null;
            game.telemetry = null;
            output.AttachToTransform(player.shuttle.transform);
            player.outcomeListener = this;
            player.progress.interactable = true;
            player.shuttle.SetSilhouette(false);
            player.telemetry = output;

            _start = player.Play;
        }
        else
        {
            game.gameObject.SetActive(true);
            game.SetReplay(replay);
            game.outcomeListener = this;
            game.telemetry = output;
            output.AttachToTransform(game.shuttle.transform);
            player.outcomeListener = null;
            player.progress.interactable = false;
            player.shuttle.SetSilhouette(true);
            player.telemetry = null;

            _start = () => game.Play(player.Play);
        }

        Restart();
    }

    public void Restart()
    {
        _start();
    }

    private static async Task<ReplayData> ReadReplayAsync()
    {
        var r = await new FileOpenDialog().WithFilter("JSON", "*.json").ShowAsync()
            .ConfigureAwait(false);

        using var reader = new JsonTextReader(new StreamReader(r.Single().LocalPath));
        var json = new JsonSerializer();
        return json.Deserialize<ReplayData>(reader);
    }

    [System.Diagnostics.CodeAnalysis.SuppressMessage("Roslynator", "RCS1213:Remove unused member declaration.", Justification = "<Pending>")]
    private void Start()
    {
        game.gameObject.SetActive(false);
        outcome.gameObject.SetActive(false);
    }

    void IOutcomeListener.OnSimulationComplete(ReplayData.GameOutcome o)
    {
        outcome.gameObject.SetActive(true);
        outcome.text = o.ToString();
        outcome.color = o switch
        {
            ReplayData.GameOutcome.Landed => Color.green,
            ReplayData.GameOutcome.Crashed => Color.red,
            _ => Color.yellow,
        };
    }

    void IOutcomeListener.OnSimulationReset()
    {
        outcome.gameObject.SetActive(false);
    }
}
