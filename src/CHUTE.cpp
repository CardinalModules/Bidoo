#include "Bidoo.hpp"
#include "dsp/digital.hpp"
#include "BidooComponents.hpp"
#include <vector>
#include "cmath"

using namespace std;


struct CHUTE : Module {
	enum ParamIds {
		ALTITUDE_PARAM,
		GRAVITY_PARAM,
		COR_PARAM,
		RUN_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		TRIG_INPUT,
		ALTITUDE_INPUT,
		GRAVITY_INPUT,
		COR_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATE_OUTPUT,
		PITCH_OUTPUT,
		PITCHSTEP_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	bool running = false;
	float phase = 0.0;
	float altitude = 0.0;
	float altitudeInit = 0.0;
	float minAlt = 0.0;
	float speed = 0.0;
	bool desc = false;

	SchmittTrigger playTrigger;
	SchmittTrigger gateTypeTrigger;

	CHUTE() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) { }

	void step() override;

};


void CHUTE::step() {

	// Running
	if (playTrigger.process(params[RUN_PARAM].value + inputs[TRIG_INPUT].value)) {
		running = true;
		desc = true;
		altitude = params[ALTITUDE_PARAM].value + inputs[ALTITUDE_INPUT].value;
		altitudeInit = altitude;
		minAlt = altitude;
		speed = 0;
	}

	// Altitude calculation
	if (running) {
		if (minAlt<0.0001) {
			running = false;
			altitude = 0;
			minAlt = 0;
		}
		else
		{
			phase = 1 / engineGetSampleRate();
			if (desc) {
				speed += (params[GRAVITY_PARAM].value + inputs[GRAVITY_INPUT].value)*phase;
				altitude = altitude - (speed * phase);
				if (altitude <= 0) {
					desc=false;
					speed = speed * (params[COR_PARAM].value + + inputs[COR_INPUT].value);
					altitude = 0;
				}
			}
			else {
				speed = speed - (params[GRAVITY_PARAM].value + inputs[GRAVITY_INPUT].value)*phase;
				if (speed<=0) {
					speed = 0.0;
					desc=true;
					minAlt=min(minAlt,altitude);
				}
				else {
					altitude = altitude + (speed * phase);
				}
			}
		}
	}

	//Calculate output
	outputs[GATE_OUTPUT].value = running ? desc ? 10.0 : 0.0 : 0.0;
	outputs[PITCH_OUTPUT].value = running ? 10 * altitude/ altitudeInit : 0.0;
	outputs[PITCHSTEP_OUTPUT].value = running ? 10 * minAlt/ altitudeInit : 0.0;
}

struct CHUTEDisplay : TransparentWidget {
	CHUTE *module;
	int frame = 0;
	shared_ptr<Font> font;

	CHUTEDisplay() {
		font = Font::load(assetPlugin(plugin, "res/DejaVuSansMono.ttf"));
	}

	void draw(NVGcontext *vg) override {
		frame = 0;
		nvgFontSize(vg, 18);
		nvgFontFaceId(vg, font->handle);
		nvgTextLetterSpacing(vg, -2);
		nvgFillColor(vg, nvgRGBA(0x00, 0x00, 0x00, 0xff));

		float altRatio = clampf(module->altitude / module->altitudeInit, 0 , 1);
		int pos = roundl(box.size.y + altRatio * (9 - box.size.y));

		nvgText(vg, 6, pos, "☻", NULL);
	}
};

CHUTEWidget::CHUTEWidget() {
	CHUTE *module = new CHUTE();
	setModule(module);
	box.size = Vec(15*9, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/ChUTE.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

	{
		CHUTEDisplay *display = new CHUTEDisplay();
		display->module = module;
		display->box.pos = Vec(110, 30);
		display->box.size = Vec(40, 180);
		addChild(display);
	}

	static const float portX[2] = {20, 60};
	static const float portY[3] = {52, 116, 178};
 	addInput(createInput<PJ301MPort>(Vec(portX[0], portY[0]), module, CHUTE::ALTITUDE_INPUT));
	addParam(createParam<BidooBlueKnob>(Vec(portX[1], portY[0]-2), module, CHUTE::ALTITUDE_PARAM, 0.01, 3, 1));
	addInput(createInput<PJ301MPort>(Vec(portX[0], portY[1]), module, CHUTE::GRAVITY_INPUT));
	addParam(createParam<BidooBlueKnob>(Vec(portX[1], portY[1]-2), module, CHUTE::GRAVITY_PARAM, 1.622, 11.15, 9.798)); // between the Moon and Neptune
	addInput(createInput<PJ301MPort>(Vec(portX[0], portY[2]), module, CHUTE::COR_INPUT));
	addParam(createParam<BidooBlueKnob>(Vec(portX[1], portY[2]-2), module, CHUTE::COR_PARAM, 0, 1, 0.69)); // 0 inelastic, 1 perfect elastic, 0.69 glass

	addParam(createParam<BlueCKD6>(Vec(51, 269), module, CHUTE::RUN_PARAM, 0.0, 1.0, 0.0));
	addInput(createInput<PJ301MPort>(Vec(11, 270), module, CHUTE::TRIG_INPUT));

	addOutput(createOutput<PJ301MPort>(Vec(11, 320), module, CHUTE::GATE_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(54, 320), module, CHUTE::PITCH_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(96, 320), module, CHUTE::PITCHSTEP_OUTPUT));
}
