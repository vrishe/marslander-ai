using UnityEngine;

[ExecuteInEditMode]
public class JetPropulsion : MonoBehaviour
{
    private float _delay, _scale;

    [Min(1)]
    public float freq = 1;
    public float scaleMax, scaleMin;

    void Update()
    {
        _delay -= Time.deltaTime;
        if (_delay < 0)
        {
            _delay = 1 / freq;
            _scale = freq * (Random.Range(scaleMax, scaleMin) - transform.localScale.y);
        }
        else
        {
            var s = transform.localScale;
            s.y += _scale * Time.deltaTime;
            transform.localScale = s;
        }
    }
}
