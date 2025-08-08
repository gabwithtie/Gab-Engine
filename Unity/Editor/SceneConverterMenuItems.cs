using UnityEngine;
using UnityEditor;
using System.IO;

public static class SceneConverterMenuItems
{
    private const string ConverterAssetPath = "Assets/SceneConverterData.asset";

    // Method to get or create the SceneConverter ScriptableObject
    private static SceneConverter GetOrCreateConverter()
    {
        SceneConverter converter = AssetDatabase.LoadAssetAtPath<SceneConverter>(ConverterAssetPath);
        if (converter == null)
        {
            converter = ScriptableObject.CreateInstance<SceneConverter>();
            AssetDatabase.CreateAsset(converter, ConverterAssetPath);
            AssetDatabase.SaveAssets();
            Debug.Log("Created new SceneConverter asset at: " + ConverterAssetPath);
        }
        return converter;
    }

    [MenuItem("Tools/Scene Converter/Save Scene")]
    private static void SaveSceneMenuItem()
    {
        SceneConverter converter = GetOrCreateConverter();
        string path = EditorUtility.SaveFilePanel("Save Scene as JSON", "", "scene", converter.fileExtension);
        if (!string.IsNullOrEmpty(path))
        {
            converter.SaveFullSceneToJson(path);
        }
    }

    [MenuItem("Tools/Scene Converter/Load Scene")]
    private static void LoadSceneMenuItem()
    {
        SceneConverter converter = GetOrCreateConverter();
        string path = EditorUtility.OpenFilePanel("Load Scene from JSON", "", converter.fileExtension);
        if (!string.IsNullOrEmpty(path))
        {
            converter.LoadSceneFromJson(path);
        }
    }
}
