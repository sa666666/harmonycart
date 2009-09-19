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
  BS_4K   = 0,
  BS_F8   = 1,
  BS_F6   = 2,
  BS_F4   = 3,
  BS_MCF6 = 4,    // multicart not supported yet
  BS_FA   = 5,
  BS_3F   = 6,
  BS_MCF8 = 7,    // multicart not supported yet
  BS_F8SC = 8,
  BS_F6SC = 9,
  BS_F4SC = 10,
  BS_MCF4 = 11,   // multicart not supported yet
  BS_MC4K = 12,   // multicart not supported yet
  BS_EF   = 13,
  BS_CV   = 14,
  BS_3E   = 15,
  BS_UA   = 16,
  BS_F0   = 17,   // not supported at all
  BS_E0   = 18,   // not supported at all
  BS_E7   = 19,
  BS_FE   = 20,   // not supported at all
  BS_AR   = 21,   // not supported at all
  BS_EFSC = 22,
  BS_0840 = 23,
  BS_NONE
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
        case BS_MCF6: return "MCF6";
        case BS_FA:   return "FASC";
        case BS_3F:   return "3F";
        case BS_MCF8: return "MCF8";
        case BS_F8SC: return "F8SC";
        case BS_F6SC: return "F6SC";
        case BS_F4SC: return "F4SC";
        case BS_MCF4: return "MCF4";
        case BS_MC4K: return "MC4K";
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
        case BS_NONE: return "NONE";
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
      else if(s == "mcf6")  return  BS_MCF6;
      else if(s == "fasc")  return  BS_FA;
      else if(s == "3f")    return  BS_3F;
      else if(s == "mcf8")  return  BS_MCF8;
      else if(s == "f8sc")  return  BS_F8SC;
      else if(s == "f6sc")  return  BS_F6SC;
      else if(s == "f4sc")  return  BS_F4SC;
      else if(s == "mcf4")  return  BS_MCF4;
      else if(s == "mc4k")  return  BS_MC4K;
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
      else                  return  BS_NONE;
    }
};

#endif
