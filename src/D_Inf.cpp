#include "21kHz.hpp"
//#include "dsp/digital.hpp"


struct D_Inf : Module {
	enum ParamIds {
        OCTAVE_PARAM,
        COARSE_PARAM,
        HALF_SHARP_PARAM,
        INVERT_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
        INVERT_INPUT,
        TRANSPOSE_INPUT,
        A_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
        A_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};
    
    bool invert = true;
    bool transpose = true;
    
    SchmittTrigger invertTrigger;
    SchmittTrigger transposeTrigger;

	D_Inf() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
  }
	void step() override;

};


void newState(bool &state, bool inactive, bool triggered) {
    if (inactive) {
        state = true;
    }
    else {
        if (triggered) {
            state = !state;
        }
    }
}


void D_Inf::step() {
    if (params[INVERT_PARAM].value == 0) {
        invert = false;
    }
    else {
        newState(invert, !inputs[INVERT_INPUT].active, invertTrigger.process(inputs[INVERT_INPUT].value));
    }
    newState(transpose, !inputs[TRANSPOSE_INPUT].active, transposeTrigger.process(inputs[TRANSPOSE_INPUT].value));
    
    float output = inputs[A_INPUT].value;
    if (invert) {
        output *= -1.0f;
    }
    if (transpose) {
        output += params[OCTAVE_PARAM].value + 0.083333 * params[COARSE_PARAM].value + 0.041667 * params[HALF_SHARP_PARAM].value;
    }
    outputs[A_OUTPUT].value = output;
}


struct D_InfWidget : ModuleWidget {
	D_InfWidget(D_Inf *module) {
    setModule(module);
		setPanel(SVG::load(assetPlugin(pluginInstance, "res/Panels/D_Inf.svg")));

        addChild(createWidget<kHzScrew>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<kHzScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        
        addParam(createParam<kHzKnobSmallSnap>(Vec(14, 40), module, D_Inf::OCTAVE_PARAM, -4, 4, 0));
        addParam(createParam<kHzKnobSmallSnap>(Vec(14, 96), module, D_Inf::COARSE_PARAM, -7, 7, 0));
        
        addParam(createParam<kHzButton>(Vec(10, 150), module, D_Inf::HALF_SHARP_PARAM, 0, 1, 0));
        addParam(createParam<kHzButton>(Vec(36, 150), module, D_Inf::INVERT_PARAM, 0, 1, 0));
        
        addInput(createPort<kHzPort>(Vec(17, 192), PortWidget::INPUT, module, D_Inf::INVERT_INPUT));
        addInput(createPort<kHzPort>(Vec(17, 234), PortWidget::INPUT, module, D_Inf::TRANSPOSE_INPUT));
        addInput(createPort<kHzPort>(Vec(17, 276), PortWidget::INPUT, module, D_Inf::A_INPUT));
        addOutput(createPort<kHzPort>(Vec(17, 318), PortWidget::OUTPUT, module, D_Inf::A_OUTPUT));
	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelD_Inf = createModel<D_Inf, D_InfWidget>("D_Inf");


// history
// 0.6.1
//	create
