using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;

[ExecuteInEditMode]
[RequireComponent(typeof(MeshFilter))]
public class SurfaceMesh : MonoBehaviour
{
    private MeshFilter _mf;

    public Vector2 size;
    public Camera eye;

    private static readonly Vector2 _center = new Vector2(-.5f, -.5f);

    public void GenerateMesh(Vector2 surfaceSize, IEnumerable<Vector2> surface)
    {
        var uvs = surface.SelectMany(p =>
        {
            var odd = p / surfaceSize;
            return AsTuple(new Vector2(odd.x, 0), odd);
        }).ToArray();

        var m = new Mesh();
        if (uvs.Length > 2 && uvs.Length % 2 == 0) {
            var verts = uvs.Select(p => (Vector3)((p + _center) * size)).ToArray();
            var inds = new int[(verts.Length - 2) * 3];
            for (int i = 0, imax = verts.Length - 2, j = 0; i < imax; i += 2)
            {
                inds[j++] = i + 0;
                inds[j++] = i + 1;
                inds[j++] = i + 2;
                inds[j++] = i + 1;
                inds[j++] = i + 3;
                inds[j++] = i + 2;
            }

            m.SetVertices(verts);
            m.SetIndices(inds, MeshTopology.Triangles, 0);
            m.SetUVs(0, uvs);
        }
        _mf.mesh = m;
    }

    public Vector2 GetSurfacePosition(Vector2 surfaceSize, Vector2 p)
    {
        return size * (p / surfaceSize + _center);
    }

    private static IEnumerable<T> AsTuple<T>(T t1, T t2) {
        yield return t1;
        yield return t2;
    }

    // Start is called before the first frame update
    private void Start()
    {
        _mf = GetComponent<MeshFilter>();

        UpdateSizeFromEye();
    }

    private void Update()
    {
        if (!Application.isEditor) return;

        UpdateSizeFromEye();
    }

    private void OnDrawGizmos()
    {
        Gizmos.color = Color.green;
        Gizmos.DrawWireCube(Vector3.zero, size);
    }

    private void UpdateSizeFromEye()
    {
        if (!eye) return;

        var max = eye.ViewportToWorldPoint(new Vector3(1, 1), Camera.MonoOrStereoscopicEye.Mono);
        var min = eye.ViewportToWorldPoint(new Vector3(0, 0), Camera.MonoOrStereoscopicEye.Mono);

        size = max - min;
    }
}
