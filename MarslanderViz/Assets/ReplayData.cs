using Newtonsoft.Json;
using UnityEngine;

internal sealed class ReplayData
{
    public sealed record Span<T>(
        [JsonProperty("start")] T Start,
        [JsonProperty("end")] T End);

    public sealed record GameInit(
        [JsonProperty("surface")] Vector2Int[] Points,
        [JsonProperty("safe_area")] Span<int> SafeArea);

    public sealed record GameTurn(
        [JsonProperty("fuel")] int Fuel,
        [JsonProperty("thrust")] int Thrust,
        [JsonProperty("tilt")] int Tilt,
        [JsonProperty("position")] Vector2Int Position,
        [JsonProperty("velocity")] Vector2 Velocity);

    public enum GameOutcome { Aerial = -1, Landed, Crashed, Lost }

    [JsonProperty("case_id")] public ulong CaseId;
    [JsonProperty("gene_id")] public ulong GeneId;
    [JsonProperty("outcome")] public GameOutcome Outcome;
    [JsonProperty("surface")] public GameInit Surface;
    [JsonProperty("turns")] public GameTurn[] Turns;

    [JsonProperty("state")] public string State;

}
