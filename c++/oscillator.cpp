/**
 * Example Audio Worklet using Emscripten's WASM Audio Worklets API
 * (https://emscripten.org/docs/api_reference/wasm_audio_worklets.html)
 */
#include <emscripten/em_math.h>
#include <emscripten/webaudio.h>
#include <math.h>
#include <stdio.h>
#include <vector>

// The worklet name
#define AWP_NAME "Test"

typedef void (*js_cb_func)(int data);

namespace audio {
// This was written by Claude mainly :) Basic oscillator class
class Oscillator {
private:
  float phase = 0.0f;
  float frequency = 440.0f;
  float phaseOffset = 0.0f;
  static constexpr float TWO_PI = 6.28318530718f;
  static constexpr float SAMPLE_RATE = 48000.0f;

public:
  void setFrequency(float freq) { frequency = freq; }
  void setPhaseOffset(float offset) { phaseOffset = offset; }
  void resetPhase(float newPhase) { phase = newPhase; }

  float process() {
    float sample = sin(phase + phaseOffset);
    phase += TWO_PI * frequency / SAMPLE_RATE;
    if (phase >= TWO_PI) {
      phase -= TWO_PI;
    }
    return sample;
  }
};

class OscillatorBank {
private:
  std::vector<Oscillator> oscillators;
  std::vector<float> mixWeights;
  size_t numOscillators;

public:
  explicit OscillatorBank(size_t n) : numOscillators(n) {
    oscillators.resize(n);
    mixWeights.resize(n, 1.0f / n); // Equal mix by default
  }

  void setFrequency(size_t index, float freq) {
    if (index < numOscillators) {
      oscillators[index].setFrequency(freq);
    }
  }

  void setPhaseOffset(size_t index, float phase) {
    if (index < numOscillators) {
      oscillators[index].setPhaseOffset(phase);
    }
  }

  void setMixWeight(size_t index, float weight) {
    if (index < numOscillators) {
      mixWeights[index] = weight;
    }
  }

  void resetPhase(size_t index, float newPhase) {
    if (index < numOscillators) {
      oscillators[index].resetPhase(newPhase);
    }
  }

  float process() {
    float output = 0.0f;
    for (size_t i = 0; i < numOscillators; ++i) {
      output += oscillators[i].process() * mixWeights[i];
    }
    return output;
  }

  size_t size() const { return numOscillators; }
};

constexpr size_t NUM_OSCILLATORS = 3;
OscillatorBank oscBank(NUM_OSCILLATORS);
uint8_t audioThreadStack[4096];
js_cb_func js_cb;

// This is the callback which generates samples
EM_BOOL awp_process_cb(int numInputs, const AudioSampleFrame *inputs,
                       int numOutputs, AudioSampleFrame *outputs, int numParams,
                       const AudioParamFrame *params, void *userData) {
  // Example of reading Audio Worklet parmaters. A-rate parameters might have an
  // array of 128 values as they can change value throughout the quantum, but
  // these are really being used as k-rate here so just have 1.
  oscBank.setFrequency(0, params[0].data[0]);
  oscBank.setPhaseOffset(0, params[1].data[0]);

  for (int i = 0; i < numOutputs; ++i) {
    for (int j = 0;
         j < outputs[i].samplesPerChannel * outputs[i].numberOfChannels; ++j) {
      outputs[i].data[j] = oscBank.process();
    }
  }

  return EM_TRUE;
}

// This is called once the worklet has been created and creates a
// AudioWorkletNode then calls back to JS with the node for it to add it to the
// audio graph
void awp_create_cb(EMSCRIPTEN_WEBAUDIO_T audio_ctx, EM_BOOL success,
                   void *init_js_cb) {
#ifdef TD_DEBUG
  printf("tsig_awp_create_cb(audio_ctx=%d, success=%d, init_js_cb=%p)\n",
         audio_ctx, success, init_js_cb);
#endif /* TD_DEBUG */

  if (!success)
    return;

  oscBank.setFrequency(1, 660);
  oscBank.setFrequency(2, 880);

  int request_output_channels[] = {1};

  EmscriptenAudioWorkletNodeCreateOptions options = {
      .numberOfInputs = 0,
      .numberOfOutputs = 1,
      .outputChannelCounts = request_output_channels,
  };

  EMSCRIPTEN_AUDIO_WORKLET_NODE_T awn_handle =
      emscripten_create_wasm_audio_worklet_node(audio_ctx, AWP_NAME, &options,
                                                awp_process_cb, NULL);

  ((js_cb_func)init_js_cb)(awn_handle);
}

// This is called once the worklet thread is initialized, and creates the
// worklet
void aw_thread_init_cb(EMSCRIPTEN_WEBAUDIO_T audio_ctx, EM_BOOL success,
                       void *init_js_cb) {
#ifdef TD_DEBUG
  printf("tsig_aw_thread_init_cb(audio_ctx=%d, success=%d, init_js_cb=%p)\n",
         audio_ctx, success, init_js_cb);
#endif /* TD_DEBUG */

  if (!success)
    return;

  // This is how you can define Audio Worklet parameters. There's no way to give
  // parameters a name, so you have to use the index when modifying it from JS.
  WebAudioParamDescriptor freqParam = {.defaultValue = 20.0f,
                                       .minValue = 1.0f,
                                       .maxValue = 40.0f,
                                       .automationRate = WEBAUDIO_PARAM_A_RATE};

  WebAudioParamDescriptor phaseParam = {.defaultValue = 0.0f,
                                        .minValue = -3.14f,
                                        .maxValue = 3.14f,
                                        .automationRate =
                                            WEBAUDIO_PARAM_A_RATE};

  WebAudioParamDescriptor paramDescriptors[] = {freqParam, phaseParam};

  WebAudioWorkletProcessorCreateOptions opts = {.name = AWP_NAME,
                                                .numAudioParams = 2,
                                                .audioParamDescriptors =
                                                    paramDescriptors};

  emscripten_create_wasm_audio_worklet_processor_async(
      audio_ctx, &opts, awp_create_cb, init_js_cb);
}

} // namespace audio

// C API implementations
extern "C" {
EMSCRIPTEN_KEEPALIVE void reset_phase(float newPhase) {
  audio::oscBank.resetPhase(0, newPhase);
}

/**
 * Initialize the Wasm module. This is called from JS.
 *
 * @param audio_ctx Handle of a Web Audio API AudioContext.
 * @param sample_rate Sample rate of the AudioContext.
 * @param init_js_cb Pointer to JS callback that will be used for exporting an
 *  AudioWorkletNode created as the result of module initialization.
 * @note Should be called once per page load. `audio_ctx` can be obtained in
 *  JS via emscriptenRegisterAudioObject() on a preexisting AudioContext.
 */
EMSCRIPTEN_KEEPALIVE void module_init(EMSCRIPTEN_WEBAUDIO_T audio_ctx,
                                      uint32_t sample_rate,
                                      js_cb_func init_js_cb,
                                      js_cb_func _js_cb) {
  audio::js_cb = _js_cb;

  emscripten_start_wasm_audio_worklet_thread_async(
      audio_ctx, audio::audioThreadStack, sizeof(audio::audioThreadStack),
      audio::aw_thread_init_cb, (void *)init_js_cb);
}
}
