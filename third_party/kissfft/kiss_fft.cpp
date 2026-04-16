#include "kiss_fft.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// Simple Cooley-Tukey radix-2 FFT operating on kiss_fft_cpx arrays
static void internal_fft(kiss_fft_cpx* data, int n) {
    if (n <= 1) return;
    // bit reverse
    int i, j = 0;
    for (i = 1; i < n; ++i) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            kiss_fft_cpx tmp = data[i]; data[i] = data[j]; data[j] = tmp;
        }
    }
    for (int len = 2; len <= n; len <<= 1) {
        float angle = -2.0f * 3.14159265358979323846f / (float)len;
        float cosv = cosf(angle);
        float sinv = sinf(angle);
        for (i = 0; i < n; i += len) {
            float wr = 1.0f, wi = 0.0f;
            for (j = 0; j < len/2; ++j) {
                // u = data[i+j]
                float ur = data[i+j].r;
                float ui = data[i+j].i;
                // v = data[i+j+len/2] * w
                float vr = data[i+j+len/2].r * wr - data[i+j+len/2].i * wi;
                float vi = data[i+j+len/2].r * wi + data[i+j+len/2].i * wr;
                // data[i+j] = u+v
                data[i+j].r = ur + vr;
                data[i+j].i = ui + vi;
                // data[i+j+len/2] = u-v
                data[i+j+len/2].r = ur - vr;
                data[i+j+len/2].i = ui - vi;
                // w *= wlen
                float nwr = wr * cosv - wi * sinv;
                float nwi = wr * sinv + wi * cosv;
                wr = nwr; wi = nwi;
            }
        }
    }
}

kiss_fft_cfg* kiss_fft_alloc(int nfft, int inverse_fft, void* mem, size_t* lenmem) {
    (void)mem; (void)lenmem;
    if (nfft <= 0) return NULL;
    // require power of two
    int p = 1; while (p < nfft) p <<= 1;
    kiss_fft_cfg* cfg = (kiss_fft_cfg*)malloc(sizeof(kiss_fft_cfg));
    if (!cfg) return NULL;
    cfg->nfft = p;
    cfg->inverse = inverse_fft ? 1 : 0;
    return cfg;
}

void kiss_fft_free(kiss_fft_cfg* cfg) {
    if (cfg) free(cfg);
}

void kiss_fft(const kiss_fft_cfg* cfg, const kiss_fft_cpx* fin, kiss_fft_cpx* fout) {
    if (!cfg || !fin || !fout) return;
    int n = cfg->nfft;
    // copy fin into scratch (ensure we operate on n samples)
    kiss_fft_cpx* buf = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * n);
    if (!buf) return;
    int i;
    for (i = 0; i < n; ++i) {
        if (i < cfg->nfft) buf[i] = fin[i]; else { buf[i].r = 0.0f; buf[i].i = 0.0f; }
    }
    internal_fft(buf, n);
    // copy to fout
    for (i = 0; i < n; ++i) fout[i] = buf[i];
    free(buf);
}

#ifdef __cplusplus
}
#endif
