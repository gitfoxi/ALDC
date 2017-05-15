
#include <stdio.h>
#include <string.h>
#include "aldc.h"

int round_trip(char *s)
{
  char *compressed;
  size_t compressed_len;
  char *decompressed;
  size_t decompressed_len;
  int r;

  r = EncodeAldcString(s, strlen(s), &compressed, &compressed_len);

  printf("Compressed to %lu bytes.\nCompressed Data:", compressed_len);
  /* fwrite(compressed, 1, compressed_len, stdout); */

  r = DecodeAldcString(compressed, compressed_len, &decompressed, &decompressed_len);
  free(compressed);

  printf("\nDecompressed to %lu bytes.\ndecompressed Data:", decompressed_len);
  fwrite(decompressed, 1, decompressed_len, stdout);

  return 0;
}

char big_s[99999999];
int main()
{

  round_trip( "I am a string.\n");
  round_trip( "");
  round_trip( "a");

  memset(big_s, 'a', 99999999);
  big_s[99999998] = 0;
  round_trip(big_s);


  return 0;
}
