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
        // Draw the default inspector properties first
        DrawDefaultInspector();

        EditorGUILayout.Space();

        // Add a button to save the scene to a JSON file
        if (GUILayout.Button("Save Current Scene to JSON"))
        {
            // Use the fileExtension field from the SceneConverter ScriptableObject
            string path = EditorUtility.SaveFilePanel("Save Scene as JSON", "", "scene", converter.fileExtension);
            if (!string.IsNullOrEmpty(path))
            {
                converter.SaveFullSceneToJson(path);
            }
        }

        // Add a button to load a scene from a JSON file
        if (GUILayout.Button("Load Scene from JSON"))
        {
            // Use the fileExtension field from the SceneConverter ScriptableObject
            string path = EditorUtility.OpenFilePanel("Load Scene from JSON", "", converter.fileExtension);
            if (!string.IsNullOrEmpty(path))
            {
                converter.LoadSceneFromJson(path);
            }
        }
    }
}
