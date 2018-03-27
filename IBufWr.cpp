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

inline long wrap_index(long index, long arrayLength)
{
    while(index >= arrayLength)
        index -= arrayLength;
    return index;
}


struct IBufWr : public Unit {
  float m_fbufnum;
  SndBuf *m_buf;

  long l_index_precedent;
  long l_nb_val;
  double *l_valeurs;
  double *l_coeffs;
};

void IBufWr_Ctor(IBufWr *unit);
void IBufWr_Dtor(IBufWr *unit);
void IBufWr_next(IBufWr *unit, int n);

void IBufWr_Ctor(IBufWr *unit) {
  //declares the unit buf num as unasigned (<0)
  unit->m_fbufnum = -1.f;

  //defines the counters and initialise them to 0
  unit->l_valeurs = (double *)RTAlloc(unit->mWorld, ((unit->mNumInputs - 4) * sizeof(double)));
  memset(unit->l_valeurs,0,((unit->mNumInputs - 4) * sizeof(double)));

  //defines the coeffs and initialise them to 0
  unit->l_coeffs = (double *)RTAlloc(unit->mWorld, ((unit->mNumInputs - 4) * sizeof(double)));
  memset(unit->l_coeffs,0,((unit->mNumInputs - 4) * sizeof(double)));

  //initialise the other instance variables
  unit->l_index_precedent = -1;

  // defines the ugen in the tree
  SETCALC(IBufWr_next);

  // sends one sample of silence
  ClearUnitOutputs(unit, 1);
}

void IBufWr_Dtor(IBufWr *unit){
  RTFree(unit->mWorld, unit->l_valeurs);
  RTFree(unit->mWorld, unit->l_coeffs);
}

void IBufWr_next(IBufWr *unit, int n) {
  float *inind  = IN(1);
  bool interp = (bool)(IN0(2));
  double overdub = (double)(IN0(3));
  float *inval;

  GET_BUF //this macro, defined in  SC_Unit.h, does all the sanity check, locks the buffer and assigns valutes to bufData, bufChannels, bufFrames
  long nc = unit->mNumInputs - 4;// minus 4 because the arguments are all passed after the input array

  // other sanity check, mostly of size
  if (!checkBuffer(unit, bufData, bufChannels, nc, n))
    return;

  // taken from ipoke~, with the following replacements
  // remove all the headers, remove dirty_flag
  // replace frames by bufFrames partout
  // change all valeur_entree with a loop to go through all chan = valeur[chan] += IN(chan+4)[j];
  // change while loops to iterate j
  // remove tab and change to bufData, iterating through inputs

  double *valeur, *coeff, index_tampon;
  long nb_val, index, index_precedent, pas, i, chan, j;

  double demivie = (long)(bufFrames * 0.5);

  index_precedent = unit->l_index_precedent;
  valeur = unit->l_valeurs;
  coeff = unit->l_coeffs;
  nb_val = unit->l_nb_val;

  if (overdub != 0.)
  {
      if (interp)
      {
          for (j = 0; j < n; j++)    // dsp loop with interpolation
          {
              index_tampon = *inind++;

              if (index_tampon < 0.0)                                            // if the writing is stopped
              {
                  if (index_precedent >= 0)                                    // and if it is the 1st one to be stopped
                  {
                    for(chan = 0; chan < nc;chan++)
                    {
                      bufData[index_precedent * bufChannels + chan] = (bufData[index_precedent * bufChannels + chan] * overdub) + (valeur[chan]/nb_val);        // write the average value at the last given index
                      valeur[chan] = 0.0;
                    }
                      index_precedent = -1;
                  }
              }
              else
              {
                  index = wrap_index((long)(index_tampon + 0.5),bufFrames);        // round the next index and make sure he is in the buffer's boundaries

                  if (index_precedent < 0)                                    // if it is the first index to write, resets the averaging and the values
                  {
                      index_precedent = index;
                      nb_val = 0;
                  }

                  if (index == index_precedent)                                // if the index has not moved, accumulate the value to average later.
                  {
                    for(chan = 0; chan < nc;chan++)
                      valeur[chan] += IN(chan+4)[j];
                    nb_val += 1;
                  }
                  else                                                        // if it moves
                  {
                      if (nb_val != 1)                                        // is there more than one values to average
                      {
                        for(chan = 0; chan < nc;chan++)
                          valeur[chan] = valeur[chan]/nb_val;                                // if yes, calculate the average
                          nb_val = 1;
                      }

                      for(chan = 0; chan < nc;chan++)
                        bufData[index_precedent * bufChannels + chan] = (bufData[index_precedent * bufChannels + chan] * overdub) + valeur[chan];// write the average value at the last index

                      pas = index - index_precedent;                            // calculate the step to do

                      if (pas > 0)                                            // are we going up
                      {
                          if (pas > demivie)                                    // is it faster to go the other way round?
                          {
                              pas -= bufFrames;                                    // calculate the new number of steps
                              for(chan = 0; chan < nc;chan++)
                                coeff[chan] = (IN(chan+4)[j] - valeur[chan]) / pas;            // calculate the interpolation coefficient

                              for(i=(index_precedent-1);i>=0;i--)                    // fill the gap to zero
                                for(chan = 0; chan < nc;chan++){
                                  valeur[chan] -= coeff[chan];
                                  bufData[i * bufChannels + chan] = (bufData[i * bufChannels + chan] * overdub) + valeur[chan];
                              }
                              for(i=(bufFrames-1);i>index;i--)                        // fill the gap from the top
                                for(chan = 0; chan < nc;chan++){
                                  valeur[chan] -= coeff[chan];
                                  bufData[i * bufChannels + chan] = (bufData[i * bufChannels + chan] * overdub) + valeur[chan];
                              }
                          }
                          else                                                // if not, just fill the gaps
                          {
                            for(chan = 0; chan < nc;chan++)
                              coeff[chan] = (IN(chan+4)[j] - valeur[chan]) / pas;            // calculate the interpolation coefficient

                              for (i=(index_precedent+1); i<index; i++)
                                for(chan = 0; chan < nc;chan++){
                                  valeur[chan] += coeff[chan];
                                  bufData[i * bufChannels + chan] = (bufData[i * bufChannels + chan] * overdub) + valeur[chan];
                              }
                          }
                      }
                      else                                                    // if we are going down
                      {
                          if ((-pas) > demivie)                                // is it faster to go the other way round?
                          {
                              pas += bufFrames;                                    // calculate the new number of steps
                              for(chan = 0; chan < nc;chan++)
                                coeff[chan] = (IN(chan+4)[j] - valeur[chan]) / pas;            // calculate the interpolation coefficient

                              for(i=(index_precedent+1);i<bufFrames;i++)            // fill the gap to the top
                                for(chan = 0; chan < nc;chan++){
                                valeur[chan] += coeff[chan];
                                bufData[i * bufChannels + chan] = (bufData[i * bufChannels + chan] * overdub) + valeur[chan];
                              }
                              for(i=0;i<index;i++)                            // fill the gap from zero
                                for(chan = 0; chan < nc;chan++){
                                valeur[chan] += coeff[chan];
                                bufData[i * bufChannels + chan] = (bufData[i * bufChannels + chan] * overdub) + valeur[chan];
                              }
                          }
                          else                                                // if not, just fill the gaps
                          {
                            for(chan = 0; chan < nc;chan++)
                              coeff[chan] = (IN(chan+4)[j] - valeur[chan]) / pas;            // calculate the interpolation coefficient

                              for (i=(index_precedent-1); i>index; i--)
                                for(chan = 0; chan < nc;chan++)
                              {
                                  valeur[chan] -= coeff[chan];
                                  bufData[i * bufChannels + chan] = (bufData[i * bufChannels + chan] * overdub) + valeur[chan];
                              }
                          }
                      }

                      for(chan = 0; chan < nc;chan++)
                        valeur[chan] = IN(chan+4)[j];                                    // transfer the new previous value
                  }
              }
              index_precedent = index;                                        // transfer the new previous address
          }
      }
      else
      {
          for (j = 0; j < n; j++)    // dsp loop without interpolation
          {
              index_tampon = *inind++;

              if (index_tampon < 0.0)                                            // if the writing is stopped
              {
                  if (index_precedent >= 0)                                    // and if it is the 1st one to be stopped
                  {
                    for(chan = 0; chan < nc;chan++)
                    {
                      bufData[index_precedent * bufChannels + chan] = (bufData[index_precedent * bufChannels + chan] * overdub) + (valeur[chan]/nb_val);        // write the average value at the last given index
                      valeur[chan] = 0.0;
                    }
                    index_precedent = -1;
                  }
              }
              else
              {
                  index = wrap_index((long)(index_tampon + 0.5),bufFrames);            // round the next index and make sure he is in the buffer's boundaries

                  if (index_precedent < 0)                                    // if it is the first index to write, resets the averaging and the values
                  {
                      index_precedent = index;
                      nb_val = 0;
                  }

                  if (index == index_precedent)                                // if the index has not moved, accumulate the value to average later.
                  {
                    for(chan = 0; chan < nc;chan++)
                      valeur[chan] += IN(chan+4)[j];
                    nb_val += 1;
                  }
                  else                                                        // if it moves
                  {
                      if (nb_val != 1)                                        // is there more than one values to average
                      {
                          for(chan = 0; chan < nc;chan++)
                            valeur[chan] = valeur[chan]/nb_val;                                // if yes, calculate the average
                          nb_val = 1;
                      }

                      for(chan = 0; chan < nc;chan++)
                        bufData[index_precedent * bufChannels + chan] = (bufData[index_precedent * bufChannels + chan] * overdub) + valeur[chan];                // write the average value at the last index

                      pas = index - index_precedent;                            // calculate the step to do

                      if (pas > 0)                                            // are we going up
                      {
                          if (pas > demivie)                                    // is it faster to go the other way round?
                          {
                              for(i=(index_precedent-1);i>=0;i--)                // fill the gap to zero
                                for(chan = 0; chan < nc;chan++)
                                  bufData[i * bufChannels + chan] = (bufData[i * bufChannels + chan] * overdub) + valeur[chan];
                              for(i=(bufFrames-1);i>index;i--)                    // fill the gap from the top
                                for(chan = 0; chan < nc;chan++)
                                  bufData[i * bufChannels + chan] = (bufData[i * bufChannels + chan] * overdub) + valeur[chan];
                          }
                          else                                                // if not, just fill the gaps
                          {
                              for (i=(index_precedent+1); i<index; i++)
                                for(chan = 0; chan < nc;chan++)
                                  bufData[i * bufChannels + chan] = (bufData[i * bufChannels + chan] * overdub) + valeur[chan];
                          }
                      }
                      else                                                    // if we are going down
                      {
                          if ((-pas) > demivie)                                // is it faster to go the other way round?
                          {
                              for(i=(index_precedent+1);i<bufFrames;i++)            // fill the gap to the top
                                for(chan = 0; chan < nc;chan++)
                                  bufData[i * bufChannels + chan] = (bufData[i * bufChannels + chan] * overdub) + valeur[chan];
                              for(i=0;i<index;i++)                            // fill the gap from zero
                                for(chan = 0; chan < nc;chan++)
                                  bufData[i * bufChannels + chan] = (bufData[i * bufChannels + chan] * overdub) + valeur[chan];
                          }
                          else                                                // if not, just fill the gaps
                          {
                              for (i=(index_precedent-1); i>index; i--)
                                for(chan = 0; chan < nc;chan++)
                                  bufData[i * bufChannels + chan] = (bufData[i * bufChannels + chan] * overdub) + valeur[chan];
                          }
                      }
                      for(chan = 0; chan < nc;chan++)
                        valeur[chan] = IN(chan+4)[j];                            // transfer the new previous value
                  }
              }
              index_precedent = index;                                        // transfer the new previous address
          }
      }
  }
  else
  {
      if (interp)
      {
          for (j = 0; j < n; j++)    // dsp loop with interpolation
          {
              index_tampon = *inind++;

              if (index_tampon < 0.0)                                            // if the writing is stopped
              {
                  if (index_precedent >= 0)                                    // and if it is the 1st one to be stopped
                  {
                    for(chan = 0; chan < nc;chan++)
                    {
                      bufData[index_precedent * bufChannels + chan] = valeur[chan]/nb_val;        // write the average value at the last given index
                      valeur[chan] = 0.0;
                    }
                    index_precedent = -1;
                  }
              }
              else
              {
                  index = wrap_index((long)(index_tampon + 0.5),bufFrames);        // round the next index and make sure he is in the buffer's boundaries

                  if (index_precedent < 0)                                    // if it is the first index to write, resets the averaging and the values
                  {
                      index_precedent = index;
                      nb_val = 0;
                  }

                  if (index == index_precedent)                                // if the index has not moved, accumulate the value to average later.
                  {
                      for(chan = 0; chan < nc;chan++)
                        valeur[chan] += IN(chan+4)[j];
                      nb_val += 1;
                  }
                  else                                                        // if it moves
                  {
                      if (nb_val != 1)                                        // is there more than one values to average
                      {
                        for(chan = 0; chan < nc;chan++)
                          valeur[chan] = valeur[chan]/nb_val;                                 // if yes, calculate the average
                        nb_val = 1;
                      }

                      for(chan = 0; chan < nc;chan++)
                        bufData[index_precedent * bufChannels + chan] = valeur[chan];   // write the average value at the last index

                      pas = index - index_precedent;                            // calculate the step to do

                      if (pas > 0)                                            // are we going up
                      {
                          if (pas > demivie)                                    // is it faster to go the other way round?
                          {
                              pas -= bufFrames;                                    // calculate the new number of steps
                              for(chan = 0; chan < nc;chan++)
                                coeff[chan] = (IN(chan+4)[j] - valeur[chan]) / pas;// calculate the interpolation coefficient

                              for(i=(index_precedent-1);i>=0;i--)                    // fill the gap to zero
                                for(chan = 0; chan < nc;chan++){
                                  valeur[chan] -= coeff[chan];
                                  bufData[i * bufChannels + chan] = valeur[chan];
                                }

                              for(i=(bufFrames-1);i>index;i--)                        // fill the gap from the top
                                for(chan = 0; chan < nc;chan++){
                                  valeur[chan] -= coeff[chan];
                                  bufData[i * bufChannels + chan] = valeur[chan];
                                }
                          }
                          else                                                // if not, just fill the gaps
                          {
                            for(chan = 0; chan < nc;chan++)
                              coeff[chan] = (IN(chan+4)[j] - valeur[chan]) / pas;            // calculate the interpolation coefficient

                            for (i=(index_precedent+1); i<index; i++)
                              for(chan = 0; chan < nc;chan++){
                                valeur[chan] += coeff[chan];
                                bufData[i * bufChannels + chan] = valeur[chan];
                              }

                          }
                      }
                      else                                                    // if we are going down
                      {
                          if ((-pas) > demivie)                                // is it faster to go the other way round?
                          {
                              pas += bufFrames;                                    // calculate the new number of steps
                              for(chan = 0; chan < nc;chan++)
                                coeff[chan] = (IN(chan+4)[j] - valeur[chan]) / pas;            // calculate the interpolation coefficient

                              for(i=(index_precedent+1);i<bufFrames;i++)            // fill the gap to the top
                                for(chan = 0; chan < nc;chan++){
                                  valeur[chan] += coeff[chan];
                                  bufData[i * bufChannels + chan] = valeur[chan];
                                }

                              for(i=0;i<index;i++)                            // fill the gap from zero
                                for(chan = 0; chan < nc;chan++){
                                  valeur[chan] += coeff[chan];
                                  bufData[i * bufChannels + chan] = valeur[chan];
                                }
                          }
                          else                                                // if not, just fill the gaps
                          {
                              for(chan = 0; chan < nc;chan++)
                                coeff[chan] = (IN(chan+4)[j] - valeur[chan]) / pas;            // calculate the interpolation coefficient
                              for (i=(index_precedent-1); i>index; i--)
                                for(chan = 0; chan < nc;chan++){
                                  valeur[chan] -= coeff[chan];
                                  bufData[i * bufChannels + chan] = valeur[chan];
                                }

                          }
                      }

                      for(chan = 0; chan < nc;chan++)
                        valeur[chan] = IN(chan+4)[j];                         // transfer the new previous value
                  }
              }
              index_precedent = index;                                        // transfer the new previous address
          }
      }
      else
      {
          for (j = 0; j < n; j++)    // dsp loop without interpolation
          {
              index_tampon = *inind++;

              if (index_tampon < 0.0)                                            // if the writing is stopped
              {
                  if (index_precedent >= 0)                                    // and if it is the 1st one to be stopped
                  {
                      for(chan = 0; chan < nc;chan++)
                      {
                        bufData[index_precedent * bufChannels + chan] = valeur[chan]/nb_val;        // write the average value at the last given index
                        valeur[chan] = 0.0;
                      }
                      index_precedent = -1;
                  }
              }
              else
              {
                  index = wrap_index((long)(index_tampon + 0.5),bufFrames);            // round the next index and make sure he is in the buffer's boundaries

                  if (index_precedent < 0)                                    // if it is the first index to write, resets the averaging and the values
                  {
                      index_precedent = index;
                      nb_val = 0;
                  }

                  if (index == index_precedent)                                // if the index has not moved, accumulate the value to average later.
                  {
                      for(chan = 0; chan < nc;chan++)
                        valeur[chan] += IN(chan+4)[j];
                      nb_val += 1;
                  }
                  else                                                        // if it moves
                  {
                      if (nb_val != 1)                                        // is there more than one values to average
                      {
                        for(chan = 0; chan < nc;chan++)
                          valeur[chan] = valeur[chan]/nb_val;                                // if yes, calculate the average
                        nb_val = 1;
                      }

                      for(chan = 0; chan < nc;chan++)
                        bufData[index_precedent * bufChannels + chan] = valeur[chan];   // write the average value at the last index

                      pas = index - index_precedent;                            // calculate the step to do

                      if (pas > 0)                                            // are we going up
                      {
                          if (pas > demivie)                                    // is it faster to go the other way round?
                          {
                              for(i=(index_precedent-1);i>=0;i--)                // fill the gap to zero
                                for(chan = 0; chan < nc;chan++)
                                  bufData[i * bufChannels + chan] = valeur[chan];
                              for(i=(bufFrames-1);i>index;i--)                    // fill the gap from the top
                                for(chan = 0; chan < nc;chan++)
                                  bufData[i * bufChannels + chan] = valeur[chan];
                          }
                          else                                                // if not, just fill the gaps
                          {
                              for (i=(index_precedent+1); i<index; i++)
                                for(chan = 0; chan < nc;chan++)
                                  bufData[i * bufChannels + chan] = valeur[chan];
                          }
                      }
                      else                                                    // if we are going down
                      {
                          if ((-pas) > demivie)                                // is it faster to go the other way round?
                          {
                              for(i=(index_precedent+1);i<bufFrames;i++)            // fill the gap to the top
                                for(chan = 0; chan < nc;chan++)
                                  bufData[i * bufChannels + chan] = valeur[chan];
                              for(i=0;i<index;i++)                            // fill the gap from zero
                                for(chan = 0; chan < nc;chan++)
                                  bufData[i * bufChannels + chan] = valeur[chan];
                          }
                          else                                                // if not, just fill the gaps
                          {
                              for (i=(index_precedent-1); i>index; i--)
                                for(chan = 0; chan < nc;chan++)
                                  bufData[i * bufChannels + chan] = valeur[chan];
                          }
                      }

                      for(chan = 0; chan < nc;chan++)
                        valeur[chan] = IN(chan+4)[j];                        // transfer the new previous value
                  }
              }
              index_precedent = index;                                        // transfer the new previous address
          }
      }
  }

  unit->l_index_precedent = index_precedent;
  unit->l_nb_val = nb_val;
}


PluginLoad(IButtUGens) {
  ft = inTable;
  DefineDtorUnit(IBufWr);
}
