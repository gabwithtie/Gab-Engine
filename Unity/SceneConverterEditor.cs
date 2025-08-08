using UnityEngine;
using UnityEditor;
using System.IO;

[CustomEditor(typeof(SceneConverter))]
public class SceneConverterEditor : Editor
{
    private SceneConverter converter;

    private void OnEnable()
    {
        converter = (SceneConverter)target;
    }

    public override void OnInspectorGUI()
    {
        DrawDefaultInspector();

        EditorGUILayout.Space();

        if (GUILayout.Button("Save Current Scene to JSON"))
        {
            string path = EditorUtility.SaveFilePanel("Save Scene as JSON", "", "scene", converter.fileExtension);
            if (!string.IsNullOrEmpty(path))
            {
                converter.SaveFullSceneToJson(path);
            }
        }

        if (GUILayout.Button("Load Scene from JSON"))
        {
            string path = EditorUtility.OpenFilePanel("Load Scene from JSON", "", converter.fileExtension);
            if (!string.IsNullOrEmpty(path))
            {
                converter.LoadSceneFromJson(path);
            }
        }
    }
}
