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

#ifndef __SERIALPORT_HXX
#define __SERIALPORT_HXX

#include "bspf.hxx"

#include <vector>
typedef vector<string> StringList;

/**
  This class provides an interface for a standard serial port.
  For now, this is used when connecting a Krokodile Cart,
  and as such it always uses 115200, 8n1, no flow control.

  @author  Stephen Anthony
*/
class SerialPort
{
  public:
    SerialPort() : myBaud(9600), mySerialTimeoutCount(0) { }
    virtual ~SerialPort() { }

    /**
      Open the given serial port with the specified attributes.

      @param device  The name of the port
      @param baud    The transfer rate for the port
      @return  False on any errors, else true
    */
    virtual bool openPort(const string& device) = 0;

    /**
      Close a previously opened serial port.
    */
    virtual void closePort() = 0;

    /**
      Answers if the port is currently open and ready for I/O.

      @return  True if open and ready, else false
    */
    virtual bool isOpen() = 0;

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
    virtual int ReceiveComPortBlock(void* answer, uInt32 max_size, uInt32* real_size) = 0;

    /**
      Write block of bytes to the serial port.

      @param data  The byte(s) to write to the port
      @param size  The size of the block
      @return  The number of bytes written (-1 indicates error)
    */
    virtual int SendComPortBlock(const void* data, uInt32 size) = 0;

    /**
      Sets (or resets) the timeout to the timout period requested.  Starts
      counting to this period.  This timeout support is a little odd in that
      the timeout specifies the accumulated deadtime waiting to read not the
      total time waiting to read. They should be close enough to the same for
      this use. Used by the serial input routines, the actual counting takes
      place in ReceiveComPortBlock.

      @param timeout_milliseconds  The time in milliseconds to use for timeout
    */
    virtual void SerialTimeoutSet(uInt32 timeout_milliseconds) = 0;

    /**
      Empty the serial port buffers.  Cleans things to a known state.
    */
    virtual void ClearSerialPortBuffers() = 0;




    /**
      Get all valid serial ports detected on this system.
    */
    virtual const StringList& getPortNames() = 0;


    /**
      Receives a buffer from the open com port. Returns when the buffer is
      filled, the numer of requested linefeeds has been received or the timeout
      period has passed.

      @param Answer   Buffer to hold the bytes read from the serial port
      @param MaxSize  The size of buffer pointed to by Answer
      @param RealSize Pointer to a long that returns the amout of the
                      buffer that is actually used
      @param Wanted   The maximum number of linefeeds to accept before
                      returning
      @param timeout  The maximum amount of time to wait before
                      reading with an incomplete buffer (in milliseconds)
      @return  The number of bytes read (-1 indicates error)
    */
    int ReceiveComPort(const char* Ans, uInt32 MaxSize, uInt32* RealSize,
                       uInt32 Wanted, uInt32 timeout)
    {
      uInt32 tmp_realsize;
      uInt32 nr_of_0x0A = 0;
      uInt32 nr_of_0x0D = 0;
      int eof = 0;
      uInt32 p;
      uInt8* Answer = (uInt8*) Ans;
      SerialTimeoutSet(timeout);
      (*RealSize) = 0;

      do {
        ReceiveComPortBlock(Answer + (*RealSize), MaxSize - 1 - (*RealSize), &tmp_realsize);
        if(tmp_realsize != 0)
        {
          for(p = (*RealSize); p < (*RealSize) + tmp_realsize; p++)
          {
            if(Answer[p] == 0x0a)        nr_of_0x0A++;
            else if (Answer[p] == 0x0d)  nr_of_0x0D++;
            else if (((signed char) Answer[p]) < 0)  eof = 1;
          }
        }
        (*RealSize) += tmp_realsize;
      } while (((*RealSize) < MaxSize) && (SerialTimeoutCheck() == 0) && (nr_of_0x0A < Wanted) && (nr_of_0x0D < Wanted) && !eof);

      Answer[(*RealSize)] = 0;
      return (*RealSize);
    }

    /**
      Utility function to write a string to the serial port, automatically
      determining the size of the block.

      @param data  The string to write to the port
      @return  The number of bytes written (-1 indicates error)
    */
    int SendComPort(const char* data)
    {
      return SendComPortBlock(data, strlen(data));
    }

    /**
      Receives a fixed block from the open com port. Returns when the
      block is completely filled or the timeout period has passed.

      @param block   Buffer to hold the bytes read from the serial port
      @param size    The size of the buffer pointed to by block
      @param timeOut The maximum amount of time to wait before giving up on
                     completing the read
      @return 0 if successful, non-zero otherwise.
    */
    int ReceiveComPortBlockComplete(void* block, uInt32 size, uInt32 timeout)
    {
      uInt32 realsize = 0, read;
      char *result = (char*)block;

      SerialTimeoutSet(timeout);
      do {
        ReceiveComPortBlock(result + realsize, size - realsize, &read);
        realsize += read;
      } while ((realsize < size) && (SerialTimeoutCheck() == 0));

      return realsize != size ? 1 : 0;
    }

    /**
      Check to see if the serial timeout timer has run down.

      @return  1 if timer has run out, 0 if timer still has time left
    */
    int SerialTimeoutCheck()
    {
      return mySerialTimeoutCount == 0 ? 1 : 0;
    }

    /**
      Performs a timer tick.  In this simple case all we do is count down
      with protection against underflow and wrapping at the low end.
    */
    void SerialTimeoutTick()
    {
      if(mySerialTimeoutCount <= 1)
        mySerialTimeoutCount = 0;
      else
        mySerialTimeoutCount--;
    }



    /**
      Get/set the baud rate for this port.
      Note that the port must be opened for this to take effect.
    */
    int getBaud() const    { return myBaud; }
    void setBaud(int baud) { myBaud = baud; }

    /**
      Get/set ID string for this port.
    */
    const string& getID() const  { return myID; }
    void setID(const string& id) { myID = id;   }

  protected:
    uInt32 myBaud;
    uInt32 mySerialTimeoutCount;
    string myID;
    StringList myPortNames;
};

#endif
