#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include "TFile.h"
#include "TTree.h"

namespace {
constexpr int kMaxMod = 64;
constexpr int kNTs = 6;

struct RechitStats {
  long long entries = 0;
  long long nonzero = 0;
  double sumPlus = 0.0;
  double sumMinus = 0.0;
  double maxPlus = 0.0;
  double maxMinus = 0.0;
};

struct DigiStats {
  long long entries = 0;
  long long nonempty = 0;
  long long adcAbovePedestal = 0;
  long long chargePulseLike = 0;
  double maxChargePlus[kNTs] = {};
  double maxChargeMinus[kNTs] = {};
  double maxAdcPlus[kNTs] = {};
  double maxAdcMinus[kNTs] = {};
  double maxDelta21Plus = -1e99;
  double maxDelta21Minus = -1e99;
};

void PrintLine(std::ostream &out, const std::string &label, const RechitStats &rh,
               const DigiStats &dg) {
  out << std::setprecision(8)
      << label << '\t'
      << rh.entries << '\t'
      << rh.nonzero << '\t'
      << rh.maxPlus << '\t'
      << rh.maxMinus << '\t'
      << (rh.entries ? rh.sumPlus / rh.entries : 0.0) << '\t'
      << (rh.entries ? rh.sumMinus / rh.entries : 0.0) << '\t'
      << dg.entries << '\t'
      << dg.nonempty << '\t'
      << dg.adcAbovePedestal << '\t'
      << dg.chargePulseLike << '\t'
      << dg.maxDelta21Plus << '\t'
      << dg.maxDelta21Minus;
  for (int ts = 0; ts < kNTs; ++ts) out << '\t' << dg.maxChargePlus[ts];
  for (int ts = 0; ts < kNTs; ++ts) out << '\t' << dg.maxChargeMinus[ts];
  for (int ts = 0; ts < kNTs; ++ts) out << '\t' << dg.maxAdcPlus[ts];
  for (int ts = 0; ts < kNTs; ++ts) out << '\t' << dg.maxAdcMinus[ts];
  out << '\n';
}

void PrintHeader(std::ostream &out) {
  out << "label"
      << "\trechit_entries"
      << "\trechit_nonzero_events"
      << "\trechit_max_sumPlus"
      << "\trechit_max_sumMinus"
      << "\trechit_mean_sumPlus"
      << "\trechit_mean_sumMinus"
      << "\tdigi_entries"
      << "\tdigi_nonempty_events"
      << "\tdigi_events_adc_gt_10"
      << "\tdigi_events_ts2_minus_ts1_gt_10fc"
      << "\tdigi_max_ts2_minus_ts1_plus"
      << "\tdigi_max_ts2_minus_ts1_minus";
  for (int ts = 0; ts < kNTs; ++ts) out << "\tdigi_max_charge_plus_ts" << ts;
  for (int ts = 0; ts < kNTs; ++ts) out << "\tdigi_max_charge_minus_ts" << ts;
  for (int ts = 0; ts < kNTs; ++ts) out << "\tdigi_max_adc_plus_ts" << ts;
  for (int ts = 0; ts < kNTs; ++ts) out << "\tdigi_max_adc_minus_ts" << ts;
  out << '\n';
}
}  // namespace

void summarize_zdc_forest(const char *path, const char *label = "sample",
                          const char *outTsv = "") {
  TFile file(path, "READ");
  if (file.IsZombie()) {
    std::cerr << "Could not open " << path << "\n";
    return;
  }

  RechitStats rh;
  if (auto *tree = dynamic_cast<TTree *>(file.Get("zdcanalyzer/zdcrechit"))) {
    float sumPlus = 0.0;
    float sumMinus = 0.0;
    tree->SetBranchAddress("sumPlus", &sumPlus);
    tree->SetBranchAddress("sumMinus", &sumMinus);
    rh.entries = tree->GetEntries();
    for (long long i = 0; i < rh.entries; ++i) {
      tree->GetEntry(i);
      rh.sumPlus += sumPlus;
      rh.sumMinus += sumMinus;
      rh.maxPlus = std::max(rh.maxPlus, static_cast<double>(sumPlus));
      rh.maxMinus = std::max(rh.maxMinus, static_cast<double>(sumMinus));
      if (std::abs(sumPlus) > 1e-6 || std::abs(sumMinus) > 1e-6) ++rh.nonzero;
    }
  }

  DigiStats dg;
  if (auto *tree = dynamic_cast<TTree *>(file.Get("zdcanalyzer/zdcdigi"))) {
    int n = 0;
    int zside[kMaxMod] = {};
    int section[kMaxMod] = {};
    int adc[kNTs][kMaxMod] = {};
    float charge[kNTs][kMaxMod] = {};
    tree->SetBranchAddress("n", &n);
    tree->SetBranchAddress("zside", zside);
    tree->SetBranchAddress("section", section);
    for (int ts = 0; ts < kNTs; ++ts) {
      tree->SetBranchAddress(Form("adcTs%d", ts), adc[ts]);
      tree->SetBranchAddress(Form("chargefCTs%d", ts), charge[ts]);
    }

    dg.entries = tree->GetEntries();
    for (long long i = 0; i < dg.entries; ++i) {
      tree->GetEntry(i);
      if (n > 0) ++dg.nonempty;
      bool adcHigh = false;
      double sideCharge[kNTs][2] = {};
      double sideAdc[kNTs][2] = {};
      for (int j = 0; j < n && j < kMaxMod; ++j) {
        if (!(section[j] == 1 || section[j] == 2)) continue;
        const int side = zside[j] > 0 ? 1 : 0;
        for (int ts = 0; ts < kNTs; ++ts) {
          sideCharge[ts][side] += charge[ts][j];
          sideAdc[ts][side] += adc[ts][j];
          if (adc[ts][j] > 10) adcHigh = true;
        }
      }
      if (adcHigh) ++dg.adcAbovePedestal;
      const double plusDelta21 = sideCharge[2][1] - sideCharge[1][1];
      const double minusDelta21 = sideCharge[2][0] - sideCharge[1][0];
      dg.maxDelta21Plus = std::max(dg.maxDelta21Plus, plusDelta21);
      dg.maxDelta21Minus = std::max(dg.maxDelta21Minus, minusDelta21);
      if (plusDelta21 > 10.0 || minusDelta21 > 10.0) ++dg.chargePulseLike;
      for (int ts = 0; ts < kNTs; ++ts) {
        dg.maxChargePlus[ts] = std::max(dg.maxChargePlus[ts], sideCharge[ts][1]);
        dg.maxChargeMinus[ts] = std::max(dg.maxChargeMinus[ts], sideCharge[ts][0]);
        dg.maxAdcPlus[ts] = std::max(dg.maxAdcPlus[ts], sideAdc[ts][1]);
        dg.maxAdcMinus[ts] = std::max(dg.maxAdcMinus[ts], sideAdc[ts][0]);
      }
    }
  }

  PrintHeader(std::cout);
  PrintLine(std::cout, label, rh, dg);

  if (std::string(outTsv).size() > 0) {
    const bool exists = std::ifstream(outTsv).good();
    std::ofstream out(outTsv, std::ios::app);
    if (!exists) PrintHeader(out);
    PrintLine(out, label, rh, dg);
  }
}
