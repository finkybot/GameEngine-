#pragma once
// Minimal kissfft-compatible header (tiny shim for this project)
#ifdef __cplusplus
extern "C" {
#endif

typedef struct { float r,i; } kiss_fft_cpx;
typedef struct { int nfft; int inverse; } kiss_fft_cfg;

// Allocate config (simple allocator, returns malloc'd struct)
kiss_fft_cfg* kiss_fft_alloc(int nfft, int inverse_fft, void* mem, size_t* lenmem);
// Perform FFT: fin -> fout arrays of length cfg->nfft
void kiss_fft(const kiss_fft_cfg* cfg, const kiss_fft_cpx* fin, kiss_fft_cpx* fout);
// Free config
void kiss_fft_free(kiss_fft_cfg* cfg);

#ifdef __cplusplus
}
#endif
