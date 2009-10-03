//=========================================================================
//
//  H   H    A    RRRR   M   M   OOO   N   N  Y   Y
//  H   H   A A   R   R  MM MM  O   O  NN  N   Y Y
//  HHHHH  AAAAA  RRRR   M M M  O   O  N N N    Y   "Harmony Cart software"
//  H   H  A   A  R R    M   M  O   O  N  NN    Y
//  H   H  A   A  R  R   M   M   OOO   N   N    Y
//
// Copyright (c) 2009 by Stephen Anthony <stephena@users.sourceforge.net>
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

#include "bspf.hxx"

enum BSType
{
  BS_0840,
  BS_3E,
  BS_3F,
  BS_4K,
  BS_AR,
  BS_CV,
  BS_DPC,
  BS_E0,
  BS_E7,
  BS_EF,
  BS_EFSC,
  BS_F0,
  BS_F4,
  BS_F6,
  BS_F8,
  BS_F4SC,
  BS_F6SC,
  BS_F8SC,
  BS_FA,
  BS_FE,
  BS_UA,
  BS_4A50,
  BS_X07,
  BS_SB,
  BS_MC,
  BS_NONE,
  BS_AUTO
};

class Bankswitch
{
  public:
    static string typeToName(BSType type)
    {
      switch(type)
      {
        case BS_4K:   return "4K";
        case BS_F8:   return "F8";
        case BS_F6:   return "F6";
        case BS_F4:   return "F4";
        case BS_FA:   return "FA";
        case BS_3F:   return "3F";
        case BS_F8SC: return "F8SC";
        case BS_F6SC: return "F6SC";
        case BS_F4SC: return "F4SC";
        case BS_EF:   return "EF";
        case BS_CV:   return "CV";
        case BS_3E:   return "3E";
        case BS_UA:   return "UA";
        case BS_F0:   return "MB";
        case BS_E0:   return "E0";
        case BS_E7:   return "E7";
        case BS_FE:   return "FE";
        case BS_AR:   return "AR";
        case BS_EFSC: return "EFSC";
        case BS_0840: return "0840";
        case BS_DPC:  return "DPC";
        case BS_4A50: return "4A50";
        case BS_X07:  return "X07";
        case BS_SB:   return "SB";
        case BS_MC:   return "MC";
        case BS_NONE: return "NONE/UNKNOWN";
        case BS_AUTO: return "AUTO";
      }
      return "NONE";
    }

    static BSType nameToType(const string& name)
    {
      string s = BSPF_tolower(name);

      if(s == "4k") 	    return BS_4K;
      else if(s == "f8")    return  BS_F8;
      else if(s == "f6")    return  BS_F6;
      else if(s == "f4")    return  BS_F4;
      else if(s == "fasc")  return  BS_FA;
      else if(s == "3f")    return  BS_3F;
      else if(s == "f8sc")  return  BS_F8SC;
      else if(s == "f6sc")  return  BS_F6SC;
      else if(s == "f4sc")  return  BS_F4SC;
      else if(s == "ef")    return  BS_EF;
      else if(s == "cv")    return  BS_CV;
      else if(s == "3e")    return  BS_3E;
      else if(s == "ua")    return  BS_UA;
      else if(s == "mb")    return  BS_F0;
      else if(s == "e0")    return  BS_E0;
      else if(s == "e7")    return  BS_E7;
      else if(s == "fe")    return  BS_FE;
      else if(s == "ar")    return  BS_AR;
      else if(s == "efsc")  return  BS_EFSC;
      else if(s == "0840")  return  BS_0840;
      else if(s == "dpc")   return  BS_DPC;
      else if(s == "4a50")  return  BS_4A50;
      else if(s == "x07")   return  BS_X07;
      else if(s == "sb")    return  BS_SB;
      else if(s == "mc")    return  BS_MC;
      else if(s == "auto")  return  BS_AUTO;
      else                  return  BS_NONE;
    }
};

#endif
