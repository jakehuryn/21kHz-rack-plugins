#include "rack.hpp"


using namespace rack;

// Forward-declare the Plugin, defined in Template.cpp
extern Plugin *plugin;

// Forward-declare each Model, defined in each module source file
extern Model *modelPalmLoop;

////////////////////////////////

// Knobs

struct kHzKnob : RoundKnob {
    kHzKnob() {
        setSVG(SVG::load(assetPlugin(plugin, "res/Components/kHzKnob.svg")));
        shadow->box.pos = Vec(0.0, 3.5);
    }
};

struct kHzKnobSmall : RoundKnob {
    kHzKnobSmall() {
        setSVG(SVG::load(assetPlugin(plugin, "res/Components/kHzKnobSmall.svg")));
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
        
// Ports

struct kHzPort : SVGPort {
    kHzPort() {
        setSVG(SVG::load(assetPlugin(plugin, "res/Components/kHzPort.svg")));
        shadow->box.pos = Vec(0.0, 2.0);
    }
};

// Misc

struct kHzScrew : SVGScrew {
    kHzScrew() {
        sw->setSVG(SVG::load(assetPlugin(plugin, "res/Components/kHzScrew.svg")));
    }
};
