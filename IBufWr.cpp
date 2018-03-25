#include "SC_PlugIn.h"

#define CLIP(a, lo, hi) ( (a)>(lo)?( (a)<(hi)?(a):(hi) ):(lo) )

static InterfaceTable *ft;

static inline bool checkBuffer(Unit * unit, const float * bufData, uint32 bufChannels, uint32 expectedChannels, int inNumSamples) {
  if (!bufData)// if the pointer to the data is null, exit
    goto handle_failure;

  // if the number of input streams in the input array (pass here as expectedChannels) is larger than the number of channels in the buffer, exit
  if (expectedChannels > bufChannels) {
    //moan if verbose
    if(unit->mWorld->mVerbosity > -1 && !unit->mDone)
      Print("Buffer UGen channel mismatch: expected %i, yet buffer has %i channels\n", expectedChannels, bufChannels);
    goto handle_failure;
  }
  // exit positively
  return true;
  // exit negatively
  handle_failure:
  // declares the UGEN as done and fills the output buffer with 0s
  unit->mDone = true;
  ClearUnitOutputs(unit, inNumSamples);
  return false;
}

struct IBufWr : public Unit {
  float m_fbufnum;
  SndBuf *m_buf;
  double *valeurs;
};

void IBufWr_Ctor(IBufWr *unit);
void IBufWr_Dtor(IBufWr *unit);
void IBufWr_next(IBufWr *unit, int inNumSamples);

void IBufWr_Ctor(IBufWr *unit) {
  //declares the unit buf num as unasigned (<0)
  unit->m_fbufnum = -1.f;

  //defines the counters and initialise them to 0
  unit->valeurs = (double *)RTAlloc(unit->mWorld, ((unit->mNumInputs - 2) * sizeof(double)));
  memset(unit->valeurs,0,((unit->mNumInputs - 2) * sizeof(double)));

  // defines the ugen in the tree
  SETCALC(IBufWr_next);

  // sends one sample of silence
  ClearUnitOutputs(unit, 1);
}

void IBufWr_Dtor(IBufWr *unit){
  RTFree(unit->mWorld, unit->valeurs);
}

void IBufWr_next(IBufWr *unit, int inNumSamples) {
  float *phasein  = IN(1);// instead of ZIN which was coping with the offset

  GET_BUF //this macro, defined in  SC_Unit.h, does all the sanity check, locks the buffer and assigns valutes to bufData, bufChannels, numInputChannels, bufFrames
  uint32 numInputChannels = unit->mNumInputs - 2;// minus 2 because the arguments are all passed after the input array

  // other sanity check, mostly of size
  if (!checkBuffer(unit, bufData, bufChannels, numInputChannels, inNumSamples))
    return;

  //iterates through the input samples
  for (uint32 k=0; k<inNumSamples; ++k) {
    // for each channel of the input array, increments the counter with the input, and writes it a buffer index 0+channel
    for (uint32 channel=0; channel<numInputChannels; ++channel) {
      unit->valeurs[channel] += IN(channel+2)[k];// adds 2 to offset the 2 arguments
      bufData[channel] = unit->valeurs[channel];
    }
  }
}

PluginLoad(IButtUGens) {
  ft = inTable;
  DefineDtorUnit(IBufWr);
}
