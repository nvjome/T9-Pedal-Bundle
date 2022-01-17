/*
    Nolan Jome

    Contains all function and global variable definitions.
    Also creates the EffectClass objects for each effect,
    and the EffectClass object pointer array.

    Procedure to add an effect:
        Add the Teensy Audio objects definitions.
        Add the Teensy Audio connection definitions.
        Add parameter change functions for each effect parameter.
        Create EffectClass object.
        Append new EffectClass object to pointer array.
*/

#include <Arduino.h>
#include "T9_Pedal_Bundle.h"
#include "effect_class.h"

///////////////////////////////////////
// Audio Objects
// All audio objects for each effect, input, output, and others.
// Use format for effects:  effectXXShortBlockName
// Where XX is effect number.
///////////////////////////////////////

// System input objects:
//AudioInputUSB               lineIn;
AudioInputI2S               lineIn;

AudioAmplifier              preGain;

// Effect 0: Bypass
AudioEffectPassthrough      effect00Bypass;

// Effect 1: Low Pass Filter
AudioFilterStateVariable    effect01LPF;

// Effect 2: Freeverb
AudioEffectPassthrough      effect02Buffer;
AudioEffectFreeverb         effect02Freeverb;
AudioMixer4                 effect02Mixer;

// Effect 3: 


// System output objects:
AudioAmplifier              postGain;
AudioOutputI2S              lineOut;

// Peak detection blocks
AudioAnalyzePeak            lineInPeak;
AudioAnalyzePeak            lineOutPeak;

//Codec
AudioControlSGTL5000        sgtl5000;


///////////////////////////////////////
// Audio Connections
// ALL possible connections between objects.
// Use format for effects:  effectXXInput
//                          effectXXOutput
//                          effectXXSubYY
// Where XX is effect number and YY is another number for the effect sub connections.
// The first audio block's input is the output of the preGain block.
// The last audio block's output is the input of the postGain block.
// Effects may need multiple sub connections between objects in the effect.
// Use sensical fomat for other constant connections with postfix _c.
///////////////////////////////////////

// Pre-effect connections

AudioConnection             lineInToPreGain_c(lineIn, 0, preGain, 0);
AudioConnection             lineInToPeakL_c(lineIn, 0, lineInPeak, 0);

// Post-effect connections

AudioConnection             postGainToLineOutL_c(postGain, 0, lineOut, 0);
AudioConnection             postGainToLineOutR_c(postGain, 0, lineOut, 1);
AudioConnection             postGainToPeak_c(postGain, 0, lineOutPeak, 0);

// Effect 0: Bypass
AudioConnection             effect00Input(preGain, 0, effect00Bypass, 0);
AudioConnection             effect00Output(effect00Bypass, 0, postGain, 0);

// Effect 1: Low Pass Filter
AudioConnection             effect01Input(preGain, 0, effect01LPF, 0);
AudioConnection             effect01Output(effect01LPF, 0, postGain, 0);

// Effect 2: Freeverb
AudioConnection             effect02Input(preGain, 0, effect02Buffer, 0);
AudioConnection             effect02Sub01(effect02Buffer, 0, effect02Mixer, 0);
AudioConnection             effect02Sub02(effect02Buffer, 0, effect02Freeverb, 0);
AudioConnection             effect02Sub03(effect02Freeverb, 0, effect02Mixer, 1);
AudioConnection             effect02Output(effect02Mixer, 0, postGain, 0);


/*
    T9PB_begin
    Initializes the Teensy Audio library and settings.

    NOTE: memory argument removed due to compilation error,
    the argument for the AudioMemory macro needs to be a constant
    at compile time.
*/
void T9PB_begin(void) {
    T9PB_disconnect_all_effects();

    sgtl5000.enable();
    sgtl5000.inputSelect(AUDIO_INPUT_LINEIN);
    sgtl5000.lineInLevel(0);    // max input level
    //sgtl5000.lineOutLevel(13);  // max output level
    sgtl5000.volume(0.3);

    preGain.gain(1.0);
    postGain.gain(1.0);
}

/*
    T9PB_disconnect_all_effects
    Disconnects all effects.

    TODO: May change to just iterating through an array of
    pointers to the AudioConnection objects that are the input
    and output connections to each effect.
*/
void T9PB_disconnect_all_effects(void) {
    for (int i = 0; i <= NUM_EFFECTS; i++) {
        (*effectObjects_a[i]).disconnect();
    }
}

void T9PB_hp_volume(float volume) {
    sgtl5000.volume(volume);
}

/*
    T9PB_peak_detect
    Checks for peaks at the input or output.
    Arguments:
        source: Where to check for peak, 0 checks input, 1 checks output.
    Returns:
        The greatest sample value since the last check, 0.0 to 1.0 float.
*/
float T9PB_peak_detect(int source) {
    float ret = 0.0;
    if (source == 0) {
        if (lineInPeak.available()) {
            ret = lineInPeak.read();
        }
    } else if (source == 1) {
        if (lineOutPeak.available()) {
            ret = lineOutPeak.read();
        }
    }

    return ret;
}


///////////////////////////////////////
// Effect parameter functions
///////////////////////////////////////

// Null function, used for empty parameters
// Would ideally be optimized away, but not sure.
void nullFunc(float n) {}

// Effect 1: Low Pass Filter
void T9PB_effect01_frequency(float freq) {
    effect01LPF.frequency(freq);
}

// Effect 2: Freeverb
void T9PB_effect02_roomsize(float size) {
    effect02Freeverb.roomsize(size);
}

void T9PB_effect02_damping(float damp) {
    effect02Freeverb.damping(damp);
}

void T9PB_effect02_wetdry(float wet) {
    if (wet > 1.0 || wet < 0.0) return;
    // "dry" gain
    effect02Mixer.gain(0,wet-1.0);
    // "wet" gain
    effect02Mixer.gain(1,wet);
}


///////////////////////////////////////
// Effect objects
// Each EffectClass object needs the following:
//      Effect name
//      Parameter name(s)
//      Input connection object pointer
//      Output connection object pointer
//      Parameter change function pointers
///////////////////////////////////////

// Effect 0: Bypass
EffectClass effect00Bypass_o(
    "Bypass", "NA", "NA", "NA",
    &effect00Input,
    &effect00Output,
    nullFunc, nullFunc, nullFunc
);

// Effect 1: Low Pass Filter
EffectClass effect01LPF_o(
    "LPF", "Frequency", "NA", "NA",
    &effect01Input,
    &effect01Output,
    T9PB_effect01_frequency, nullFunc, nullFunc
);

// Effect 2: Freeverb
EffectClass effect02Freeverb_o(
    "Freeverb", "Roomsize", "Damping", "Wet/Dry",
    &effect02Input,
    &effect02Output,
    T9PB_effect02_roomsize, T9PB_effect02_damping, T9PB_effect02_wetdry
);


///////////////////////////////////////
// Effect object pointer array
///////////////////////////////////////

EffectClass* effectObjects_a[NUM_EFFECTS+1] = {
    &effect00Bypass_o,
    &effect01LPF_o,
    &effect02Freeverb_o
};