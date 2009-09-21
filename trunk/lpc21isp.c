
#include "lpc21isp.h"
#include "lpcprog.h"

// Don't forget to update the version string that is on the next line
#define VERSION_STR "1.70"

#if !defined COMPILE_FOR_LPC21
int debug_level = 2;
#endif

/************* Portability layer. Serial and console I/O differences    */
/* are taken care of here.                                              */

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
///
#if defined COMPILE_FOR_LINUX
static int OpenSerialPort(ISP_ENVIRONMENT *IspEnvironment)
{
    IspEnvironment->fdCom = open(IspEnvironment->serial_port, O_RDWR | O_NOCTTY | O_NONBLOCK);

    if (IspEnvironment->fdCom < 0)
        return FALSE;

    /* clear input & output buffers, then switch to "blocking mode" */
    tcflush(IspEnvironment->fdCom, TCOFLUSH);
    tcflush(IspEnvironment->fdCom, TCIFLUSH);
    fcntl(IspEnvironment->fdCom, F_SETFL, fcntl(IspEnvironment->fdCom, F_GETFL) & ~O_NONBLOCK);

    tcgetattr(IspEnvironment->fdCom, &IspEnvironment->oldtio); /* save current port settings */

    bzero(&IspEnvironment->newtio, sizeof(IspEnvironment->newtio));
    IspEnvironment->newtio.c_cflag = CS8 | CLOCAL | CREAD;

#if defined(__FreeBSD__) || defined(__OpenBSD__)

    if(cfsetspeed(&IspEnvironment->newtio,(speed_t) strtol(IspEnvironment->baud_rate,NULL,10))) {
                  DebugPrintf(1, "baudrate %s not supported\n", IspEnvironment->baud_rate);
                  exit(3);
              };
#else

#ifdef __APPLE__
#define NEWTERMIOS_SETBAUDARTE(bps) IspEnvironment->newtio.c_ispeed = IspEnvironment->newtio.c_ospeed = bps;
#else
#define NEWTERMIOS_SETBAUDARTE(bps) IspEnvironment->newtio.c_cflag |= bps;
#endif

    switch (atol(IspEnvironment->baud_rate))
    {
#ifdef B1152000
          case 1152000: NEWTERMIOS_SETBAUDARTE(B1152000); break;
#endif // B1152000
#ifdef B576000
          case  576000: NEWTERMIOS_SETBAUDARTE(B576000); break;
#endif // B576000
#ifdef B230400
          case  230400: NEWTERMIOS_SETBAUDARTE(B230400); break;
#endif // B230400
#ifdef B115200
          case  115200: NEWTERMIOS_SETBAUDARTE(B115200); break;
#endif // B115200
#ifdef B57600
          case   57600: NEWTERMIOS_SETBAUDARTE(B57600); break;
#endif // B57600
#ifdef B38400
          case   38400: NEWTERMIOS_SETBAUDARTE(B38400); break;
#endif // B38400
#ifdef B19200
          case   19200: NEWTERMIOS_SETBAUDARTE(B19200); break;
#endif // B19200
#ifdef B9600
          case    9600: NEWTERMIOS_SETBAUDARTE(B9600); break;
#endif // B9600
          default:
              {
                  DebugPrintf(1, "unknown baudrate %s\n", IspEnvironment->baud_rate);
                  return FALSE;
              }
    }

#endif

    IspEnvironment->newtio.c_iflag = IGNPAR | IGNBRK | IXON | IXOFF;
    IspEnvironment->newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    IspEnvironment->newtio.c_lflag = 0;

    cfmakeraw(&IspEnvironment->newtio);
    IspEnvironment->newtio.c_cc[VTIME]    = 1;   /* inter-character timer used */
    IspEnvironment->newtio.c_cc[VMIN]     = 0;   /* blocking read until 0 chars received */

    tcflush(IspEnvironment->fdCom, TCIFLUSH);
    if(tcsetattr(IspEnvironment->fdCom, TCSANOW, &IspEnvironment->newtio))
    {
       DebugPrintf(1, "Could not change serial port behaviour (wrong baudrate?)\n");
       return FALSE;
    }

    return TRUE;
}
#endif // defined COMPILE_FOR_LINUX
///
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
///
#if defined COMPILE_FOR_LINUX
static void CloseSerialPort(ISP_ENVIRONMENT *IspEnvironment)
{
    tcflush(IspEnvironment->fdCom, TCOFLUSH);
    tcflush(IspEnvironment->fdCom, TCIFLUSH);
    tcsetattr(IspEnvironment->fdCom, TCSANOW, &IspEnvironment->oldtio);

    close(IspEnvironment->fdCom);
}
#endif // defined COMPILE_FOR_LINUX
///
/////////////////////////////////////////////////////////////////////////////////////////

// SerialPort ///////////////////////////////////////////////////////////////////////////
///
/***************************** SendComPort ******************************/
/**  Sends a string out the opened com port.
\param [in] s string to send.
*/
void SendComPort(ISP_ENVIRONMENT *IspEnvironment, const char *s)
{
    SendComPortBlock(IspEnvironment, s, strlen(s));
}
///
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
///
#if defined COMPILE_FOR_WINDOWS || defined COMPILE_FOR_CYGWIN
static int OpenSerialPort(ISP_ENVIRONMENT *IspEnvironment)
{
    DCB    dcb;
    COMMTIMEOUTS commtimeouts;

    IspEnvironment->hCom = CreateFileA(IspEnvironment->serial_port, GENERIC_READ | GENERIC_WRITE,
                           0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (IspEnvironment->hCom == INVALID_HANDLE_VALUE)
        return FALSE;

    DebugPrintf(3, "COM-Port %s opened...\n", IspEnvironment->serial_port);

    GetCommState(IspEnvironment->hCom, &dcb);
    dcb.BaudRate    = atol(IspEnvironment->baud_rate);
    dcb.ByteSize    = 8;
    dcb.StopBits    = ONESTOPBIT;
    dcb.Parity      = NOPARITY;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fOutX       = FALSE;
    dcb.fInX        = FALSE;
    dcb.fNull       = FALSE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;

    // added by Herbert Demmel - iF CTS line has the wrong state, we would never send anything!
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;

    if (SetCommState(IspEnvironment->hCom, &dcb) == 0)
    {
        DebugPrintf(1, "Can't set baudrate %s ! - Error: %ld", IspEnvironment->baud_rate, GetLastError());
        exit(3);
    }

   /*
    *  Peter Hayward 02 July 2008
    *
    *  The following call is only needed if the WaitCommEvent
    *  or possibly the GetCommMask functions are used.  They are
    *  *not* in this implimentation.  However, under Windows XP SP2
    *  on my laptop the use of this call causes XP to freeze (crash) while
    *  this program is running, e.g. in section 5/6/7 ... of a largish
    *  download.  Removing this *unnecessary* call fixed the problem.
    *  At the same time I've added a call to SetupComm to request
    *  (not necessarity honoured) the operating system to provide
    *  large I/O buffers for high speed I/O without handshaking.
    *
    *   SetCommMask(IspEnvironment->hCom,EV_RXCHAR | EV_TXEMPTY);
    */
    SetupComm(IspEnvironment->hCom, 32000, 32000);

    SetCommMask(IspEnvironment->hCom, EV_RXCHAR | EV_TXEMPTY);

    commtimeouts.ReadIntervalTimeout         = MAXDWORD;
    commtimeouts.ReadTotalTimeoutMultiplier  =    0;
    commtimeouts.ReadTotalTimeoutConstant    =    1;
    commtimeouts.WriteTotalTimeoutMultiplier =    0;
    commtimeouts.WriteTotalTimeoutConstant   =    0;
    SetCommTimeouts(IspEnvironment->hCom, &commtimeouts);
}

static void CloseSerialPort(ISP_ENVIRONMENT *IspEnvironment)
{
    CloseHandle(IspEnvironment->hCom);
}
#endif // defined COMPILE_FOR_WINDOWS || defined COMPILE_FOR_CYGWIN
///
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
///
/***************************** SendComPortBlock *************************/
/**  Sends a block of bytes out the opened com port.
\param [in] s block to send.
\param [in] n size of the block.
*/
void SendComPortBlock(ISP_ENVIRONMENT *IspEnvironment, const void *s, size_t n)
{
#if defined COMPILE_FOR_WINDOWS || defined COMPILE_FOR_CYGWIN

    unsigned long realsize;
    size_t m;
    unsigned long rxsize;
    char * pch;
    char * rxpch;
#endif // defined COMPILE_FOR_WINDOWS || defined COMPILE_FOR_CYGWIN

    DumpString(4, s, n, "Sending ");

#if defined COMPILE_FOR_WINDOWS || defined COMPILE_FOR_CYGWIN

    if (IspEnvironment->HalfDuplex == 0)
    {
        WriteFile(IspEnvironment->hCom, s, n, &realsize, NULL);
    }
    else
    {
        pch = (char *)s;
        rxpch = RxTmpBuf;
        pRxTmpBuf = RxTmpBuf;

        // avoid buffer otherflow
        if (n > sizeof (RxTmpBuf))
            n = sizeof (RxTmpBuf);

        for (m = 0; m < n; m++)
        {
            WriteFile(IspEnvironment->hCom, pch, 1, &realsize, NULL);

            if ((*pch != '?') || (n != 1))
            {
                do
                {
                    ReadFile(IspEnvironment->hCom, rxpch, 1, &rxsize, NULL);
                }while (rxsize == 0);
            }
            pch++;
            rxpch++;
        }
        *rxpch = 0;        // terminate echo string
    }
#endif // defined COMPILE_FOR_WINDOWS || defined COMPILE_FOR_CYGWIN

#if defined COMPILE_FOR_LINUX || defined COMPILE_FOR_LPC21

    write(IspEnvironment->fdCom, s, n);

#endif // defined COMPILE_FOR_LINUX || defined COMPILE_FOR_LPC21
}
///
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
///
/***************************** SerialTimeoutSet *************************/
/**  Sets (or resets) the timeout to the timout period requested.  Starts
counting to this period.  This timeout support is a little odd in that the
timeout specifies the accumulated deadtime waiting to read not the total
time waiting to read. They should be close enought to the same for this
use. Used by the serial input routines, the actual counting takes place in
ReceiveComPortBlock.
\param [in] timeout_milliseconds the time in milliseconds to use for
timeout.  Note that just because it is set in milliseconds doesn't mean
that the granularity is that fine.  In many cases (particularly Linux) it
will be coarser.
*/
static void SerialTimeoutSet(ISP_ENVIRONMENT *IspEnvironment, unsigned timeout_milliseconds)
{
#if defined COMPILE_FOR_LINUX
    IspEnvironment->serial_timeout_count = timeout_milliseconds / 100;
#else
    IspEnvironment->serial_timeout_count = timeout_milliseconds;
#endif
}
///
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
///
/***************************** SerialTimeoutCheck ***********************/
/**  Check to see if the serial timeout timer has run down.
\retval 1 if timer has run out.
\retval 0 if timer still has time left.
*/
static int SerialTimeoutCheck(ISP_ENVIRONMENT *IspEnvironment)
{
    if (IspEnvironment->serial_timeout_count == 0)
    {
        return 1;
    }
    return 0;
}
///
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
///
/***************************** SerialTimeoutTick ************************/
/**  Performs a timer tick.  In this simple case all we do is count down
with protection against underflow and wrapping at the low end.
*/
static void SerialTimeoutTick(ISP_ENVIRONMENT *IspEnvironment)
{
    if (IspEnvironment->serial_timeout_count <= 1)
    {
        IspEnvironment->serial_timeout_count = 0;
    }
    else
    {
        IspEnvironment->serial_timeout_count--;
    }
}
///
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
///
/***************************** ReceiveComPortBlock **********************/
/**  Receives a buffer from the open com port. Returns all the characters
ready (waits for up to 'n' milliseconds before accepting that no more
characters are ready) or when the buffer is full. 'n' is system dependant,
see SerialTimeout routines.
\param [out] answer buffer to hold the bytes read from the serial port.
\param [in] max_size the size of buffer pointed to by answer.
\param [out] real_size pointer to a long that returns the amout of the
buffer that is actually used.
*/
static void ReceiveComPortBlock(ISP_ENVIRONMENT *IspEnvironment,
                                          void *answer, unsigned long max_size,
                                          unsigned long *real_size)
{
    char tmp_string[32];

#if defined COMPILE_FOR_WINDOWS || defined COMPILE_FOR_CYGWIN

    if (IspEnvironment->HalfDuplex == 0)
        ReadFile(IspEnvironment->hCom, answer, max_size, real_size, NULL);
    else
    {
        *real_size = strlen (pRxTmpBuf);
        if (*real_size)
        {
            if (max_size >= *real_size)
            {
                strncpy((char*) answer, pRxTmpBuf, *real_size);
                RxTmpBuf[0] = 0;
                pRxTmpBuf = RxTmpBuf;
            }
            else
            {
                strncpy((char*) answer, pRxTmpBuf, max_size);
                *real_size = max_size;
                pRxTmpBuf += max_size;
            }
        }
        else
            ReadFile(IspEnvironment->hCom, answer, max_size, real_size, NULL);
    }

#endif // defined COMPILE_FOR_WINDOWS || defined COMPILE_FOR_CYGWIN

#if defined COMPILE_FOR_LINUX || defined COMPILE_FOR_LPC21

    *real_size = read(IspEnvironment->fdCom, answer, max_size);

#endif // defined COMPILE_FOR_LINUX

    sprintf(tmp_string, "Read(Length=%ld): ", (*real_size));
    DumpString(5, answer, (*real_size), tmp_string);

    if (*real_size == 0)
    {
        SerialTimeoutTick(IspEnvironment);
    }
}
///
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
///
/***************************** ClearSerialPortBuffers********************/
/**  Empty the serial port buffers.  Cleans things to a known state.
*/
void ClearSerialPortBuffers(ISP_ENVIRONMENT *IspEnvironment)
{
#if defined COMPILE_FOR_LINUX
    /* variables to store the current tty state, create a new one */
    struct termios origtty, tty;

    /* store the current tty settings */
    tcgetattr(IspEnvironment->fdCom, &origtty);

    // Flush input and output buffers
    tty=origtty;
    tcsetattr(IspEnvironment->fdCom, TCSAFLUSH, &tty);

    /* reset the tty to its original settings */
    tcsetattr(IspEnvironment->fdCom, TCSADRAIN, &origtty);
#endif // defined COMPILE_FOR_LINUX
#if defined COMPILE_FOR_WINDOWS || defined COMPILE_FOR_CYGWIN
    PurgeComm(IspEnvironment->hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
#endif // defined COMPILE_FOR_WINDOWS || defined COMPILE_FOR_CYGWIN
}
///
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
///
/***************************** ReceiveComPortBlockComplete **************/
/**  Receives a fixed block from the open com port. Returns when the
block is completely filled or the timeout period has passed
\param [out] block buffer to hold the bytes read from the serial port.
\param [in] size the size of the buffer pointed to by block.
\param [in] timeOut the maximum amount of time to wait before guvung up on
completing the read.
\return 0 if successful, non-zero otherwise.
*/
int ReceiveComPortBlockComplete(ISP_ENVIRONMENT *IspEnvironment,
                                                    void *block, size_t size, unsigned timeout)
{
    unsigned long realsize = 0, read;
    char *result;
    char tmp_string[32];

    result = (char*) block;

    SerialTimeoutSet(IspEnvironment, timeout);

    do
    {
        ReceiveComPortBlock(IspEnvironment, result + realsize, size - realsize, &read);

        realsize += read;

    } while ((realsize < size) && (SerialTimeoutCheck(IspEnvironment) == 0));

    sprintf(tmp_string, "Answer(Length=%ld): ", realsize);
    DumpString(3, result, realsize, tmp_string);

    if (realsize != size)
    {
        return 1;
    }
    return 0;
}
///
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
///
/***************************** ReceiveComPort ***************************/
/**  Receives a buffer from the open com port. Returns when the buffer is
filled, the numer of requested linefeeds has been received or the timeout
period has passed
\param [in] ISPEnvironment.
\param [out] Answer buffer to hold the bytes read from the serial port.
\param [in] MaxSize the size of buffer pointed to by Answer.
\param [out] RealSize pointer to a long that returns the amout of the
buffer that is actually used.
\param [in] WantedNr0x0A the maximum number of linefeeds to accept before
returning.
\param [in] timeOutMilliseconds the maximum amount of time to wait before
reading with an incomplete buffer.
*/
void ReceiveComPort(ISP_ENVIRONMENT *IspEnvironment,
                                    const char *Ans, unsigned long MaxSize,
                                    unsigned long *RealSize, unsigned long WantedNr0x0A,
                                    unsigned timeOutMilliseconds)
{
    unsigned long tmp_realsize;
    unsigned long nr_of_0x0A = 0;
    unsigned long nr_of_0x0D = 0;
    int eof = 0;
    unsigned long p;
    unsigned char *Answer;
    char tmp_string[32];

    Answer = (unsigned char*) Ans;

    SerialTimeoutSet(IspEnvironment, timeOutMilliseconds);

    (*RealSize) = 0;

    do
    {
        ReceiveComPortBlock(IspEnvironment, Answer + (*RealSize), MaxSize - 1 - (*RealSize), &tmp_realsize);

        if (tmp_realsize != 0)
        {
            for (p = (*RealSize); p < (*RealSize) + tmp_realsize; p++)
            {
                if (Answer[p] == 0x0a)
                {
                    nr_of_0x0A++;
                }
                else if (Answer[p] == 0x0d)
                {
                    nr_of_0x0D++;
                }
                else if (((signed char) Answer[p]) < 0)
                {
                    eof = 1;
                }
            }
        }

        (*RealSize) += tmp_realsize;

    } while (((*RealSize) < MaxSize) && (SerialTimeoutCheck(IspEnvironment) == 0) && (nr_of_0x0A < WantedNr0x0A) && (nr_of_0x0D < WantedNr0x0A) && !eof);

    Answer[(*RealSize)] = 0;

    sprintf(tmp_string, "Answer(Length=%ld): ", (*RealSize));
    DumpString(3, Answer, (*RealSize), tmp_string);
}
///
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////





/***************************** ControlModemLines ************************/
/**  Controls the modem lines to place the microcontroller into various
states during the programming process.
error rather abruptly terminates the program.
\param [in] DTR the state to set the DTR line to.
\param [in] RTS the state to set the RTS line to.
*/
void ControlModemLines(ISP_ENVIRONMENT *IspEnvironment, unsigned char DTR, unsigned char RTS)
{
    //handle wether to invert the control lines:
    DTR ^= IspEnvironment->ControlLinesInverted;
    RTS ^= IspEnvironment->ControlLinesInverted;

    //handle wether to swap the control lines
    if (IspEnvironment->ControlLinesSwapped)
    {
        unsigned char tempRTS;
        tempRTS = RTS;
        RTS = DTR;
        DTR = tempRTS;
    }

#if defined COMPILE_FOR_LINUX
    int status;

    if (ioctl(IspEnvironment->fdCom, TIOCMGET, &status) == 0)
    {
        DebugPrintf(3, "ioctl get ok, status = %X\n",status);
    }
    else
    {
        DebugPrintf(1, "ioctl get failed\n");
    }

    if (DTR) status |=  TIOCM_DTR;
    else    status &= ~TIOCM_DTR;

    if (RTS) status |=  TIOCM_RTS;
    else    status &= ~TIOCM_RTS;

    if (ioctl(IspEnvironment->fdCom, TIOCMSET, &status) == 0)
    {
        DebugPrintf(3, "ioctl set ok, status = %X\n",status);
    }
    else
    {
        DebugPrintf(1, "ioctl set failed\n");
    }

    if (ioctl(IspEnvironment->fdCom, TIOCMGET, &status) == 0)
    {
        DebugPrintf(3, "ioctl get ok, status = %X\n",status);
    }
    else
    {
        DebugPrintf(1, "ioctl get failed\n");
    }

#endif // defined COMPILE_FOR_LINUX
#if defined COMPILE_FOR_WINDOWS || defined COMPILE_FOR_CYGWIN

    if (DTR) EscapeCommFunction(IspEnvironment->hCom, SETDTR);
    else    EscapeCommFunction(IspEnvironment->hCom, CLRDTR);

    if (RTS) EscapeCommFunction(IspEnvironment->hCom, SETRTS);
    else    EscapeCommFunction(IspEnvironment->hCom, CLRRTS);

#endif // defined COMPILE_FOR_WINDOWS || defined COMPILE_FOR_CYGWIN

    DebugPrintf(3, "DTR (%d), RTS (%d)\n", DTR, RTS);
}


#if defined COMPILE_FOR_LINUX
/***************************** Sleep ************************************/
/**  Provide linux replacement for windows function.
\param [in] Milliseconds the time to wait for in milliseconds.
*/
void Sleep(unsigned long MilliSeconds)
{
    usleep(MilliSeconds*1000); //convert to microseconds
}
#endif // defined COMPILE_FOR_LINUX


/***************************** ResetTarget ******************************/
/**  Resets the target leaving it in either download (program) mode or
run mode.
\param [in] mode the mode to leave the target in.
*/
void ResetTarget(ISP_ENVIRONMENT *IspEnvironment, TARGET_MODE mode)
{
    if (IspEnvironment->ControlLines)
    {
        switch (mode)
        {
        /* Reset and jump to boot loader.                       */
        case PROGRAM_MODE:
            ControlModemLines(IspEnvironment, 1, 1);
            Sleep(100);
            ClearSerialPortBuffers(IspEnvironment);
            Sleep(100);
            ControlModemLines(IspEnvironment, 0, 1);
            //Longer delay is the Reset signal is conected to an external rest controller
            Sleep(500);
            // Clear the RTS line after having reset the micro
            // Needed for the "GO <Address> <Mode>" ISP command to work */
            ControlModemLines(IspEnvironment, 0, 0);
            break;

        /* Reset and start uploaded program                     */
        case RUN_MODE:
            ControlModemLines(IspEnvironment, 1, 0);
            Sleep(100);
            ClearSerialPortBuffers(IspEnvironment);
            Sleep(100);
            ControlModemLines(IspEnvironment, 0, 0);
            Sleep(100);
            break;
        }
    }
}













/***************************** DebugPrintf ******************************/
/**  Prints a debug string depending the current debug level. The higher
the debug level the more detail that will be printed.  Each print
has an associated level, the higher the level the more detailed the
debugging information being sent.
\param [in] level the debug level of the print statement, if the level
is less than or equal to the current debug level it will be printed.
\param [in] fmt a standard printf style format string.
\param [in] ... the usual printf parameters.
*/
void DebugPrintf(int level, const char *fmt, ...)
{
    va_list ap;

    if (level <= debug_level)
    {
        char pTemp[2000];
        va_start(ap, fmt);
        //vprintf(fmt, ap);
        vsprintf(pTemp, fmt, ap);
        TRACE(pTemp);
        va_end(ap);
        fflush(stdout);
    }
}

/***************************** Ascii2Hex ********************************/
/**  Converts a hex character to its equivalent number value. In case of an
error rather abruptly terminates the program.
\param [in] c the hex digit to convert.
\return the value of the hex digit.
*/
unsigned char Ascii2Hex(unsigned char c)
{
    if (c >= '0' && c <= '9')
    {
        return (unsigned char)(c - '0');
    }

    if (c >= 'A' && c <= 'F')
    {
        return (unsigned char)(c - 'A' + 10);
    }

    if (c >= 'a' && c <= 'f')
    {
        return (unsigned char)(c - 'a' + 10);
    }

    DebugPrintf(1, "Wrong Hex-Nibble %c (%02X)\n", c, c);
    exit(1);

    return 0;  // this "return" will never be reached, but some compilers give a warning if it is not present
}


/***************************** LoadFile *********************************/
/**  Loads the requested file to download into memory.
\param [in] IspEnvironment  structure containing input filename
*/
int LoadFile(ISP_ENVIRONMENT *IspEnvironment)
{
    int            fd;
    int            BinaryOffsetDefined;
    unsigned long  FileLength;
    BINARY        *FileContent;              /**< Used to store the content of a hex */
                                             /*   file before converting to binary.  */
    unsigned long  BinaryMemSize;

    fd = open(IspEnvironment->input_file, O_RDONLY | O_BINARY);
    if (fd == -1)
    {
        DebugPrintf(1, "Can't open file %s\n", IspEnvironment->input_file);
        return FALSE;
    }

    FileLength = lseek(fd, 0L, 2);      // Get file size

    if (FileLength == (size_t)-1)
    {
        DebugPrintf(1, "\nFileLength = -1 !?!\n");
        return FALSE;
    }

    lseek(fd, 0L, 0);

    FileContent = (BINARY*) malloc(FileLength);

    BinaryMemSize = FileLength * 2;

    IspEnvironment->BinaryLength = 0;   /* Increase length as needed.       */
    IspEnvironment->BinaryOffset = 0;
    IspEnvironment->StartAddress = 0;
    BinaryOffsetDefined = 0;

    IspEnvironment->BinaryContent = (BINARY*) malloc(BinaryMemSize);

    read(fd, FileContent, FileLength);

    close(fd);

    DebugPrintf(2, "File %s:\n\tloaded...\n", IspEnvironment->input_file);

    memcpy(IspEnvironment->BinaryContent, FileContent, FileLength);
    IspEnvironment->BinaryLength = FileLength;

    DebugPrintf(2, "\timage size : %ld\n", IspEnvironment->BinaryLength);

    // check length to flash for correct alignment, can happen with broken ld-scripts
    if (IspEnvironment->BinaryLength % 4 != 0)
    {
        unsigned long NewBinaryLength = ((IspEnvironment->BinaryLength + 3)/4) * 4;

        DebugPrintf(2, "Warning:  data not aligned to 32 bits, padded (length was %lX, now %lX)\n", IspEnvironment->BinaryLength, NewBinaryLength);

        IspEnvironment->BinaryLength = NewBinaryLength;
    }

    return TRUE;
}


const char* lpc_AutoDetect(const char* device)
{
    ISP_ENVIRONMENT IspEnvironment;
    const char* result;

    // Initialize debug level
    debug_level = 2;

    // Initialize ISP Environment
    //   SA: These values are hard-coded for the Harmony cart
    memset(&IspEnvironment, 0, sizeof(IspEnvironment));
    IspEnvironment.micro               = PHILIPS_ARM;
    IspEnvironment.FileFormat          = FORMAT_BINARY;  // -bin
    IspEnvironment.ProgramChip         = FALSE;          // -detectonly
    IspEnvironment.DetectOnly          = TRUE;           //
    IspEnvironment.ControlLines        = TRUE;           // -control
    IspEnvironment.ControlLinesSwapped = TRUE;           // -controlswap
    IspEnvironment.serial_port         = (char*)device;
    IspEnvironment.baud_rate           = "38400";        // HC baud is always 38400
    strcpy(IspEnvironment.StringOscillator, "10000");    // HC oscillator is always 10000

    /* Open the serial port to the microcontroller. */
    if(OpenSerialPort(&IspEnvironment) == FALSE)
    {
      result = "ERROR: Can't open port";
      return result;
    }

    ResetTarget(&IspEnvironment, PROGRAM_MODE);

    ClearSerialPortBuffers(&IspEnvironment);

    // Get the version #, if any
    result = PhilipsChipVersion(&IspEnvironment);
    if (strncmp(result, "ERROR:", 6) == 0)
    {
        printf("%s\n", result);
        CloseSerialPort(&IspEnvironment);
    }
    else if (IspEnvironment.StartAddress == 0)
    {
        /* Only reset target if startaddress = 0
        * Otherwise stay with the running program as started in Download()
        */
        ResetTarget(&IspEnvironment, RUN_MODE);
    }

    IspEnvironment.debug_level = 1;    /* From now on there is no more debug output !! */
                                        /* Therefore switch it off...                   */

    CloseSerialPort(&IspEnvironment);  /*  All done, close the serial port to the      */

    return result;
}


int lpc_Download(const char* device, const char* file, void(*func)())
{
    ISP_ENVIRONMENT IspEnvironment;
    int result = TRUE;

    // Initialize debug level
    debug_level = 2;

    // Initialize ISP Environment
    //   SA: These values are hard-coded for the Harmony cart
    memset(&IspEnvironment, 0, sizeof(IspEnvironment));
    IspEnvironment.micro               = PHILIPS_ARM;
    IspEnvironment.FileFormat          = FORMAT_BINARY;  // -bin
    IspEnvironment.ProgramChip         = TRUE;           //
    IspEnvironment.input_file          = (char*)file;    // filename
    IspEnvironment.DetectOnly          = FALSE;          //
    IspEnvironment.ControlLines        = TRUE;           // -control
    IspEnvironment.ControlLinesSwapped = TRUE;           // -controlswap
    IspEnvironment.serial_port         = (char*)device;
    IspEnvironment.baud_rate           = "38400";        // HC baud is always 38400
    strcpy(IspEnvironment.StringOscillator, "10000");    // HC oscillator is always 10000

    /* Download requested, read in the input file. */
    if(LoadFile(&IspEnvironment) == FALSE)
    {
      return FALSE;
    }

    /* Open the serial port to the microcontroller. */
    if(OpenSerialPort(&IspEnvironment) == FALSE)
    {
//      result = "ERROR: Can't open port";
      return FALSE;
    }

    ResetTarget(&IspEnvironment, PROGRAM_MODE);

    ClearSerialPortBuffers(&IspEnvironment);

    // Attempt download of data
    result = PhilipsDownload(&IspEnvironment, func);
    if (result != TRUE)
    {
        printf("%d\n", result);
        CloseSerialPort(&IspEnvironment);
    }
    else if (IspEnvironment.StartAddress == 0)
    {
        /* Only reset target if startaddress = 0
        * Otherwise stay with the running program as started in Download()
        */
        ResetTarget(&IspEnvironment, RUN_MODE);
    }

    IspEnvironment.debug_level = 1;    /* From now on there is no more debug output !! */
                                        /* Therefore switch it off...                   */

    CloseSerialPort(&IspEnvironment);  /*  All done, close the serial port to the      */

    return result;
}


/***************************** DumpString ******************************/
/**  Prints an area of memory to stdout. Converts non-printables to hex.
\param [in] level the debug level of the block to be dumped.  If this is
less than or equal to the current debug level than the dump will happen
otherwise this just returns.
\param [in] b pointer to an area of memory.
\param [in] size the length of the memory block to print.
\param [in] prefix string is a pointer to a prefix string.
*/
void DumpString(int level, const void *b, size_t size, const char *prefix_string)
{
    size_t i;
    const char * s = (const char*) b;
    unsigned char c;

    DebugPrintf(level, prefix_string);

    DebugPrintf(level, "'");
    for (i = 0; i < size; i++)
    {
        c = s[i];
        if (c >= 0x20 && c <= 0x7e) /*isprint?*/
        {
            DebugPrintf(level, "%c", c);
        }
        else
        {
            DebugPrintf(level, "(%02X)", c);
        }
    }
    DebugPrintf(level, "'\n");
}


#if 0
// ***************************** ReadArguments ****************************
// Reads the command line arguments and parses it for the various
// options. Uses the same arguments as main.  Used to separate the command
// line parsing from main and improve its readability.  This should also make
// it easier to modify the command line parsing in the future.
// \param [in] argc the number of arguments.
// \param [in] argv an array of pointers to the arguments.
static void ReadArguments(ISP_ENVIRONMENT *IspEnvironment, unsigned int argc, char *argv[])
{
    unsigned int i;

    if (argc >= 5)
    {
        for (i = 1; i < argc - 4; i++)
        {
            if (stricmp(argv[i], "-wipe") == 0)
            {
                IspEnvironment->WipeDevice = 1;
                DebugPrintf(3, "Wipe entire device before writing.\n");
                continue;
            }

            if (stricmp(argv[i], "-bin") == 0)
            {
                IspEnvironment->FileFormat = FORMAT_BINARY;
                DebugPrintf(3, "Binary format file input.\n");
                continue;
            }

            if (stricmp(argv[i], "-hex") == 0)
            {
                IspEnvironment->FileFormat = FORMAT_HEX;
                DebugPrintf(3, "Hex format file input.\n");
                continue;
            }

            if (stricmp(argv[i], "-logfile") == 0)
            {
                IspEnvironment->LogFile = 1;
                DebugPrintf(3, "Log terminal output.\n");
                continue;
            }

            if (stricmp(argv[i], "-detectonly") == 0)
            {
                IspEnvironment->DetectOnly  = 1;
                IspEnvironment->ProgramChip = 0;
                DebugPrintf(3, "Only detect LPC chip part id.\n");
                continue;
            }

            if (stricmp(argv[i], "-debug") == 0)
            {
                debug_level = 4;
                DebugPrintf(3, "Turn on debug.\n");
                continue;
            }

            if (stricmp(argv[i], "-control") == 0)
            {
                IspEnvironment->ControlLines = 1;
                DebugPrintf(3, "Use RTS/DTR to control target state.\n");
                continue;
            }

            if (stricmp(argv[i], "-controlswap") == 0)
            {
                IspEnvironment->ControlLinesSwapped = 1;
                DebugPrintf(3, "Use RTS to control reset, and DTR to control P0.14(ISP).\n");
                continue;
            }

            if (stricmp(argv[i], "-controlinv") == 0)
            {
                IspEnvironment->ControlLinesInverted = 1;
                DebugPrintf(3, "Invert state of RTS & DTR (0=true/assert/set, 1=false/deassert/clear).\n");
                continue;
            }

            if (stricmp(argv[i], "-halfduplex") == 0)
            {
                IspEnvironment->HalfDuplex = 1;
                DebugPrintf(3, "halfduplex serial communication.\n");
                continue;
            }

            if (stricmp(argv[i], "-ADARM") == 0)
            {
                IspEnvironment->micro = ANALOG_DEVICES_ARM;
                DebugPrintf(2, "Target: Analog Devices.\n");
                continue;
            }

            if (stricmp(argv[i], "-PHILIPSARM") == 0)
            {
                IspEnvironment->micro = PHILIPS_ARM;
                DebugPrintf(2, "Target: Philips.\n");
                continue;
            }

            if (stricmp(argv[i], "-Verify") == 0)
            {
                IspEnvironment->Verify = 1;
                DebugPrintf(2, "Verify after copy RAM to Flash.\n");
                continue;
            }

#ifdef INTEGRATED_IN_WIN_APP
            if (stricmp(argv[i], "-nosync") == 0)
            {
                IspEnvironment->NoSync = 1;
                DebugPrintf(2, "Performing no syncing, already done.\n");
                continue;
            }
#endif

#ifdef TERMINAL_SUPPORT
            if (CheckTerminalParameters(IspEnvironment, argv[i]))
            {
                continue;
            }
#endif

            DebugPrintf(2, "Unknown command line option: \"%s\"\n", argv[i]);
        }

        IspEnvironment->input_file = argv[argc - 4];

        // Newest cygwin delivers a '\x0d' at the end of argument
        // when calling lpc21isp from batch file
        for (i = 0; i < strlen(argv[argc - 1]) && i < (sizeof(IspEnvironment->StringOscillator) - 1) &&
            argv[argc - 1][i] >= '0' && argv[argc - 1][i] <= '9'; i++)
        {
            IspEnvironment->StringOscillator[i] = argv[argc - 1][i];
        }
        IspEnvironment->StringOscillator[i] = 0;

        IspEnvironment->serial_port = argv[argc - 3];
        IspEnvironment->baud_rate = argv[argc - 2];
    }

    if (argc < 5)
    {
        debug_level = (debug_level < 2) ? 2 : debug_level;
    }

    if (argc < 5)
    {
        DebugPrintf(2, "\n"
                       "Portable command line ISP for NXP LPC2000 family and Analog Devices ADUC 70xx\n"
                       "Version " VERSION_STR " compiled for " COMPILED_FOR ": " __DATE__ ", " __TIME__ "\n"
                       "Copyright (c) by Martin Maurer, 2003-2009, Email: Martin.Maurer@clibb.de\n"
                       "Portions Copyright (c) by Aeolus Development 2004, www.aeolusdevelopment.com\n"
                       "\n");

        DebugPrintf(1, "Syntax:  lpc21isp [Options] file comport baudrate Oscillator_in_kHz\n\n"
                       "Example: lpc21isp test.hex com1 115200 14746\n\n"
                       "Options: -bin         for uploading binary file\n"
                       "         -hex         for uploading file in intel hex format (default)\n"
                       "         -term        for starting terminal after upload\n"
                       "         -termonly    for starting terminal without an upload\n"
                       "         -localecho   for local echo in terminal\n"
                       "         -detectonly  detect only used LPC chiptype (PHILIPSARM only)\n"
                       "         -debug       for creating a lot of debug infos\n"
                       "         -wipe        Erase entire device before upload\n"
                       "         -control     for controlling RS232 lines for easier booting\n"
                       "                      (Reset = DTR, EnableBootLoader = RTS)\n"
#ifdef INTEGRATED_IN_WIN_APP
                       "         -nosync      Do not synchronize device via '?'\n"
#endif
                       "         -controlswap swap RS232 control lines\n"
                       "                      (Reset = RTS, EnableBootLoader = DTR)\n"
                       "         -controlinv  Invert state of RTS & DTR \n"
                       "                      (0=true/assert/set, 1=false/deassert/clear).\n"
                       "         -verify      Verify the data in Flash after every writes to\n"
                       "                      sector. To detect errors in writing to Flash ROM\n"
                       "         -logfile     for enabling logging of terminal output to lpc21isp.log\n"
                       "         -halfduplex  use halfduplex serial communication (i.e. with K-Line)\n"
                       "         -ADARM       for downloading to an Analog Devices\n"
                       "                      ARM microcontroller ADUC70xx\n"
                       "         -PHILIPSARM  for downloading to a microcontroller from\n"
                       "                      NXP(Philips) LPC17xx / LPC2000 family (default)\n");

        exit(1);
    }

    if (IspEnvironment->micro == PHILIPS_ARM)
    {
        // If StringOscillator is bigger than 100 MHz, there seems to be something wrong
        if (strlen(IspEnvironment->StringOscillator) > 5)
        {
            DebugPrintf(1, "Invalid crystal frequency %s\n", IspEnvironment->StringOscillator);
            exit(1);
        }
    }
}
#endif
