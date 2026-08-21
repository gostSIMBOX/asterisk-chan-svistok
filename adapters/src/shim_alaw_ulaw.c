/*
 * Asterisk compatibility shim for chan_simbox
 * shim_alaw_ulaw.c - G.711 mu-law and A-law lookup tables
 */
#include <asterisk/ulaw.h>
#include <asterisk/alaw.h>

/* Standard G.711 mu-law compression table (14-bit linear signed -> 8-bit mu-law) */
unsigned char __ast_lin2mu[16384];

/* Standard G.711 mu-law decompression table (8-bit mu-law -> 16-bit linear signed) */
short __ast_mulaw[256];

/* Standard G.711 A-law compression table (13-bit linear signed -> 8-bit A-law) */
unsigned char __ast_lin2a[16384];

/* Standard G.711 A-law decompression table (8-bit A-law -> 16-bit linear signed) */
short __ast_alaw[256];

static int linear_to_ulaw(int sample)
{
    static const int exp_lut[256] = {
        0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
        4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
    };
    int sign = (sample >> 8) & 0x80;
    if (sign) sample = -sample;
    if (sample > 32767) sample = 32767;
    sample += 0x84;
    int exponent = exp_lut[(sample >> 7) & 0xFF];
    int mantissa = (sample >> (exponent + 3)) & 0x0F;
    unsigned char ulawbyte = ~(sign | (exponent << 4) | mantissa);
    return ulawbyte;
}

static int ulaw_to_linear(unsigned char u_val)
{
    u_val = ~u_val;
    int t = ((u_val & 0x0F) << 3) + 0x84;
    t <<= (u_val & 0x70) >> 4;
    return ((u_val & 0x80) ? (0x84 - t) : (t - 0x84));
}

static int linear_to_alaw(int sample)
{
    int sign = ((~sample) >> 8) & 0x80;
    if (!sign) sample = -sample;
    if (sample > 32767) sample = 32767;
    int exponent = 7;
    for (int expMask = 0x4000; (sample & expMask) == 0 && exponent > 0; expMask >>= 1) {
        exponent--;
    }
    int mantissa = (sample >> ((exponent == 0) ? 4 : (exponent + 3))) & 0x0F;
    unsigned char alawbyte = (sign | (exponent << 4) | mantissa) ^ 0xD5;
    return alawbyte;
}

static int alaw_to_linear(unsigned char a_val)
{
    a_val ^= 0xD5;
    int t = (a_val & 0x0F) << 4;
    int seg = (a_val & 0x70) >> 4;
    switch (seg) {
    case 0: t += 8; break;
    case 1: t += 0x108; break;
    default: t += 0x108; t <<= (seg - 1); break;
    }
    return ((a_val & 0x80) ? t : -t);
}

__attribute__((constructor))
static void init_g711_tables(void)
{
    for (int i = 0; i < 16384; i++) {
        short sample = (short)(i << 2);
        __ast_lin2mu[i] = (unsigned char)linear_to_ulaw(sample);
        __ast_lin2a[i] = (unsigned char)linear_to_alaw(sample);
    }
    for (int i = 0; i < 256; i++) {
        __ast_mulaw[i] = (short)ulaw_to_linear((unsigned char)i);
        __ast_alaw[i] = (short)alaw_to_linear((unsigned char)i);
    }
}
