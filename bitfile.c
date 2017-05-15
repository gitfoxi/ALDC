/***************************************************************************
*                        Bit Stream File Implementation
*
*   File    : bitfile.c
*   Purpose : This file implements a simple library of I/O functions for
*             files that contain data in sizes that aren't integral bytes.
*             An attempt was made to make the functions in this library
*             analogous to functions provided to manipulate byte streams.
*             The functions contained in this library were created with
*             compression algorithms in mind, but may be suited to other
*             applications.
*   Author  : Michael Dipperstein
*   Date    : January 9, 2004
*
****************************************************************************
*
* Bitfile: Bit stream File I/O Routines
* Copyright (C) 2004-2014 by Michael Dipperstein (mdipper@alumni.cs.ucsb.edu)
*
* This file is part of the bit file library.
*
* The bit file library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 3 of the
* License, or (at your option) any later version.
*
* The bit file library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
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
#include <stdlib.h>
#include <errno.h>
#include "bitfile.h"

/***************************************************************************
*                            TYPE DEFINITIONS
***************************************************************************/

/***************************************************************************
* type to point to the kind of functions that put/get bits from/to numerical
* data types (short, int, long, ...)
* parameters: file pointer, data structure, number of bits, sizeof data.
***************************************************************************/
typedef int (*num_func_t)(bit_file_t*, void*, const unsigned int, const size_t);

struct bit_file_t
{
    FILE *fp;                   /* file pointer used by stdio functions */
    unsigned char bitBuffer;    /* bits waiting to be read/written */
    unsigned char bitCount;     /* number of bits in bitBuffer */
    BF_MODES mode;              /* open for read, write, or append */
};

typedef enum
{
    BF_UNKNOWN_ENDIAN,
    BF_LITTLE_ENDIAN,
    BF_BIG_ENDIAN
} endian_t;

/* union used to test for endianess */
typedef union
{
    unsigned long word;
    unsigned char bytes[sizeof(unsigned long)];
} endian_test_t;

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

/***************************************************************************
*   Function   : BitFileOpen
*   Description: This function opens a bit file for reading, writing,
*                or appending.  If successful, a bit_file_t data
*                structure will be allocated and a pointer to the
*                structure will be returned.
*   Parameters : fileName - NULL terminated string containing the name of
*                           the file to be opened.
*                mode - The mode of the file to be opened
*   Effects    : The specified file will be opened and file structure will
*                be allocated.
*   Returned   : Pointer to the bit_file_t structure for the bit file
*                opened, or NULL on failure.  errno will be set for all
*                failure cases.
***************************************************************************/
bit_file_t *BitFileOpen(const char *fileName, const BF_MODES mode)
{
    const char modes[3][3] = {"rb", "wb", "ab"};    /* binary modes for fopen */
    bit_file_t *bf;

    bf = (bit_file_t *)malloc(sizeof(bit_file_t));

    if (bf == NULL)
    {
        /* malloc failed */
        errno = ENOMEM;
    }
    else
    {
        bf->fp = fopen(fileName, modes[mode]);

        if (bf->fp == NULL)
        {
            /* fopen failed */
            free(bf);
            bf = NULL;
        }
        else
        {
            /* fopen succeeded fill in remaining bf data */
            bf->bitBuffer = 0;
            bf->bitCount = 0;
            bf->mode = mode;

            /* I don't care about endianness! */

            /***************************************************************
            * TO DO: Consider using the last byte in a file to indicate
            * the number of bits in the previous byte that actually have
            * data.  If I do that, I'll need special handling of files
            * opened with a mode of BF_APPEND.
            ***************************************************************/
        }
    }

    return (bf);
}

/***************************************************************************
*   Function   : MakeBitFile
*   Description: This function naively wraps a standard file in a
*                bit_file_t structure.  ANSI-C doesn't support file status
*                functions commonly found in other C variants, so the
*                caller must be passed as a parameter.
*   Parameters : stream - pointer to the standard file being wrapped.
*                mode - The mode of the file being wrapped.
*   Effects    : A bit_file_t structure will be created for the stream
*                passed as a parameter.
*   Returned   : Pointer to the bit_file_t structure for the bit file
*                or NULL on failure.  errno will be set for all failure
*                cases.
***************************************************************************/
bit_file_t *MakeBitFile(FILE *stream, const BF_MODES mode)
{
    bit_file_t *bf;

    if (stream == NULL)
    {
        /* can't wrapper empty steam */
        errno = EBADF;
        bf = NULL;
    }
    else
    {
        bf = (bit_file_t *)malloc(sizeof(bit_file_t));

        if (bf == NULL)
        {
            /* malloc failed */
            errno = ENOMEM;
        }
        else
        {
            /* set structure data */
            bf->fp = stream;
            bf->bitBuffer = 0;
            bf->bitCount = 0;
            bf->mode = mode;

            /* Repeat: I don't care about endianness! */
        }
    }

    return (bf);
}

/***************************************************************************
*   Function   : BitFileClose
*   Description: This function closes a bit file and frees all associated
*                data.
*   Parameters : stream - pointer to bit file stream being closed
*   Effects    : The specified file will be closed and the file structure
*                will be freed.
*   Returned   : 0 for success or EOF for failure.
***************************************************************************/
int BitFileClose(bit_file_t *stream)
{
    int returnValue = 0;

    if (stream == NULL)
    {
        return(EOF);
    }

    if ((stream->mode == BF_WRITE) || (stream->mode == BF_APPEND))
    {
        /* write out any unwritten bits */
        if (stream->bitCount != 0)
        {
            (stream->bitBuffer) <<= 8 - (stream->bitCount);
            fputc(stream->bitBuffer, stream->fp);   /* handle error? */
        }
    }

    /***********************************************************************
    *  TO DO: Consider writing an additional byte indicating the number of
    *  valid bits (bitCount) in the previous byte.
    ***********************************************************************/

    /* close file */
    returnValue = fclose(stream->fp);

    /* free memory allocated for bit file */
    free(stream);

    return(returnValue);
}

/***************************************************************************
*   Function   : BitFileToFILE
*   Description: This function flushes and frees the bitfile structure,
*                returning a pointer to a stdio file.
*   Parameters : stream - pointer to bit file stream being closed
*   Effects    : The specified bitfile will be made usable as a stdio
*                FILE.
*   Returned   : Pointer to FILE.  NULL for failure.
***************************************************************************/
FILE *BitFileToFILE(bit_file_t *stream)
{
    FILE *fp = NULL;

    if (stream == NULL)
    {
        return(NULL);
    }

    if ((stream->mode == BF_WRITE) || (stream->mode == BF_APPEND))
    {
        /* write out any unwritten bits */
        if (stream->bitCount != 0)
        {
            (stream->bitBuffer) <<= 8 - (stream->bitCount);
            fputc(stream->bitBuffer, stream->fp);   /* handle error? */
        }
    }

    /***********************************************************************
    *  TO DO: Consider writing an additional byte indicating the number of
    *  valid bits (bitCount) in the previous byte.
    ***********************************************************************/

    /* close file */
    fp = stream->fp;

    /* free memory allocated for bit file */
    free(stream);

    return(fp);
}

/***************************************************************************
*   Function   : BitFileByteAlign
*   Description: This function aligns the bitfile to the nearest byte.  For
*                output files, this means writing out the bit buffer with
*                extra bits set to 0.  For input files, this means flushing
*                the bit buffer.
*   Parameters : stream - pointer to bit file stream to align
*   Effects    : Flushes out the bit buffer.
*   Returned   : EOF if stream is NULL or write fails.  Writes return the
*                byte aligned contents of the bit buffer.  Reads returns
*                the unaligned contents of the bit buffer.
***************************************************************************/
int BitFileByteAlign(bit_file_t *stream)
{
    int returnValue;

    if (stream == NULL)
    {
        return(EOF);
    }

    returnValue = stream->bitBuffer;

    if ((stream->mode == BF_WRITE) || (stream->mode == BF_APPEND))
    {
        /* write out any unwritten bits */
        if (stream->bitCount != 0)
        {
            (stream->bitBuffer) <<= 8 - (stream->bitCount);
            fputc(stream->bitBuffer, stream->fp);   /* handle error? */
        }
    }

    stream->bitBuffer = 0;
    stream->bitCount = 0;

    return (returnValue);
}

/***************************************************************************
*   Function   : BitFileFlushOutput
*   Description: This function flushes the output bit buffer.  This means
*                left justifying any pending bits, and filling spare bits
*                with the fill value.
*   Parameters : stream - pointer to bit file stream to align
*                onesFill - non-zero if spare bits are filled with ones
*   Effects    : Flushes out the bit buffer, filling spare bits with ones
*                or zeros.
*   Returned   : EOF if stream is NULL or not writeable.  Otherwise, the
*                bit buffer value written. -1 if no data was written.
***************************************************************************/
int BitFileFlushOutput(bit_file_t *stream, const unsigned char onesFill)
{
    int returnValue;

    if (stream == NULL)
    {
        return(EOF);
    }

    returnValue = -1;

    /* write out any unwritten bits */
    if (stream->bitCount != 0)
    {
        stream->bitBuffer <<= (8 - stream->bitCount);

        if (onesFill)
        {
            stream->bitBuffer |= (0xFF >> stream->bitCount);
        }

        returnValue = fputc(stream->bitBuffer, stream->fp);
    }

    stream->bitBuffer = 0;
    stream->bitCount = 0;

    return (returnValue);
}

/***************************************************************************
*   Function   : BitFileGetChar
*   Description: This function returns the next byte from the file passed as
*                a parameter.
*   Parameters : stream - pointer to bit file stream to read from
*   Effects    : Reads next byte from file and updates buffer accordingly.
*   Returned   : EOF if a whole byte cannot be obtained.  Otherwise,
*                the character read.
***************************************************************************/
int BitFileGetChar(bit_file_t *stream)
{
    int returnValue;
    unsigned char tmp;

    if (stream == NULL)
    {
        return(EOF);
    }

    returnValue = fgetc(stream->fp);

    if (stream->bitCount == 0)
    {
        /* we can just get byte from file */
        return returnValue;
    }

    /* we have some buffered bits to return too */
    if (returnValue != EOF)
    {
        /* figure out what to return */
        tmp = ((unsigned char)returnValue) >> (stream->bitCount);
        tmp |= ((stream->bitBuffer) << (8 - (stream->bitCount)));

        /* put remaining in buffer. count shouldn't change. */
        stream->bitBuffer = returnValue;

        returnValue = tmp;
    }

    return returnValue;
}

/***************************************************************************
*   Function   : BitFilePutChar
*   Description: This function writes the byte passed as a parameter to the
*                file passed a parameter.
*   Parameters : c - the character to be written
*                stream - pointer to bit file stream to write to
*   Effects    : Writes a byte to the file and updates buffer accordingly.
*   Returned   : On success, the character written, otherwise EOF.
***************************************************************************/
int BitFilePutChar(const int c, bit_file_t *stream)
{
    unsigned char tmp;
    /* fprintf(stderr,"PutChar: 0x%x\n", c); */

    if (stream == NULL)
    {
        return(EOF);
    }

    if (stream->bitCount == 0)
    {
        /* we can just put byte from file */
        return fputc(c, stream->fp);
    }

    /* figure out what to write */
    tmp = ((unsigned char)c) >> (stream->bitCount);
    tmp = tmp | ((stream->bitBuffer) << (8 - stream->bitCount));

    if (fputc(tmp, stream->fp) != EOF)
    {
        /* put remaining in buffer. count shouldn't change. */
        stream->bitBuffer = c;
    }
    else
    {
        return EOF;
    }

    return tmp;
}

/***************************************************************************
*   Function   : BitFileGetBit
*   Description: This function returns the next bit from the file passed as
*                a parameter.  The bit value returned is the msb in the
*                bit buffer.
*   Parameters : stream - pointer to bit file stream to read from
*   Effects    : Reads next bit from bit buffer.  If the buffer is empty,
*                a new byte will be read from the file.
*   Returned   : 0 if bit == 0, 1 if bit == 1, and EOF if operation fails.
***************************************************************************/
int BitFileGetBit(bit_file_t *stream)
{
    int returnValue;

    if (stream == NULL)
    {
        return(EOF);
    }

    if (stream->bitCount == 0)
    {
        /* buffer is empty, read another character */
        if ((returnValue = fgetc(stream->fp)) == EOF)
        {
            return EOF;
        }
        else
        {
            stream->bitCount = 8;
            stream->bitBuffer = returnValue;
        }
    }

    /* bit to return is msb in buffer */
    stream->bitCount--;
    returnValue = (stream->bitBuffer) >> (stream->bitCount);

    return (returnValue & 0x01);
}

/***************************************************************************
*   Function   : BitFilePutBit
*   Description: This function writes the bit passed as a parameter to the
*                file passed a parameter.
*   Parameters : c - the bit value to be written
*                stream - pointer to bit file stream to write to
*   Effects    : Writes a bit to the bit buffer.  If the buffer has a byte,
*                the buffer is written to the file and cleared.
*   Returned   : On success, the bit value written, otherwise EOF.
***************************************************************************/
int BitFilePutBit(const int c, bit_file_t *stream)
{
    int returnValue = c;
    /* fprintf(stderr,"PutBit: %d\n", c); */

    if (stream == NULL)
    {
        return(EOF);
    }

    stream->bitCount++;
    stream->bitBuffer <<= 1;

    if (c != 0)
    {
        stream->bitBuffer |= 1;
    }

    /* write bit buffer if we have 8 bits */
    if (stream->bitCount == 8)
    {
        if (fputc(stream->bitBuffer, stream->fp) == EOF)
        {
            returnValue = EOF;
        }

        /* reset buffer */
        stream->bitCount = 0;
        stream->bitBuffer = 0;
    }

    return returnValue;
}

/***************************************************************************
*   Function   : BitFileGetBits
*   Description: This function reads the specified number of bits from the
*                file passed as a parameter and writes them to the
*                requested memory location (msb to lsb).
*   Parameters : stream - pointer to bit file stream to read from
*                bits - address to store bits read
*                count - number of bits to read
*   Effects    : Reads bits from the bit buffer and file stream.  The bit
*                buffer will be modified as necessary.
*   Returned   : EOF for failure, otherwise the number of bits read.  If
*                an EOF is reached before all the bits are read, bits
*                will contain every bit through the last complete byte.
***************************************************************************/
int BitFileGetBits(bit_file_t *stream, unsigned int *bits, const unsigned int count)
{
    int offset, remaining, returnValue;

    if ((stream == NULL) || (bits == NULL))
    {
        return(EOF);
    }

    offset = 0;
    remaining = count;
    *bits = 0;

    /* read whole bytes */
    while (remaining >= 8)
    {
        returnValue = BitFileGetChar(stream);

        if (returnValue == EOF)
        {
            return EOF;
        }

        *bits = (*bits << 8) | (unsigned char)returnValue;
        remaining -= 8;
        offset++;
    }

    while (remaining > 0)
    {
        returnValue = BitFileGetBit(stream);

        if (returnValue == EOF)
        {
            return EOF;
        }

        *bits = (*bits << 1) | (returnValue & 0x01);
        remaining--;
    }

    return count;
}

/***************************************************************************
*   Function   : BitFilePutBits
*   Description: This function writes the specified number of bits from the
*                memory location passed as a parameter to the file passed
*                as a parameter.   Bits are written msb to lsb.
*   Parameters : stream - pointer to bit file stream to write to
*                bits - pointer to bits to write
*                count - number of bits to write
*   Effects    : Writes bits to the bit buffer and file stream.  The bit
*                buffer will be modified as necessary.
*   Returned   : EOF for failure, otherwise the number of bits written.  If
*                an error occurs after a partial write, the partially
*                written bits will not be unwritten.
***************************************************************************/
int BitFilePutBits(bit_file_t *stream, unsigned int bits, const unsigned int count)
{
    unsigned int tmp;
    int offset, remaining, returnValue;

    if (stream == NULL)
    {
        return(EOF);
    }

    remaining = count;
    offset = (remaining + 1)/8;
    tmp = (bits >> (offset * 8)) & 0xFF;

    while (remaining % 8 != 0)
      {
        returnValue = BitFilePutBit((tmp >> ((remaining % 8) - 1)) & 1, stream);

            if (returnValue == EOF)
              {
                return EOF;
              }

            remaining--;
      }


    /* write whole bytes */
    while (remaining >= 8)
    {
      returnValue = BitFilePutChar(bits >> ((remaining/8 )  - 1) & 0xFF, stream);

        if (returnValue == EOF)
        {
            return EOF;
        }

        remaining -= 8;
    }

    return count;
}
