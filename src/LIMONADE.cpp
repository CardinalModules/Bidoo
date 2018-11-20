#include "Bidoo.hpp"
#include "dsp/digital.hpp"
#include "BidooComponents.hpp"
#include "window.hpp"
#include <thread>
#include "dsp/resampler.hpp"
#include "dsp/fir.hpp"
#include "osdialog.h"
#include "dep/dr_wav/dr_wav.h"
#include "dep/osc/wtOsc.h"
#include "../pffft/pffft.h"
#include <iostream>
#include <fstream>
#include "dep/lodepng/lodepng.h"

using namespace std;

void tUpdateWaveTable(wtTable &table, float index) {
	size_t i = index*(table.frames.size()-1);
	table.frames[i].calcWav();
}

void tLoadSample(wtTable &table, string path, size_t frameLen, bool interpolate, std::mutex &mtx) {
	unsigned int c;
  unsigned int sr;
  drwav_uint64 sc;
	float *pSampleData;
	float *sample;

  pSampleData = drwav_open_and_read_file_f32(path.c_str(), &c, &sr, &sc);
  if (pSampleData != NULL)  {
		sc = sc/c;
		sample = (float*)calloc(sc,sizeof(float));
		for (unsigned int i=0; i < sc; i++) {
			if (c == 1) sample[i] = pSampleData[i];
			else sample[i] = 0.5f*(pSampleData[2*i]+pSampleData[2*i+1]);
		}
		drwav_free(pSampleData);

		{
			std::lock_guard<std::mutex> lck {mtx};
			table.loadSample(sc, frameLen, interpolate, sample);
		}
		free(sample);
	}
}

void tLoadFrame(wtTable &table, string path, float index, bool interpolate, std::mutex &mtx) {
	unsigned int c;
  unsigned int sr;
  drwav_uint64 sc;
	float *pSampleData;
	float *sample;

  pSampleData = drwav_open_and_read_file_f32(path.c_str(), &c, &sr, &sc);
  if (pSampleData != NULL)  {
		sc = sc/c;
		sample = (float*)calloc(sc,sizeof(float));
		for (unsigned int i=0; i < sc; i++) {
			if (c == 1) sample[i] = pSampleData[i];
			else sample[i] = 0.5f*(pSampleData[2*i]+pSampleData[2*i+1]);
		}
		drwav_free(pSampleData);

		{
			std::lock_guard<std::mutex> lck {mtx};
			size_t i = index*(table.frames.size()-1);
			if (i<table.frames.size()) {
				table.frames[i].loadSample(sc, interpolate, sample);
			}
			else if (table.frames.size()==0) {
				table.addFrame(0);
				table.frames[0].loadSample(sc, interpolate, sample);
			}
		}

		free(sample);
	}
}

void tLoadPNG(wtTable &table, string path, std::mutex &mtx) {
	std::vector<unsigned char> image;
	float *sample;
	unsigned width = 0;
	unsigned height = 0;
	size_t sc = 0;
	unsigned error = lodepng::decode(image, width, height, path, LCT_RGB);
	if(error != 0)
  {
    std::cout << "error " << error << ": " << lodepng_error_text(error) << std::endl;
	}
  else {
		sc = width*height;
		sample = (float*)calloc(width*height,sizeof(float));
		for(size_t i=0; i<height; i++) {
			for (size_t j=0; j<width; j++) {
				sample[i*width+j] = (0.299f*image[3*j + 3*(height-i-1)*width] + 0.587f*image[3*j + 3*(height-i-1)*width +1] +  0.114f*image[3*j + 3*(height-i-1)*width +2])/255.0f - 0.5f;
			}
		}

		{
			std::lock_guard<std::mutex> lck {mtx};
			table.loadSample(sc, width, true, sample);
		}
		free(sample);
  }
}

void tWindowWt(wtTable &table) {
	table.window();
}

void tSmoothWt(wtTable &table) {
	table.smooth();
}

void tWindowFrame(wtTable &table, float index) {
	size_t i = index*(table.frames.size()-1);
	table.windowFrame(i);
}

void tSmoothFrame(wtTable &table, float index) {
	size_t i = index*(table.frames.size()-1);
	table.smoothFrame(i);
}

void tRemoveDCOffset(wtTable &table) {
	table.removeDCOffset();
}

void tNormalizeFrame(wtTable &table, float index) {
	size_t i = index*(table.frames.size()-1);
	table.frames[i].normalize();
}

void tNormalizeWt(wtTable &table) {
	table.normalize();
}

void tNormalizeAllFrames(wtTable &table) {
	table.normalizeAllFrames();
}

void tFFTSample(wtTable &table, float index) {
	size_t i = index*(table.frames.size()-1);
	table.frames[i].calcFFT();
}

void tIFFTSample(wtTable &table, float index) {
	size_t i = index*(table.frames.size()-1);
	table.frames[i].calcIFFT();
}

void tMorphWaveTable(wtTable &table, std::mutex &mtx) {
	std::lock_guard<std::mutex> lck {mtx};
	table.morphFrames();
}

void tMorphSpectrum(wtTable &table, std::mutex &mtx) {
	std::lock_guard<std::mutex> lck {mtx};
	table.morphSpectrum();
}

void tMorphSpectrumConstantPhase(wtTable &table, std::mutex &mtx) {
	std::lock_guard<std::mutex> lck {mtx};
	table.morphSpectrumConstantPhase();
}

void tAddFrame(wtTable &table, float index, std::mutex &mtx) {
	std::lock_guard<std::mutex> lck {mtx};
	size_t i = index*(table.frames.size()-1);
	table.addFrame(i);
}

void tDeleteFrame(wtTable &table, float index, std::mutex &mtx) {
	std::lock_guard<std::mutex> lck {mtx};
	size_t i = index*(table.frames.size()-1);
	table.removeFrame(i);
}

void tResetWaveTable(wtTable &table, std::mutex &mtx) {
	std::lock_guard<std::mutex> lck {mtx};
	table.reset();
}

void tDeleteMorphing(wtTable &table, std::mutex &mtx) {
	std::lock_guard<std::mutex> lck {mtx};
	table.deleteMorphing();
}

struct LIMONADE : Module {
	enum ParamIds {
		RESET_PARAM,
		SYNC_PARAM,
		FREQ_PARAM,
		FINE_PARAM,
		FM_PARAM,
		INDEX_PARAM,
		WTINDEX_PARAM,
		WTINDEXATT_PARAM,
		UNISSON_PARAM,
		UNISSONRANGE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		PITCH_INPUT,
		FM_INPUT = PITCH_INPUT+4,
		SYNC_INPUT,
		SYNCMODE_INPUT,
		WTINDEX_INPUT,
		IN,
		UNISSONRANGE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT,
		NUM_OUTPUTS = OUT+4
	};
	enum LightIds {
		NUM_LIGHTS
	};

	string lastPath;
	size_t frameSize = FS;
	std::mutex mtx;
	int morphType = -1;

	wtTable table;
	wtOscillator osc[3];

	LIMONADE() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		for(size_t i=0; i<3; i++) {
			osc[i].table = &table;
		}
	}

  ~LIMONADE() {

	}

	void step() override;
	void syncThreadData();
	void loadSample();
	void loadFrame();
	void loadPNG();
	void windowWt();
	void smoothWt();
	void windowFrame();
	void smoothFrame();
	void removeDCOffset();
	void fftSample();
	void ifftSample();
	void morphWavetable();
	void morphSpectrum();
	void morphSpectrumConstantPhase();
	void deleteMorphing();
	void updateWaveTable();
	void addFrame();
	void deleteFrame();
	void resetWaveTable();
	void normalizeFrame();
	void normalizeAllFrames();
	void normalizeWt();

	json_t *toJson() override {
		json_t *rootJ = json_object();
		json_t *framesJ = json_array();
		size_t nFrames = 0;
		{
			std::lock_guard<std::mutex> lck {mtx};
			for (size_t i=0; i<table.frames.size(); i++) {
				if (!table.frames[i].morphed) {
					json_t *frameI = json_array();
					for (size_t j=0; j<FS; j++) {
						json_t *frameJ = json_real(table.frames[i].sample[j]);
						json_array_append_new(frameI, frameJ);
					}
					json_array_append_new(framesJ, frameI);
					nFrames++;
				}
			}
		}
		json_object_set_new(rootJ, "nFrames", json_integer(nFrames));
		json_object_set_new(rootJ, "morphType", json_integer(morphType));
		json_object_set_new(rootJ, "frames", framesJ);
		return rootJ;
	}

	void fromJson(json_t *rootJ) override {

		size_t nFrames = 0;
		json_t *nFramesJ = json_object_get(rootJ, "nFrames");
		if (nFramesJ)
			nFrames = json_integer_value(nFramesJ);

		json_t *morphTypeJ = json_object_get(rootJ, "morphType");
		if (morphTypeJ)
			morphType = json_integer_value(morphTypeJ);

		if (nFrames>0)
		{
			float *wav = (float*)calloc(nFrames*FS, sizeof(float));
			{
				std::lock_guard<std::mutex> lck {mtx};

				json_t *framesJ = json_object_get(rootJ, "frames");
				for (size_t i = 0; i < nFrames; i++) {
					json_t *frameJ = json_array_get(framesJ, i);
					for (size_t j=0; j<FS; j++) {
						wav[i*FS+j] = json_number_value(json_array_get(frameJ, j));
					}
				}
				table.loadSample(nFrames*FS, FS, false, wav);
				if (morphType==0) {
					morphWavetable();
				}
				else if (morphType==1) {
					morphSpectrum();
				}
				else if (morphType==2) {
					morphSpectrumConstantPhase();
				}
			}
			delete(wav);
		}
	}

	void randomize() override {

	}

	void reset() override {
		{
			std::lock_guard<std::mutex> lck {mtx};
			table.reset();
		}
		lastPath = "";
	}
};

inline void LIMONADE::updateWaveTable() {
	thread t = thread(tUpdateWaveTable, std::ref(table), params[INDEX_PARAM].value);
	t.detach();
}

inline void LIMONADE::fftSample() {
	thread t = thread(tFFTSample, std::ref(table), params[INDEX_PARAM].value);
	t.detach();
}

inline void LIMONADE::ifftSample() {
	thread t = thread(tIFFTSample, std::ref(table), params[INDEX_PARAM].value);
	t.detach();
}

inline void LIMONADE::morphWavetable() {
	thread t = thread(tMorphWaveTable, std::ref(table), std::ref(mtx));
	morphType = 0;
	t.detach();
}

inline void LIMONADE::morphSpectrum() {
	thread t = thread(tMorphSpectrum, std::ref(table), std::ref(mtx));
	morphType = 1;
	t.detach();
}

inline void LIMONADE::morphSpectrumConstantPhase() {
	thread t = thread(tMorphSpectrumConstantPhase, std::ref(table), std::ref(mtx));
	morphType = 2;
	t.detach();
}

inline void LIMONADE::deleteMorphing() {
	thread t = thread(tDeleteMorphing, std::ref(table), std::ref(mtx));
	morphType = -1;
	t.detach();
}

void LIMONADE::addFrame() {
	thread t = thread(tAddFrame, std::ref(table), params[INDEX_PARAM].value, std::ref(mtx));
	t.detach();
}

void LIMONADE::deleteFrame() {
	thread t = thread(tDeleteFrame, std::ref(table), params[INDEX_PARAM].value, std::ref(mtx));
	t.detach();
}

void LIMONADE::resetWaveTable() {
	thread t = thread(tResetWaveTable, std::ref(table), std::ref(mtx));
	t.detach();
}

void LIMONADE::loadSample() {
	char *path = osdialog_file(OSDIALOG_OPEN, "", NULL, NULL);
	if (path) {
		lastPath=path;
		thread t = thread(tLoadSample, std::ref(table), path, frameSize, true, std::ref(mtx));
		t.detach();
		free(path);
	}
}

void LIMONADE::loadFrame() {
	char *path = osdialog_file(OSDIALOG_OPEN, "", NULL, NULL);
	if (path) {
		lastPath=path;
		thread t = thread(tLoadFrame, std::ref(table), path, params[INDEX_PARAM].value, true, std::ref(mtx));
		t.detach();
		free(path);
	}
}

void LIMONADE::loadPNG() {
	char *path = osdialog_file(OSDIALOG_OPEN, "", NULL, NULL);
	if (path) {
		lastPath=path;
		thread t = thread(tLoadPNG, std::ref(table), path, std::ref(mtx));
		t.detach();
		free(path);
	}
}

void LIMONADE::windowWt() {
	thread t = thread(tWindowWt, std::ref(table));
	t.detach();
}

void LIMONADE::smoothWt() {
	thread t = thread(tSmoothWt, std::ref(table));
	t.detach();
}

void LIMONADE::windowFrame() {
	thread t = thread(tWindowFrame, std::ref(table), params[INDEX_PARAM].value);
	t.detach();
}

void LIMONADE::smoothFrame() {
	thread t = thread(tSmoothFrame, std::ref(table), params[INDEX_PARAM].value);
	t.detach();
}

void LIMONADE::removeDCOffset() {
	thread t = thread(tRemoveDCOffset, std::ref(table));
	t.detach();
}


void LIMONADE::normalizeFrame() {
	thread t = thread(tNormalizeFrame, std::ref(table), params[INDEX_PARAM].value);
	t.detach();
}

void LIMONADE::normalizeWt() {
	thread t = thread(tNormalizeWt, std::ref(table));
	t.detach();
}

void LIMONADE::normalizeAllFrames() {
	thread t = thread(tNormalizeAllFrames, std::ref(table));
	t.detach();
}

void LIMONADE::step() {
	float pitchCv = 12.0f * inputs[PITCH_INPUT].value;
	float pitchFine = 3.0f * quadraticBipolar(params[FINE_PARAM].value);
	if (inputs[FM_INPUT].active) {
		pitchCv += quadraticBipolar(params[FM_PARAM].value) * 12.0f * inputs[FM_INPUT].value;
	}
	float idx=clamp(params[WTINDEX_PARAM].value+inputs[WTINDEX_INPUT].value*0.1f*params[WTINDEXATT_PARAM].value,0.0f,1.0f);
	outputs[OUT].value = 0.0f;

	float ur = clamp(params[UNISSONRANGE_PARAM].value + inputs[UNISSONRANGE_INPUT].value / 10.0f,0.0f,0.1f);
	for (size_t i=0; i<(size_t)params[UNISSON_PARAM].value; i++) {
		float pFC = pitchFine;
		if (params[UNISSON_PARAM].value>2) {
			if (i==1) {
				pFC += ur;
			}
			else if (i==2) {
				pFC -= ur;
			}
		}
		else if (params[UNISSON_PARAM].value>1) {
			if (i==0) {
				pFC += ur;
			}
			else {
				pFC -= ur;
			}
		}

		osc[i].soft = (params[SYNC_PARAM].value + inputs[SYNCMODE_INPUT].value) <= 0.0f;
		osc[i].setPitch(params[FREQ_PARAM].value, pFC + pitchCv);
		osc[i].syncEnabled = inputs[SYNC_INPUT].active;
		osc[i].prepare(engineGetSampleTime(), inputs[SYNC_INPUT].value);
	}

	{
		std::lock_guard<std::mutex> lck {mtx};
		for(size_t i=0; i<(size_t)params[UNISSON_PARAM].value; i++) {
			osc[i].updateBuffer(idx);
		}
	}

	for(size_t i=0; i<(size_t)params[UNISSON_PARAM].value; i++) {
			outputs[OUT].value += osc[i].out();
	}

	outputs[OUT].value *= 3.0 / sqrt(params[UNISSON_PARAM].value);
}

struct LIMONADEWidget : ModuleWidget {
	LIMONADEWidget(LIMONADE *module);
	Menu *createContextMenu() override;
};

struct LIMONADEBinsDisplay : OpaqueWidget {
	LIMONADE *module;
	shared_ptr<Font> font;
	const float width = 420.0f;
	const float heightMagn = 70.0f;
	const float heightPhas = 50.0f;
	const float graphGap = 30.0f;
	float zoomWidth = width * 28.0f;
	float zoomLeftAnchor = 0.0f;
	int refIdx = 0;
	float refY = 0.0f;
	float refX = 0.0f;
	bool write = false;
	float scrollLeftAnchor = 0.0f;
	bool scroll = false;
	float globalZoom = 1.0f;

	LIMONADEBinsDisplay() {
		font = Font::load(assetPlugin(plugin, "res/DejaVuSansMono.ttf"));
	}

	void onMouseDown(EventMouseDown &e) override {
		refX = e.pos.x;
		refY = e.pos.y;
		refIdx = ((e.pos.x - zoomLeftAnchor)/zoomWidth)*(float)FS2;
		if (refY<(heightMagn + heightPhas + graphGap)) {
			scroll = false;
		}
		else {
			if ((refX>scrollLeftAnchor) && (refX<scrollLeftAnchor+20)) scroll = true;
		}
		OpaqueWidget::onMouseDown(e);
	}

	void onDragStart(EventDragStart &e) override {
		if (!scroll) windowCursorLock();
		OpaqueWidget::onDragStart(e);
	}

	void onDragMove(EventDragMove &e) override {
		if ((!scroll) && (module->table.frames.size()>0)) {
			size_t i = module->params[LIMONADE::INDEX_PARAM].value*(module->table.frames.size()-1);
			if (refY<=heightMagn) {
				if (windowIsModPressed()) {
					module->table.frames[i].magnitude[refIdx] = 0.0f;
				}
				else {
					module->table.frames[i].magnitude[refIdx] -= e.mouseRel.y/(250/gRackScene->zoomWidget->zoom);
					module->table.frames[i].magnitude[refIdx] = clamp(module->table.frames[i].magnitude[refIdx],0.0f, 1.0f);
				}
			}
			else if (refY>=heightMagn+graphGap) {
				if (windowIsModPressed()) {
					module->table.frames[i].phase[refIdx] = 0.0f;
				}
				else {
					module->table.frames[i].phase[refIdx] -= e.mouseRel.y/(250/gRackScene->zoomWidget->zoom);
					module->table.frames[i].phase[refIdx] = clamp(module->table.frames[i].phase[refIdx],-1.0f*M_PI, M_PI);
				}
			}
			module->table.frames[i].morphed = false;
			module->updateWaveTable();
		}
		else {
				scrollLeftAnchor = clamp(scrollLeftAnchor + e.mouseRel.x / gRackScene->zoomWidget->zoom, 0.0f,width-20.0f);
				zoomLeftAnchor = rescale(scrollLeftAnchor, 0.0f, width-20.0f, 0.0f, width - zoomWidth);
		}
		OpaqueWidget::onDragMove(e);
	}

	void onDragEnd(EventDragEnd &e) override {
		if (!scroll) windowCursorUnlock();
		OpaqueWidget::onDragEnd(e);
	}

	void draw(NVGcontext *vg) override {
		wtFrame frame;
		size_t fs = 0;
		size_t idx = 0;
		size_t tag=1;

		{
			std::lock_guard<std::mutex> lck {module->mtx};
			fs = module->table.frames.size();
			if (fs>0) {
				idx = module->params[LIMONADE::INDEX_PARAM].value*(fs-1);
				frame.magnitude = module->table.frames[idx].magnitude;
				frame.phase = module->table.frames[idx].phase;
			}
		}

		nvgSave(vg);
		Rect b = Rect(Vec(zoomLeftAnchor, 0), Vec(zoomWidth, heightMagn + graphGap + heightPhas));
		nvgScissor(vg, 0, b.pos.y, width, heightMagn + graphGap + heightPhas+12);

		nvgBeginPath(vg);
		nvgRoundedRect(vg, 0, heightMagn + heightPhas + graphGap + 4, width, 8, 2);
		nvgFillColor(vg, nvgRGBA(220,220,220,80));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgRoundedRect(vg, scrollLeftAnchor, heightMagn + heightPhas + graphGap + 4, 20, 8, 2);
		nvgFillColor(vg, nvgRGBA(220,220,220,255));
		nvgFill(vg);

		nvgFontSize(vg, 16.0f);
		nvgFontFaceId(vg, font->handle);
		nvgTextLetterSpacing(vg, -2.0f);
		nvgFillColor(vg, YELLOW_BIDOO);

		nvgText(vg, 130.0f, heightMagn + graphGap/2+4, "▲ Magnitude ▼ Phase", NULL);

		if (fs>0) {
			nvgText(vg, 0.0f, heightMagn + graphGap/2+4, ("Frame " + to_string(idx+1) + " / " + to_string(fs)).c_str(), NULL);
			for (size_t i = 0; i < FS2; i++) {
				float x, y;
				x = (float)i * IFS2;
				y = frame.magnitude[i];
				Vec p;
				p.x = b.pos.x + b.size.x * x;
				p.y = heightMagn * y;

				if (i==tag){
					nvgBeginPath(vg);
					nvgFillColor(vg, nvgRGBA(45, 114, 143, 100));
					nvgRect(vg, p.x, 0, b.size.x * IFS2, heightMagn);
					nvgRect(vg, p.x, heightMagn + graphGap, b.size.x * IFS2, heightPhas);
					nvgLineCap(vg, NVG_MITER);
					nvgStrokeWidth(vg, 0);
					nvgFill(vg);
					tag *=2;
				}

				if (p.x < width) {
					nvgStrokeColor(vg, YELLOW_BIDOO);
					nvgFillColor(vg, YELLOW_BIDOO);
					nvgLineCap(vg, NVG_MITER);
					nvgStrokeWidth(vg, 1);
					nvgBeginPath(vg);
					nvgRect(vg, p.x, heightMagn - p.y, b.size.x * IFS2, p.y);
					y = module->table.frames[idx].phase[i]*IM_PI;
					p.y = heightPhas * 0.5f * y;
					nvgRect(vg, p.x, heightMagn + graphGap + heightPhas * 0.5f - p.y, b.size.x * IFS2, p.y);
					nvgStroke(vg);
					nvgFill(vg);
				}
			}
		}

		nvgResetScissor(vg);
		nvgRestore(vg);
	}
};

struct LIMONADEWavDisplay : OpaqueWidget {
	LIMONADE *module;
	shared_ptr<Font> font;
	const float width = 130.0f;
	const float height = 130.0f;
	int refIdx = 0;
	float alpha1 = 25.0f;
	float alpha2 = 35.0f;
	float a1 = alpha1 * M_PI/180.0f;
	float a2 = alpha2 * M_PI/180.0f;
	float ca1 = cos(a1);
	float sa1 = sin(a1);
	float ca2 = cos(a2);
	float sa2 = sin(a2);
	float x3D, y3D, z3D, x2D, y2D;

	LIMONADEWavDisplay() {
		font = Font::load(assetPlugin(plugin, "res/DejaVuSansMono.ttf"));
	}

	void onMouseDown(EventMouseDown &e) override {
			OpaqueWidget::onMouseDown(e);
	}

	void onDragStart(EventDragStart &e) override {
		windowCursorLock();
		OpaqueWidget::onDragStart(e);
	}

	void onDragMove(EventDragMove &e) override {
		alpha1+=e.mouseRel.y;
		alpha2-=e.mouseRel.x;
		if (alpha1>90) alpha1=90;
		if (alpha1<-90) alpha1=-90;
		if (alpha2>360) alpha2-=360;
		if (alpha2<0) alpha2+=360;

		a1 = alpha1 * M_PI/180.0f;
		a2 = alpha2 * M_PI/180.0f;
		ca1 = cos(a1);
		sa1 = sin(a1);
		ca2 = cos(a2);
		sa2 = sin(a2);
		OpaqueWidget::onDragMove(e);
	}

	void onDragEnd(EventDragEnd &e) override {
		windowCursorUnlock();
		OpaqueWidget::onDragEnd(e);
	}

	void draw(NVGcontext *vg) override {
		wtTable table;
		size_t fs = 0;
		size_t idx = 0;
		size_t wtidx = 0;
		{
			std::lock_guard<std::mutex> lck {module->mtx};
			fs = module->table.frames.size();
			if (fs>0) {
				idx = module->params[LIMONADE::INDEX_PARAM].value*(fs-1);
				wtidx = clamp(module->params[LIMONADE::WTINDEX_PARAM].value+module->inputs[LIMONADE::WTINDEX_INPUT].value*0.1f*module->params[LIMONADE::WTINDEXATT_PARAM].value,0.0f,1.0f)*(fs-1);
				for (size_t i=0; i< fs; i++) {
					wtFrame frame;
					frame.morphed = module->table.frames[i].morphed;
					frame.sample = module->table.frames[i].sample;
					table.frames.push_back(frame);
				}
			}
		}

		nvgSave(vg);

		nvgFontSize(vg, 8.0f);
		nvgFontFaceId(vg, font->handle);
		nvgTextLetterSpacing(vg, -2.0f);
		nvgFillColor(vg, YELLOW_BIDOO);

		nvgText(vg, width+6, height-10, ("V=" + to_string((int)module->params[LIMONADE::UNISSON_PARAM].value)).c_str(), NULL);


		for (size_t n=0; n<fs; n++) {
			nvgBeginPath(vg);
			for (size_t i=0; i<FS; i++) {
				y3D = 10.0f * (fs-n-1)/fs-5.0f;
				x3D = 10.0f * i * IFS -5.0f;
				z3D = table.frames[fs-1-n].sample[i];
				y2D = z3D*ca1-(ca2*y3D-sa2*x3D)*sa1+5.0f;
				x2D = ca2*x3D+sa2*y3D+7.5f;
				if (i == 0) {
					nvgMoveTo(vg, 10.0f * x2D, 10.0f * y2D);
				}
				else {
					nvgLineTo(vg, 10.0f * x2D, 10.0f * y2D);
				}
			}
			if (table.frames[fs-1-n].morphed) {
				nvgStrokeColor(vg, nvgRGBA(255, 233, 0, 15));
				nvgStrokeWidth(vg, 1.0f);
			}
			else {
				nvgStrokeColor(vg, nvgRGBA(255, 233, 0, 50));
				nvgStrokeWidth(vg, 1.0f);
			}
			nvgStroke(vg);
		}

		if (fs>0) {
			nvgBeginPath(vg);
			for (size_t i=0; i<FS; i++) {
				float x3D, y3D, z3D, x2D, y2D;
				y3D = 10.0f * idx/fs -5.0f;
				x3D = 10.0f * i * IFS -5.0f;
				z3D = table.frames[idx].sample[i];
				y2D = z3D*ca1-(ca2*y3D-sa2*x3D)*sa1+5.0f;
				x2D = ca2*x3D+sa2*y3D+7.5f;
				if (i == 0) {
					nvgMoveTo(vg, 10.0f * x2D, 10.0f * y2D);
				}
				else {
					nvgLineTo(vg, 10.0f * x2D, 10.0f * y2D);
				}
			}
			nvgStrokeColor(vg, GREEN_BIDOO);
			nvgStrokeWidth(vg, 1.0f);
			nvgStroke(vg);

			nvgBeginPath(vg);
			for (size_t i=0; i<FS; i++) {
				float x3D, y3D, z3D, x2D, y2D;
				y3D = 10.0f * wtidx/fs -5.0f;
				x3D = 10.0f * i * IFS -5.0f;
				z3D = table.frames[wtidx].sample[i];
				y2D = z3D*ca1-(ca2*y3D-sa2*x3D)*sa1+5.0f;
				x2D = ca2*x3D+sa2*y3D+7.5f;
				if (i == 0) {
					nvgMoveTo(vg, 10.0f * x2D, 10.0f * y2D);
				}
				else {
					nvgLineTo(vg, 10.0f * x2D, 10.0f * y2D);
				}
			}
			nvgStrokeColor(vg, RED_BIDOO);
			nvgStrokeWidth(vg, 1.0f);
			nvgStroke(vg);
		}

		nvgRestore(vg);
 	}
};

struct LIMONADETextField : LedDisplayTextField {
  LIMONADETextField(LIMONADE *mod) {
    module = mod;
    font = Font::load(assetPlugin(plugin, "res/DejaVuSansMono.ttf"));
  	color = YELLOW_BIDOO;
  	textOffset = Vec(2,0);
    text = "2048";
  }
	void onTextChange() override;
	LIMONADE *module;
};

void LIMONADETextField::onTextChange() {
	if (text.size() > 0) {
      string tText = text;
      tText.erase(std::remove_if(tText.begin(), tText.end(), [](unsigned char x){return std::isspace(x);}), tText.end());
      module->frameSize = std::stoi(tText);
	}
}

struct LimonadeBlueBtn : BlueBtn {
	LIMONADE *module;
	void (LIMONADE::*fncPtr)();

	virtual void onMouseDown(EventMouseDown &e) override {
		(module->*fncPtr)();
		BlueBtn::onMouseDown(e);
	}

	void draw(NVGcontext *vg) override {
		BlueBtn::draw(vg);
		nvgFontSize(vg, 12.0f);
		nvgFontFaceId(vg, font->handle);
		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgText(vg, 8.0f, 12.0f, (caption).c_str(), NULL);
		nvgStroke(vg);
	}
};

struct LimonadeBlueBtnLoadSample : LimonadeBlueBtn {
	virtual void onMouseDown(EventMouseDown &e) override {
		module->loadSample();
		BlueBtn::onMouseDown(e);
	}
};

struct LimonadeBlueBtnLoadPNG : LimonadeBlueBtn {
	virtual void onMouseDown(EventMouseDown &e) override {
		module->loadPNG();
		BlueBtn::onMouseDown(e);
	}
};

struct LimonadeBlueBtnLoadFrame : LimonadeBlueBtn {
	virtual void onMouseDown(EventMouseDown &e) override {
		module->loadFrame();
		BlueBtn::onMouseDown(e);
	}
};

struct LimonadeBlueBtnMorphWavetable : LimonadeBlueBtn {
	virtual void onMouseDown(EventMouseDown &e) override {
		module->morphWavetable();
		BlueBtn::onMouseDown(e);
	}
};

struct LimonadeBlueBtnMorphSpectrum : LimonadeBlueBtn {
	virtual void onMouseDown(EventMouseDown &e) override {
		module->morphSpectrum();
		BlueBtn::onMouseDown(e);
	}
};

struct LimonadeBlueBtnMorphSpectrumConstantPhase : LimonadeBlueBtn {
	virtual void onMouseDown(EventMouseDown &e) override {
		module->morphSpectrumConstantPhase();
		BlueBtn::onMouseDown(e);
	}
};

struct LimonadeBlueBtnDeleteMorphing : LimonadeBlueBtn {
	virtual void onMouseDown(EventMouseDown &e) override {
		module->deleteMorphing();
		BlueBtn::onMouseDown(e);
	}
};

struct LimonadeBlueBtnNormalizeAllFrames : LimonadeBlueBtn {
	virtual void onMouseDown(EventMouseDown &e) override {
		module->normalizeAllFrames();
		BlueBtn::onMouseDown(e);
	}
};

struct LimonadeBlueBtnRemoveDCOffset : LimonadeBlueBtn {
	virtual void onMouseDown(EventMouseDown &e) override {
		module->removeDCOffset();
		BlueBtn::onMouseDown(e);
	}
};

struct LimonadeBlueBtnNormalizeWt : LimonadeBlueBtn {
	virtual void onMouseDown(EventMouseDown &e) override {
		module->normalizeWt();
		BlueBtn::onMouseDown(e);
	}
};

struct LimonadeBlueBtnNormalizeFrame : LimonadeBlueBtn {
	virtual void onMouseDown(EventMouseDown &e) override {
		module->normalizeFrame();
		BlueBtn::onMouseDown(e);
	}
};

struct LimonadeBlueBtnWindowWt : LimonadeBlueBtn {
	virtual void onMouseDown(EventMouseDown &e) override {
		module->windowWt();
		BlueBtn::onMouseDown(e);
	}
};

struct LimonadeBlueBtnSmoothWt : LimonadeBlueBtn {
	virtual void onMouseDown(EventMouseDown &e) override {
		module->smoothWt();
		BlueBtn::onMouseDown(e);
	}
};

struct LimonadeBlueBtnWindowFrame : LimonadeBlueBtn {
	virtual void onMouseDown(EventMouseDown &e) override {
		module->windowFrame();
		BlueBtn::onMouseDown(e);
	}
};

struct LimonadeBlueBtnSmoothFrame : LimonadeBlueBtn {
	virtual void onMouseDown(EventMouseDown &e) override {
		module->smoothFrame();
		BlueBtn::onMouseDown(e);
	}
};

struct LimonadeBlueBtnAddFrame : LimonadeBlueBtn {
	virtual void onMouseDown(EventMouseDown &e) override {
		module->addFrame();
		BlueBtn::onMouseDown(e);
	}
};

struct LimonadeBlueBtnDeleteFrame : LimonadeBlueBtn {
	virtual void onMouseDown(EventMouseDown &e) override {
		module->deleteFrame();
		BlueBtn::onMouseDown(e);
	}
};

LIMONADEWidget::LIMONADEWidget(LIMONADE *module) : ModuleWidget(module) {
	setPanel(SVG::load(assetPlugin(plugin, "res/LIMONADE.svg")));

	addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	{
		LIMONADEBinsDisplay *display = new LIMONADEBinsDisplay();
		display->module = module;
		display->box.pos = Vec(15, 33);
		display->box.size = Vec(500, 164);
		addChild(display);
	}

	{
		LIMONADEWavDisplay *display = new LIMONADEWavDisplay();
		display->module = module;
		display->box.pos = Vec(11, 235);
		display->box.size = Vec(150, 110);
		addChild(display);
	}

	{
		LIMONADETextField *textField = new LIMONADETextField(module);
		textField->box.pos = Vec(170, 208);
		textField->box.size = Vec(38, 19);
		textField->multiline = false;
		addChild(textField);
	}

	{
		LimonadeBlueBtnLoadSample *btn = new LimonadeBlueBtnLoadSample();
		btn->box.pos = Vec(170, 235);
		btn->caption = "≈";
		btn->module = module;
		addChild(btn);
	}
	{
		LimonadeBlueBtnLoadPNG *btn = new LimonadeBlueBtnLoadPNG();
		btn->box.pos = Vec(190, 235);
		btn->caption = "☺";
		btn->module = module;
		addChild(btn);
	}
	{
		LimonadeBlueBtnLoadFrame *btn = new LimonadeBlueBtnLoadFrame();
		btn->box.pos = Vec(210, 235);
		btn->caption = "~";
		btn->module = module;
		addChild(btn);
	}
	{
		LimonadeBlueBtnMorphWavetable *btn = new LimonadeBlueBtnMorphWavetable();
		btn->box.pos = Vec(170, 255);
		btn->caption = "≡";
		btn->module = module;
		addChild(btn);
	}
	{
		LimonadeBlueBtnMorphSpectrum *btn = new LimonadeBlueBtnMorphSpectrum();
		btn->box.pos = Vec(190, 255);
		btn->caption = "∞";
		btn->module = module;
		addChild(btn);
	}
	{
		LimonadeBlueBtnMorphSpectrumConstantPhase *btn = new LimonadeBlueBtnMorphSpectrumConstantPhase();
		btn->box.pos = Vec(210, 255);
		btn->caption = "Փ";
		btn->module = module;
		addChild(btn);
	}
	{
		LimonadeBlueBtnDeleteMorphing *btn = new LimonadeBlueBtnDeleteMorphing();
		btn->box.pos = Vec(230, 255);
		btn->caption = "Ø";
		btn->module = module;
		addChild(btn);
	}
	{
		LimonadeBlueBtnNormalizeAllFrames *btn = new LimonadeBlueBtnNormalizeAllFrames();
		btn->box.pos = Vec(170, 275);
		btn->caption = "↕";
		btn->module = module;
		addChild(btn);
	}
	{
		LimonadeBlueBtnRemoveDCOffset *btn = new LimonadeBlueBtnRemoveDCOffset();
		btn->box.pos = Vec(190, 275);
		btn->caption = "↨";
		btn->module = module;
		addChild(btn);
	}
	{
		LimonadeBlueBtnNormalizeWt *btn = new LimonadeBlueBtnNormalizeWt();
		btn->box.pos = Vec(210, 275);
		btn->caption = "↕≈";
		btn->module = module;
		addChild(btn);
	}
	{
		LimonadeBlueBtnNormalizeFrame *btn = new LimonadeBlueBtnNormalizeFrame();
		btn->box.pos = Vec(230, 275);
		btn->caption = "↕~";
		btn->module = module;
		addChild(btn);
	}
	{
		LimonadeBlueBtnWindowWt *btn = new LimonadeBlueBtnWindowWt();
		btn->box.pos = Vec(170, 295);
		btn->caption = "∩≈";
		btn->module = module;
		addChild(btn);
	}
	{
		LimonadeBlueBtnSmoothWt *btn = new LimonadeBlueBtnSmoothWt();
		btn->box.pos = Vec(190, 295);
		btn->caption = "ƒ≈";
		btn->module = module;
		addChild(btn);
	}
	{
		LimonadeBlueBtnWindowFrame *btn = new LimonadeBlueBtnWindowFrame();
		btn->box.pos = Vec(210, 295);
		btn->caption = "∩~";
		btn->module = module;
		addChild(btn);
	}
	{
		LimonadeBlueBtnSmoothFrame *btn = new LimonadeBlueBtnSmoothFrame();
		btn->box.pos = Vec(230, 295);
		btn->caption = "ƒ~";
		btn->module = module;
		addChild(btn);
	}
	{
		LimonadeBlueBtnAddFrame *btn = new LimonadeBlueBtnAddFrame();
		btn->box.pos = Vec(170, 315);
		btn->caption = "+";
		btn->module = module;
		addChild(btn);
	}
	{
		LimonadeBlueBtnDeleteFrame *btn = new LimonadeBlueBtnDeleteFrame();
		btn->box.pos = Vec(190, 315);
		btn->caption = "-";
		btn->module = module;
		addChild(btn);
	}


	addParam(ParamWidget::create<CKSS>(Vec(280, 230), module, LIMONADE::SYNC_PARAM, 0.0f, 1.0f, 1.0f));
	addParam(ParamWidget::create<BidooBlueKnob>(Vec(307, 225), module, LIMONADE::FREQ_PARAM, -108.0f, 54.0f, 0.0f));
	addParam(ParamWidget::create<BidooBlueKnob>(Vec(342, 225), module, LIMONADE::FINE_PARAM, -1.0f, 1.0f, 0.0f));
	addParam(ParamWidget::create<BidooBlueKnob>(Vec(377, 225), module, LIMONADE::FM_PARAM, 0.0f, 1.0f, 0.0f));

	addParam(ParamWidget::create<BidooGreenKnob>(Vec(235.0f, 208.0f), module, LIMONADE::INDEX_PARAM, 0.0f, 1.0f, 0.0f));
	addParam(ParamWidget::create<BidooRedKnob>(Vec(412.0f, 225), module, LIMONADE::WTINDEX_PARAM, 0.0f, 1.0f, 0.0f));
	addParam(ParamWidget::create<BidooBlueTrimpot>(Vec(417.0f, 265), module, LIMONADE::WTINDEXATT_PARAM, -1.0f, 1.0f, 0.0f));

	addParam(ParamWidget::create<BidooBlueTrimpot>(Vec(170.0f, 335.0f), module, LIMONADE::UNISSON_PARAM, 1.0f, 3.0f, 1.0f));
	addParam(ParamWidget::create<BidooBlueTrimpot>(Vec(195.0f, 335.0f), module, LIMONADE::UNISSONRANGE_PARAM, 0.0f, 0.1f, 0.0f));
	addInput(Port::create<TinyPJ301MPort>(Vec(220, 337), Port::INPUT, module, LIMONADE::UNISSONRANGE_INPUT));

	addInput(Port::create<PJ301MPort>(Vec(275, 290), Port::INPUT, module, LIMONADE::SYNCMODE_INPUT));
	addInput(Port::create<PJ301MPort>(Vec(310, 290), Port::INPUT, module, LIMONADE::PITCH_INPUT));
	addInput(Port::create<PJ301MPort>(Vec(345, 290), Port::INPUT, module, LIMONADE::SYNC_INPUT));
	addInput(Port::create<PJ301MPort>(Vec(380, 290), Port::INPUT, module, LIMONADE::FM_INPUT));
	addInput(Port::create<PJ301MPort>(Vec(415, 290), Port::INPUT, module, LIMONADE::WTINDEX_INPUT));

	addInput(Port::create<PJ301MPort>(Vec(345, 340), Port::INPUT, module, LIMONADE::IN));
	addOutput(Port::create<PJ301MPort>(Vec(380, 340), Port::OUTPUT, module, LIMONADE::OUT));
}

struct LIMONADELoadSample : MenuItem {
	LIMONADE *limonadeModule;
	void onAction(EventAction &e) override {
		limonadeModule->loadSample();
	}
};

struct LIMONADELoadSampleInterpolate : MenuItem {
	LIMONADE *limonadeModule;
	void onAction(EventAction &e) override {
		limonadeModule->loadSample();
	}
};

struct LIMONADELoadFrame : MenuItem {
	LIMONADE *limonadeModule;
	void onAction(EventAction &e) override {
		limonadeModule->loadFrame();
	}
};

struct LIMONADELoadPNG : MenuItem {
	LIMONADE *limonadeModule;
	void onAction(EventAction &e) override {
		limonadeModule->loadPNG();
	}
};

struct LIMONADEAddFrame : MenuItem {
	LIMONADE *limonadeModule;
	void onAction(EventAction &e) override {
		limonadeModule->addFrame();
	}
};

struct LIMONADEDeleteFrame : MenuItem {
	LIMONADE *limonadeModule;
	void onAction(EventAction &e) override {
		limonadeModule->deleteFrame();
	}
};

struct LIMONADEResetWaveTable : MenuItem {
	LIMONADE *limonadeModule;
	void onAction(EventAction &e) override {
		limonadeModule->resetWaveTable();
	}
};

struct LIMONADEWindowSample : MenuItem {
	LIMONADE *limonadeModule;
	void onAction(EventAction &e) override {
		limonadeModule->windowWt();
	}
};

struct LIMONADESmoothSample : MenuItem {
	LIMONADE *limonadeModule;
	void onAction(EventAction &e) override {
		limonadeModule->smoothWt();
	}
};

struct LIMONADERemoveDCOffset : MenuItem {
	LIMONADE *limonadeModule;
	void onAction(EventAction &e) override {
		limonadeModule->removeDCOffset();
	}
};

struct LIMONADEFFTSample : MenuItem {
	LIMONADE *limonadeModule;
	void onAction(EventAction &e) override {
		limonadeModule->fftSample();
	}
};

struct LIMONADEIFFTSample : MenuItem {
	LIMONADE *limonadeModule;
	void onAction(EventAction &e) override {
		limonadeModule->ifftSample();
	}
};

struct LIMONADEMorphSample : MenuItem {
	LIMONADE *limonadeModule;
	void onAction(EventAction &e) override {
		limonadeModule->morphWavetable();
	}
};

struct LIMONADEMorphSpectrum : MenuItem {
	LIMONADE *limonadeModule;
	void onAction(EventAction &e) override {
		limonadeModule->morphSpectrum();
	}
};

struct LIMONADEMorphSpectrumConstantPhase : MenuItem {
	LIMONADE *limonadeModule;
	void onAction(EventAction &e) override {
	limonadeModule->morphSpectrumConstantPhase();
	}
};

struct LIMONADEDeleteMorphing : MenuItem {
	LIMONADE *limonadeModule;
	void onAction(EventAction &e) override {
		limonadeModule->deleteMorphing();
	}
};

struct LIMONADENormalizeFrame : MenuItem {
	LIMONADE *limonadeModule;
	void onAction(EventAction &e) override {
	limonadeModule->normalizeFrame();
	}
};

struct LIMONADENormalizeAllFrames : MenuItem {
	LIMONADE *limonadeModule;
	void onAction(EventAction &e) override {
	limonadeModule->normalizeAllFrames();
	}
};

struct LIMONADENormalizeWt : MenuItem {
	LIMONADE *limonadeModule;
	void onAction(EventAction &e) override {
	limonadeModule->normalizeWt();
	}
};

Menu *LIMONADEWidget::createContextMenu() {
	LIMONADE *limonadeModule = dynamic_cast<LIMONADE*>(module);
	assert(limonadeModule);
	Menu *menu = ModuleWidget::createContextMenu();

	LIMONADELoadSample *loadItem = new LIMONADELoadSample();
	loadItem->text = "Load wavetable";
	loadItem->limonadeModule = limonadeModule;
	menu->addChild(loadItem);

	LIMONADELoadSampleInterpolate *loadItemI = new LIMONADELoadSampleInterpolate();
	loadItemI->text = "Load wavetable interpolate";
	loadItemI->limonadeModule = limonadeModule;
	menu->addChild(loadItemI);

	LIMONADELoadFrame *loadItemF = new LIMONADELoadFrame();
	loadItemF->text = "Load frame";
	loadItemF->limonadeModule = limonadeModule;
	menu->addChild(loadItemF);

	LIMONADELoadPNG *loadItemPNG = new LIMONADELoadPNG();
	loadItemPNG->text = "Load PNG";
	loadItemPNG->limonadeModule = limonadeModule;
	menu->addChild(loadItemPNG);

	LIMONADEAddFrame *addFrameItem = new LIMONADEAddFrame();
	addFrameItem->text = "Add frame";
	addFrameItem->limonadeModule = limonadeModule;
	menu->addChild(addFrameItem);

	LIMONADEDeleteFrame *deleteFrameItem = new LIMONADEDeleteFrame();
	deleteFrameItem->text = "Delete frame";
	deleteFrameItem->limonadeModule = limonadeModule;
	menu->addChild(deleteFrameItem);

	LIMONADEResetWaveTable *resetWTItem = new LIMONADEResetWaveTable();
	resetWTItem->text = "Reset Wavetable";
	resetWTItem->limonadeModule = limonadeModule;
	menu->addChild(resetWTItem);

	LIMONADEWindowSample *windowItem = new LIMONADEWindowSample();
	windowItem->text = "Window wavetable";
	windowItem->limonadeModule = limonadeModule;
	menu->addChild(windowItem);

	LIMONADESmoothSample *sItem = new LIMONADESmoothSample();
	sItem->text = "Smooth wavetable";
	sItem->limonadeModule = limonadeModule;
	menu->addChild(sItem);

	LIMONADERemoveDCOffset *rDCOItem = new LIMONADERemoveDCOffset();
	rDCOItem->text = "Remove wavetable DC offset";
	rDCOItem->limonadeModule = limonadeModule;
	menu->addChild(rDCOItem);

	LIMONADEFFTSample *fftItem = new LIMONADEFFTSample();
	fftItem->text = "FFT frame";
	fftItem->limonadeModule = limonadeModule;
	menu->addChild(fftItem);

	LIMONADEIFFTSample *ifftItem = new LIMONADEIFFTSample();
	ifftItem->text = "IFFT frame";
	ifftItem->limonadeModule = limonadeModule;
	menu->addChild(ifftItem);

	LIMONADEMorphSample *morphSItem = new LIMONADEMorphSample();
	morphSItem->text = "Morph wavetable";
	morphSItem->limonadeModule = limonadeModule;
	menu->addChild(morphSItem);

	LIMONADEMorphSpectrum *morphSpItem = new LIMONADEMorphSpectrum();
	morphSpItem->text = "Morph wavetable spectrum";
	morphSpItem->limonadeModule = limonadeModule;
	menu->addChild(morphSpItem);

	LIMONADEMorphSpectrumConstantPhase *morphSpItemCP = new LIMONADEMorphSpectrumConstantPhase();
	morphSpItemCP->text = "Morph wavetable spectrum constant phase";
	morphSpItemCP->limonadeModule = limonadeModule;
	menu->addChild(morphSpItemCP);

	LIMONADEDeleteMorphing *dMorph = new LIMONADEDeleteMorphing();
	dMorph->text = "Delete morphing";
	dMorph->limonadeModule = limonadeModule;
	menu->addChild(dMorph);

	LIMONADENormalizeFrame *normFItem = new LIMONADENormalizeFrame();
	normFItem->text = "Normalize frame";
	normFItem->limonadeModule = limonadeModule;
	menu->addChild(normFItem);

	LIMONADENormalizeAllFrames *normAFItem = new LIMONADENormalizeAllFrames();
	normAFItem->text = "Normalize all frames";
	normAFItem->limonadeModule = limonadeModule;
	menu->addChild(normAFItem);

	LIMONADENormalizeWt *normWtItem = new LIMONADENormalizeWt();
	normWtItem->text = "Normalize wavetable";
	normWtItem->limonadeModule = limonadeModule;
	menu->addChild(normWtItem);

	return menu;
}

Model *modelLIMONADE = Model::create<LIMONADE, LIMONADEWidget>("Bidoo","liMonADe", "liMonADe additive osc", OSCILLATOR_TAG);
