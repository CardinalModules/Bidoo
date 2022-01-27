#include "rack.hpp"

using namespace rack;

const NVGcolor BLUE_BIDOO = nvgRGBA(42, 87, 117, 255);
const NVGcolor LIGHTBLUE_BIDOO = nvgRGBA(45, 114, 143, 255);
const NVGcolor RED_BIDOO = nvgRGBA(205, 31, 0, 255);
const NVGcolor YELLOW_BIDOO = nvgRGBA(255, 233, 0, 255);
const NVGcolor YELLOW_BIDOO_LIGHT = nvgRGBA(255, 233, 0, 25);
const NVGcolor SAND_BIDOO = nvgRGBA(230, 220, 191, 255);
const NVGcolor ORANGE_BIDOO = nvgRGBA(228, 87, 46, 255);
const NVGcolor PINK_BIDOO = nvgRGBA(164, 3, 111, 255);
const NVGcolor GREEN_BIDOO = nvgRGBA(2, 195, 154, 255);

extern Plugin *pluginInstance;

extern Model *modelDTROY;
extern Model *modelBORDL;
extern Model *modelZOUMAI;
extern Model *modelZOUMAIExpander;
extern Model *modelTOCANTE;
extern Model *modelCHUTE;
extern Model *modelLATE;
extern Model *modelACNE;
extern Model *modelMS;
extern Model *modelOUAIVE;
extern Model *modelEDSAROS;
extern Model *modelPOUPRE;
extern Model *modelMAGMA;
extern Model *modelOAI;
extern Model *modelEMILE;
extern Model *modelFORK;
extern Model *modelFLAME;
extern Model *modelTIARE;
extern Model *modelANTN;
extern Model *modelLIMONADE;
extern Model *modelLIMBO;
extern Model *modelPERCO;
extern Model *modelBAFIS;
extern Model *modelBAR;
extern Model *modelMINIBAR;
extern Model *modelZINC;
extern Model *modelDUKE;
extern Model *modelMOIRE;
extern Model *modelPILOT;
extern Model *modelHUITRE;
extern Model *modelCANARD;
extern Model *modelFREIN;
extern Model *modelLOURDE;
extern Model *modelDILEMO;
extern Model *modelLAMBDA;
extern Model *modelBANCAU;
extern Model *modelHCTIP;
extern Model *modelSPORE;
extern Model *modelDFUZE;
extern Model *modelREI;
extern Model *modelMU;
extern Model *modelSIGMA;
extern Model *modelRABBIT;
extern Model *modelBISTROT;
extern Model *modelDIKTAT;
extern Model *modelVOID;

struct InstantiateExpanderItem : MenuItem {
	Module* module;
	Model* model;
	Vec posit;
	void onAction(const event::Action &e) override;
};

struct BidooModule : Module {
	int themeId = 0;
	bool themeChanged = true;
	json_t *dataToJson() override;
	void dataFromJson(json_t *rootJ) override;
};

struct BidooWidget : ModuleWidget {
	SvgPanel* lightPanel;
	SvgPanel* darkPanel;
	SvgPanel* blackPanel;
	SvgPanel* bluePanel;
	SvgPanel* greenPanel;

	struct LightItem : MenuItem {
		BidooModule *module;
		void onAction(const event::Action &e) override {
			module->themeId = 0;
			module->themeChanged = true;
		}
	};

	struct DarkItem : MenuItem {
		BidooModule *module;
		void onAction(const event::Action &e) override {
			module->themeId = 1;
			module->themeChanged = true;
		}
	};

	struct BlackItem : MenuItem {
		BidooModule *module;
		void onAction(const event::Action &e) override {
			module->themeId = 2;
			module->themeChanged = true;
		}
	};

	struct BlueItem : MenuItem {
		BidooModule *module;
		void onAction(const event::Action &e) override {
			module->themeId = 3;
			module->themeChanged = true;
		}
	};

	struct GreenItem : MenuItem {
		BidooModule *module;
		void onAction(const event::Action &e) override {
			module->themeId = 4;
			module->themeChanged = true;
		}
	};

	void prepareThemes(const std::string& filename);
	void appendContextMenu(Menu *menu) override;
	void step() override;
};
