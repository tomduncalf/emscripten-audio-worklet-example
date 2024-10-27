emcc c++/oscillator.cpp -o oscillator.js -sAUDIO_WORKLET=1 -sWASM_WORKERS=1 -sEXPORTED_RUNTIME_METHODS="[emscriptenRegisterAudioObject]" -s MODULARIZE -O3
