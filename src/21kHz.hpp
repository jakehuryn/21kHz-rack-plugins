#include "rack.hpp"

using namespace rack;

extern Plugin *pluginInstance;

extern Model *modelPalmLoop;
extern Model *modelD_Inf;
extern Model *modelTachyonEntangler;

////////////////////////////////

// Knobs

struct kHzKnob : RoundKnob {
    kHzKnob() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/kHzKnob.svg")));
        shadow->box.pos = Vec(0.0, 2.5);
    }
};

struct kHzKnobSmall : RoundKnob {
    kHzKnobSmall() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/kHzKnobSmall.svg")));
        shadow->box.pos = Vec(0.0, 2.5);
    }
};

struct kHzKnobTiny : RoundKnob {
    kHzKnobTiny() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/kHzKnobTiny.svg")));
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
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/kHzButton_0.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/kHzButton_1.svg")));
    }
};
        
// Ports

struct kHzPort : SVGPort {
    kHzPort() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/kHzPort.svg")));
        shadow->box.pos = Vec(0.0, 1.5);
    }
};

// Misc

struct kHzScrew : SVGScrew {
    kHzScrew() {
        sw->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/kHzScrew.svg")));
    }
};
