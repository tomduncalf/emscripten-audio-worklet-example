EMCC_PARAMS=(
  "-sEXPORTED_RUNTIME_METHODS=['addFunction','emscriptenRegisterAudioObject','emscriptenGetAudioObject']"
  '-sEXPORT_NAME=createModule'
  '-sINITIAL_MEMORY=65536'
  '-sALLOW_TABLE_GROWTH'
  '-sSTACK_SIZE=32768'
  '-sAUDIO_WORKLET'
  '-sWASM_WORKERS'
  '-sEXPORT_ES6'
  '-sJS_MATH'
  '-sMODULARIZE'
  '-DTD_DEBUG'
  '-O3'
)

emcc c++/oscillator.cpp -o oscillator.js "${EMCC_PARAMS[@]}"
