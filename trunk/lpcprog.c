int PhilipsDownload(ISP_ENVIRONMENT *IspEnvironment, void(*func)())
{
    unsigned long realsize;
    char Answer[128];
    char ExpectedAnswer[128];
    char temp[128];
    /*const*/ char *strippedAnswer, *endPtr;
    int  strippedsize;
    int nQuestionMarks;
    int found;
    unsigned long Sector;
    unsigned long SectorLength;
    unsigned long SectorStart, SectorOffset, SectorChunk;
    char tmpString[128];
    char uuencode_table[64];
    int Line;
    unsigned long tmpStringPos;
    unsigned long BlockOffset;
    unsigned long Block;
    unsigned long Pos;
    unsigned long CopyLength;
    int c,k=0,i;
    unsigned long ivt_CRC;          // CRC over interrupt vector table
    unsigned long block_CRC;
    time_t tStartUpload=0, tDoneUpload=0;
    long WatchDogSeconds = 0;
    int WaitForWatchDog = 0;
    char tmp_string[64];
    char * cmdstr;

    int repeat = 0;
    // Puffer for data to resend after "RESEND\r\n" Target responce
    char sendbuf0[128];
    char sendbuf1[128];
    char sendbuf2[128];
    char sendbuf3[128];
    char sendbuf4[128];
    char sendbuf5[128];
    char sendbuf6[128];
    char sendbuf7[128];
    char sendbuf8[128];
    char sendbuf9[128];
    char sendbuf10[128];
    char sendbuf11[128];
    char sendbuf12[128];
    char sendbuf13[128];
    char sendbuf14[128];
    char sendbuf15[128];
    char sendbuf16[128];
    char sendbuf17[128];
    char sendbuf18[128];
    char sendbuf19[128];

    char * sendbuf[20] = {    sendbuf0,  sendbuf1,  sendbuf2,  sendbuf3,  sendbuf4,
                              sendbuf5,  sendbuf6,  sendbuf7,  sendbuf8,  sendbuf9,
                              sendbuf10, sendbuf11, sendbuf12, sendbuf13, sendbuf14,
                              sendbuf15, sendbuf16, sendbuf17, sendbuf18, sendbuf19};

    if (!IspEnvironment->DetectOnly)
    {
        // Build up uuencode table
        uuencode_table[0] = 0x60;           // 0x20 is translated to 0x60 !

        for (i = 1; i < 64; i++)
        {
            uuencode_table[i] = (char)(0x20 + i);
        }

        if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC2XXX)
        {
            // Patch 0x14, otherwise it is not running and jumps to boot mode

            ivt_CRC = 0;

            // Clear the vector at 0x14 so it doesn't affect the checksum:
            for (i = 0; i < 4; i++)
            {
                IspEnvironment->BinaryContent[i + 0x14] = 0;
            }

            // Calculate a native checksum of the little endian vector table:
            for (i = 0; i < (4 * 8);) {
                ivt_CRC += IspEnvironment->BinaryContent[i++];
                ivt_CRC += IspEnvironment->BinaryContent[i++] << 8;
                ivt_CRC += IspEnvironment->BinaryContent[i++] << 16;
                ivt_CRC += IspEnvironment->BinaryContent[i++] << 24;
            }

            /* Negate the result and place in the vector at 0x14 as little endian
            * again. The resulting vector table should checksum to 0. */
            ivt_CRC = (unsigned long) (0 - ivt_CRC);
            for (i = 0; i < 4; i++)
            {
                IspEnvironment->BinaryContent[i + 0x14] = (unsigned char)(ivt_CRC >> (8 * i));
            }

            DebugPrintf(3, "Position 0x14 patched: ivt_CRC = 0x%08lX\n", ivt_CRC);
        }
        else if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC17XX)
        {
            // Patch 0x1C, otherwise it is not running and jumps to boot mode

            ivt_CRC = 0;

            // Clear the vector at 0x1C so it doesn't affect the checksum:
            for (i = 0; i < 4; i++)
            {
                IspEnvironment->BinaryContent[i + 0x1C] = 0;
            }

            // Calculate a native checksum of the little endian vector table:
            for (i = 0; i < (4 * 8);) {
                ivt_CRC += IspEnvironment->BinaryContent[i++];
                ivt_CRC += IspEnvironment->BinaryContent[i++] << 8;
                ivt_CRC += IspEnvironment->BinaryContent[i++] << 16;
                ivt_CRC += IspEnvironment->BinaryContent[i++] << 24;
            }

            /* Negate the result and place in the vector at 0x1C as little endian
            * again. The resulting vector table should checksum to 0. */
            ivt_CRC = (unsigned long) (0 - ivt_CRC);
            for (i = 0; i < 4; i++)
            {
                IspEnvironment->BinaryContent[i + 0x1C] = (unsigned char)(ivt_CRC >> (8 * i));
            }

            DebugPrintf(3, "Position 0x1C patched: ivt_CRC = 0x%08lX\n", ivt_CRC);
        }
    }

    DebugPrintf(2, "Synchronizing (ESC to abort)");

    {
        /* SA_CHANGED: '100' to '3' to shorten autodetection */
        for (nQuestionMarks = found = 0; !found && nQuestionMarks < 3; nQuestionMarks++)
        {
            DebugPrintf(2, ".");
            SendComPort(IspEnvironment, "?");

            memset(Answer,0,sizeof(Answer));
            ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 1,100);

            strippedAnswer = Answer;
            strippedsize = realsize;
            while ((strippedsize > 0) && ((*strippedAnswer == '?') || (*strippedAnswer == 0)))
            {
                strippedAnswer++;
                strippedsize--;
            }

            sprintf(tmp_string, "StrippedAnswer(Length=%ld): '", strippedsize);
            DumpString(3, strippedAnswer, strippedsize, tmp_string);

            if (strcmp(strippedAnswer, "Bootloader\r\n") == 0 && IspEnvironment->TerminalOnly == 0)
            {
                long chars, xtal;
                unsigned long ticks;
                chars = (17 * IspEnvironment->BinaryLength + 1) / 10;
                WatchDogSeconds = (10 * chars + 5) / atol(IspEnvironment->baud_rate) + 10;
                xtal = atol(IspEnvironment->StringOscillator) * 1000;
                ticks = (unsigned long)WatchDogSeconds * ((xtal + 15) / 16);
                DebugPrintf(2, "Entering ISP; re-synchronizing (watchdog = %ld seconds)\n", WatchDogSeconds);
                sprintf(temp, "T %lu\r\n", ticks);
                SendComPort(IspEnvironment, temp);
                ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 1,100);
                if (strcmp(Answer, "OK\r\n") != 0)
                {
                    DebugPrintf(2, "No answer on 'watchdog timer set'\n");
                    return (NO_ANSWER_WDT);
                }
                SendComPort(IspEnvironment, "G 10356\r\n");
                Sleep(200);
                nQuestionMarks = 0;
                WaitForWatchDog = 1;
                continue;
            }

            tStartUpload = time(NULL);

            if (strcmp(strippedAnswer, "Synchronized\r\n") == 0)
            {
                found = 1;
            }
            else
            {
                ResetTarget(IspEnvironment, PROGRAM_MODE);
            }
        }
    }

    if (!found)
    {
        DebugPrintf(1, " no answer on '?'\n");
        return (NO_ANSWER_QM);
    }

    DebugPrintf(2, " OK\n");

    SendComPort(IspEnvironment, "Synchronized\n");

    ReceiveComPort(IspEnvironment, Answer, sizeof(Answer) - 1, &realsize, 2, 1000);

    if ((strcmp(Answer, "Synchronized\r\nOK\r\n") != 0) && (strcmp(Answer, "Synchronized\rOK\r\n") != 0) &&
        (strcmp(Answer, "Synchronized\nOK\r\n") != 0))
    {
        DebugPrintf(1, "No answer on 'Synchronized'\n");
        return (NO_ANSWER_SYNC);
    }

    DebugPrintf(3, "Synchronized 1\n");

    DebugPrintf(3, "Setting oscillator\n");

    sprintf(temp, "%s\n", IspEnvironment->StringOscillator);

    SendComPort(IspEnvironment, temp);

    ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 2, 1000);

    sprintf(temp, "%s\nOK\r\n", IspEnvironment->StringOscillator);

    if (strcmp(Answer, temp) != 0)
    {
        DebugPrintf(1, "No answer on Oscillator-Command\n");
        return (NO_ANSWER_OSC);
    }

    DebugPrintf(3, "Unlock\n");

    cmdstr = "U 23130\n";

    if (!SendAndVerify(IspEnvironment, cmdstr, Answer, sizeof Answer))
    {
        DebugPrintf(1, "Unlock-Command:\n");
        return (UNLOCK_ERROR + GetAndReportErrorNumber(Answer));
    }

    DebugPrintf(2, "Read bootcode version: ");

    cmdstr = "K\n";

    SendComPort(IspEnvironment, cmdstr);

    ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 4,5000);

    if (strncmp(Answer, cmdstr, strlen(cmdstr)) != 0)
    {
        DebugPrintf(1, "no answer on Read Boot Code Version\n");
        return (NO_ANSWER_RBV);
    }

    if (strncmp(Answer + strlen(cmdstr), "0\r\n", 3) == 0)
    {
        strippedAnswer = Answer + strlen(cmdstr) + 3;
        DebugPrintf(2, strippedAnswer);
    }
    else
    {
        DebugPrintf(2, "unknown\n");
    }

    DebugPrintf(2, "Read part ID: ");

    cmdstr = "J\n";

    SendComPort(IspEnvironment, cmdstr);

    ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 3,5000);

    if (strncmp(Answer, cmdstr, strlen(cmdstr)) != 0)
    {
        DebugPrintf(1, "no answer on Read Part Id\n");
        return (NO_ANSWER_RPID);
    }

    strippedAnswer = (strncmp(Answer, "J\n0\r\n", 5) == 0) ? Answer + 5 : Answer;

    Pos = strtoul(strippedAnswer, &endPtr, 10);
    *endPtr = '\0'; /* delete \r\n */
    for (i = sizeof LPCtypes / sizeof LPCtypes[0] - 1; i > 0 && LPCtypes[i].id != Pos; i--)
        /* nothing */;
        IspEnvironment->DetectedDevice = i;
    if (IspEnvironment->DetectedDevice == 0) {
        DebugPrintf(2, "unknown");
    }
    else {
        DebugPrintf(2, "LPC%d, %d kiB ROM / %d kiB SRAM",
            LPCtypes[IspEnvironment->DetectedDevice].Product,
            LPCtypes[IspEnvironment->DetectedDevice].FlashSize,
            LPCtypes[IspEnvironment->DetectedDevice].RAMSize);
    }
    DebugPrintf(2, " (0x%X)\n", Pos);//strippedAnswer);

    /* In case of a download to RAM, use full RAM for downloading
    * set the flash parameters to full RAM also.
    * This makes sure that all code is downloaded as one big sector
    */

    if (IspEnvironment->BinaryOffset >= ReturnValueLpcRamStart(IspEnvironment))
    {
        LPCtypes[IspEnvironment->DetectedDevice].FlashSectors = 1;
        LPCtypes[IspEnvironment->DetectedDevice].MaxCopySize  = LPCtypes[IspEnvironment->DetectedDevice].RAMSize*1024 - (ReturnValueLpcRamBase(IspEnvironment) - ReturnValueLpcRamStart(IspEnvironment));
        LPCtypes[IspEnvironment->DetectedDevice].SectorTable  = SectorTable_RAM;
        SectorTable_RAM[0] = LPCtypes[IspEnvironment->DetectedDevice].MaxCopySize;
    }
    if (IspEnvironment->DetectOnly)
        return (0);


    // Start with sector 1 and go upward... Sector 0 containing the interrupt vectors
    // will be loaded last, since it contains a checksum and device will re-enter
    // bootloader mode as long as this checksum is invalid.
    DebugPrintf(2, "Will start programming at Sector 1 if possible, and conclude with Sector 0 to ensure that checksum is written last.\n");
    if (LPCtypes[IspEnvironment->DetectedDevice].SectorTable[0] >= IspEnvironment->BinaryLength)
    {
        Sector = 0;
        SectorStart = 0;
    }
    else
    {
        SectorStart = LPCtypes[IspEnvironment->DetectedDevice].SectorTable[0];
        Sector = 1;
    }

    if (IspEnvironment->WipeDevice == 1)
    {
        DebugPrintf(2, "Wiping Device. ");

        sprintf(tmpString, "P %ld %ld\n", 0, LPCtypes[IspEnvironment->DetectedDevice].FlashSectors-1);

        if (!SendAndVerify(IspEnvironment, tmpString, Answer, sizeof Answer))
        {
            DebugPrintf(1, "Wrong answer on Prepare-Command\n");
            return (WRONG_ANSWER_PREP + GetAndReportErrorNumber(Answer));
        }

        sprintf(tmpString, "E %ld %ld\n", 0, LPCtypes[IspEnvironment->DetectedDevice].FlashSectors-1);

        if (!SendAndVerify(IspEnvironment, tmpString, Answer, sizeof Answer))
        {
            DebugPrintf(1, "Wrong answer on Erase-Command\n");
            return (WRONG_ANSWER_ERAS + GetAndReportErrorNumber(Answer));
        }
        DebugPrintf(2, "OK \n");
    }
    else{
        //no wiping requested: erasing sector 0 first
        DebugPrintf(2, "Erasing sector 0 first, to invalidate checksum. ");

        sprintf(tmpString, "P %ld %ld\n", 0, 0);

        if (!SendAndVerify(IspEnvironment, tmpString, Answer, sizeof Answer))
        {
            DebugPrintf(1, "Wrong answer on Prepare-Command\n");
            return (WRONG_ANSWER_PREP + GetAndReportErrorNumber(Answer));
        }

        sprintf(tmpString, "E %ld %ld\n", 0, 0);

        if (!SendAndVerify(IspEnvironment, tmpString, Answer, sizeof Answer))
        {
            DebugPrintf(1, "Wrong answer on Erase-Command\n");
            return (WRONG_ANSWER_ERAS + GetAndReportErrorNumber(Answer));
        }
        DebugPrintf(2, "OK \n");
    }
    while (1)
    {
        if (Sector >= LPCtypes[IspEnvironment->DetectedDevice].FlashSectors)
        {
            DebugPrintf(1, "Program too large; running out of Flash sectors.\n");
            return (PROGRAM_TOO_LARGE);
        }

        DebugPrintf(2, "Sector %ld: ", Sector);
        fflush(stdout);

        if (IspEnvironment->BinaryOffset < ReturnValueLpcRamStart(IspEnvironment)) // Skip Erase when running from RAM
        {
            sprintf(tmpString, "P %ld %ld\n", Sector, Sector);

            if (!SendAndVerify(IspEnvironment, tmpString, Answer, sizeof Answer))
            {
                DebugPrintf(1, "Wrong answer on Prepare-Command (1) (Sector %ld)\n", Sector);
                return (WRONG_ANSWER_PREP + GetAndReportErrorNumber(Answer));
            }

            DebugPrintf(2, ".");
            fflush(stdout);
            if (IspEnvironment->WipeDevice == 0 && (Sector!=0)) //Sector 0 already erased
            {
                sprintf(tmpString, "E %ld %ld\n", Sector, Sector);

                if (!SendAndVerify(IspEnvironment, tmpString, Answer, sizeof Answer))
                {
                    DebugPrintf(1, "Wrong answer on Erase-Command (Sector %ld)\n", Sector);
                    return (WRONG_ANSWER_ERAS + GetAndReportErrorNumber(Answer));
                }

                DebugPrintf(2, ".");
                fflush(stdout);
            }
        }

        SectorLength = LPCtypes[IspEnvironment->DetectedDevice].SectorTable[Sector];
        if (SectorLength > IspEnvironment->BinaryLength - SectorStart)
        {
            SectorLength = IspEnvironment->BinaryLength - SectorStart;
        }

        for (SectorOffset = 0; SectorOffset < SectorLength; SectorOffset += SectorChunk)
        {
            if (SectorOffset > 0)
            {
                // Add a visible marker between segments in a sector
                DebugPrintf(2, "|");  /* means: partial segment copied */
                fflush(stdout);
            }

            // If the Flash ROM sector size is bigger than the number of bytes
            // we can copy from RAM to Flash, we must "chop up" the sector and
            // copy these individually.
            // This is especially needed in the case where a Flash sector is
            // bigger than the amount of SRAM.
            SectorChunk = SectorLength - SectorOffset;
            if (SectorChunk > (unsigned)LPCtypes[IspEnvironment->DetectedDevice].MaxCopySize)
            {
                SectorChunk = LPCtypes[IspEnvironment->DetectedDevice].MaxCopySize;
            }

            // Write multiple of 45 * 4 Byte blocks to RAM, but copy maximum of on sector to Flash
            // In worst case we transfer up to 180 byte to much to RAM
            // but then we can always use full 45 byte blocks and length is multiple of 4
            CopyLength = SectorChunk;
            if ((CopyLength % (45 * 4)) != 0)
            {
                CopyLength += ((45 * 4) - (CopyLength % (45 * 4)));
            }

            sprintf(tmpString, "W %ld %ld\n", ReturnValueLpcRamBase(IspEnvironment), CopyLength);

            if (!SendAndVerify(IspEnvironment, tmpString, Answer, sizeof Answer))
            {
                DebugPrintf(1, "Wrong answer on Write-Command\n");
                return (WRONG_ANSWER_WRIT + GetAndReportErrorNumber(Answer));
            }

            DebugPrintf(2, ".");
            fflush(stdout);

            block_CRC = 0;
            Line = 0;

            // Transfer blocks of 45 * 4 bytes to RAM
            for (Pos = SectorStart + SectorOffset; (Pos < SectorStart + SectorOffset + CopyLength) && (Pos < IspEnvironment->BinaryLength); Pos += (45 * 4))
            {
                for (Block = 0; Block < 4; Block++)  // Each block 45 bytes
                {
                    DebugPrintf(2, ".");
                    fflush(stdout);

                    // inform the calling application about having written another chuck of data
                    func();

                    // Uuencode one 45 byte block
                    tmpStringPos = 0;

                    sendbuf[Line][tmpStringPos++] = (char)(' ' + 45);    // Encode Length of block

                    for (BlockOffset = 0; BlockOffset < 45; BlockOffset++)
                    {
                        if (IspEnvironment->BinaryOffset < ReturnValueLpcRamStart(IspEnvironment))
                        { // Flash: use full memory
                            c = IspEnvironment->BinaryContent[Pos + Block * 45 + BlockOffset];
                        }
                        else
                        { // RAM: Skip first 0x200 bytes, these are used by the download program in LPC21xx
                            c = IspEnvironment->BinaryContent[Pos + Block * 45 + BlockOffset + 0x200];
                        }

                        block_CRC += c;

                        k = (k << 8) + (c & 255);

                        if ((BlockOffset % 3) == 2)   // Collecting always 3 Bytes, then do processing in 4 Bytes
                        {
                            sendbuf[Line][tmpStringPos++] = uuencode_table[(k >> 18) & 63];
                            sendbuf[Line][tmpStringPos++] = uuencode_table[(k >> 12) & 63];
                            sendbuf[Line][tmpStringPos++] = uuencode_table[(k >>  6) & 63];
                            sendbuf[Line][tmpStringPos++] = uuencode_table[ k        & 63];
                        }
                    }


                    sendbuf[Line][tmpStringPos++] = '\n';
                    sendbuf[Line][tmpStringPos++] = 0;

                    SendComPort(IspEnvironment, sendbuf[Line]);
                    // receive only for debug proposes
                    ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 1,5000);

                    Line++;

                    DebugPrintf(3, "Line = %d\n", Line);

                    if (Line == 20)
                    {
                        for (repeat = 0; repeat < 3; repeat++)
                        {

                            // printf("block_CRC = %ld\n", block_CRC);

                            sprintf(tmpString, "%ld\n", block_CRC);

                            SendComPort(IspEnvironment, tmpString);

                            ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 2,5000);

                            sprintf(tmpString, "%ld\nOK\r\n", block_CRC);

                            if (strcmp(Answer, tmpString) != 0)
                            {
                                for (i = 0; i < Line; i++)
                                {
                                    SendComPort(IspEnvironment, sendbuf[i]);
                                    ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 1,5000);
                                }
                            }
                            else
                                break;
                        }

                        if (repeat >= 3)
                        {
                            DebugPrintf(1, "Error on writing block_CRC (1)\n");
                            return (ERROR_WRITE_CRC);
                        }
                        Line = 0;
                        block_CRC = 0;
                    }
                }
            }

            if (Line != 0)
            {
                for (repeat = 0; repeat < 3; repeat++)
                {
                    sprintf(tmpString, "%ld\n", block_CRC);

                    SendComPort(IspEnvironment, tmpString);

                    ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 2,5000);

                    sprintf(tmpString, "%ld\nOK\r\n", block_CRC);

                    if (strcmp(Answer, tmpString) != 0)
                    {
                        for (i = 0; i < Line; i++)
                        {
                            SendComPort(IspEnvironment, sendbuf[i]);
                            ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 1,5000);
                        }
                    }
                    else
                        break;
                }

                if (repeat >= 3)
                {
                    DebugPrintf(1, "Error on writing block_CRC (3)\n");
                    return (ERROR_WRITE_CRC2);
                }
            }

            if (IspEnvironment->BinaryOffset < ReturnValueLpcRamStart(IspEnvironment))
            {
                // Prepare command must be repeated before every write
                sprintf(tmpString, "P %ld %ld\n", Sector, Sector);

                if (!SendAndVerify(IspEnvironment, tmpString, Answer, sizeof Answer))
                {
                    DebugPrintf(1, "Wrong answer on Prepare-Command (2) (Sector %ld)\n", Sector);
                    return (WRONG_ANSWER_PREP2 + GetAndReportErrorNumber(Answer));
                }

                // Round CopyLength up to one of the following values: 512, 1024,
                // 4096, 8192; but do not exceed the maximum copy size (usually
                // 8192, but chip-dependent)
                if (CopyLength < 512)
                {
                    CopyLength = 512;
                }
                else if (SectorLength < 1024)
                {
                    CopyLength = 1024;
                }
                else if (SectorLength < 4096)
                {
                    CopyLength = 4096;
                }
                else
                {
                    CopyLength = 8192;
                }
                if (CopyLength > (unsigned)LPCtypes[IspEnvironment->DetectedDevice].MaxCopySize)
                {
                    CopyLength = LPCtypes[IspEnvironment->DetectedDevice].MaxCopySize;
                }

                sprintf(tmpString, "C %ld %ld %ld\n", SectorStart + SectorOffset, ReturnValueLpcRamBase(IspEnvironment), CopyLength);

                if (!SendAndVerify(IspEnvironment, tmpString, Answer, sizeof Answer))
                {
                    DebugPrintf(1, "Wrong answer on Copy-Command\n");
                    return (WRONG_ANSWER_COPY + GetAndReportErrorNumber(Answer));
                }

                if (IspEnvironment->Verify)
                {

                    //Avoid compare first 64 bytes.
                    //Because first 64 bytes are re-mapped to flash boot sector,
                    //and the compare result may not be correct.
                    if (SectorStart + SectorOffset<64)
                    {
                        sprintf(tmpString, "M %ld %ld %ld\n", 64, ReturnValueLpcRamBase(IspEnvironment) + (64 - SectorStart - SectorOffset), CopyLength-(64 - SectorStart - SectorOffset));
                    }
                    else
                    {
                        sprintf(tmpString, "M %ld %ld %ld\n", SectorStart + SectorOffset, ReturnValueLpcRamBase(IspEnvironment), CopyLength);
                    }

                    if (!SendAndVerify(IspEnvironment, tmpString, Answer, sizeof Answer))
                    {
                        DebugPrintf(1, "Wrong answer on Compare-Command\n");
                        return (WRONG_ANSWER_COPY + GetAndReportErrorNumber(Answer));
                    }
                }
            }
        }

        DebugPrintf(2, "\n");
        fflush(stdout);

        if ((SectorStart + SectorLength) >= IspEnvironment->BinaryLength && Sector!=0)
        {
            Sector = 0;
            SectorStart = 0;
        }
        else if (Sector == 0) {
            break;
        }
        else {
            SectorStart += LPCtypes[IspEnvironment->DetectedDevice].SectorTable[Sector];
            Sector++;
        }
    }

    tDoneUpload = time(NULL);
    if (IspEnvironment->Verify)
        DebugPrintf(2, "Download Finished and Verified correct... taking %d seconds\n", tDoneUpload - tStartUpload);
    else
        DebugPrintf(2, "Download Finished... taking %d seconds\n", tDoneUpload - tStartUpload);

    if (WaitForWatchDog)
    {
        DebugPrintf(2, "Wait for restart, in %d seconds from now\n", WatchDogSeconds - (tDoneUpload - tStartUpload));
    }
    else
    {
        DebugPrintf(2, "Now launching the brand new code\n");
        fflush(stdout);

        if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC2XXX)
        {
            sprintf(tmpString, "G %ld A\n", IspEnvironment->StartAddress);
        }
        else if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC17XX)
        {
            sprintf(tmpString, "G %ld T\n", IspEnvironment->StartAddress & ~1);
        }
        else
        {
            printf("Internal Error %s %s\n", __FILE__, __LINE__);
            exit(1);
        }

        SendComPort(IspEnvironment, tmpString); //goto 0 : run this fresh new downloaded code code
        if (IspEnvironment->BinaryOffset < ReturnValueLpcRamStart(IspEnvironment))
        { // Skip response on G command - show response on Terminal instead
            ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 2, 5000);
            /* the reply string is frequently terminated with a -1 (EOF) because the
            * connection gets broken; zero-terminate the string ourselves
            */
            while (realsize > 0 && ((signed char) Answer[(int)realsize - 1]) < 0)
                realsize--;
            Answer[(int)realsize] = '\0';
            /* Better to check only the first 9 chars instead of complete receive buffer,
            * because the answer can contain the output by the started programm
            */
            if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC2XXX)
            {
                sprintf(ExpectedAnswer, "G %ld A\n0", IspEnvironment->StartAddress);
            }
            else if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC17XX)
            {
                sprintf(ExpectedAnswer, "G %ld T\n0", IspEnvironment->StartAddress & ~1);
            }
            else
            {
                printf("Internal Error %s %s\n", __FILE__, __LINE__);
                exit(1);
            }

            if (realsize == 0 || strncmp((const char *)Answer, /*cmdstr*/ExpectedAnswer, strlen(/*cmdstr*/ExpectedAnswer)) != 0)
            {
                DebugPrintf(2, "Failed to run the new downloaded code: ");
                return (FAILED_RUN + GetAndReportErrorNumber(Answer));
            }
        }

        fflush(stdout);
    }
    return (0);
}
