#include "21kHz.hpp"
#include "dsp/digital.hpp"


struct D_Inf : Module {
	enum ParamIds {
        INVERT_OCTAVE_PARAM,
        INVERT_COARSE_PARAM,
        INVERT_SWITCH_PARAM,
        HALF_SHARP_PARAM,
        TRANSPOSE_OCTAVE_PARAM,
        TRANSPOSE_COARSE_PARAM,
        TRANSPOSE_SWITCH_PARAM,
        SWITCH_BUS_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
        INVERT_INPUT,
        INVERT_SWITCH_INPUT,
        TRANSPOSE_INPUT,
        TRANSPOSE_SWITCH_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
        TRANSPOSE_OUTPUT,
        INVERT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	D_Inf() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;
    
    SchmittTrigger invertTrigger;
    SchmittTrigger transposeTrigger;
    
    bool invert = true;
    bool transpose = true;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};


void transformState(Input &input, Param &param, bool &state, SchmittTrigger &trigger) {
    if (!input.active) {
        state = true;
    }
    else {
        if (param.value == 0) {
            if (trigger.process(input.value)) {
                state = !state;
            }
        }
        else {
            if (input.value <= 4.9f) {
                state = false;
            }
            else {
                state = true;
            }
        }
    }
}


void D_Inf::step() {
    transformState(inputs[INVERT_SWITCH_INPUT], params[INVERT_SWITCH_PARAM], invert, invertTrigger);
    if (SWITCH_BUS_PARAM == 1) {
        transpose = invert;
    }
    else {
        transformState(inputs[TRANSPOSE_SWITCH_INPUT], params[TRANSPOSE_SWITCH_PARAM], transpose, transposeTrigger);
    }
    
    float invertOutput = inputs[INVERT_INPUT].value;
    if (invert && inputs[INVERT_INPUT].active) {
        float axis = params[INVERT_OCTAVE_PARAM].value + 0.083333 * params[INVERT_COARSE_PARAM].value + 0.041667 * params[HALF_SHARP_PARAM].value;
        invertOutput = 2 * axis - inputs[INVERT_INPUT].value;
    }
    outputs[INVERT_OUTPUT].value = invertOutput;
    
    float transposeOutput;
    if (inputs[TRANSPOSE_INPUT].active) {
        transposeOutput = inputs[TRANSPOSE_INPUT].value;
    }
    else {
        transposeOutput = invertOutput;
    }
    if (transpose && outputs[TRANSPOSE_OUTPUT].active) {
        transposeOutput += params[TRANSPOSE_OCTAVE_PARAM].value + 0.083333 * params[TRANSPOSE_COARSE_PARAM].value;
    }
    outputs[TRANSPOSE_OUTPUT].value = transposeOutput;
}


struct D_InfWidget : ModuleWidget {
	D_InfWidget(D_Inf *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/Panels/D_Inf.svg")));

        addChild(Widget::create<kHzScrew>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(Widget::create<kHzScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(Widget::create<kHzScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(Widget::create<kHzScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        
        addParam(ParamWidget::create<kHzKnobSmallSnap>(Vec(16, 58), module, D_Inf::INVERT_OCTAVE_PARAM, -4, 4, 0));
        addParam(ParamWidget::create<kHzKnobSmallSnap>(Vec(72, 58), module, D_Inf::INVERT_COARSE_PARAM, 0, 12, 0));
        addParam(ParamWidget::create<kHzButton>(Vec(25, 112), module, D_Inf::INVERT_SWITCH_PARAM, 0, 1, 0));
        addParam(ParamWidget::create<kHzButton>(Vec(81, 112), module, D_Inf::HALF_SHARP_PARAM, 0, 1, 0));
        
        addParam(ParamWidget::create<kHzKnobSmallSnap>(Vec(16, 176), module, D_Inf::TRANSPOSE_OCTAVE_PARAM, -4, 4, 0));
        addParam(ParamWidget::create<kHzKnobSmallSnap>(Vec(72, 176), module, D_Inf::TRANSPOSE_COARSE_PARAM, 0, 12, 0));
        addParam(ParamWidget::create<kHzButton>(Vec(25, 230), module, D_Inf::TRANSPOSE_SWITCH_PARAM, 0, 1, 0));
        addParam(ParamWidget::create<kHzButton>(Vec(81, 230), module, D_Inf::SWITCH_BUS_PARAM, 0, 1, 0));
        
        addInput(Port::create<kHzPort>(Vec(10, 276), Port::INPUT, module, D_Inf::INVERT_SWITCH_INPUT));
        addInput(Port::create<kHzPort>(Vec(47, 276), Port::INPUT, module, D_Inf::INVERT_INPUT));
        addOutput(Port::create<kHzPort>(Vec(84, 276), Port::OUTPUT, module, D_Inf::INVERT_OUTPUT));
        
        addInput(Port::create<kHzPort>(Vec(10, 318), Port::INPUT, module, D_Inf::TRANSPOSE_SWITCH_INPUT));
        addInput(Port::create<kHzPort>(Vec(47, 318), Port::INPUT, module, D_Inf::TRANSPOSE_INPUT));
        addOutput(Port::create<kHzPort>(Vec(84, 318), Port::OUTPUT, module, D_Inf::TRANSPOSE_OUTPUT));
	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelD_Inf = Model::create<D_Inf, D_InfWidget>("21kHz", "kHzD_Inf", "D∞ — pitch tools — 8hp");
