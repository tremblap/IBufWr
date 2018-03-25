#include "SC_PlugIn.h"

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


inline double sc_loop(Unit *unit, double in, double hi, int loop) {
  // avoid the divide if possible
  if (in >= hi) {
    if (!loop) {
      unit->mDone = true;
      return hi;
    }
    in -= hi;
    if (in < hi) return in;
  } else if (in < 0.) {
    if (!loop) {
      unit->mDone = true;
      return 0.;
    }
    in += hi;
    if (in >= 0.) return in;
  } else return in;

  return in - hi * floor(in/hi);
}

struct IBufWr : public Unit
{
  float m_fbufnum;
  SndBuf *m_buf;
};

void IBufWr_Ctor(IBufWr *unit);
void IBufWr_next(IBufWr *unit, int inNumSamples);

void IBufWr_Ctor(IBufWr *unit)
{
  unit->m_fbufnum = -1.f;//declares the unit buf num as unasigned (<0)

  Print("nb of inputs %i\n", unit->mNumInputs);

  SETCALC(IBufWr_next);

  ClearUnitOutputs(unit, 1);
}

void IBufWr_next(IBufWr *unit, int inNumSamples)
{
  float *phasein  = ZIN(1);
  uint32 loop     = (uint32)ZIN0(2);

  GET_BUF //this macro, defined in  SC_Unit.h, does all the sanity check and assigns valutes to bufData, bufChannels, numInputChannels, bufFrames
  uint32 numInputChannels = unit->mNumInputs - 3;// minus 3 because the arguments are all passed after the input array

  // other sanity check, mostly of size
  if (!checkBuffer(unit, bufData, bufChannels, numInputChannels, inNumSamples))
    return;

  //defines the maximum loop bound.
  double loopMax = (double)(bufFrames - (loop ? 0 : 1));

  //iterates through the input samples
  for (uint32 k=0; k<inNumSamples; ++k) {
    // this unwraps the index should loop be on. then casts it as int32
    double phase = sc_loop((Unit*)unit, ZXP(phasein), loopMax, loop);
    int32 iphase = (int32)phase;
    // defines a 'table' which is a simple float pointer as a facilitator for pointer arythmetics, using the index (iphase) multipled by the number of channels in the buffer (the data is interleaved)
    float* table0 = bufData + iphase * bufChannels;
    // iterator for the width of the input array/stream
    for (uint32 channel=0; channel<numInputChannels; ++channel)
      table0[channel] = IN(channel+3)[k];
  }
}

PluginLoad(IButtUGens) {
  ft = inTable;
  DefineSimpleUnit(IBufWr);
}
