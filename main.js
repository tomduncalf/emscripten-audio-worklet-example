document.getElementById("startButton").addEventListener("click", async () => {
  // Load the WebAssembly module
  const Module = await import("./oscillator.js");

  // Create the AudioContext
  const audioContext = new (window.AudioContext || window.webkitAudioContext)();

  // Resume the AudioContext on user interaction
  await audioContext.resume();
});
