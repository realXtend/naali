//var isServer = server.IsRunning();

print("[ScenePartManager] Loading script...");

var part2ents = []; //a map of part.name -> ents in that part

function print(s) {
    console.LogInfo(s);
}

function loadAll() {
    var parts = scene.EntitiesWithComponent("EC_DynamicComponent", "ScenePart");
    //print(parts);

    var partfile;
    for (var i=0; i<parts.length; i++) {
        //print(i + ":" + parts[i]);

        var placeholder = parts[i];
        partfile = placeholder.dynamiccomponent.GetAttribute("sceneref");
        loadPart(placeholder, partfile);
    }
}

function pathForAsset(assetref) {
    return asset.GetAsset(assetref).DiskSource();
}

function loadPart(placeholder, partfile) {
    print(placeholder + ":" + partfile);
    var ents = scene.LoadSceneXML(pathForAsset(partfile), false, false, 2); //, changetype);
    var ent;
    for (var i=0; i<ents.length; i++) {
        ent = ents[i];
        if (ent.placeable) {
            ent.placeable.SetParent(placeholder, false);
            ent.SetTemporary(true);
        }
    }
    part2ents[placeholder.name] = ents;
}

loadAll();
//print(pathForAsset("part_a.txml"));