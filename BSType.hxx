//=========================================================================
//
//  H   H    A    RRRR   M   M   OOO   N   N  Y   Y
//  H   H   A A   R   R  MM MM  O   O  NN  N   Y Y
//  HHHHH  AAAAA  RRRR   M M M  O   O  N N N    Y   "Harmony Cart software"
//  H   H  A   A  R R    M   M  O   O  N  NN    Y
//  H   H  A   A  R  R   M   M   OOO   N   N    Y
//
// Copyright (c) 2009-2015 by Stephen Anthony <stephena@users.sf.net>
//
// See the file "License.txt" for information on usage and redistribution
// of this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//=========================================================================

/**
  Bankswitching types as defined by Armin (Krokodile Cart designer).

  @author  Stephen Anthony
*/

#ifndef BSTYPE_HXX
#define BSTYPE_HXX

#include "bspf_harmony.hxx"

enum BSType {
  BS_0840, BS_2IN1, BS_4IN1, BS_8IN1, BS_16IN1, BS_32IN1, BS_2K,
  BS_3E,   BS_3F,   BS_4A50, BS_4K,   BS_AR,    BS_CV,    BS_DPC,
  BS_DPCP, BS_E0,   BS_E7,   BS_EF,   BS_EFSC,  BS_F0,    BS_F4,
  BS_F4SC, BS_F6,   BS_F6SC, BS_F8,   BS_F8SC,  BS_FA,    BS_FE,
  BS_MC,   BS_SB,   BS_UA,   BS_X07,  BS_CTY,   BS_FA2,   BS_CUSTOM,

  BS_AUTO
};

class Bankswitch
{
  public:
    static string typeToName(BSType type)
    {
      switch(type)
      {
        case BS_2K:
        case BS_4K:    return "4K";
        case BS_0840:  return "0840";
        case BS_2IN1:  return "2IN1";
        case BS_4IN1:  return "4IN1";
        case BS_8IN1:  return "8IN1";
        case BS_16IN1: return "16IN1";
        case BS_32IN1: return "32IN1";
        case BS_3E:    return "3E";
        case BS_3F:    return "3F";
        case BS_4A50:  return "4A50";
        case BS_AR:    return "AR";
        case BS_CV:    return "CV";
        case BS_CTY:   return "CTY";
        case BS_DPC:   return "DPC";
        case BS_DPCP:  return "DPC+";
        case BS_E0:    return "E0";
        case BS_E7:    return "E7";
        case BS_EF:    return "EF";
        case BS_EFSC:  return "EFSC";
        case BS_F0:    return "F0";
        case BS_F4:    return "F4";
        case BS_F4SC:  return "F4SC";
        case BS_F6:    return "F6";
        case BS_F6SC:  return "F6SC";
        case BS_F8:    return "F8";
        case BS_F8SC:  return "F8SC";
        case BS_FA:    return "FA";
        case BS_FA2:   return "FA2";
        case BS_FE:    return "FE";
        case BS_MC:    return "MC";
        case BS_SB:    return "SB";
        case BS_UA:    return "UA";
        case BS_X07:   return "X07";
        case BS_CUSTOM:return "CUSTOM";
        case BS_AUTO:  return "AUTO";
      }
      return "NONE";
    }

    static BSType nameToType(const string& name)
    {
      if(BSPF_equalsIgnoreCase(name, "2K") ||
         BSPF_equalsIgnoreCase(name, "4K"))           return BS_4K;
      else if(BSPF_equalsIgnoreCase(name, "084") ||
              BSPF_equalsIgnoreCase(name, "0840"))    return BS_0840;
      else if(BSPF_equalsIgnoreCase(name, "2IN1"))    return BS_2IN1;
      else if(BSPF_equalsIgnoreCase(name, "4IN1"))    return BS_4IN1;
      else if(BSPF_equalsIgnoreCase(name, "8IN1"))    return BS_8IN1;
      else if(BSPF_equalsIgnoreCase(name, "16IN1"))   return BS_16IN1;
      else if(BSPF_equalsIgnoreCase(name, "32IN1"))   return BS_32IN1;
      else if(BSPF_equalsIgnoreCase(name, "3E"))      return BS_3E;
      else if(BSPF_equalsIgnoreCase(name, "3F"))      return BS_3F;
      else if(BSPF_equalsIgnoreCase(name, "4A50"))    return BS_4A50;
      else if(BSPF_equalsIgnoreCase(name, "AR"))      return BS_AR;
      else if(BSPF_equalsIgnoreCase(name, "CV"))      return BS_CV;
      else if(BSPF_equalsIgnoreCase(name, "CTY"))     return BS_CTY;
      else if(BSPF_equalsIgnoreCase(name, "DPC"))     return BS_DPC;
      else if(BSPF_equalsIgnoreCase(name, "DPC+"))    return BS_DPCP;
      else if(BSPF_equalsIgnoreCase(name, "E0"))      return BS_E0;
      else if(BSPF_equalsIgnoreCase(name, "E7"))      return BS_E7;
      else if(BSPF_equalsIgnoreCase(name, "EF"))      return BS_EF;
      else if(BSPF_equalsIgnoreCase(name, "EFSC"))    return BS_EFSC;
      else if(BSPF_equalsIgnoreCase(name, "F0"))      return BS_F0;
      else if(BSPF_equalsIgnoreCase(name, "F4"))      return BS_F4;
      else if(BSPF_equalsIgnoreCase(name, "F4S") ||
              BSPF_equalsIgnoreCase(name, "F4SC"))    return BS_F4SC;
      else if(BSPF_equalsIgnoreCase(name, "F6"))      return BS_F6;
      else if(BSPF_equalsIgnoreCase(name, "F6S") ||
              BSPF_equalsIgnoreCase(name, "F6SC"))    return BS_F6SC;
      else if(BSPF_equalsIgnoreCase(name, "F8"))      return BS_F8;
      else if(BSPF_equalsIgnoreCase(name, "F8S") ||
              BSPF_equalsIgnoreCase(name, "F8SC"))    return BS_F8SC;
      else if(BSPF_equalsIgnoreCase(name, "FA"))      return BS_FA;
      else if(BSPF_equalsIgnoreCase(name, "FA2"))     return BS_FA2;
      else if(BSPF_equalsIgnoreCase(name, "FE"))      return BS_FE;
      else if(BSPF_equalsIgnoreCase(name, "MC"))      return BS_MC;
      else if(BSPF_equalsIgnoreCase(name, "SB"))      return BS_SB;
      else if(BSPF_equalsIgnoreCase(name, "UA"))      return BS_UA;
      else if(BSPF_equalsIgnoreCase(name, "X07"))     return BS_X07;
      else if(BSPF_equalsIgnoreCase(name, "CU") ||
              BSPF_equalsIgnoreCase(name, "CUSTOM"))  return BS_CUSTOM;
      else                                            return BS_AUTO;
    }
};

#endif
