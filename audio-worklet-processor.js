class OscillatorProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
    this.buffer = new Float32Array(128);
    this.port.onmessage = this.handleMessage.bind(this);
    this.ready = false;

    // Load the WebAssembly module using an inline script approach since fetch is not available in AudioWorkletProcessor
    this.port.onmessage = (event) => {
      if (event.data.type === "loadModule") {
        const scriptText = event.data.moduleText;
        try {
          const blob = new Blob([scriptText], {
            type: "application/javascript",
          });
          const scriptUrl = URL.createObjectURL(blob);
          import(scriptUrl).then((Module) => {
            this.Module = Module();
            this.setFrequency = this.Module.cwrap("setFrequency", "void", [
              "number",
            ]);
            this.generateSamples = this.Module.cwrap(
              "generateSamples",
              "void",
              ["number", "number"]
            );
            this.ready = true;
          });
        } catch (e) {
          console.error("Blob is not available in this environment:", e);
        }
      }
    };
  }

  handleMessage(event) {
    if (event.data.type === "setFrequency") {
      if (this.ready) {
        this.setFrequency(event.data.value);
      }
    }
  }

  process(inputs, outputs, parameters) {
    if (!this.ready) return true;

    const output = outputs[0];
    const channel = output[0];

    // Generate samples using WebAssembly
    const ptr = this.Module._malloc(
      channel.length * Float32Array.BYTES_PER_ELEMENT
    );
    this.generateSamples(ptr, channel.length);
    const generatedSamples = new Float32Array(
      this.Module.HEAPF32.buffer,
      ptr,
      channel.length
    );

    // Copy the generated samples to the output buffer
    channel.set(generatedSamples);

    // Free allocated memory
    this.Module._free(ptr);

    return true;
  }
}

registerProcessor("oscillator-processor", OscillatorProcessor);
