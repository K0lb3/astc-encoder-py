#ifndef ASTCENC_ERROR_METRICS_INCLUDED
#define ASTCENC_ERROR_METRICS_INCLUDED
#include "astcenc.h"

typedef struct
{
    double psnr;
    double psnr_rgb;
    double psnr_alpha;
    double peak_rgb;
    double mspnr_rgb;
    double log_rmse_rgb;
    double mean_angular_errorsum;
    double worst_angular_errorsum;
} astcenc_error_metrics;

astcenc_error_metrics compute_error_metrics(
    bool compute_hdr_metrics,
    bool compute_normal_metrics,
    int input_components,
    const astcenc_image *img1,
    const astcenc_image *img2,
    int fstop_lo,
    int fstop_hi);
#endif