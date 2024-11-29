// IBufWr, an interpolating buffer writer
// Pierre Alexandre Tremblay, 2018
// porting from the bespoke Max object ipoke~ v4.1
// (http://www.no-tv.org/MaxMSP/) thanks to the FluCoMa project funded by the
// European Research Council (ERC) under the European Union’s Horizon 2020
// research and innovation programme (grant agreement No 725899)

#include "SC_PlugIn.h"

static InterfaceTable *ft;

static inline bool checkBuffer(Unit *unit, const float *bufferData,
                               uint32 bufferChannels, uint32 expectedChannels,
                               int inNumSamples) {
  if (!bufferData) // if the pointer to the data is null, exit
    goto handle_failure;

  // if the number of input streams in the input array (passed here as
  // expectedChannels) is larger than the number of channels in the buffer, exit
  if (expectedChannels > bufferChannels) { // moan if verbose
    if (unit->mWorld->mVerbosity > -1 && !unit->mDone)
      Print("Buffer UGen channel mismatch: expected %i, yet buffer has %i "
            "channels\n",
            expectedChannels, bufferChannels);
    goto handle_failure;
  }
  return true; // exit positively

handle_failure: // exit negatively
  // declares the UGEN as done and fills the output buffer with 0s
  unit->mDone = true;
  ClearUnitOutputs(unit, inNumSamples);
  return false;
}

struct IBufWr : public Unit {
  float m_fbufnum;
  SndBuf *m_buf;

  long m_last_index;
  long l_nb_val;
  double *m_values;
  double *l_coeffs;
};

void IBufWr_Ctor(IBufWr *unit);
void IBufWr_Dtor(IBufWr *unit);
void IBufWr_next(IBufWr *unit, int n);

void IBufWr_Ctor(IBufWr *unit) {
  // declares the unit buffer number as unassigned (<0)
  unit->m_fbufnum = -1.f;
  // defines the counters and initialize them to 0
  unit->m_values = (double *)RTAlloc(unit->mWorld,
                                     ((unit->mNumInputs - 4) * sizeof(double)));
  memset(unit->m_values, 0, ((unit->mNumInputs - 4) * sizeof(double)));
  // defines the coefficients and initialize them to 0
  unit->l_coeffs = (double *)RTAlloc(unit->mWorld,
                                     ((unit->mNumInputs - 4) * sizeof(double)));
  memset(unit->l_coeffs, 0, ((unit->mNumInputs - 4) * sizeof(double)));
  // initialize the other instance variables
  unit->m_last_index = -1;

  // defines the ugen in the tree
  SETCALC(IBufWr_next);

  // sends one sample of silence
  ClearUnitOutputs(unit, 1);
}

void IBufWr_Dtor(IBufWr *unit) {
  RTFree(unit->mWorld, unit->m_values);
  RTFree(unit->mWorld, unit->l_coeffs);
}

void IBufWr_next(IBufWr *unit, int n) {
  auto inputIndex = IN(1);
  bool interpolate = static_cast<bool>(IN0(2));
  double feedback = static_cast<double>(IN0(3));

  GET_BUF // this macro, defined in SC_Unit.h, does all the sanity check, locks
          // the buffer and assigns values to bufData, bufChannels, bufFrames
      long numChannels =
          unit->mNumInputs - 4; // minus 4 because the arguments are all passed
                                // after the input array

  // other sanity check, mostly of size
  if (!checkBuffer(unit, bufData, bufChannels, numChannels, n))
    return;

  double halfLife = static_cast<long>(static_cast<double>(bufFrames) * 0.5);

  auto previousIndex = unit->m_last_index;
  auto &values = unit->m_values;
  auto &coefficients = unit->l_coeffs;
  auto numberOfValues = unit->l_nb_val;

  auto writeAverageValue = [&](long index) {
    for (long chan = 0; chan < numChannels; ++chan) {
      bufData[index * bufChannels + chan] =
          zapgremlins(static_cast<float>((bufData[index * bufChannels + chan]
                      * feedback) + (values[chan] / numberOfValues)));
      values[chan] = 0.0;
    }
  };

  auto calculateCoefficients = [&](long step, int j) {
    for (long chan = 0; chan < numChannels; ++chan) {
      coefficients[chan] = (IN(chan + 4)[j] - values[chan]) / step;
    }
  };

  auto fillGap = [&](long start, long end, long step) {
    for (long i = start; i != end; i += step) {
      for (long chan = 0; chan < numChannels; ++chan) {
        values[chan] += coefficients[chan];
        bufData[i * bufChannels + chan] = zapgremlins(static_cast<float>(
            (bufData[i * bufChannels + chan] * feedback) + values[chan]));
      }
    }
  };

  auto processSample = [&](double indexBuffer, int j) {
    if (indexBuffer < 0.0) {    // if the writing is stopped
      if (previousIndex >= 0) { // and if it is the 1st one to be stopped
        writeAverageValue(previousIndex);
        previousIndex = -1;
      }
    } else {
      // round the next index and make sure it is in the buffer's boundaries
      long index = sc_wrap(static_cast<long>(indexBuffer), 0, bufFrames);

      if (previousIndex < 0) { // if it is the first index to write, resets the
                               // averaging and the values
        previousIndex = index;
        numberOfValues = 0;
      }

      if (index == previousIndex) { // if the index has not moved, accumulate
                                    // the value to average later.
        for (long chan = 0; chan < numChannels; ++chan)
          values[chan] += IN(chan + 4)[j];
        numberOfValues += 1;
      } else {                     // if it moves
        if (numberOfValues != 1) { // is there more than one value to average
          for (long chan = 0; chan < numChannels; ++chan)
            values[chan] /= numberOfValues; // if yes, calculate the average
          numberOfValues = 1;
        }

        for (long chan = 0; chan < numChannels; ++chan)
          bufData[previousIndex * bufChannels + chan] = zapgremlins(
              static_cast<float>((bufData[previousIndex * bufChannels + chan]
              * feedback) + values[chan])); // write the average value at the
              // last index

        long step = index - previousIndex; // calculate the step to do

        if (step > 0) {          // are we going up
          if (step > halfLife) { // is it faster to go the other way round?
            step -= bufFrames;   // calculate the new number of steps
            calculateCoefficients(step, j);
            // Fill the gap to zero
            fillGap(previousIndex - 1, -1, -1);
            // Fill the gap from the top
            fillGap(bufFrames - 1, index, -1);
          } else { // if not, just fill the gaps
            calculateCoefficients(step, j);
            fillGap(previousIndex + 1, index, 1);
          }
        } else {                  // if we are going down
          if (-step > halfLife) { // is it faster to go the other way round?
            step += bufFrames;    // calculate the new number of steps
            calculateCoefficients(step, j);
            // Fill the gap to the top
            fillGap(previousIndex + 1, bufFrames, 1);
            // Fill the gap from zero
            fillGap(0, index, 1);
          } else { // if not, just fill the gaps
            calculateCoefficients(step, j);
            fillGap(previousIndex - 1, index, -1);
          }
        }

        for (long chan = 0; chan < numChannels; ++chan)
          values[chan] = IN(chan + 4)[j]; // transfer the new previous value
      }
      previousIndex = index; // transfer the new previous address
    }
  };

  for (int j = 0; j < n; ++j) {
    double indexBuffer = *inputIndex++;
    processSample(indexBuffer, j);
  }

  unit->m_last_index = previousIndex;
  unit->l_nb_val = numberOfValues;
}

PluginLoad(IBufWrUGens) {
  ft = inTable;
  DefineDtorUnit(IBufWr);
}
