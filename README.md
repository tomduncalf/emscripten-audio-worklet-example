# Emscripten Audio Worklet Example

Emscripten now has a WASM Audio Worklets API (https://emscripten.org/docs/api_reference/wasm_audio_worklets.html) which lets you build WebAudio Audio Worklets in pure C/C++ without needing to write the JS wrapper yourself, but there isn't much by way of documentation, particularly for how to integrate it with an existing audio graph.

I found the https://github.com/kangtastic/timestation/ project which uses it, and this is a simplified version of how that project works.

The worklet itself is just a basic oscillator bank written in C++, and C for the parts exposed to Emscripten.

I had a project in mind for this but I might never get to it. The code won't win any prizes (largely AI generated or copied from Timestation!) but I figured I'd post this as I couldn't find any simple examples of the full API out there.

## Files

- `c++/oscillator.cpp` contains the worklet code, which is the interesting bit.
- `oscillator.js` contains the setup JS, which creates the audio graph and hooks everyrthing up.

## Building

Run `./build.sh` to build the worklet. The build files end up in the top-level directory, it's probably possible to make them be written to a different directory!

## Serving

The worklet needs to be served with certain headers due to the use of `SharedArrayBuffer`. There's a simple Node server in `server/server.js` which can be used to serve it locally, like `node server/server.js`.
