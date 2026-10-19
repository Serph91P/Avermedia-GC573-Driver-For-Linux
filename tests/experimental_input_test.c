// SPDX-License-Identifier: GPL-2.0-only
/* Synthetic timings, not a captured monitor EDID or hardware evidence. */
#include <assert.h>
#include <limits.h>
#include <linux/errno.h>
#include <stdio.h>
#include <string.h>
#include "gc573_modes.h"
#include "gc573_passthrough.h"

static void checksum(unsigned char *p)
{
    unsigned int i, sum = 0;
    for (i = 0; i < 127; i++) sum += p[i];
    p[127] = -sum;
}
static void fixture(unsigned char e[256], unsigned int rate)
{
    unsigned int clock = 2720U * 1480U / 10U * rate / 1000U;
    memset(e, 0, 256);
    memset(e + 1, 255, 6);
    e[126] = 1;
    e[54] = clock; e[55] = clock >> 8;
    e[56] = 0; e[57] = 160; e[58] = 0xa0; /* 2560 + 160 */
    e[59] = 160; e[60] = 40; e[61] = 0x50; /* 1440 + 40 */
    e[71] = 0x1a;
    {
        const unsigned char c[] = {2,3,20,0,0x67,3,12,0,0x10,0,0,120,
            0x67,0xd8,0x5d,0xc4,1,120,0x80,0};
        memcpy(e + 128, c, sizeof(c));
    }
    checksum(e); checksum(e + 128);
}
int main(void)
{
    unsigned char e[256];
    struct gc573_passthrough_edid out;
    assert(gc573_capture_profile_max_fps(2560,1440,16000,0) == 120);
    assert(gc573_capture_profile_max_fps(2560,1440,16000,1) == 60);
    assert(!gc573_capture_profile_max_fps(2560,1440,15999,1));
    assert(gc573_capture_profile_max_fps(1920,1080,8000,1) == 60);
    assert(!gc573_capture_profile_max_fps(3840,2160,32000,1));
    assert(gc573_capture_rate(144,gc573_capture_profile_max_fps(2560,1440,16000,1)) == 60);
    assert(gc573_input_period_supported(1920,1080,1666667,0));
    assert(gc573_input_period_supported(2560,1440,833333,0));
    assert(!gc573_input_period_supported(2560,1440,694444,0));
    assert(gc573_input_period_supported(2560,1440,694444,1));
    assert(gc573_input_period_supported(2560,1440,689656,1));
    assert(gc573_input_period_supported(2560,1440,699300,1));
    assert(gc573_input_period_supported(2560,1440,100000000U / 145,1));
    assert(gc573_input_period_supported(2560,1440,100000000U / 143,1));
    assert(!gc573_input_period_supported(2560,1440,689654,1));
    assert(!gc573_input_period_supported(2560,1440,699301,1));
    assert(!gc573_input_period_supported(1920,1080,694444,1));
    assert(!gc573_input_period_supported(3840,2160,694444,1));
    assert(!gc573_input_period_supported(2560,1440,0,1));
    assert(!gc573_input_period_supported(2560,1440,UINT_MAX,1));
    fixture(e,120);
    assert(!gc573_scaled_edid_profile(e,256,&out,0));
    fixture(e,144);
    assert(gc573_scaled_edid_profile(e,256,&out,0) == -EOPNOTSUPP);
    assert(!gc573_scaled_edid_profile(e,256,&out,1));
    assert(out.timings == 1 && out.max_tmds_khz == 600000 && out.scdc);
    assert(out.data[131] == 0); /* no HDR/YUV/VRR promises */
    fixture(e,145);
    assert(gc573_scaled_edid_profile(e,256,&out,1) == -EOPNOTSUPP);
    fixture(e,144); e[71] |= 0x80; checksum(e);
    assert(gc573_scaled_edid_profile(e,256,&out,1) == -EOPNOTSUPP);
    fixture(e,144); e[146] = 0; checksum(e + 128);
    assert(gc573_scaled_edid_profile(e,256,&out,1) == -EOPNOTSUPP);
    puts("PASS: opt-in 1440p input timing/EDID bounds, <=60 capture, legacy defaults unchanged");
    return 0;
}
