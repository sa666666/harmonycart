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

#include "bspf.hxx"

#if defined(BSPF_WIN32)

#ifndef __SERIALPORT_WIN32_HXX
#define __SERIALPORT_WIN32_HXX

#include <windows.h>
#include "SerialPort.hxx"

/**
  Implement reading and writing from a serial port under Windows systems.

  @author  Stephen Anthony
  @version $Id$
*/
class SerialPortWin32 : public SerialPort
{
  public:
    SerialPortWin32();
    virtual ~SerialPortWin32();

    /**
      Open the given serial port with the specified attributes.

      @param device  The name of the port
      @return  False on any errors, else true
    */
    bool openPort(const string& device);

    /**
      Close a previously opened serial port.
    */
    void closePort();

    /**
      Answers if the port is currently open and ready for I/O.

      @return  True if open and ready, else false
    */
    bool isOpen();

    /**
      Receives a buffer from the open com port. Returns all the characters
      ready (waits for up to 'n' milliseconds before accepting that no more
      characters are ready) or when the buffer is full. 'n' is system dependent,
      see SerialTimeout routines.

      @param answer    Buffer to hold the bytes read from the serial port
      @param max_size  The size of buffer pointed to by answer
      @param real_size Pointer to a long that returns the amount of the
                       buffer that is actually used
      @return  The number of bytes read (-1 indicates error)
    */
    int ReceiveComPortBlock(void* answer, uInt32 max_size, uInt32* real_size);

    /**
      Write block of bytes to the serial port.

      @param data  The byte(s) to write to the port
      @param size  The size of the block
      @return  The number of bytes written (-1 indicates error)
    */
    int SendComPortBlock(const void* data, uInt32 size);

    /**
      Sets (or resets) the timeout to the timout period requested.  Starts
      counting to this period.  This timeout support is a little odd in that
      the timeout specifies the accumulated deadtime waiting to read not the
      total time waiting to read. They should be close enough to the same for
      this use. Used by the serial input routines, the actual counting takes
      place in ReceiveComPortBlock.

      @param timeout_milliseconds  The time in milliseconds to use for timeout
    */
    void SerialTimeoutSet(uInt32 timeout_milliseconds);

    /**
      Empty the serial port buffers.  Cleans things to a known state.
    */
    void ClearSerialPortBuffers();

    /**
      Controls the modem lines to place the microcontroller into various
      states during the programming process.

      @param DTR  The state to set the DTR line to
      @param RTS  The state to set the RTS line to
    */
    void ControlModemLines(bool DTR, bool RTS);

    /**
      Get all valid serial ports detected on this system.
    */
    const StringList& getPortNames();

  private:
    // Handle to serial port
    HANDLE myHandle;
};

#endif

#endif // BSPF_WIN32
