
ALDC
=====

Adaptive Lossless Data Compression

aka: ALDC-0, IBM-LZ1, ECMA-222, QIC-154A, ISO/IEC 15200:1996

https://www.ecma-international.org/publications/files/ECMA-ST/Ecma-222.pdf
http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.133.99&rep=rep1&type=pdf
http://www.qic.org/html/standards/15x.x/qic154a.pdf
https://www.hotchips.org/wp-content/uploads/hc_archives/hc07/3_Tue/HC7.S5/HC7.5.3.pdf

Based on Michael Dipperstein's LZSS which is a great place to start -- Big thanks Michael! -- given that ALDC itself is based on LZSS.

http://michael.dipperstein.com/lzss/

[Michael's very intesting, original README is here.](../blob/master/README.orig)

Sadly, this could be faster. Using the brute-force method it encrypts at about 3MB/s on my 3.2 GHz i5, 2013 iMac. All of Michael's prior methods for improving speed fall over due to ALDC's 271-byte maximum-repeat length. The various algoriths seem optimized for a maximum-repeat length of 4.

One idea to make this faster is use SSE4.2 and an algorithm very similar to the hardware design described in "A fast hardware data compression algorithm and some algorithmic extensions".

The IBM chips like AHA3580 use parallel comparisons and a 512-bit bitset to achieve 80MB/s throughput. It seems almost exactly like what instructions like [PCMPESTRM](http://www.felixcloutier.com/x86/PCMPESTRM.html) were designed for.

I might get around to trying that.

If you need ALDC-1 or ALDC-2, it should be a simple matter of changing OFFSET_BITS to 10 or 11. But I haven't tried it. 


Build
------

    make


Example
---------

    cat /usr/share/dict/words | ./sample -c | ./sample -d > words.roundtrip


License
---------
Copyright (C) 2004-2014 by Michael Dipperstein
Copyright (C) Michael Fox 2017

GPL-3
