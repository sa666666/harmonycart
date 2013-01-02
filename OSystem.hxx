//=========================================================================
//
//  H   H    A    RRRR   M   M   OOO   N   N  Y   Y
//  H   H   A A   R   R  MM MM  O   O  NN  N   Y Y
//  HHHHH  AAAAA  RRRR   M M M  O   O  N N N    Y   "Harmony Cart software"
//  H   H  A   A  R R    M   M  O   O  N  NN    Y
//  H   H  A   A  R  R   M   M   OOO   N   N    Y
//
// Copyright (c) 2009-2013 by Stephen Anthony <stephena@users.sf.net>
//
// See the file "License.txt" for information on usage and redistribution
// of this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//=========================================================================

#ifndef __OSYSTEM_HXX
#define __OSYSTEM_HXX

#include <QDir>
#include <QString>
#include "bspf_harmony.hxx"

/**
  This class provides an interface for accessing operating system specific
  functions.

  @author  Stephen Anthony
*/
class OSystem
{
  public:
    OSystem() : myARMPath("") { }
    virtual ~OSystem() { }

  public:
    /**
      Gives the full pathname to the ARM directory.
      This method should be overloaded for most systems, since it will
      vary greatly from system to system.

      @return  Default ARM dir
    */
    const QString& defaultARMPath() const { return myARMPath; }

  protected:
    QString myARMPath;
};

#endif
