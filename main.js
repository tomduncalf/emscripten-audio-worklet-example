document.getElementById("startButton").addEventListener("click", async () => {
  let audioWorkletNode;
  let analyserNode;

  const createModule = (await import("./oscillator.js")).default;
  const module = await createModule();

  const audioContext = new AudioContext({ latencyHint: "playback" });
  const audioContextHandle = module.emscriptenRegisterAudioObject(audioContext);

  /* Init continues below when Wasm invokes #finishInit() as a callback. */
  const finishInitPtr = module.addFunction(finishInit, "vi");
  const communicatePtr = module.addFunction(communicate, "vi");

  // Initialize the worklet, passing in the audio context
  module._module_init(
    audioContextHandle,
    audioContext.sampleRate,
    finishInitPtr,
    communicatePtr
  );

  // This is called when the worklet is initialized. Continue setting up the audio graph.
  function finishInit(audioWorkletNodeHandle) {
    audioWorkletNode = module.emscriptenGetAudioObject(audioWorkletNodeHandle);
    analyserNode = audioContext.createAnalyser();
    audioWorkletNode.connect(analyserNode);

    analyserNode.connect(audioContext.destination);

    analyserNode.fftSize = 2048;
    const canvas = document.getElementById("visualizer");
    const canvasCtx = canvas.getContext("2d");

    function draw() {
      const bufferLength = analyserNode.frequencyBinCount;
      const dataArray = new Uint8Array(bufferLength);

      requestAnimationFrame(draw);
      analyserNode.getByteTimeDomainData(dataArray);

      canvasCtx.fillStyle = "rgb(200, 200, 200)";
      canvasCtx.fillRect(0, 0, canvas.width, canvas.height);

      canvasCtx.lineWidth = 2;
      canvasCtx.strokeStyle = "rgb(0, 0, 0)";
      canvasCtx.beginPath();

      const sliceWidth = (canvas.width * 1.0) / bufferLength;
      let x = 0;

      for (let i = 0; i < bufferLength; i++) {
        const v = dataArray[i] / 128.0;
        const y = (v * canvas.height) / 2;

        if (i === 0) {
          canvasCtx.moveTo(x, y);
        } else {
          canvasCtx.lineTo(x, y);
        }

        x += sliceWidth;
      }

      canvasCtx.lineTo(canvas.width, canvas.height / 2);
      canvasCtx.stroke();
    }

    draw();
    audioContext.resume();
  }

  function communicate(state) {}

  const frequencySlider = document.getElementById("frequencySlider");
  frequencySlider.addEventListener("input", (event) => {
    audioWorkletNode.parameters
      .get(0)
      .setValueAtTime(event.target.value, audioContext.currentTime);
  });

  const phaseSlider = document.getElementById("phaseSlider");
  phaseSlider.addEventListener("input", (event) => {
    audioWorkletNode.parameters
      .get(1)
      .setValueAtTime(event.target.value, audioContext.currentTime);
  });

  const resetPhaseButton = document.getElementById("resetPhaseButton");
  resetPhaseButton.addEventListener("click", () => {
    module._reset_phase(0);
  });
});
