//=========================================================================
//
//  H   H    A    RRRR   M   M   OOO   N   N  Y   Y
//  H   H   A A   R   R  MM MM  O   O  NN  N   Y Y
//  HHHHH  AAAAA  RRRR   M M M  O   O  N N N    Y   "Harmony Cart software"
//  H   H  A   A  R R    M   M  O   O  N  NN    Y
//  H   H  A   A  R  R   M   M   OOO   N   N    Y
//
// Copyright (c) 2009-2010 by Stephen Anthony <stephena@users.sf.net>
//
// See the file "License.txt" for information on usage and redistribution
// of this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//=========================================================================

/**
  The bspf.hxx file comes from Stella, and was originally written by
  Brad Mott.  HarmonyCart needs to add stuff to it, so we wrap access and
  use 'bspf_harmony.hxx' instead.

  This makes it easier to integrate new changes from Stella, since
  bspf.hxx is just dropped in here and doesn't have to be modified.

  @author  Stephen Anthony
*/

#ifndef BSPF_HARMONY_HXX
#define BSPF_HARMONY_HXX

#include <vector>

/** What system are we using? */
#if defined(WIN32) || defined(_WIN32)
  #define BSPF_WIN32
#elif defined(__APPLE__) || defined(MAC_OS_X)
  #define BSPF_MAC_OSX
#else
  #define BSPF_UNIX
#endif
#include "bspf.hxx"

typedef vector<string> StringList;

#endif // BSPF_HARMONY
