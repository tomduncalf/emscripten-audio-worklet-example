#include <emscripten/em_math.h>
#include <emscripten/webaudio.h>
#include <math.h>

uint8_t audioThreadStack[4096];
constexpr float TWO_PI = 6.28318530718f;
constexpr float SAMPLE_RATE = 48000.0f;
float phase = 0.0f;
float frequency = 440.0f; // Default frequency is 440 Hz (A4)
float phaseOffset = 0.0f; // Phase offset value

// Function to set frequency from the main thread (can be exposed later for
// interactivity)
extern "C" {
void set_frequency(float newFrequency) { frequency = newFrequency; }

void set_phase_offset(float newPhaseOffset) { phaseOffset = newPhaseOffset; }

void reset_phase(float newPhase) { phase = newPhase; }
}

int main() {
  // Create an audio context
  EMSCRIPTEN_WEBAUDIO_T context = emscripten_create_audio_context(0);

  // Start the Wasm Audio Worklet thread
  emscripten_start_wasm_audio_worklet_thread_async(
      context, audioThreadStack, sizeof(audioThreadStack), nullptr, 0);

  // Define the audio processor after initializing the thread
  WebAudioWorkletProcessorCreateOptions opts = {
      .name = "sine-oscillator",
  };

  emscripten_create_wasm_audio_worklet_processor_async(context, &opts, nullptr,
                                                       0);

  int outputChannelCounts[1] = {1};
  EmscriptenAudioWorkletNodeCreateOptions options = {.numberOfInputs = 0,
                                                     .numberOfOutputs = 1,
                                                     .outputChannelCounts =
                                                         outputChannelCounts};

  // Create an oscillator node
  EMSCRIPTEN_AUDIO_WORKLET_NODE_T wasmAudioWorklet =
      emscripten_create_wasm_audio_worklet_node(context, "sine-oscillator",
                                                &options, nullptr, 0);

  // Connect the oscillator node to the audio context destination
  emscripten_audio_node_connect(wasmAudioWorklet, context, 0, 0);

  return 0;
}

bool GenerateSineWave(int numInputs, const AudioSampleFrame *inputs,
                      int numOutputs, AudioSampleFrame *outputs, int numParams,
                      const AudioParamFrame *params, void *userData) {
  for (int i = 0; i < numOutputs; ++i) {
    for (int j = 0;
         j < outputs[i].samplesPerChannel * outputs[i].numberOfChannels; ++j) {
      outputs[i].data[j] = sin(phase + phaseOffset);
      phase += TWO_PI * frequency / SAMPLE_RATE;
      // Wrap phase within 0 to TWO_PI
      if (phase >= TWO_PI) {
        phase -= TWO_PI;
      }
    }
  }
  return true; // Keep the output running
}