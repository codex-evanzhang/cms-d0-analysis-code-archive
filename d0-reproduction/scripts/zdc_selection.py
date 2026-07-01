#!/usr/bin/env python3
"""Canonical ZDC signal and 0nXn selection for the D0 reproduction.

The nominal cut follows the AN-defined calibrated offline signal:

    ZDCsignal = HADsum + 0.1 * EMsum

with one-neutron thresholds of 1100 GeV on ZDC plus and 1000 GeV on
ZDC minus.  Existing reduced-tree or HiForest branches should already contain
that calibrated offline sum; use :func:`offline_signal` only when rebuilding
the sum from separate HAD/EM inputs.
"""

from __future__ import annotations

from dataclasses import dataclass
import math


EM_WEIGHT = 0.1
PLUS_1N_THRESHOLD_GEV = 1100.0
MINUS_1N_THRESHOLD_GEV = 1000.0
EM_PILEUP_SURVIVAL = 0.96
EM_PILEUP_PEAK_SURVIVAL = 0.92


@dataclass(frozen=True)
class ZdcDecision:
    pass_xn0n: bool
    pass_0nxn: bool

    @property
    def pass_xor(self) -> bool:
        return self.pass_xn0n != self.pass_0nxn

    @property
    def xn_side(self) -> str:
        if self.pass_xn0n:
            return "plus"
        if self.pass_0nxn:
            return "minus"
        return "none"


def finite(value: float) -> bool:
    return math.isfinite(float(value))


def offline_signal(had_sum: float, em_sum: float, em_weight: float = EM_WEIGHT) -> float:
    """Return the AN offline ZDC signal from calibrated HAD and EM sums."""
    return float(had_sum) + em_weight * float(em_sum)


def decide(
    zdc_plus: float,
    zdc_minus: float,
    plus_threshold: float = PLUS_1N_THRESHOLD_GEV,
    minus_threshold: float = MINUS_1N_THRESHOLD_GEV,
) -> ZdcDecision:
    """Classify the AN 0nXn/Xn0n XOR topology from calibrated ZDC sums."""
    if not (finite(zdc_plus) and finite(zdc_minus)):
        return ZdcDecision(False, False)
    plus = float(zdc_plus)
    minus = float(zdc_minus)
    return ZdcDecision(
        pass_xn0n=(plus > plus_threshold and minus < minus_threshold),
        pass_0nxn=(minus > minus_threshold and plus < plus_threshold),
    )


def pass_xor(zdc_plus: float, zdc_minus: float) -> bool:
    return decide(zdc_plus, zdc_minus).pass_xor


def pass_xn0n(zdc_plus: float, zdc_minus: float) -> bool:
    return decide(zdc_plus, zdc_minus).pass_xn0n


def pass_0nxn(zdc_plus: float, zdc_minus: float) -> bool:
    return decide(zdc_plus, zdc_minus).pass_0nxn
