#include "21kHz.hpp"
#include "dsp/digital.hpp"
#include "dsp/math.hpp"
#include <math.h>
#include <array>


using std::array;


struct TachyonEntangler : Module {
	enum ParamIds {
        A_OCTAVE_PARAM,
        A_COARSE_PARAM,
        A_FINE_PARAM,
        B_RATIO_PARAM,
        A_EXP_FM_PARAM,
        A_LIN_FM_PARAM,
        B_EXP_FM_PARAM,
        B_LIN_FM_PARAM,
        A_CHAOS_PARAM,
        A_SYNC_PROB_PARAM,
        B_CHAOS_PARAM,
        B_SYNC_PROB_PARAM,
        A_CHAOS_MOD_PARAM,
        A_SYNC_PROB_MOD_PARAM,
        B_CHAOS_MOD_PARAM,
        B_SYNC_PROB_MOD_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
        A_EXP_FM_INPUT,
        A_LIN_FM_INPUT,
        B_EXP_FM_INPUT,
        B_LIN_FM_INPUT,
        A_CHAOS_INPUT,
        A_SYNC_PROB_INPUT,
        B_CHAOS_INPUT,
        B_SYNC_PROB_INPUT,
        A_RESET_INPUT,
        B_RESET_INPUT,
        A_V_OCT_INPUT,
        B_V_OCT_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
        A_SAW_OUTPUT,
        A_SQR_OUTPUT,
        B_SAW_OUTPUT,
        B_SQR_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};
    
    float phaseA = 0.0f;
    float phaseB = 0.0f;
    float squareA = 1.0f;
    float squareB = 1.0f;
    float oldDecrA = 0.0f;
    float oldDecrB = 0.0f;
    int discontA = 0;
    int discontB = 0;
    int syncDiscontA = 0;
    int syncDiscontB = 0;
    int oldDiscontA = 0;
    int oldDiscontB = 0;
    int oldSyncDiscontA = 0;
    int oldSyncDiscontB = 0;
    
    array<float, 4> sawBufferA;
    array<float, 4> sawBufferB;
    array<float, 4> sqrBufferA;
    array<float, 4> sqrBufferB;
    array<float, 3> oldPhasesA;
    array<float, 3> oldPhasesB;
    array<float, 3> oldIncrsA;
    array<float, 3> oldIncrsB;
    
    float log2sampleFreq = 15.4284f;
    
    SchmittTrigger resetTriggerA;
    SchmittTrigger resetTriggerB;

	TachyonEntangler() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;
    void onSampleRateChange() override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};


float advancePhase(float &phase, float &square, float incr, float rand, int &discont) {
    phase += incr;
    float decr = 1.0f;
    if (phase >= 0.0f && phase < 1.0f) {
        discont = 0;
    }
    else if (phase >= 1.0f && incr >= 0.0f) {
        discont = 1;
        decr = 1.0f - 2.0f * rand * (randomUniform() - 0.5f);
        phase -= decr;
        square *= -1.0f;
        if (phase >= 1.0f) {
            phase = 1.0f;
        }
    }
    else if (phase < 0.0f && incr < 0.0f) {
        discont = -1;
        decr = 1.0f - 2.0f * rand * (randomUniform() - 0.5f);
        phase += decr;
        square *= -1.0f;
        if (phase <= -1.0f) {
            phase = -1.0f;
        }
    }
    return decr;
}


void TachyonEntangler::onSampleRateChange() {
    log2sampleFreq = log2f(1.0f / engineGetSampleTime()) - 0.00009f;
}


void TachyonEntangler::step() {
    if (resetTriggerA.process(inputs[A_RESET_INPUT].value)) {
        phaseA = 0.0f;
        squareA = 1.0f;
    }
    if (resetTriggerB.process(inputs[B_RESET_INPUT].value)) {
        phaseB = 0.0f;
        squareB = 1.0f;
    }
    
    for (int i = 0; i <= 2; ++i) {
        sawBufferA[i] = sawBufferA[i + 1];
        sawBufferB[i] = sawBufferB[i + 1];
        sqrBufferA[i] = sqrBufferA[i + 1];
        sqrBufferB[i] = sqrBufferB[i + 1];
    }
    for (int i = 0; i <= 1; ++i) {
        oldPhasesA[i] = oldPhasesA[i + 1];
        oldPhasesB[i] = oldPhasesB[i + 1];
        oldIncrsA[i] = oldIncrsA[i + 1];
        oldIncrsB[i] = oldIncrsB[i + 1];
    }
    
    float sampleTime = engineGetSampleTime();
    float centerPitch = params[A_OCTAVE_PARAM].value + 0.031360 + 0.083333 * params[A_COARSE_PARAM].value + params[A_FINE_PARAM].value;
    float pitchA = centerPitch + inputs[A_V_OCT_INPUT].value;
    if (inputs[A_EXP_FM_INPUT].active) {
        pitchA += 0.2 * inputs[A_EXP_FM_INPUT].value * params[A_EXP_FM_PARAM].value * params[A_EXP_FM_PARAM].value * params[A_EXP_FM_PARAM].value;
    }
    if (pitchA >= log2sampleFreq) {
        pitchA = log2sampleFreq;
    }
    float incrA = 0.0f;
    if (inputs[A_LIN_FM_INPUT].active) {
        incrA = sampleTime * (powf(2.0f, pitchA) + params[A_LIN_FM_PARAM].value * params[A_LIN_FM_PARAM].value * params[A_LIN_FM_PARAM].value * inputs[A_LIN_FM_INPUT].value);
        if (incrA > 1.0f) {
            incrA = 1.0f;
        }
        else if (incrA < -1.0f) {
            incrA = -1.0f;
        }
    }
    else {
        incrA = sampleTime * powf(2.0f, pitchA);
    }
    float pitchB = params[B_RATIO_PARAM].value;
    if (inputs[B_V_OCT_INPUT].active) {
        pitchB += centerPitch + inputs[B_V_OCT_INPUT].value;
    }
    else {
        pitchB += pitchA;
    }
    if (inputs[B_EXP_FM_INPUT].active) {
        pitchB += 0.2 * inputs[B_EXP_FM_INPUT].value * params[B_EXP_FM_PARAM].value * params[B_EXP_FM_PARAM].value * params[B_EXP_FM_PARAM].value;
    }
    if (pitchB >= log2sampleFreq) {
        pitchB = log2sampleFreq;
    }
    float incrB = 0.0f;
    if (inputs[B_LIN_FM_INPUT].active) {
        incrB = sampleTime * (powf(2.0f, pitchB) + params[B_LIN_FM_PARAM].value * params[B_LIN_FM_PARAM].value * params[B_LIN_FM_PARAM].value * inputs[B_LIN_FM_INPUT].value);
        if (incrB > 1.0f) {
            incrB = 1.0f;
        }
        else if (incrB < -1.0f) {
            incrB = -1.0f;
        }
    }
    else {
        incrB = sampleTime * powf(2.0f, pitchB);
    }
    
    float decrA = advancePhase(phaseA, squareA, incrA, params[A_CHAOS_PARAM].value + params[A_CHAOS_MOD_PARAM].value * inputs[A_CHAOS_INPUT].value, discontA);
    float decrB = 1.0f;
    if ((discontA == 1 || discontA == -1) && randomUniform() >= 1.0f - (params[B_SYNC_PROB_PARAM].value + params[B_SYNC_PROB_MOD_PARAM].value * inputs[B_SYNC_PROB_INPUT].value)) {
        syncDiscontA = discontA;
    }
    else {
        syncDiscontA = 0;
    }
    if (syncDiscontA == 0) {
        decrB = advancePhase(phaseB, squareB, incrB, params[B_CHAOS_PARAM].value + params[A_CHAOS_MOD_PARAM].value * inputs[A_CHAOS_INPUT].value, discontB);
    }
    else {
        decrB = advancePhase(phaseB, squareB, incrB, params[B_CHAOS_PARAM].value + params[A_CHAOS_MOD_PARAM].value * inputs[A_CHAOS_INPUT].value, discontB);
        if (outputs[B_SAW_OUTPUT].active || outputs[B_SQR_OUTPUT].active) {
            if (discontB == 0) {
            }
            if (discontB == 1) {
                if (syncDiscontA == 1) {
                    if (incrA * phaseB <= incrB * phaseA) {
                        discontB = 0;
                        squareB *= -1.0f;
                    }
                }
                else {
                    if (incrA * (phaseB - 1.0f) <= incrB * phaseA) {
                        discontB = 0;
                        squareB *= -1.0f;
                    }
                }
            }
            else {
                if (syncDiscontA == 1) {
                    if (incrA * phaseB <= incrB * (phaseA - 1.0f)) {
                        discontB = 0;
                        squareB *= -1.0f;
                    }
                }
                else {
                    if (incrA * (phaseB - 1.0f) <= incrB * (phaseA - 1.0f)) {
                        discontB = 0;
                        squareB *= -1.0f;
                    }
                }
            }
        }
        if (incrA >= 0.0f) {
            phaseB = phaseA / incrA * incrB;
        }
        else {
            phaseB = (phaseA - 1) / incrA * incrB;
        }
        if (incrB <= 0.0f) {
            ++phaseB;
        }
    }
    if ((discontB == 1 || discontB == -1) && randomUniform() >= 1.0f - (params[A_SYNC_PROB_PARAM].value + params[A_SYNC_PROB_MOD_PARAM].value * inputs[A_SYNC_PROB_INPUT].value)) {
        syncDiscontB = discontB;
    }
    else {
        syncDiscontB = 0;
    }
    if (syncDiscontB == 1 || syncDiscontB == -1) {
        if (outputs[A_SAW_OUTPUT].active || outputs[A_SQR_OUTPUT].active) {
            if (discontA == 0) {
            }
            if (discontA == 1) {
                if (syncDiscontB == 1) {
                    if (incrB * phaseA <= incrA * phaseB) {
                        discontA = 0;
                        squareA *= -1.0f;
                    }
                }
                else {
                    if (incrB * (phaseA - 1.0f) <= incrA * phaseB) {
                        discontA = 0;
                        squareA *= -1.0f;
                    }
                }
            }
            else if (discontA == -1) {
                if (syncDiscontB == 1) {
                    if (incrB * phaseA <= incrA * (phaseB - 1.0f)) {
                        discontA = 0;
                        squareA *= -1.0f;
                    }
                }
                else {
                    if (incrB * (phaseA - 1.0f) <= incrA * (phaseB - 1.0f)) {
                        discontA = 0;
                        squareA *= -1.0f;
                    }
                }
            }
        }
        if (incrB >= 0.0f) {
            phaseA = phaseB / incrB * incrA;
        }
        else {
            phaseA = (phaseB - 1) / incrB * incrA;
        }
        if (incrA <= 0.0f) {
            ++phaseA;
        }
    }
    sawBufferA[3] = phaseA;
    sawBufferB[3] = phaseB;
    sqrBufferA[3] = squareA;
    sqrBufferB[3] = squareB;
    oldPhasesA[2] = phaseA;
    oldPhasesB[2] = phaseB;
    oldIncrsA[2] = incrA;
    oldIncrsB[2] = incrB;
    
    if (outputs[A_SAW_OUTPUT].active || outputs[A_SQR_OUTPUT].active) {
        if (oldSyncDiscontB == 0) {
            if (oldDiscontA == 0) {
            }
            else if (oldDiscontA == 1) {
                float offsetA = 1.0f - oldPhasesA[1] / oldIncrsA[1];
                polyblep4(sawBufferA, offsetA, oldDecrA);
                if (discontA == 0) {
                    polyblep4(sqrBufferA, offsetA, -2.0f * squareA);
                }
                else {
                    polyblep4(sqrBufferA, offsetA, 2.0f * squareA);
                }
            }
            else {
                float offsetA = 1.0f - (oldPhasesA[1] - 1.0f) / oldIncrsA[1];
                polyblep4(sawBufferA, offsetA, -oldDecrA);
                if (discontA == 0) {
                    polyblep4(sqrBufferA, offsetA, -2.0f * squareA);
                }
                else {
                    polyblep4(sqrBufferA, offsetA, 2.0f * squareA);
                }
            }
        }
        else {
            float offsetB = 0.0f;
            if (oldSyncDiscontB == 1) {
                offsetB = 1.0f - oldPhasesB[1] / oldIncrsB[1];
            }
            else {
                offsetB = 1.0f - (oldPhasesB[1] - 1.0f) / oldIncrsB[1];
            }
            if (oldDiscontA == 0) {
                if (oldIncrsA[1] >= 0.0f) {
                    polyblep4(sawBufferA, offsetB, oldPhasesA[0] + oldIncrsA[1] * offsetB);
                }
                else {
                    polyblep4(sawBufferA, offsetB, oldPhasesA[0] - oldIncrsA[1] * offsetB - 1);
                }
            }
            else {
                if (oldIncrsA[1] >= 0.0f) {
                    float offsetA = (1.0f - oldPhasesA[0]) / oldIncrsA[0];
                    polyblep4(sawBufferA, offsetA, oldDecrA);
                    polyblep4(sawBufferA, offsetB, oldIncrsA[1] * (offsetB - offsetA));
                    if (discontB == 0) {
                        polyblep4(sqrBufferA, offsetA, -2.0f * squareA);
                    }
                    else {
                        polyblep4(sqrBufferA, offsetA, 2.0f * squareA);
                    }
                }
                else {
                    float offsetA = 1.0f - (oldPhasesA[1] - 1.0f) / oldIncrsA[1];
                    polyblep4(sawBufferA, offsetA, -oldDecrB);
                    polyblep4(sawBufferA, offsetB, oldIncrsA[1] * (offsetB - offsetA));
                    if (discontB == 0) {
                        polyblep4(sqrBufferA, offsetA, -2.0f * squareA);
                    }
                    else {
                        polyblep4(sqrBufferA, offsetA, 2.0f * squareA);
                    }
                }
            }
        }
        outputs[A_SAW_OUTPUT].value = clampf(10.0f * ((sawBufferA[0] + params[A_CHAOS_PARAM].value) / (1.0f + params[A_CHAOS_PARAM].value) - 0.5f), -5.0f, 5.0f);
        outputs[A_SQR_OUTPUT].value = clampf(5.0f * sqrBufferA[0], -5.0f, 5.0f);
    }
    if (outputs[B_SAW_OUTPUT].active || outputs[B_SQR_OUTPUT].active) {
        if (oldSyncDiscontA == 0) {
            if (oldDiscontB == 0) {
            }
            else if (oldDiscontB == 1) {
                float offsetB = 1.0f - oldPhasesB[1] / oldIncrsB[1];
                polyblep4(sawBufferB, offsetB, oldDecrB);
                if (discontB == 0) {
                    polyblep4(sqrBufferB, offsetB, -2.0f * squareB);
                }
                else {
                    polyblep4(sqrBufferB, offsetB, 2.0f * squareB);
                }
            }
            else {
                float offsetB = 1.0f - (oldPhasesB[1] - 1.0f) / oldIncrsB[1];
                polyblep4(sawBufferB, offsetB, -oldDecrB);
                if (discontB == 0) {
                    polyblep4(sqrBufferB, offsetB, -2.0f * squareB);
                }
                else {
                    polyblep4(sqrBufferB, offsetB, 2.0f * squareB);
                }
            }
        }
        else {
            float offsetA = 0.0f;
            if (oldSyncDiscontA == 1) {
                offsetA = 1.0f - oldPhasesA[1] / oldIncrsA[1];
            }
            else {
                offsetA = 1.0f - (oldPhasesA[1] - 1.0f) / oldIncrsA[1];
            }
            if (oldDiscontB == 0) {
                if (oldIncrsB[1] >= 0) {
                    polyblep4(sawBufferB, offsetA, oldPhasesB[0] + oldIncrsB[1] * offsetA);
                }
                else {
                    polyblep4(sawBufferB, offsetA, oldPhasesB[0] - oldIncrsB[1] * offsetA - 1);
                }
            }
            else {
                if (oldIncrsB[1] >= 0.0f) {
                    float offsetB = (1.0f - oldPhasesB[0]) / oldIncrsB[0];
                    polyblep4(sawBufferB, offsetB, oldDecrA);
                    polyblep4(sawBufferB, offsetA, oldIncrsB[1] * (offsetA - offsetB));
                    if (discontB == 0) {
                        polyblep4(sqrBufferB, offsetB, -2.0f * squareB);
                    }
                    else {
                        polyblep4(sqrBufferB, offsetB, 2.0f * squareB);
                    }
                }
                else {
                    float offsetB = 1.0f - (oldPhasesB[1] - 1.0f) / oldIncrsB[1];
                    polyblep4(sawBufferB, offsetB, -oldDecrA);
                    polyblep4(sawBufferB, offsetA, oldIncrsB[1] * (offsetA - offsetB));
                    if (discontB == 0) {
                        polyblep4(sqrBufferB, offsetB, -2.0f * squareB);
                    }
                    else {
                        polyblep4(sqrBufferB, offsetB, 2.0f * squareB);
                    }
                }
            }
        }
        outputs[B_SAW_OUTPUT].value = clampf(10.0f * ((sawBufferB[0] + params[B_CHAOS_PARAM].value) / (1.0f + params[B_CHAOS_PARAM].value) - 0.5f), -5.0f, 5.0f);
        outputs[B_SQR_OUTPUT].value = clampf(5.0f * sqrBufferB[0], -5.0f, 5.0f);
    }
    
    oldDecrA = decrA;
    oldDecrB = decrB;
    oldDiscontA = discontA;
    oldDiscontB = discontB;
    oldSyncDiscontA = syncDiscontA;
    oldSyncDiscontB = syncDiscontB;
}


struct TachyonEntanglerWidget : ModuleWidget {
	TachyonEntanglerWidget(TachyonEntangler *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/Panels/TachyonEntangler.svg")));

		addChild(Widget::create<kHzScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<kHzScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<kHzScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(Widget::create<kHzScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        
        addParam(ParamWidget::create<kHzKnobSnap>(Vec(36, 40), module, TachyonEntangler::A_OCTAVE_PARAM, 4, 12, 8));
        addParam(ParamWidget::create<kHzKnobSmallSnap>(Vec(134, 112), module, TachyonEntangler::A_COARSE_PARAM, -7, 7, 0));
        addParam(ParamWidget::create<kHzKnobSmall>(Vec(134, 168), module, TachyonEntangler::A_FINE_PARAM, -0.083333, 0.083333, 0.0));
        addParam(ParamWidget::create<kHzKnob>(Vec(216, 40), module, TachyonEntangler::B_RATIO_PARAM, -1.0, 4.0, 0.0));
        
        addParam(ParamWidget::create<kHzKnobSmall>(Vec(16, 112), module, TachyonEntangler::A_EXP_FM_PARAM, -1.7, 1.7, 0.0));
        addParam(ParamWidget::create<kHzKnobSmall>(Vec(72, 112), module, TachyonEntangler::A_LIN_FM_PARAM, -11.7, 11.7, 0.0));
        addParam(ParamWidget::create<kHzKnobSmall>(Vec(196, 112), module, TachyonEntangler::B_EXP_FM_PARAM, -1.7, 1.7, 0.0));
        addParam(ParamWidget::create<kHzKnobSmall>(Vec(252, 112), module, TachyonEntangler::B_LIN_FM_PARAM, -11.7, 11.7, 0.0));
        
        addParam(ParamWidget::create<kHzKnobSmall>(Vec(16, 168), module, TachyonEntangler::A_CHAOS_PARAM, 0.0, 1.0, 0.0));
        addParam(ParamWidget::create<kHzKnobSmall>(Vec(72, 168), module, TachyonEntangler::A_SYNC_PROB_PARAM, 0.0, 1.0, 0.0));
        addParam(ParamWidget::create<kHzKnobSmall>(Vec(196, 168), module, TachyonEntangler::B_CHAOS_PARAM, 0.0, 1.0, 0.0));
        addParam(ParamWidget::create<kHzKnobSmall>(Vec(252, 168), module, TachyonEntangler::B_SYNC_PROB_PARAM, 0.0, 1.0, 1.0));
        
        addParam(ParamWidget::create<kHzKnobTiny>(Vec(50, 224), module, TachyonEntangler::A_CHAOS_MOD_PARAM, -0.1, 0.1, 0.0));
        addParam(ParamWidget::create<kHzKnobTiny>(Vec(106, 224), module, TachyonEntangler::A_SYNC_PROB_MOD_PARAM, -0.1, 0.1, 0.0));
        addParam(ParamWidget::create<kHzKnobTiny>(Vec(174, 224), module, TachyonEntangler::B_CHAOS_MOD_PARAM, -0.1, 0.1, 0.0));
        addParam(ParamWidget::create<kHzKnobTiny>(Vec(230, 224), module, TachyonEntangler::B_SYNC_PROB_MOD_PARAM, -0.1, 0.1, 0.0));
        
        addInput(Port::create<kHzPort>(Vec(7.5, 276), Port::INPUT, module, TachyonEntangler::A_EXP_FM_INPUT));
        addInput(Port::create<kHzPort>(Vec(44.5, 276), Port::INPUT, module, TachyonEntangler::A_LIN_FM_INPUT));
        addInput(Port::create<kHzPort>(Vec(81.5, 276), Port::INPUT, module, TachyonEntangler::A_CHAOS_INPUT));
        addInput(Port::create<kHzPort>(Vec(118.5, 276), Port::INPUT, module, TachyonEntangler::A_SYNC_PROB_INPUT));
        addInput(Port::create<kHzPort>(Vec(155.5, 276), Port::INPUT, module, TachyonEntangler::B_CHAOS_INPUT));
        addInput(Port::create<kHzPort>(Vec(192.5, 276), Port::INPUT, module, TachyonEntangler::B_SYNC_PROB_INPUT));
        addInput(Port::create<kHzPort>(Vec(229.5, 276), Port::INPUT, module, TachyonEntangler::B_EXP_FM_INPUT));
        addInput(Port::create<kHzPort>(Vec(266.5, 276), Port::INPUT, module, TachyonEntangler::B_LIN_FM_INPUT));
        
        addInput(Port::create<kHzPort>(Vec(7.5, 318), Port::INPUT, module, TachyonEntangler::A_V_OCT_INPUT));
        addInput(Port::create<kHzPort>(Vec(44.5, 318), Port::INPUT, module, TachyonEntangler::A_RESET_INPUT));
        addOutput(Port::create<kHzPort>(Vec(81.5, 318), Port::OUTPUT, module, TachyonEntangler::A_SAW_OUTPUT));
        addOutput(Port::create<kHzPort>(Vec(118.5, 318), Port::OUTPUT, module, TachyonEntangler::A_SQR_OUTPUT));
        addOutput(Port::create<kHzPort>(Vec(155.5, 318), Port::OUTPUT, module, TachyonEntangler::B_SAW_OUTPUT));
        addOutput(Port::create<kHzPort>(Vec(192.5, 318), Port::OUTPUT, module, TachyonEntangler::B_SQR_OUTPUT));
        addInput(Port::create<kHzPort>(Vec(229.5, 318), Port::INPUT, module, TachyonEntangler::B_V_OCT_INPUT));
        addInput(Port::create<kHzPort>(Vec(266.5, 318), Port::INPUT, module, TachyonEntangler::B_RESET_INPUT));
	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelTachyonEntangler = Model::create<TachyonEntangler, TachyonEntanglerWidget>("21kHz", "kHzTachyonEntangler", "Tachyon Entangler — chaotic sync VCO — 20hp", OSCILLATOR_TAG);

// 0.6.1
//  create
