//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2024 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef FS_NODE_POSIX_HXX
#define FS_NODE_POSIX_HXX

#include "FSNode.hxx"

#ifdef BSPF_MACOS
  #include <sys/types.h>
#endif

#include <sys/param.h>
#include <sys/stat.h>
#include <dirent.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

#ifndef MAXPATHLEN // No MAXPATHLEN, as happens on Hurd
  #define MAXPATHLEN 1024
#endif

/*
 * Implementation of the Stella file system API based on POSIX (for Linux
 * and macOS)
 *
 * Parts of this class are documented in the base interface class,
 * AbstractFSNode.
 */
class FSNodePOSIX : public AbstractFSNode
{
  public:
    /**
     * Creates a FSNodePOSIX with the root node as path.
     */
    FSNodePOSIX();

    /**
     * Creates a FSNodePOSIX for a given path.
     *
     * @param path    String with the path the new node should point to.
     * @param verify  true if the isValid and isDirectory/isFile flags should
     *                be verified during the construction.
     */
    explicit FSNodePOSIX(string_view path, bool verify = true);

    bool exists() const override { return access(_path.c_str(), F_OK) == 0; }
    const string& getName() const override  { return _displayName; }
    void setName(string_view name) override { _displayName = name; }
    const string& getPath() const override { return _path; }
    string getShortPath() const override;
    bool isDirectory() const override { return _isDirectory; }
    bool isFile() const override      { return _isFile;      }
    bool isReadable() const override  { return access(_path.c_str(), R_OK) == 0; }
    bool isWritable() const override  { return access(_path.c_str(), W_OK) == 0; }
    bool makeDir() override;
    bool rename(string_view newfile) override;

    size_t getSize() const override;
    bool hasParent() const override;
    AbstractFSNodePtr getParent() const override;
    bool getChildren(AbstractFSList& list, ListMode mode) const override;

  private:
    /**
     * Set the _isDirectory/_isFile/_size flags using stat().
     *
     * @return  Success/failure of stat() function
     */
    bool setFlags();

  private:
    string _path, _displayName;
    bool _isFile{false}, _isDirectory{true};
    mutable size_t _size{0};

    static const char* const ourHomeDir;
    static std::array<char, MAXPATHLEN> ourBuf;
};

#endif
