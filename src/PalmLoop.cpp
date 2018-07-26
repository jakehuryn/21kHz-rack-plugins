#include "21kHz.hpp"
#include "dsp/digital.hpp"
#include <array>


using std::array;


struct PalmLoop : Module {
	enum ParamIds {
        OCT_PARAM,
        COARSE_PARAM,
        FINE_PARAM,
        EXP_FM_PARAM,
        LIN_FM_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
        RESET_INPUT,
        V_OCT_INPUT,
        EXP_FM_INPUT,
        LIN_FM_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
        SAW_OUTPUT,
        SQR_OUTPUT,
        TRI_OUTPUT,
        SIN_OUTPUT,
        SUB_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};
    
    float phase = 0.0f;
    float oldPhase = 0.0f;
    float square = 1.0f;
    
    array<float, 4> buffer;
    array<float, 4> sqrBuffer;
    array<float, 4> triBuffer;
    array<int, 4> discont;
    
    SchmittTrigger resetTrigger;

	PalmLoop() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void polyblep(array<float, 4> &buffer, float d, float u) {
    if (d > 1.0f) {
        d = 1.0f;
    }
    else if (d < 0.0f) {
        d = 0.0f;
    }
    
    float d2 = d * d;
    float d3 = d2 * d;
    float d4 = d3 * d;
    float dd3 = 0.16667 * (d + d3);
    float cd2 = 0.041667 + 0.25 * d2;
    float d4_1 = 0.041667 * d4;
    
    buffer[3] += u * (d4_1);
    buffer[2] += u * (cd2 + dd3 - 0.125 * d4);
    buffer[1] += u * (-0.5 + 0.66667 * d - 0.33333 * d3 + 0.125 * d4);
    buffer[0] += u * (-cd2 + dd3 - d4_1);
}

void polyblamp(array<float, 4> &buffer, float d, float u) {
    if (d > 1.0f) {
        d = 1.0f;
    }
    else if (d < 0.0f) {
        d = 0.0f;
    }
    
    float d2 = d * d;
    float d3 = d2 * d;
    float d4 = d3 * d;
    float d5 = d4 * d;
    float d5_1 = 0.0083333 * d5;
    float d5_2 = 0.025 * d5;
    
    buffer[3] += u * (d5_1);
    buffer[2] += u * (0.0083333 + 0.083333 * (d2 + d3) + 0.041667 * (d + d4) - d5_2);
    buffer[1] += u * (0.23333 - 0.5 * d + 0.33333 * d2 - 0.083333 * d4 + d5_2);
    buffer[0] += u * (0.0083333 + 0.041667 * (d4 - d) + 0.083333 * (d2 - d3) - d5_1);
}

float sin_01(float t) {
    if (t > 1.0f) {
        t = 1.0f;
    }
    else if (t > 0.5) {
        t = 1.0f - t;
    }
    else if (t < 0.0f) {
        t = 0.0f;
    }
    t = 2.0f * t - 0.5f;
    float t2 = t * t;
    t = (((-0.540347 * t2 + 2.53566) * t2 - 5.16651) * t2 + 3.14159) * t;
    return t;
}

void PalmLoop::step() {
    if (resetTrigger.process(inputs[RESET_INPUT].value)) {
        phase = 0.0f;
    }
    
    if (outputs[SAW_OUTPUT].active || outputs[SQR_OUTPUT].active || outputs[TRI_OUTPUT].active) {
        for (int i = 0; i <= 2; ++i) {
            buffer[i] = buffer[i + 1];
            sqrBuffer[i] = sqrBuffer[i + 1];
            triBuffer[i] = triBuffer[i + 1];
            discont[i] = discont[i + 1];
        }
    }
    
    float freq = int(params[OCT_PARAM].value) + 0.031360 + 0.083333 * int(params[COARSE_PARAM].value) + params[FINE_PARAM].value + inputs[V_OCT_INPUT].value;
    
    if (inputs[EXP_FM_INPUT].active) {
        freq += params[EXP_FM_PARAM].value * inputs[EXP_FM_INPUT].value;
        if (freq >= 15.42f) {
            freq = 15.42f;
        }
        freq = powf(2.0f, freq);
    }
    else {
        if (freq >= 15.42f) {
            freq = 15.42f;
        }
        freq = powf(2.0f, freq);
    }
    
    float incr = 0.0f;
    if (inputs[LIN_FM_INPUT].active) {
        freq += params[LIN_FM_PARAM].value * inputs[LIN_FM_INPUT].value;
        incr = engineGetSampleTime() * freq;
        if (incr > 1.0f) {
            incr = 1.0f;
        }
        else if (incr < -1.0f) {
            incr = -1.0f;
        }
    }
    else {
        incr = engineGetSampleTime() * freq;
    }
    
    phase += incr;
    if (phase >= 0.0f && phase < 1.0f) {
        discont[3] = 0;
    }
    else if (phase >= 1.0f) {
        discont[3] = 1;
        --phase;
        square *= -1.0f;
    }
    else {
        discont[3] = 2;
        ++phase;
        square *= -1.0f;
    }
    buffer[3] = phase;
    sqrBuffer[3] = square;
    if (square >= 0.0f) {
        triBuffer[3] = phase;
    }
    else {
        triBuffer[3] = 1.0f - phase;
    }
    
    if (outputs[SAW_OUTPUT].active) {
        if (discont[2] == 1) {
            polyblep(buffer, 1.0f - oldPhase / incr, 1.0f);
        }
        else if (discont[2] == 2) {
            polyblep(buffer, 1.0f - (oldPhase - 1.0f) / incr, -1.0f);
        }
        outputs[SAW_OUTPUT].value = clampf(10.0f * (buffer[0] - 0.5f), -5.0f, 5.0f);
    }
    
    if (outputs[SQR_OUTPUT].active) {
        if (discont[2] == 1) {
            polyblep(sqrBuffer, 1.0f - oldPhase / incr, -2.0f * square);
        }
        else if (discont[2] == 2) {
            polyblep(sqrBuffer, 1.0f - (oldPhase - 1.0f) / incr, -2.0f * square);
        }
        outputs[SQR_OUTPUT].value = clampf(4.9999f * sqrBuffer[0], -5.0f, 5.0f);
    }
    
    if (outputs[TRI_OUTPUT].active) {
        if (discont[2] == 1) {
            polyblamp(triBuffer, 1.0f - oldPhase / incr, 2.0f * square * incr);
        }
        else if (discont[2] == 2) {
            polyblamp(triBuffer, 1.0f - (oldPhase - 1.0f) / incr, 2.0f * square * incr);
        }
        outputs[TRI_OUTPUT].value = clampf(10.0f * (triBuffer[0] - 0.5f), -5.0f, 5.0f);
    }
    
    if (outputs[SIN_OUTPUT].active) {
        outputs[SIN_OUTPUT].value = 5.0f * sin_01(phase);
    }
    
    if (outputs[SUB_OUTPUT].active) {
        if (square >= 0.0f) {
            outputs[SUB_OUTPUT].value = 5.0f * sin_01(0.5f * phase);
        }
        else {
            outputs[SUB_OUTPUT].value = 5.0f * sin_01(0.5f * (1.0f - phase));
        }
    }
    
    oldPhase = phase;
}


struct PalmLoopWidget : ModuleWidget {
	PalmLoopWidget(PalmLoop *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/Panels/PalmLoop.svg")));

		addChild(Widget::create<kHzScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<kHzScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<kHzScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(Widget::create<kHzScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(ParamWidget::create<kHzKnobSnap>(Vec(36, 40), module, PalmLoop::OCT_PARAM, 4.0, 12.0, 8.0));
        
        addParam(ParamWidget::create<kHzKnobSmallSnap>(Vec(16, 112), module, PalmLoop::COARSE_PARAM, 0.0, 12.0, 0.0));
        addParam(ParamWidget::create<kHzKnobSmall>(Vec(72, 112), module, PalmLoop::FINE_PARAM, -0.083333, 0.083333, 0.0));
        
        addParam(ParamWidget::create<kHzKnobSmall>(Vec(16, 168), module, PalmLoop::EXP_FM_PARAM, -1.0, 1.0, 0.0));
        addParam(ParamWidget::create<kHzKnobSmall>(Vec(72, 168), module, PalmLoop::LIN_FM_PARAM, -400.0, 400.0, 0.0));
        
        addInput(Port::create<kHzPort>(Vec(10, 234), Port::INPUT, module, PalmLoop::EXP_FM_INPUT));
        addInput(Port::create<kHzPort>(Vec(47, 234), Port::INPUT, module, PalmLoop::V_OCT_INPUT));
        addInput(Port::create<kHzPort>(Vec(84, 234), Port::INPUT, module, PalmLoop::LIN_FM_INPUT));
        
        addInput(Port::create<kHzPort>(Vec(10, 276), Port::INPUT, module, PalmLoop::RESET_INPUT));
        addOutput(Port::create<kHzPort>(Vec(47, 276), Port::OUTPUT, module, PalmLoop::SQR_OUTPUT));
        addOutput(Port::create<kHzPort>(Vec(84, 276), Port::OUTPUT, module, PalmLoop::TRI_OUTPUT));
        
        addOutput(Port::create<kHzPort>(Vec(10, 318), Port::OUTPUT, module, PalmLoop::SAW_OUTPUT));
        addOutput(Port::create<kHzPort>(Vec(47, 318), Port::OUTPUT, module, PalmLoop::SIN_OUTPUT));
        addOutput(Port::create<kHzPort>(Vec(84, 318), Port::OUTPUT, module, PalmLoop::SUB_OUTPUT));
        
	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelPalmLoop = Model::create<PalmLoop, PalmLoopWidget>("21kHz", "kHzPalmLoop", "Palm Loop — basic VCO — 8hp", OSCILLATOR_TAG);
