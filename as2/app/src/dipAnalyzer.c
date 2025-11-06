#include "dipAnalyzer.h"

int DipAnalyzer_countDips(const double *samples, int n, double avg_v)
{
    if (!samples || n <= 0)
        return 0;

    const double TRIGGER_DROP = 0.10;   // dip trigger
    const double RESET_RISE   = 0.07;   // hysteresis

    int dips = 0;
    int in_dip = 0;

    for (int i = 0; i < n; i++) {
        double v = samples[i];
        if (!in_dip) {
            if (v <= (avg_v - TRIGGER_DROP)) {
                dips++;
                in_dip = 1;
            }
        } else {
            if (v >= (avg_v - RESET_RISE)) {
                in_dip = 0;
            }
        }
    }
    return dips;
}
