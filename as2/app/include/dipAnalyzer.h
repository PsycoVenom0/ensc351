#pragma once
/**
 * dipAnalyzer.h
 *
 * Detects light dips in a sampled voltage signal.
 * Applies a hysteresis-based threshold method to count dips.
 *
 * Trigger: voltage falls below (average - 0.10 V)
 * Reset:   voltage rises above (average - 0.07 V)
 *
 * Returns the number of dips detected in the sample buffer.
 */

int DipAnalyzer_countDips(const double *samples, int n, double avg_v);
