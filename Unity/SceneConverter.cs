using UnityEngine;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using Newtonsoft.Json;

public class SceneConverter : ScriptableObject
{
    public GameObject[] renderObjectPrefabs;

    public string fileExtension = "json";

    [System.Serializable]
    public class SceneNodeData
    {
        public string type;
        public bool enabled;
        public float[] local_position;
        public float[] local_scale;
        public float[] local_euler_rotation;
        public Dictionary<string, string> serialized_variables;
        public List<SceneNodeData> children;

        public Vector3 GetPosition() => new Vector3(local_position[0], local_position[1], local_position[2]);
        public Vector3 GetScale() => new Vector3(local_scale[0], local_scale[1], local_scale[2]);
        public Vector3 GetRotation() => new Vector3(local_euler_rotation[0], local_euler_rotation[1], local_euler_rotation[2]);
    }

    [System.Serializable]
    public class RootNodeData
    {
        public string type;
        public bool enabled;
        public float[] local_position;
        public float[] local_scale;
        public float[] local_euler_rotation;
        public Dictionary<string, string> serialized_variables;
        public List<SceneNodeData> children;
    }

    public void LoadSceneFromJson(string filePath)
    {
        if (!File.Exists(filePath))
        {
            Debug.LogError($"JSON file not found at: {filePath}");
            return;
        }

        string json = File.ReadAllText(filePath);

        RootNodeData rootNode = JsonConvert.DeserializeObject<RootNodeData>(json);

        if (rootNode == null || rootNode.children == null)
        {
            Debug.LogError("Failed to deserialize JSON. Check file format.");
            return;
        }

        foreach (var childNode in rootNode.children)
        {
            CreateGameObjectFromNode(childNode, null);
        }
    }

    private void CreateGameObjectFromNode(SceneNodeData node, Transform parent)
    {
        if (node.type == "class gbe::Root")
        {
            foreach (var child in node.children)
            {
                CreateGameObjectFromNode(child, parent);
            }
            return;
        }

        GameObject go = new GameObject(node.type);
        go.transform.SetParent(parent);

        go.transform.localPosition = node.GetPosition();

        if (node.type == "class gbe::RenderObject" ||
    node.type == "class gbe::BoxCollider" ||
    node.type == "class gbe::SphereCollider")
        {
            go.transform.localScale = node.GetScale() * 2;
        }
        else
        {
            go.transform.localScale = node.GetScale();
        }

        go.transform.localRotation = Quaternion.Euler(node.GetRotation());

        Dictionary<string, string> variablesDict = node.serialized_variables;

        switch (node.type)
        {
            case "class gbe::RenderObject":
                AddRenderObject(go, variablesDict);
                break;
            case "class gbe::RigidObject":
                AddRigidObject(go, variablesDict);
                break;
            case "class gbe::BoxCollider":
                go.AddComponent<BoxCollider>();
                break;
            case "class gbe::SphereCollider":
                go.AddComponent<SphereCollider>();
                break;
            default:
                Debug.LogWarning($"Unknown object type: {node.type}");
                break;
        }

        if (node.children != null)
        {
            foreach (var child in node.children)
            {
                CreateGameObjectFromNode(child, go.transform);
            }
        }
    }

    private void AddRenderObject(GameObject go, Dictionary<string, string> variables)
    {
        if (variables != null && variables.TryGetValue("primitive", out string primitiveName) && renderObjectPrefabs != null)
        {
            GameObject prefab = System.Array.Find(renderObjectPrefabs, p => string.Equals(p.name, primitiveName, System.StringComparison.OrdinalIgnoreCase));

            if (prefab != null)
            {
                GameObject instantiatedPrefab = Instantiate(prefab, go.transform);
                instantiatedPrefab.transform.localPosition = Vector3.zero;
                instantiatedPrefab.transform.localRotation = Quaternion.identity;
                instantiatedPrefab.transform.localScale = Vector3.one;

                MeshFilter prefabMeshFilter = instantiatedPrefab.GetComponent<MeshFilter>();
                MeshRenderer prefabMeshRenderer = instantiatedPrefab.GetComponent<MeshRenderer>();

                if (prefabMeshFilter != null)
                {
                    go.AddComponent<MeshFilter>().mesh = prefabMeshFilter.sharedMesh;
                }
                if (prefabMeshRenderer != null)
                {
                    go.AddComponent<MeshRenderer>().sharedMaterial = prefabMeshRenderer.sharedMaterial;
                }

                DestroyImmediate(instantiatedPrefab);
                go.name = primitiveName;
            }
            else
            {
                Debug.LogWarning($"Prefab for primitive '{primitiveName}' not found in the assigned array.");
            }
        }
        else
        {
            Debug.LogWarning("No primitive name found or prefab array is not assigned.");
        }
    }

    private void AddRigidObject(GameObject go, Dictionary<string, string> variables)
    {
        Rigidbody rb = go.AddComponent<Rigidbody>();
        if (variables != null && variables.TryGetValue("static", out string isStaticString))
        {
            rb.isKinematic = (isStaticString == "1");
        }
    }

    public void SaveFullSceneToJson(string filePath)
    {
        RootNodeData rootNode = new RootNodeData
        {
            type = "class gbe::Root",
            enabled = true,
            local_position = new float[] { 0, 0, 0 },
            local_scale = new float[] { 1, 1, 1 },
            local_euler_rotation = new float[] { 0, 0, 0 },
            serialized_variables = new Dictionary<string, string>(),
            children = new List<SceneNodeData>()
        };

        var rootGameObjects = UnityEngine.SceneManagement.SceneManager.GetActiveScene().GetRootGameObjects();
        foreach (var go in rootGameObjects)
        {
            rootNode.children.Add(CreateNodeFromGameObject(go));
        }

        string json = JsonConvert.SerializeObject(rootNode, Formatting.Indented);
        File.WriteAllText(filePath, json);

        Debug.Log($"Entire scene saved to: {filePath}");
    }

    private SceneNodeData CreateNodeFromGameObject(GameObject go)
    {
        SceneNodeData node = new SceneNodeData
        {
            type = "Unknown",
            enabled = go.activeSelf,
            local_position = new float[] { go.transform.localPosition.x, go.transform.localPosition.y, go.transform.localPosition.z },
            local_euler_rotation = new float[] { go.transform.localEulerAngles.x, go.transform.localEulerAngles.y, go.transform.localEulerAngles.z },
            serialized_variables = new Dictionary<string, string>(),
            children = new List<SceneNodeData>()
        };

        if (go.GetComponent<MeshFilter>() != null ||
    go.GetComponent<BoxCollider>() != null ||
    go.GetComponent<SphereCollider>() != null)
        {
            node.local_scale = new float[] { go.transform.localScale.x / 2, go.transform.localScale.y / 2, go.transform.localScale.z / 2 };
        }
        else
        {
            node.local_scale = new float[] { go.transform.localScale.x, go.transform.localScale.y, go.transform.localScale.z };
        }

        if (go.GetComponent<Rigidbody>() != null)
        {
            node.type = "class gbe::RigidObject";
            Rigidbody rb = go.GetComponent<Rigidbody>();
            node.serialized_variables.Add("static", rb.isKinematic ? "1" : "0");
        }
        else if (go.GetComponent<MeshFilter>() != null && go.GetComponent<MeshRenderer>() != null)
        {
            node.type = "class gbe::RenderObject";
            node.serialized_variables.Add("primitive", go.name);
            node.serialized_variables.Add("tex", "");
            node.serialized_variables.Add("mesh", "");
            node.serialized_variables.Add("mat", "");
        }
        else if (go.GetComponent<BoxCollider>() != null)
        {
            node.type = "class gbe::BoxCollider";
            node.serialized_variables.Add("size", go.GetComponent<BoxCollider>().size.ToString());
        }
        else if (go.GetComponent<SphereCollider>() != null)
        {
            node.type = "class gbe::SphereCollider";
            node.serialized_variables.Add("radius", go.GetComponent<SphereCollider>().radius.ToString());
        }
        else if (go.GetComponent<CapsuleCollider>() != null)
        {
            node.type = "class gbe::CapsuleCollider";
            node.serialized_variables.Add("radius", go.GetComponent<SphereCollider>().radius.ToString());
        }

        if (go.transform.childCount > 0)
        {
            foreach (Transform childTransform in go.transform)
            {
                node.children.Add(CreateNodeFromGameObject(childTransform.gameObject));
            }
        }

        return node;
    }
}
