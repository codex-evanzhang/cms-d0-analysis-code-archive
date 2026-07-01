#ifndef CODEX_D0_ZDC_SELECTION_H
#define CODEX_D0_ZDC_SELECTION_H

#include <cmath>

namespace d0zdc {

constexpr double kEmWeight = 0.1;
constexpr double kPlus1nThresholdGeV = 1100.0;
constexpr double kMinus1nThresholdGeV = 1000.0;
constexpr double kEmPileupSurvival = 0.96;
constexpr double kEmPileupPeakSurvival = 0.92;

inline bool Valid(double value) {
  return std::isfinite(value) && value > -9000.0;
}

inline double OfflineSignal(double hadSum, double emSum) {
  return hadSum + kEmWeight * emSum;
}

inline bool PassXn0n(double plusSignal, double minusSignal,
                     double plusThreshold = kPlus1nThresholdGeV,
                     double minusThreshold = kMinus1nThresholdGeV) {
  return Valid(plusSignal) && Valid(minusSignal) && plusSignal > plusThreshold && minusSignal < minusThreshold;
}

inline bool Pass0nXn(double plusSignal, double minusSignal,
                     double plusThreshold = kPlus1nThresholdGeV,
                     double minusThreshold = kMinus1nThresholdGeV) {
  return Valid(plusSignal) && Valid(minusSignal) && minusSignal > minusThreshold && plusSignal < plusThreshold;
}

inline bool PassXor0nXn(double plusSignal, double minusSignal,
                        double plusThreshold = kPlus1nThresholdGeV,
                        double minusThreshold = kMinus1nThresholdGeV) {
  return PassXn0n(plusSignal, minusSignal, plusThreshold, minusThreshold) !=
         Pass0nXn(plusSignal, minusSignal, plusThreshold, minusThreshold);
}

}  // namespace d0zdc

#endif
