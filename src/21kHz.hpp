#include "rack0.hpp"


using namespace rack;

// Forward-declare the Plugin, defined in Template.cpp
extern Plugin *pluginInstance;

// Forward-declare each Model, defined in each module source file
extern Model *modelPalmLoop;
extern Model *modelD_Inf;
extern Model *modelTachyonEntangler;

////////////////////////////////

// Knobs

struct kHzKnob : RoundKnob {
    kHzKnob() {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/Components/kHzKnob.svg")));
        shadow->box.pos = Vec(0.0, 2.5);
    }
};

struct kHzKnobSmall : RoundKnob {
    kHzKnobSmall() {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/Components/kHzKnobSmall.svg")));
        shadow->box.pos = Vec(0.0, 2.5);
    }
};

struct kHzKnobTiny : RoundKnob {
    kHzKnobTiny() {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/Components/kHzKnobTiny.svg")));
        shadow->box.pos = Vec(0.0, 2.5);
    }
};

struct kHzKnobSnap : kHzKnob {
    kHzKnobSnap() {
        snap = true;
    }
};

struct kHzKnobSmallSnap : kHzKnobSmall {
    kHzKnobSmallSnap() {
        snap = true;
    }
};

// Buttons

struct kHzButton : SVGSwitch {
    kHzButton() {
        addFrame(SVG::load(assetPlugin(pluginInstance, "res/Components/kHzButton_0.svg")));
        addFrame(SVG::load(assetPlugin(pluginInstance, "res/Components/kHzButton_1.svg")));
    }
};
        
// Ports

struct kHzPort : SVGPort {
    kHzPort() {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/Components/kHzPort.svg")));
        shadow->box.pos = Vec(0.0, 1.5);
    }
};

// Misc

struct kHzScrew : SVGScrew {
    kHzScrew() {
        sw->setSVG(SVG::load(assetPlugin(pluginInstance, "res/Components/kHzScrew.svg")));
    }
};
