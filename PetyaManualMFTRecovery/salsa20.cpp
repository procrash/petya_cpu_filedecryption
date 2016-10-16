/**
Original: https://github.com/alexwebr/salsa20
*/
#include <stdint.h>
#include <stddef.h>
#include "salsa20.h"
#include <iostream>
#include  <iomanip>
#include "util.h"

using namespace std;

static uint32_t rotl(uint32_t value, int shift)
{
  return (value << shift) | (value >> (32 - shift));
}

static void s20_quarterround(uint32_t *y0, uint32_t *y1, uint32_t *y2, uint32_t *y3)
{
  *y1 = *y1 ^ rotl(*y0 + *y3, 7);
  *y2 = *y2 ^ rotl(*y1 + *y0, 9);
  *y3 = *y3 ^ rotl(*y2 + *y1, 13);
  *y0 = *y0 ^ rotl(*y3 + *y2, 18);
}

static void s20_rowround(uint32_t y[16])
{
  s20_quarterround(&y[0], &y[1], &y[2], &y[3]);
  s20_quarterround(&y[5], &y[6], &y[7], &y[4]);
  s20_quarterround(&y[10], &y[11], &y[8], &y[9]);
  s20_quarterround(&y[15], &y[12], &y[13], &y[14]);
}

static void s20_columnround(uint32_t x[16])
{
  s20_quarterround(&x[0], &x[4], &x[8], &x[12]);
  s20_quarterround(&x[5], &x[9], &x[13], &x[1]);
  s20_quarterround(&x[10], &x[14], &x[2], &x[6]);
  s20_quarterround(&x[15], &x[3], &x[7], &x[11]);
}

static void s20_doubleround(uint32_t x[16])
{
  s20_columnround(x);
  s20_rowround(x);
}

static int16_t s20_littleendian(uint8_t *b)
{
  return b[0] +
         (b[1] << 8);
}

static void s20_rev_littleendian(uint8_t *b, uint32_t w)
{
  b[0] = w;
  b[1] = w >> 8;
  b[2] = w >> 16;
  b[3] = w >> 24;
}

static void s20_hash(uint8_t seq[64])
{
  int i;
  uint32_t x[16];
  uint32_t z[16];

  for (i = 0; i < 16; ++i)
    x[i] = z[i] = s20_littleendian(seq + (4 * i));

  for (i = 0; i < 10; ++i)
    s20_doubleround(z);

  for (i = 0; i < 16; ++i) {
    z[i] += x[i];
    s20_rev_littleendian(seq + (4 * i), z[i]);
  }
}

static void s20_expand16(uint8_t *k,
                         uint8_t n[16],
                         uint8_t keystream[64])
{
  int i, j;
  uint8_t t[4][4] = {
    { 'e', 'x', 'p', 'a' },
    { 'n', 'd', ' ', '1' },
    { '6', '-', 'b', 'y' },
    { 't', 'e', ' ', 'k' }
  };

  for (i = 0; i < 64; i += 20)
    for (j = 0; j < 4; ++j)
      keystream[i + j] = t[i / 20][j];

  for (i = 0; i < 16; ++i) {
    keystream[4+i]  = k[i];
    keystream[44+i] = k[i];
    keystream[24+i] = n[i];
  }

  s20_hash(keystream);
}


uint8_t keystream[64];
uint8_t n[16] = { 0 };
uint8_t key[16];
uint64_t si;
uint64_t chiffreIndex = 0;

  
void initChiffre(uint8_t nonce[8], uint8_t *chiffreKey, uint32_t startIndex) {
    memset(keystream,0,64);
    
    for (int i = 0; i < 8; ++i)
    n[i] = nonce[i];

    for (int i = 0; i < 16; ++i)
    key[i] = chiffreKey[i];
    
    si = startIndex;
    chiffreIndex = 0;    
}




unsigned char getNextChiffreByte() {
  
  /*
  if (si % 64 != 0) {
    s20_rev_littleendian(n+8, si / 64);
    s20_expand16(key, n, keystream);
  }*/
  
  if ((si + chiffreIndex) % 64 == 0) {
      s20_rev_littleendian(n+8, ((si + chiffreIndex) / 64));
      s20_expand16(key, n, keystream);
  }

  unsigned char result = keystream[(si + chiffreIndex) % 64];
  chiffreIndex++;
  
  return result;
}


enum s20_status_t s20_crypt(uint8_t *key,
                            uint8_t nonce[8],
                            uint32_t si,
                            uint8_t *buf,
                            uint32_t buflen)
{
  uint8_t keystream[64];
  uint8_t n[16] = { 0 };
  uint32_t i;

  for (i = 0; i < 8; ++i)
    n[i] = nonce[i];

  if (si % 64 != 0) {
    s20_rev_littleendian(n+8, si / 64);
    s20_expand16(key, n, keystream);
  }

  for (i = 0; i < buflen; ++i) {
    if ((si + i) % 64 == 0) {
      s20_rev_littleendian(n+8, ((si + i) / 64));
      s20_expand16(key, n, keystream);
    }

    buf[i] ^= keystream[(si + i) % 64];
    
//     cout << setfill('0') << setw(2) << hex << (unsigned int)keystream[(si + i) % 64] <<" ";
  }

  return S20_SUCCESS;
}

enum s20_status_t s20_crypt_debug(uint8_t *key,
                            uint8_t nonce[8],
                            uint32_t si,
                            uint8_t *buf,
                            uint32_t buflen)
{
  uint8_t keystream[64];
  uint8_t n[16] = { 0 };
  uint32_t i;

  for (i = 0; i < 8; ++i)
    n[i] = nonce[i];

  n[0]=0xa7;
  n[1]=0xcb;
  n[2]=0x57;
  n[3]=0x94;
  n[4]=0xfa;
  n[5]=0xfc;
  n[6]=0x02;
  n[7]=0x72;
  
  n[8]=0x86;
  n[9]=0x02;
  n[10]=0x00;
  n[11]=0x00;
  n[12]=0x00;
  n[13]=0x00;
  n[14]=0x00;
  n[15]=0x00;
              
  s20_expand16(key, n, keystream);

    
  for (i = 0; i < buflen; ++i) {
  /*
    if ((si + i) % 64 == 0) {
      s20_rev_littleendian(n+8, ((si + i) / 64));
      s20_expand16(key, n, keystream);
    }
*/
    buf[i] ^= keystream[(si + i) % 64];
    
//     cout << setfill('0') << setw(2) << hex << (unsigned int)keystream[(si + i) % 64] <<" ";
  }

  return S20_SUCCESS;
}





void s20_crypt_sector(uint8_t *key,
                      uint8_t nonce[8],
                      uint8_t *buf,
                      uint32_t buflen,
                      uint32_t sector)
{
  uint8_t keystream[64];
  uint8_t n[16] = { 0 };
  uint32_t i;

  for (i = 0; i < 8; ++i)
    n[i] = nonce[i];

  if ((uint8_t) sector % 64!=0) {
    s20_rev_littleendian(n+8, sector / 64);
    s20_expand16(key, n, keystream);
    
  }

  uint8_t si = (uint8_t)sector;
  for (i = 0; i < buflen; ++i) {
    
    if ((si+i) % 64==0) {
      //  cout << "Generating new chiffre stream " << endl;
      s20_rev_littleendian(n+8, ((sector + i) / 64));
      s20_expand16(key, n, keystream);
    }

    // cout << "Index is " << (uint16_t)si << endl;
    buf[i] ^= keystream[(si+i)%64];
    
  }

}