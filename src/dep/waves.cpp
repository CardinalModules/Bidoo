#include "waves.hpp"
#include "AudioFile/AudioFile.h"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav/dr_wav.h"
#include <dsp/resampler.hpp>

namespace waves {

  std::vector<rack::dsp::Frame<1>> getMonoWav(const std::string path, const float currentSampleRate, std::string &waveFileName, std::string &waveExtension, int &sampleChannels, int &sampleRate, int &sampleCount) {
    waveFileName = rack::string::filename(path);
    waveExtension = rack::string::filenameExtension(rack::string::filename(path));
    std::vector<rack::dsp::Frame<1>> result;
    if (waveExtension == "wav") {
      unsigned int c;
      unsigned int sr;
      drwav_uint64 sc;
      float* pSampleData;
      pSampleData = drwav_open_file_and_read_f32(path.c_str(), &c, &sr, &sc);
      if (pSampleData != NULL)  {
        sampleChannels = c;
        sampleRate = sr;
        for (long long unsigned int i=0; i < sc; i = i + c) {
          rack::dsp::Frame<1> frame;
          if (sampleChannels == 2) {
            frame.samples[0] = (pSampleData[i] + pSampleData[i+1])/2.0f;
          }
          else {
            frame.samples[0] = pSampleData[i];
          }
          result.push_back(frame);
        }
        sampleCount = sc/c;
        drwav_free(pSampleData);
      }
    }
    else if (waveExtension == "aiff") {
      AudioFile<float> audioFile;
      if (audioFile.load (path.c_str()))  {
        sampleChannels = audioFile.getNumChannels();
        sampleRate = audioFile.getSampleRate();
        sampleCount = audioFile.getNumSamplesPerChannel();
        for (int i=0; i < sampleCount; i++) {
          rack::dsp::Frame<1> frame;
          if (sampleChannels == 2) {
            frame.samples[0] = (audioFile.samples[0][i] + audioFile.samples[1][i])/2.0f;
          }
          else {
            frame.samples[0] = audioFile.samples[0][i];
          }
          result.push_back(frame);
        }
      }
    }

    if ((sampleRate != currentSampleRate) && (sampleCount>0)) {
      rack::dsp::SampleRateConverter<1> conv;
      conv.setRates(currentSampleRate, sampleRate);
      int outCount = sampleCount;
      std::vector<rack::dsp::Frame<1>> subResult;
      for (int i=0;i<sampleCount;i++) {
        rack::dsp::Frame<1> frame;
        frame.samples[0]=0.0f;
        subResult.push_back(frame);
      }
      conv.process(&result[0], &sampleCount, &subResult[0], &outCount);
      sampleCount = outCount;
      return subResult;
    }

    return result;
  }

  std::vector<rack::dsp::Frame<2>> getStereoWav(const std::string path, const float currentSampleRate, std::string &waveFileName, std::string &waveExtension, int &sampleChannels, int &sampleRate, int &sampleCount) {
    waveFileName = rack::string::filename(path);
    waveExtension = rack::string::filenameExtension(rack::string::filename(path));
    std::vector<rack::dsp::Frame<2>> result;
    if (waveExtension == "wav") {
      unsigned int c;
      unsigned int sr;
      drwav_uint64 sc;
      float* pSampleData;
      pSampleData = drwav_open_file_and_read_f32(path.c_str(), &c, &sr, &sc);
      if (pSampleData != NULL)  {
        sampleChannels = c;
        sampleRate = sr;
        for (long long unsigned int i=0; i < sc; i = i + c) {
          rack::dsp::Frame<2> frame;
          frame.samples[0] = pSampleData[i];
  				if (sampleChannels == 2)
  					frame.samples[1] = (float)pSampleData[i+1];
  				else
  					frame.samples[1] = (float)pSampleData[i];
          result.push_back(frame);
        }
        sampleCount = sc/c;
        drwav_free(pSampleData);
      }
    }
    else if (waveExtension == "aiff") {
      AudioFile<float> audioFile;
      if (audioFile.load (path.c_str()))  {
        sampleChannels = audioFile.getNumChannels();
        sampleRate = audioFile.getSampleRate();
        sampleCount = audioFile.getNumSamplesPerChannel();
        for (int i=0; i < sampleCount; i++) {
          rack::dsp::Frame<2> frame;
          frame.samples[0] = audioFile.samples[0][i];
  				if (sampleChannels == 2)
  					frame.samples[1] = audioFile.samples[1][i];
  				else
  					frame.samples[1] = audioFile.samples[0][i];
          result.push_back(frame);
        }
      }
    }

    if ((sampleRate != currentSampleRate) && (sampleCount>0)) {
      rack::dsp::SampleRateConverter<2> conv;
      conv.setRates(sampleRate, currentSampleRate);
      conv.setQuality(SPEEX_RESAMPLER_QUALITY_DESKTOP);
      int outCount = 16*sampleCount;
      std::vector<rack::dsp::Frame<2>> subResult;
      for (int i=0;i<outCount;i++) {
        rack::dsp::Frame<2> frame;
        frame.samples[0]=0.0f;
        frame.samples[1]=0.0f;
        subResult.push_back(frame);
      }
      conv.process(&result[0], &sampleCount, &subResult[0], &outCount);
      sampleCount = outCount;
      return subResult;
    }

    return result;
  }

}
