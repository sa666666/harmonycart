//=========================================================================
//
//  H   H    A    RRRR   M   M   OOO   N   N  Y   Y
//  H   H   A A   R   R  MM MM  O   O  NN  N   Y Y
//  HHHHH  AAAAA  RRRR   M M M  O   O  N N N    Y   "Harmony Cart software"
//  H   H  A   A  R R    M   M  O   O  N  NN    Y
//  H   H  A   A  R  R   M   M   OOO   N   N    Y
//
// Copyright (c) 2009-2017 by Stephen Anthony <sa666666@gmail.com>
//
// See the file "License.txt" for information on usage and redistribution
// of this file, and for a DISCLAIMER OF ALL WARRANTIES.
//=========================================================================

#ifndef __SERIALPORT_UNIX_HXX
#define __SERIALPORT_UNIX_HXX

#include <termios.h>

#include "SerialPort.hxx"

/**
  Implement reading and writing from a serial port under UNIX.

  @author  Stephen Anthony
*/
class SerialPortUNIX : public SerialPort
{
  public:
    SerialPortUNIX();
    virtual ~SerialPortUNIX();

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
      @return  The number of bytes read
    */
    uInt32 receiveBlock(void* answer, uInt32 max_size);

    /**
      Write block of bytes to the serial port.

      @param data  The byte(s) to write to the port
      @param size  The size of the block
      @return  The number of bytes written
    */
    uInt32 sendBlock(const void* data, uInt32 size);

    /**
      Sets (or resets) the timeout to the timout period requested.  Starts
      counting to this period.  This timeout support is a little odd in that
      the timeout specifies the accumulated deadtime waiting to read not the
      total time waiting to read. They should be close enough to the same for
      this use. Used by the serial input routines, the actual counting takes
      place in receiveBlock.

      @param timeout_milliseconds  The time in milliseconds to use for timeout
    */
    void setTimeout(uInt32 timeout_milliseconds);

    /**
      Empty the serial port buffers.  Cleans things to a known state.
    */
    void clearBuffers();

    /**
      Controls the modem lines to place the microcontroller into various
      states during the programming process.

      @param DTR  The state to set the DTR line to
      @param RTS  The state to set the RTS line to
    */
    void controlModemLines(bool DTR, bool RTS);

    /**
      Sleep the specified amount of time (in milliseconds).
    */
    void sleepMillis(uInt32 milliseconds);

    /**
      Get all valid serial ports detected on this system.
    */
    const StringList& getPortNames();

  private:
    // File descriptor for serial connection
    int myHandle;

    struct termios myOldtio, myNewtio;

  private:
    // Following constructors and assignment operators not supported
    SerialPortUNIX(const SerialPortUNIX&) = delete;
    SerialPortUNIX(SerialPortUNIX&&) = delete;
    SerialPortUNIX& operator=(const SerialPortUNIX&) = delete;
    SerialPortUNIX& operator=(SerialPortUNIX&&) = delete;
};

#endif
