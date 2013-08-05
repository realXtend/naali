// For conditions of distribution and use, see copyright notice in LICENSE

// !ref: compositor.ui

if (framework.IsHeadless())
{
    console.LogError("PostProcessEffect.js: Cannot be run in headless mode!");
    return;
}

engine.ImportExtension("qt.core");
engine.ImportExtension("qt.gui");

var widget_ = null;

var activeEffects = [];
var effects = [
    "Bloom", "UnderWater", "Glass","B&W", "Embossed", "Sharpen Edges",
    "Invert", "Posterize", "Laplace", "Tiling","HDR","Strong HDR",
    "Gaussian Blur", "Motion Blur", "Radial Blur", "WetLens"
];

// TRICK how to get information about sender..
/*
function ParameterHandler(effect,index) {
    this.effectName = effect;
}

ParameterHandler.prototype.textChanged = function() {
    print("UpdateEffect, in parameterHandler..");
    UpdateEffect(this.effectName);
}
*/

function ReturnDefaultParameters() {
    if (widget_ != null) {
        for (var i = 0; i < effects.length; ++i) {
            var str = effects[i].split(' ').join('');
            if (str == "B&W")
                str = "BW";
            name = "edit" + str;
            var line = findChild(widget_, name);
            if (line != null) {
                var text = "";
                if (effects[i] == "UnderWater") {
                    text = " speedX=0.1 \n speedY=0.1 \n normalStr=0.02";
                } else if (effects[i] == "WetLens") {
                    text = " strength=0.1 \n animSpeed=0.01";
                } else if (effects[i] == "Bloom") {
                    text = " RT=1 \n Blur1=1 ";
                } else if (effects[i] == "Motion Blur") {
                    text = " blur=0.7 \n ";
                } else if (effects[i] == "Laplace") {
                    text = " pixelSize=0.0031 \n scale=1.0";
                } else if (effects[i] == "Tiling") {
                    text = " NumTiles=75.0 \n Threshhold=0.15";
                } else if (effects[i] == "HDR") {
                    text = " inRTT=0 \n inLum=1 \n inBloom=1";
                } else if (effects[i] == "Radial Blur") {
                    text = " sampleDist=1.0 \n sampleStrength=2.2";
                }

                line.plainText = text;
            }
        }
    }
}

function ShowWidget() {
    var file = "local://compositor.ui";
    widget_ = ui.LoadFromFile(file, false);
    if (widget_ == null) {
        print("post_process_effects.js :  LoadFromFile ui-file:" + file + " failed.");
        return;
    }

    var proxy = ui.AddWidgetToScene(widget_);

    // No window borders.
    //proxy.windowFlags = 0;

    var gscene = ui.GraphicsScene();
    gscene.sceneRectChanged.connect(OnWindowSizeChanged);
    widget_.move(widget_.width + 10, widget_.height/2.0 );
    proxy.visible = true;

    for (var i = 0; i < effects.length; ++i) {
        var str = effects[i].split(' ').join('');
        if (str == "B&W")
            str = "BW";
        var name = "box" + str;
        var box = findChild(widget_, name);
        if (box != null) {
            box.stateChanged.connect(ToggleEffect);
        }
        
        /*
        name = "edit" + str;
        
        var line = findChild(widget_, name);
        if (line != null) {
        
            var handler = new ParameterHandler(effects[i]);
            line.textChanged.connect(handler, handler.textChanged);
        }
        */

        butApply = findChild(widget_, "butApply");
        if (butApply != null) {
            butApply.clicked.connect(UpdateEffectParams);
        }

        butCancel = findChild(widget_, "butCancel");
        if (butCancel != null) {
            butCancel.clicked.connect(ReturnDefaultParameters);
        }
        
    }

    ReturnDefaultParameters();
}

function OnWindowSizeChanged() {
    if (widget_ != null) {
        widget_.move(widget_.width + 10, widget_.height/2.0);
    }
}

function UpdateEffectParams() {
    for (var i = 0; i < activeEffects.length; ++i) {
        UpdateEffect(activeEffects[i]);
    }
}

function UpdateEffect(name) {
    var str = name.split(' ').join('');

    if (str == "B&W")
        str = "BW";

    var editName = "edit" + str;
    var line = findChild(widget_, editName);
    if (line != null) {
        var text = line.plainText;
        SetParamsToEffect(text, me.Component("OgreCompositor", name));
    }
}

function SetParamsToEffect(params, component) {
    
    var p = component.parameters;
    var list = [];
    list = params.split("\n");
    for (var i = 0; i < list.length; ++i) {
        // Trim string.
        p[i] = list[i].replace(/^\s\s*/, '').replace(/\s\s*$/, ''); 
    }
   
    component.parameters = p;
}

function ToggleEffect(state) {
    if (state == 2) { // Effect is checked, so show it.
        for (var i = 0; i < effects.length; ++i) {
            var str = effects[i].split(' ').join('');
            if (str == "B&W")
                str = "BW";

            var name = "box" + str;
            var box = findChild(widget_, name);
            if (box != null && box.checked) {
                var contains = false;
                for (var j = 0; j < activeEffects.length; ++j) {
                    if (activeEffects[j] == effects[i]) {
                        contains = true;
                        break;
                    }
                }
                if (!contains) {
                    ShowEffect(effects[i]);
                    break;
                }
            }
        }
    } else { // Effect is unchecked so hide it.
        for (var i = 0; i < activeEffects.length; ++i) { // Go through active effects
            var str = activeEffects[i].split(' ').join('');
            if (str == "B&W")
                str = "BW";
       
            var name = "box" + str;
            var box = findChild(widget_, name);

            if (box != null && !box.checked) {
                // Found what effect was unchecked remove it.
                HideEffect(activeEffects[i]);
                break;
            }
        }
    }
}

function HideEffect(name) {
    me.RemoveComponent("OgreCompositor", name);
    for (var i = 0; i < activeEffects.length; ++i)
    {
        if (activeEffects[i] == name) {
            activeEffects.splice(i, 1);
            break;
        }
    }
}

function ShowEffect(name) {
    activeEffects.push(name);
    // Create component for each effect.
    var component = me.CreateComponent("OgreCompositor", name);
    component.compositorName = name;
}

function OnScriptDestroyed() {
    widget_.deleteLater();
    delete widget_;
}

// Starts script..
ShowWidget();
