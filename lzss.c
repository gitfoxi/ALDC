/***************************************************************************
*                 Lempel, Ziv, Storer, and Szymanski Encoding
*
*   File    : lzss.c
*   Purpose : Use lzss coding (Storer and Szymanski's modified LZ77) to
*             compress lzss data files.
*   Author  : Michael Dipperstein
*   Date    : November 28, 2014
*
****************************************************************************
*
* LZss: An ANSI C LZSS Encoding/Decoding Routines
* Copyright (C) 2003 - 2007, 2014 by
* Michael Dipperstein (mdipper@alumni.engr.ucsb.edu)
*
* This file is part of the lzss library.
*
* The lzss library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 3 of the
* License, or (at your option) any later version.
*
* The lzss library is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
* General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************/

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "lzlocal.h"
#include "bitfile.h"

/***************************************************************************
*                            TYPE DEFINITIONS
***************************************************************************/

/***************************************************************************
*                                CONSTANTS
***************************************************************************/

/***************************************************************************
*                            GLOBAL VARIABLES
***************************************************************************/
/* cyclic buffer sliding window of already read characters */
unsigned char slidingWindow[WINDOW_SIZE];
unsigned char uncodedLookahead[MAX_CODED];

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

typedef struct
{
  unsigned int code;
  unsigned int bits;
}length_code_t;

length_code_t lengthCode(unsigned int length)
{
  if(length < 4) {
    length_code_t r;
    r.code = length - 2;
    r.bits   = 2;
    return r;
  }
  else if(length < 8) {
    length_code_t r;
    r.code = 0x8 | length - 4;
    r.bits   = 4;
    return r;
  }
  else if(length < 16) {
    length_code_t r;
    r.code =0x30 | length - 8;
    r.bits   = 6;
    return r;
  }
  else if(length < 32) {
    length_code_t r;
    r.code = 0xE0 | length - 16;
    r.bits   = 8;
    return r;
  }
  else {
    length_code_t r;
    r.code   = 0xF00 | length - 32;
    r.bits   = 12;
    return r;
  }
}

/****************************************************************************
*   Function   : EncodeLZSS
*   Description: This function will read an input file and write an output
*                file encoded according to the traditional LZSS algorithm.
*                This algorithm encodes strings as 16 bits (a 12 bit offset
*                + a 4 bit length).
*   Parameters : fpIn - pointer to the open binary file to encode
*                fpOut - pointer to the open binary file to write encoded
*                       output
*   Effects    : fpIn is encoded and written to fpOut.  Neither file is
*                closed after exit.
*   Returned   : 0 for success, -1 for failure.  errno will be set in the
*                event of a failure.
****************************************************************************/
int EncodeLZSS(FILE *fpIn, FILE *fpOut)
{
    bit_file_t *bfpOut;
    encoded_string_t matchData;
    int c;
    unsigned int i;
    unsigned int len;                       /* length of string */

    /* head of sliding window and lookahead */
    unsigned int windowHead, uncodedHead;

    /* validate arguments */
    if ((NULL == fpIn) || (NULL == fpOut))
    {
        errno = ENOENT;
        return -1;
    }

    /* convert output file to bitfile */
    bfpOut = MakeBitFile(fpOut, BF_WRITE);

    if (NULL == bfpOut)
    {
        perror("Making Output File a BitFile");
        return -1;
    }

    windowHead = 0;
    uncodedHead = 0;

    /************************************************************************
    * Fill the sliding window buffer with some known vales.  DecodeLZSS must
    * use the same values.  If common characters are used, there's an
    * increased chance of matching to the earlier strings.
    ************************************************************************/
    memset(slidingWindow, 0, WINDOW_SIZE * sizeof(unsigned char));

    /************************************************************************
    * Copy MAX_CODED bytes from the input file into the uncoded lookahead
    * buffer.
    ************************************************************************/
    for (len = 0; len < MAX_CODED && (c = getc(fpIn)) != EOF; len++)
    {
        uncodedLookahead[len] = c;
    }

    if (0 == len)
    {
        return 0;   /* inFile was empty */
    }

    /* Look for matching string in sliding window */
    i = InitializeSearchStructures();

    if (0 != i)
    {
        return i;       /* InitializeSearchStructures returned an error */
    }

    matchData = FindMatch(windowHead, uncodedHead);

    /* now encode the rest of the file until an EOF is read */
    while (len > 0)
    {
        if (matchData.length > len)
        {
            /* garbage beyond last data happened to extend match length */
            matchData.length = len;
        }
        fprintf(stderr, "matchData: offset %d length %d\n", matchData.offset, matchData.length);

        if (matchData.length <= MAX_UNCODED)
        {
            /* not long enough match.  write uncoded flag and character */
            BitFilePutBit(UNCODED, bfpOut);
            BitFilePutChar(uncodedLookahead[uncodedHead], bfpOut);
            fprintf(stderr,"Put literal: %c\n", uncodedLookahead[uncodedHead]);

            matchData.length = 1;   /* set to 1 for 1 byte uncoded */
        }
        else
        {
          length_code_t length_code = lengthCode(matchData.length);

            /* match length > MAX_UNCODED.  Encode as offset and length. */
            BitFilePutBit(ENCODED, bfpOut);
            if (length_code.bits > 8) {
              BitFilePutBitsNum(bfpOut, &( length_code.code) - 1, length_code.bits - 8,
                                sizeof(unsigned int));
              BitFilePutBitsNum(bfpOut, &( length_code.code ),  8,
                                sizeof(unsigned int));
            } else
              {
                BitFilePutBitsNum(bfpOut, &( length_code.code ), length_code.bits,
                                  sizeof(unsigned int));
              }
            fprintf(stderr,"Put length: 0x%x, %d bits\n", length_code.code, length_code.bits);
            /* ALDC calls this displacement - hope it means the same thing */
            BitFilePutBitsNum(bfpOut, &matchData.offset, OFFSET_BITS,
                sizeof(unsigned int));
            fprintf(stderr,"Put offset: %d\n", matchData.offset);
        }

        /********************************************************************
        * Replace the matchData.length worth of bytes we've matched in the
        * sliding window with new bytes from the input file.
        ********************************************************************/
        i = 0;
        while ((i < matchData.length) && ((c = getc(fpIn)) != EOF))
        {
            /* add old byte into sliding window and new into lookahead */
            ReplaceChar(windowHead, uncodedLookahead[uncodedHead]);
            uncodedLookahead[uncodedHead] = c;
            windowHead = Wrap((windowHead + 1), WINDOW_SIZE);
            uncodedHead = Wrap((uncodedHead + 1), MAX_CODED);
            i++;
        }

        /* handle case where we hit EOF before filling lookahead */
        while (i < matchData.length)
        {
            ReplaceChar(windowHead, uncodedLookahead[uncodedHead]);
            /* nothing to add to lookahead here */
            windowHead = Wrap((windowHead + 1), WINDOW_SIZE);
            uncodedHead = Wrap((uncodedHead + 1), MAX_CODED);
            len--;
            i++;
        }

        /* find match for the remaining characters */
        matchData = FindMatch(windowHead, uncodedHead);
    }

    {
      /* ALDC end marker */
      unsigned int end_marker = 0x1FFF;
      /* ALDC end marker */
      BitFilePutBitsNum(bfpOut, &end_marker, 13,
                        sizeof(unsigned int));
    }

    /* we've encoded everything, free bitfile structure */
    BitFileToFILE(bfpOut);

   return 0;
}

/****************************************************************************
*   Function   : DecodeLZSSByFile
*   Description: This function will read an LZSS encoded input file and
*                write an output file.  This algorithm encodes strings as 16
*                bits (a 12 bit offset + a 4 bit length).
*   Parameters : fpIn - pointer to the open binary file to decode
*                fpOut - pointer to the open binary file to write decoded
*                       output
*   Effects    : fpIn is decoded and written to fpOut.  Neither file is
*                closed after exit.
*   Returned   : 0 for success, -1 for failure.  errno will be set in the
*                event of a failure.
****************************************************************************/
int DecodeLZSS(FILE *fpIn, FILE *fpOut)
{
    bit_file_t *bfpIn;
    int c;
    unsigned int i, nextChar;
    encoded_string_t code;              /* offset/length code for string */

    /* use stdin if no input file */
    if ((NULL == fpIn) || (NULL == fpOut))
    {
        errno = ENOENT;
        return -1;
    }

    /* convert input file to bitfile */
    bfpIn = MakeBitFile(fpIn, BF_READ);

    if (NULL == bfpIn)
    {
        perror("Making Input File a BitFile");
        return -1;
    }

    /************************************************************************
    * Fill the sliding window buffer with some known vales.  EncodeLZSS must
    * use the same values.  If common characters are used, there's an
    * increased chance of matching to the earlier strings.
    ************************************************************************/
    memset(slidingWindow, ' ', WINDOW_SIZE * sizeof(unsigned char));

    nextChar = 0;

 while (1)
    {
        if ((c = BitFileGetBit(bfpIn)) == EOF)
        {
            /* we hit the EOF */
            break;
        }

        if (c == UNCODED)
        {
            /* uncoded character */
            if ((c = BitFileGetChar(bfpIn)) == EOF)
            {
                break;
            }

            /* write out byte and put it in sliding window */
            putc(c, fpOut);
            slidingWindow[nextChar] = c;
            nextChar = Wrap((nextChar + 1), WINDOW_SIZE);
        }
        else
        {
          unsigned int length_bits = 0;
          unsigned int prefix = 0;
          int bit = 0;
            /* offset and length */
            code.offset = 0;
            code.length = 0;

        for(i = 0; i < 4; i++)
              {
                if ((bit = BitFileGetBit(bfpIn)) == EOF)
                  {
                    goto BREAK_OUTER;
                  }
                fprintf(stderr, "bit: %d\n", bit);
                if (bit == 1) { prefix++; }
                else { goto BREAK_INNER; }
              }
        BREAK_INNER:
            if (prefix == 0) { length_bits = 1; }
            else if (prefix == 1) { length_bits = 2; }
            else if (prefix == 2) { length_bits = 3; }
            else if (prefix == 3) { length_bits = 4; }
            else /* if (prefix == 4) */ { length_bits = 8; }


            if ((BitFileGetBitsNum(bfpIn, &code.length, length_bits,
                                   sizeof(unsigned int))) == EOF)
              {
                break;
              }

            fprintf(stderr, "READ code.length: %d\n", code.length);
            if (code.length == 0xFF) { break ; } /* end code 0xFFF */

            if (prefix == 0) { code.length += 2; }
            else if (prefix == 1) { code.length += 4; }
            else if (prefix == 2) { code.length += 8; }
            else if (prefix == 3) { code.length += 16; }
            else if (prefix == 4) { code.length += 32; }

            if ((BitFileGetBitsNum(bfpIn, &code.offset, OFFSET_BITS,
                sizeof(unsigned int))) == EOF)
            {
                break;
            }

            fprintf(stderr, "prefix length_bits code.length code.offset slidingWindow\n");
            fprintf(stderr, "%d %d %d %d %s\n"
                    , prefix, length_bits, code.length, code.offset, slidingWindow
                    );

            /****************************************************************
            * Write out decoded string to file and lookahead.  It would be
            * nice to write to the sliding window instead of the lookahead,
            * but we could end up overwriting the matching string with the
            * new string if abs(offset - next char) < match length.
            ****************************************************************/
            for (i = 0; i < code.length; i++)
            {
                c = slidingWindow[Wrap((code.offset + i), WINDOW_SIZE)];
                putc(c, fpOut);
                uncodedLookahead[i] = c;
            }

            /* write out decoded string to sliding window */
            for (i = 0; i < code.length; i++)
            {
                slidingWindow[Wrap((nextChar + i), WINDOW_SIZE)] =
                    uncodedLookahead[i];
            }

            nextChar = Wrap((nextChar + code.length), WINDOW_SIZE);
        }
    }
 BREAK_OUTER:
    /* we've decoded everything, free bitfile structure */
    BitFileToFILE(bfpIn);

    return 0;
}
