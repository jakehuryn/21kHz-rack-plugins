#include "21kHz.hpp"
#include "dsp/math.hpp"
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
    int discont = 0;
    int oldDiscont = 0;
    
    array<float, 4> sawBuffer;
    array<float, 4> sqrBuffer;
    array<float, 4> triBuffer;
    
    float log2sampleFreq = 15.4284f;
    
    dsp::SchmittTrigger resetTrigger;

	PalmLoop() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    configParam(OCT_PARAM, 4, 12, 8);
    configParam(COARSE_PARAM, -7, 7, 0);
    configParam(FINE_PARAM, -0.083333, 0.083333, 0.0);
    configParam(EXP_FM_PARAM, -1.0, 1.0, 0.0);
    configParam(LIN_FM_PARAM, -11.7, 11.7, 0.0);
  }
	void process(const ProcessArgs &args) override;
  void onSampleRateChange() override;

};


void PalmLoop::onSampleRateChange() {
    log2sampleFreq = log2f(1.0f / APP->engine->getSampleTime()) - 0.00009f;
}


// quick explanation: the whole thing is driven by a naive sawtooth, which writes to a four-sample buffer for each
// (non-sine) waveform. the waves are calculated such that their discontinuities (or in the case of triangle, derivative
// discontinuities) only occur each time the phasor exceeds a [0, 1) range. when we calculate the outputs, we look to see
// if a discontinuity occured in the previous sample. if one did, we calculate the polyblep or polyblamp and add it to
// each sample in the buffer. the output is the oldest buffer sample, which gets overwritten in the following step.

void PalmLoop::process(const ProcessArgs &args) {
    if (resetTrigger.process(inputs[RESET_INPUT].value)) {
        phase = 0.0f;
    }
    
    for (int i = 0; i <= 2; ++i) {
        sawBuffer[i] = sawBuffer[i + 1];
        sqrBuffer[i] = sqrBuffer[i + 1];
        triBuffer[i] = triBuffer[i + 1];
    }
    
    float freq = params[OCT_PARAM].value + 0.031360 + 0.083333 * params[COARSE_PARAM].value + params[FINE_PARAM].value + inputs[V_OCT_INPUT].value;
    freq += params[EXP_FM_PARAM].value * inputs[EXP_FM_INPUT].value;
    if (freq >= log2sampleFreq) {
        freq = log2sampleFreq;
    }
    freq = powf(2.0f, freq);
    float incr = 0.0f;
    if (inputs[LIN_FM_INPUT].active) {
        freq += params[LIN_FM_PARAM].value * params[LIN_FM_PARAM].value * params[LIN_FM_PARAM].value * inputs[LIN_FM_INPUT].value;
        incr = args.sampleTime * freq;
        if (incr > 1.0f) {
            incr = 1.0f;
        }
        else if (incr < -1.0f) {
            incr = -1.0f;
        }
    }
    else {
        incr = args.sampleTime * freq;
    }
    
    phase += incr;
    if (phase >= 0.0f && phase < 1.0f) {
        discont = 0;
    }
    else if (phase >= 1.0f) {
        discont = 1;
        --phase;
        square *= -1.0f;
    }
    else {
        discont = -1;
        ++phase;
        square *= -1.0f;
    }
    
    sawBuffer[3] = phase;
    sqrBuffer[3] = square;
    if (square >= 0.0f) {
        triBuffer[3] = phase;
    }
    else {
        triBuffer[3] = 1.0f - phase;
    }
    
    if (outputs[SAW_OUTPUT].active) {
        if (oldDiscont == 1) {
            polyblep4(sawBuffer, 1.0f - oldPhase / incr, 1.0f);
        }
        else if (oldDiscont == -1) {
            polyblep4(sawBuffer, 1.0f - (oldPhase - 1.0f) / incr, -1.0f);
        }
        outputs[SAW_OUTPUT].value = clamp(10.0f * (sawBuffer[0] - 0.5f), -5.0f, 5.0f);
    }
    if (outputs[SQR_OUTPUT].active) {
        if (discont == 0) {
            if (oldDiscont == 1) {
                polyblep4(sqrBuffer, 1.0f - oldPhase / incr, -2.0f * square);
            }
            else if (oldDiscont == -1) {
                polyblep4(sqrBuffer, 1.0f - (oldPhase - 1.0f) / incr, -2.0f * square);
            }
        }
        else {
            if (oldDiscont == 1) {
                polyblep4(sqrBuffer, 1.0f - oldPhase / incr, 2.0f * square);
            }
            else if (oldDiscont == -1) {
                polyblep4(sqrBuffer, 1.0f - (oldPhase - 1.0f) / incr, 2.0f * square);
            }
        }
        outputs[SQR_OUTPUT].value = clamp(4.9999f * sqrBuffer[0], -5.0f, 5.0f);
    }
    if (outputs[TRI_OUTPUT].active) {
        if (discont == 0) {
            if (oldDiscont == 1) {
                polyblamp4(triBuffer, 1.0f - oldPhase / incr, 2.0f * square * incr);
            }
            else if (oldDiscont == -1) {
                polyblamp4(triBuffer, 1.0f - (oldPhase - 1.0f) / incr, 2.0f * square * incr);
            }
        }
        else {
            if (oldDiscont == 1) {
                polyblamp4(triBuffer, 1.0f - oldPhase / incr, -2.0f * square * incr);
            }
            else if (oldDiscont == -1) {
                polyblamp4(triBuffer, 1.0f - (oldPhase - 1.0f) / incr, -2.0f * square * incr);
            }
        }
        outputs[TRI_OUTPUT].value = clamp(10.0f * (triBuffer[0] - 0.5f), -5.0f, 5.0f);
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
    oldDiscont = discont;
}


struct PalmLoopWidget : ModuleWidget {
	PalmLoopWidget(PalmLoop *module) {
    setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Panels/PalmLoop.svg")));

		addChild(createWidget<kHzScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<kHzScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<kHzScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<kHzScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    addParam(createParam<kHzKnobSnap>(Vec(36, 40), module, PalmLoop::OCT_PARAM));
        
    addParam(createParam<kHzKnobSmallSnap>(Vec(16, 112), module, PalmLoop::COARSE_PARAM));
    addParam(createParam<kHzKnobSmall>(Vec(72, 112), module, PalmLoop::FINE_PARAM));
       
    addParam(createParam<kHzKnobSmall>(Vec(16, 168), module, PalmLoop::EXP_FM_PARAM));
    addParam(createParam<kHzKnobSmall>(Vec(72, 168), module, PalmLoop::LIN_FM_PARAM));
        
    addInput(createInput<kHzPort>(Vec(10, 234), module, PalmLoop::EXP_FM_INPUT));
    addInput(createInput<kHzPort>(Vec(47, 234), module, PalmLoop::V_OCT_INPUT));
    addInput(createInput<kHzPort>(Vec(84, 234), module, PalmLoop::LIN_FM_INPUT));
      
    addInput(createInput<kHzPort>(Vec(10, 276), module, PalmLoop::RESET_INPUT));
    addOutput(createOutput<kHzPort>(Vec(47, 276), module, PalmLoop::SAW_OUTPUT));
    addOutput(createOutput<kHzPort>(Vec(84, 276), module, PalmLoop::SIN_OUTPUT));
        
    addOutput(createOutput<kHzPort>(Vec(10, 318), module, PalmLoop::SQR_OUTPUT));
    addOutput(createOutput<kHzPort>(Vec(47, 318), module, PalmLoop::TRI_OUTPUT));
    addOutput(createOutput<kHzPort>(Vec(84, 318), module, PalmLoop::SUB_OUTPUT));
        
	}
};

Model *modelPalmLoop = createModel<PalmLoop, PalmLoopWidget>("PalmLoop");

