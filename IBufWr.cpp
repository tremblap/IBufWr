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
};

void IBufWr_Ctor(IBufWr *unit);
void IBufWr_Dtor(IBufWr *unit);
void IBufWr_next(IBufWr *unit, int inNumSamples);

void IBufWr_Ctor(IBufWr *unit) {
  //declares the unit buf num as unasigned (<0)
  unit->m_fbufnum = -1.f;

  //defines the counters and initialise them to 0
  unit->l_valeurs = (double *)RTAlloc(unit->mWorld, ((unit->mNumInputs - 4) * sizeof(double)));
  memset(unit->l_valeurs,0,((unit->mNumInputs - 4) * sizeof(double)));

  //initialise the other instance variables
  unit->l_index_precedent = -1;

  Print("nb of inputs %i\n", unit->mNumInputs);

  // defines the ugen in the tree
  SETCALC(IBufWr_next);

  // sends one sample of silence
  ClearUnitOutputs(unit, 1);
}

void IBufWr_Dtor(IBufWr *unit){
  RTFree(unit->mWorld, unit->l_valeurs);
}

void IBufWr_next(IBufWr *unit, int inNumSamples) {
  float *inind  = IN(1);
  bool interp = (bool)(IN0(2));
  double overdub = (double)(IN0(3));

  GET_BUF //this macro, defined in  SC_Unit.h, does all the sanity check, locks the buffer and assigns valutes to bufData, bufChannels, bufFrames
  uint32 numInputChannels = unit->mNumInputs - 4;// minus 4 because the arguments are all passed after the input array

  // other sanity check, mostly of size
  if (!checkBuffer(unit, bufData, bufChannels, numInputChannels, inNumSamples))
    return;

  // taken from ipoke~, with the following replacements
  // remove all the headers
  // replace frames by bufFrames in demivie
  // valeur temporarily become valeurs[0]

  double valeur_entree, valeur, index_tampon, coeff;
  long frames, nb_val, index, index_precedent, pas, i;
  bool dirty_flag = false;

  double demivie = (long)(bufFrames * 0.5);

  index_precedent = unit->l_index_precedent;
  valeur = unit->l_valeurs[0];
  nb_val = unit->l_nb_val;

  //temporary to check 1 input
  float *inval = IN(4);
  int n = inNumSamples;
  int nc = numInputChannels;
  int chan = 0;
  float *tab = bufData;
  frames = bufFrames;

  if (overdub != 0.)
  {
      if (interp)
      {
          while (n--)    // dsp loop with interpolation
          {
              valeur_entree = *inval++;
              index_tampon = *inind++;

              if (index_tampon < 0.0)                                            // if the writing is stopped
              {
                  if (index_precedent >= 0)                                    // and if it is the 1st one to be stopped
                  {
                      tab[index_precedent * nc + chan] = (tab[index_precedent * nc + chan] * overdub) + (valeur/nb_val);        // write the average value at the last given index
                      valeur = 0.0;
                      index_precedent = -1;
                      dirty_flag = true;
                  }
              }
              else
              {
                  index = wrap_index((long)(index_tampon + 0.5),frames);        // round the next index and make sure he is in the buffer's boundaries

                  if (index_precedent < 0)                                    // if it is the first index to write, resets the averaging and the values
                  {
                      index_precedent = index;
                      nb_val = 0;
                  }

                  if (index == index_precedent)                                // if the index has not moved, accumulate the value to average later.
                  {
                      valeur += valeur_entree;
                      nb_val += 1;
                  }
                  else                                                        // if it moves
                  {
                      if (nb_val != 1)                                        // is there more than one values to average
                      {
                          valeur = valeur/nb_val;                                // if yes, calculate the average
                          nb_val = 1;
                      }

                      tab[index_precedent * nc + chan] = (tab[index_precedent * nc + chan] * overdub) + valeur;                // write the average value at the last index
                      dirty_flag = true;

                      pas = index - index_precedent;                            // calculate the step to do

                      if (pas > 0)                                            // are we going up
                      {
                          if (pas > demivie)                                    // is it faster to go the other way round?
                          {
                              pas -= frames;                                    // calculate the new number of steps
                              coeff = (valeur_entree - valeur) / pas;            // calculate the interpolation coefficient

                              for(i=(index_precedent-1);i>=0;i--)                    // fill the gap to zero
                              {
                                  valeur -= coeff;
                                  tab[i * nc + chan] = (tab[i * nc + chan] * overdub) + valeur;
                              }
                              for(i=(frames-1);i>index;i--)                        // fill the gap from the top
                              {
                                  valeur -= coeff;
                                  tab[i * nc + chan] = (tab[i * nc + chan] * overdub) + valeur;
                              }
                          }
                          else                                                // if not, just fill the gaps
                          {
                              coeff = (valeur_entree - valeur) / pas;            // calculate the interpolation coefficient
                              for (i=(index_precedent+1); i<index; i++)
                              {
                                  valeur += coeff;
                                  tab[i * nc + chan] = (tab[i * nc + chan] * overdub) + valeur;
                              }
                          }
                      }
                      else                                                    // if we are going down
                      {
                          if ((-pas) > demivie)                                // is it faster to go the other way round?
                          {
                              pas += frames;                                    // calculate the new number of steps
                              coeff = (valeur_entree - valeur) / pas;            // calculate the interpolation coefficient

                              for(i=(index_precedent+1);i<frames;i++)            // fill the gap to the top
                              {
                                  valeur += coeff;
                                  tab[i * nc + chan] = (tab[i * nc + chan] * overdub) + valeur;
                              }
                              for(i=0;i<index;i++)                            // fill the gap from zero
                              {
                                  valeur += coeff;
                                  tab[i * nc + chan] = (tab[i * nc + chan] * overdub) + valeur;
                              }
                          }
                          else                                                // if not, just fill the gaps
                          {
                              coeff = (valeur_entree - valeur) / pas;            // calculate the interpolation coefficient
                              for (i=(index_precedent-1); i>index; i--)
                              {
                                  valeur -= coeff;
                                  tab[i * nc + chan] = (tab[i * nc + chan] * overdub) + valeur;
                              }
                          }
                      }

                      valeur = valeur_entree;                                    // transfer the new previous value
                  }
              }
              index_precedent = index;                                        // transfer the new previous address
          }
      }
      else
      {
          while (n--)    // dsp loop without interpolation
          {
              valeur_entree = *inval++;
              index_tampon = *inind++;

              if (index_tampon < 0.0)                                            // if the writing is stopped
              {
                  if (index_precedent >= 0)                                    // and if it is the 1st one to be stopped
                  {
                      tab[index_precedent * nc + chan] = (tab[index_precedent * nc + chan] * overdub) + (valeur/nb_val);        // write the average value at the last given index
                      valeur = 0.0;
                      index_precedent = -1;
                      dirty_flag = true;
                  }
              }
              else
              {
                  index = wrap_index((long)(index_tampon + 0.5),frames);            // round the next index and make sure he is in the buffer's boundaries

                  if (index_precedent < 0)                                    // if it is the first index to write, resets the averaging and the values
                  {
                      index_precedent = index;
                      nb_val = 0;
                  }

                  if (index == index_precedent)                                // if the index has not moved, accumulate the value to average later.
                  {
                      valeur += valeur_entree;
                      nb_val += 1;
                  }
                  else                                                        // if it moves
                  {
                      if (nb_val != 1)                                        // is there more than one values to average
                      {
                          valeur = valeur/nb_val;                                // if yes, calculate the average
                          nb_val = 1;
                      }

                      tab[index_precedent * nc + chan] = (tab[index_precedent * nc + chan] * overdub) + valeur;                // write the average value at the last index
                      dirty_flag = true;

                      pas = index - index_precedent;                            // calculate the step to do

                      if (pas > 0)                                            // are we going up
                      {
                          if (pas > demivie)                                    // is it faster to go the other way round?
                          {
                              for(i=(index_precedent-1);i>=0;i--)                // fill the gap to zero
                                  tab[i * nc + chan] = (tab[i * nc + chan] * overdub) + valeur;
                              for(i=(frames-1);i>index;i--)                    // fill the gap from the top
                                  tab[i * nc + chan] = (tab[i * nc + chan] * overdub) + valeur;
                          }
                          else                                                // if not, just fill the gaps
                          {
                              for (i=(index_precedent+1); i<index; i++)
                                  tab[i * nc + chan] = (tab[i * nc + chan] * overdub) + valeur;
                          }
                      }
                      else                                                    // if we are going down
                      {
                          if ((-pas) > demivie)                                // is it faster to go the other way round?
                          {
                              for(i=(index_precedent+1);i<frames;i++)            // fill the gap to the top
                                  tab[i * nc + chan] = (tab[i * nc + chan] * overdub) + valeur;
                              for(i=0;i<index;i++)                            // fill the gap from zero
                                  tab[i * nc + chan] = (tab[i * nc + chan] * overdub) + valeur;
                          }
                          else                                                // if not, just fill the gaps
                          {
                              for (i=(index_precedent-1); i>index; i--)
                                  tab[i * nc + chan] = (tab[i * nc + chan] * overdub) + valeur;
                          }
                      }

                      valeur = valeur_entree;                                    // transfer the new previous value
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
          while (n--)    // dsp loop with interpolation
          {
              valeur_entree = *inval++;
              index_tampon = *inind++;

              if (index_tampon < 0.0)                                            // if the writing is stopped
              {
                  if (index_precedent >= 0)                                    // and if it is the 1st one to be stopped
                  {
                      tab[index_precedent * nc + chan] = valeur/nb_val;        // write the average value at the last given index
                      valeur = 0.0;
                      index_precedent = -1;
                      dirty_flag = true;
                  }
              }
              else
              {
                  index = wrap_index((long)(index_tampon + 0.5),frames);        // round the next index and make sure he is in the buffer's boundaries

                  if (index_precedent < 0)                                    // if it is the first index to write, resets the averaging and the values
                  {
                      index_precedent = index;
                      nb_val = 0;
                  }

                  if (index == index_precedent)                                // if the index has not moved, accumulate the value to average later.
                  {
                      valeur += valeur_entree;
                      nb_val += 1;
                  }
                  else                                                        // if it moves
                  {
                      if (nb_val != 1)                                        // is there more than one values to average
                      {
                          valeur = valeur/nb_val;                                // if yes, calculate the average
                          nb_val = 1;
                      }

                      tab[index_precedent * nc + chan] = valeur;                // write the average value at the last index
                      dirty_flag = true;

                      pas = index - index_precedent;                            // calculate the step to do

                      if (pas > 0)                                            // are we going up
                      {
                          if (pas > demivie)                                    // is it faster to go the other way round?
                          {
                              pas -= frames;                                    // calculate the new number of steps
                              coeff = (valeur_entree - valeur) / pas;            // calculate the interpolation coefficient

                              for(i=(index_precedent-1);i>=0;i--)                    // fill the gap to zero
                              {
                                  valeur -= coeff;
                                  tab[i * nc + chan] = valeur;
                              }
                              for(i=(frames-1);i>index;i--)                        // fill the gap from the top
                              {
                                  valeur -= coeff;
                                  tab[i * nc + chan] = valeur;
                              }
                          }
                          else                                                // if not, just fill the gaps
                          {
                              coeff = (valeur_entree - valeur) / pas;            // calculate the interpolation coefficient
                              for (i=(index_precedent+1); i<index; i++)
                              {
                                  valeur += coeff;
                                  tab[i * nc + chan] = valeur;
                              }
                          }
                      }
                      else                                                    // if we are going down
                      {
                          if ((-pas) > demivie)                                // is it faster to go the other way round?
                          {
                              pas += frames;                                    // calculate the new number of steps
                              coeff = (valeur_entree - valeur) / pas;            // calculate the interpolation coefficient

                              for(i=(index_precedent+1);i<frames;i++)            // fill the gap to the top
                              {
                                  valeur += coeff;
                                  tab[i * nc + chan] = valeur;
                              }
                              for(i=0;i<index;i++)                            // fill the gap from zero
                              {
                                  valeur += coeff;
                                  tab[i * nc + chan] = valeur;
                              }
                          }
                          else                                                // if not, just fill the gaps
                          {
                              coeff = (valeur_entree - valeur) / pas;            // calculate the interpolation coefficient
                              for (i=(index_precedent-1); i>index; i--)
                              {
                                  valeur -= coeff;
                                  tab[i * nc + chan] = valeur;
                              }
                          }
                      }

                      valeur = valeur_entree;                                    // transfer the new previous value
                  }
              }
              index_precedent = index;                                        // transfer the new previous address
          }
      }
      else
      {
          while (n--)    // dsp loop without interpolation
          {
              valeur_entree = *inval++;
              index_tampon = *inind++;

              if (index_tampon < 0.0)                                            // if the writing is stopped
              {
                  if (index_precedent >= 0)                                    // and if it is the 1st one to be stopped
                  {
                      tab[index_precedent * nc + chan] = valeur/nb_val;        // write the average value at the last given index
                      valeur = 0.0;
                      index_precedent = -1;
                      dirty_flag = true;
                  }
              }
              else
              {
                  index = wrap_index((long)(index_tampon + 0.5),frames);            // round the next index and make sure he is in the buffer's boundaries

                  if (index_precedent < 0)                                    // if it is the first index to write, resets the averaging and the values
                  {
                      index_precedent = index;
                      nb_val = 0;
                  }

                  if (index == index_precedent)                                // if the index has not moved, accumulate the value to average later.
                  {
                      valeur += valeur_entree;
                      nb_val += 1;
                  }
                  else                                                        // if it moves
                  {
                      if (nb_val != 1)                                        // is there more than one values to average
                      {
                          valeur = valeur/nb_val;                                // if yes, calculate the average
                          nb_val = 1;
                      }

                      tab[index_precedent * nc + chan] = valeur;                // write the average value at the last index
                      dirty_flag = true;

                      pas = index - index_precedent;                            // calculate the step to do

                      if (pas > 0)                                            // are we going up
                      {
                          if (pas > demivie)                                    // is it faster to go the other way round?
                          {
                              for(i=(index_precedent-1);i>=0;i--)                // fill the gap to zero
                                  tab[i * nc + chan] = valeur;
                              for(i=(frames-1);i>index;i--)                    // fill the gap from the top
                                  tab[i * nc + chan] = valeur;
                          }
                          else                                                // if not, just fill the gaps
                          {
                              for (i=(index_precedent+1); i<index; i++)
                                  tab[i * nc + chan] = valeur;
                          }
                      }
                      else                                                    // if we are going down
                      {
                          if ((-pas) > demivie)                                // is it faster to go the other way round?
                          {
                              for(i=(index_precedent+1);i<frames;i++)            // fill the gap to the top
                                  tab[i * nc + chan] = valeur;
                              for(i=0;i<index;i++)                            // fill the gap from zero
                                  tab[i * nc + chan] = valeur;
                          }
                          else                                                // if not, just fill the gaps
                          {
                              for (i=(index_precedent-1); i>index; i--)
                                  tab[i * nc + chan] = valeur;
                          }
                      }

                      valeur = valeur_entree;                                    // transfer the new previous value
                  }
              }
              index_precedent = index;                                        // transfer the new previous address
          }
      }
  }

  unit->l_index_precedent = index_precedent;
  unit->l_valeurs[0] = valeur;
  unit->l_nb_val = nb_val;
}


PluginLoad(IButtUGens) {
  ft = inTable;
  DefineDtorUnit(IBufWr);
}
