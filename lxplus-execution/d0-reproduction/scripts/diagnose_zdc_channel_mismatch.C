#include <TFile.h>
#include <TString.h>
#include <TSystem.h>
#include <TTree.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr int kMaxMod = 64;
constexpr int kNTs = 6;

struct EventKey {
  unsigned int run = 0;
  unsigned int lumi = 0;
  unsigned long long evt = 0;
  bool operator==(const EventKey &other) const {
    return run == other.run && lumi == other.lumi && evt == other.evt;
  }
};

struct EventKeyHash {
  std::size_t operator()(const EventKey &key) const {
    std::size_t seed = std::hash<unsigned int>{}(key.run);
    seed ^= std::hash<unsigned int>{}(key.lumi) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<unsigned long long>{}(key.evt) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
  }
};

struct Channel {
  int zside = 0;
  int section = 0;
  int channel = 0;
  float energy = 0;
  float charge[kNTs] = {};
  int adc[kNTs] = {};
};

struct ZdcEvent {
  Long64_t entry = -1;
  float sumPlus = 0;
  float sumMinus = 0;
  int rechitN = 0;
  int digiN = 0;
  std::vector<Channel> channels;
  double energyPlus = 0;
  double energyMinus = 0;
  double deltaChargePlus = 0;
  double deltaChargeMinus = 0;
};

struct Sample {
  std::unique_ptr<TFile> file;
  TTree *hi = nullptr;
  TTree *rechit = nullptr;
  TTree *digi = nullptr;
  std::unordered_map<EventKey, ZdcEvent, EventKeyHash> events;
};

struct SlopeStats {
  double xy = 0;
  double xx = 0;
  long long n = 0;
  double ratioSum = 0;
  double maxAbsDiff = 0;
  void add(double x, double y) {
    if (std::abs(x) < 1e-6) return;
    xy += x * y;
    xx += x * x;
    ratioSum += y / x;
    maxAbsDiff = std::max(maxAbsDiff, std::abs(y - x));
    ++n;
  }
  double slope() const { return xx > 0 ? xy / xx : 0; }
  double meanRatio() const { return n > 0 ? ratioSum / n : 0; }
};

bool ownedOutput(const TString &path) {
  return path.BeginsWith("/afs/cern.ch/user/e/evzhang") ||
         path.BeginsWith("/afs/cern.ch/work/e/evzhang") ||
         path.BeginsWith("/eos/user/e/evzhang");
}

int channelId(const Channel &c) {
  return (c.zside > 0 ? 1 : -1) * 1000 + c.section * 100 + c.channel;
}

bool useForZdcSum(int section, int channel) {
  if (!(section == 1 || section == 2)) return false;
  if (section == 1 && channel > 5) return false;
  return true;
}

bool openSample(const char *path, Sample &sample, const char *label) {
  sample.file.reset(TFile::Open(path, "READ"));
  if (!sample.file || sample.file->IsZombie()) {
    std::cerr << "Could not open " << label << ": " << path << "\n";
    return false;
  }
  sample.hi = dynamic_cast<TTree *>(sample.file->Get("hiEvtAnalyzer/HiTree"));
  sample.rechit = dynamic_cast<TTree *>(sample.file->Get("zdcanalyzer/zdcrechit"));
  sample.digi = dynamic_cast<TTree *>(sample.file->Get("zdcanalyzer/zdcdigi"));
  if (!sample.hi || !sample.rechit || !sample.digi) {
    std::cerr << "Missing hi/rechit/digi tree in " << label << "\n";
    return false;
  }
  return true;
}

void buildSample(Sample &sample) {
  unsigned int run = 0;
  unsigned int lumi = 0;
  unsigned long long evt = 0;
  sample.hi->SetBranchStatus("*", 0);
  sample.hi->SetBranchStatus("run", 1);
  sample.hi->SetBranchStatus("lumi", 1);
  sample.hi->SetBranchStatus("evt", 1);
  sample.hi->SetBranchAddress("run", &run);
  sample.hi->SetBranchAddress("lumi", &lumi);
  sample.hi->SetBranchAddress("evt", &evt);

  int rn = 0;
  int rzside[kMaxMod] = {};
  int rsection[kMaxMod] = {};
  int rchannel[kMaxMod] = {};
  float renergy[kMaxMod] = {};
  float rsumPlus = 0;
  float rsumMinus = 0;
  const char *energyBranch = sample.rechit->GetBranch("energy") ? "energy" : "e";
  sample.rechit->SetBranchStatus("*", 0);
  sample.rechit->SetBranchStatus("n", 1);
  sample.rechit->SetBranchStatus("zside", 1);
  sample.rechit->SetBranchStatus("section", 1);
  sample.rechit->SetBranchStatus("channel", 1);
  sample.rechit->SetBranchStatus(energyBranch, 1);
  sample.rechit->SetBranchStatus("sumPlus", 1);
  sample.rechit->SetBranchStatus("sumMinus", 1);
  sample.rechit->SetBranchAddress("n", &rn);
  sample.rechit->SetBranchAddress("zside", rzside);
  sample.rechit->SetBranchAddress("section", rsection);
  sample.rechit->SetBranchAddress("channel", rchannel);
  sample.rechit->SetBranchAddress(energyBranch, renergy);
  sample.rechit->SetBranchAddress("sumPlus", &rsumPlus);
  sample.rechit->SetBranchAddress("sumMinus", &rsumMinus);

  int dn = 0;
  int dzside[kMaxMod] = {};
  int dsection[kMaxMod] = {};
  int dchannel[kMaxMod] = {};
  float charge[kNTs][kMaxMod] = {};
  int adc[kNTs][kMaxMod] = {};
  sample.digi->SetBranchStatus("*", 0);
  sample.digi->SetBranchStatus("n", 1);
  sample.digi->SetBranchStatus("zside", 1);
  sample.digi->SetBranchStatus("section", 1);
  sample.digi->SetBranchStatus("channel", 1);
  sample.digi->SetBranchAddress("n", &dn);
  sample.digi->SetBranchAddress("zside", dzside);
  sample.digi->SetBranchAddress("section", dsection);
  sample.digi->SetBranchAddress("channel", dchannel);
  for (int ts = 0; ts < kNTs; ++ts) {
    sample.digi->SetBranchStatus(Form("chargefCTs%d", ts), 1);
    sample.digi->SetBranchAddress(Form("chargefCTs%d", ts), charge[ts]);
    if (sample.digi->GetBranch(Form("adcTs%d", ts))) {
      sample.digi->SetBranchStatus(Form("adcTs%d", ts), 1);
      sample.digi->SetBranchAddress(Form("adcTs%d", ts), adc[ts]);
    }
  }

  const Long64_t entries = std::min(sample.hi->GetEntries(), sample.rechit->GetEntries());
  sample.events.reserve(entries);
  for (Long64_t i = 0; i < entries; ++i) {
    sample.hi->GetEntry(i);
    sample.rechit->GetEntry(i);
    sample.digi->GetEntry(i);
    ZdcEvent event;
    event.entry = i;
    event.sumPlus = rsumPlus;
    event.sumMinus = rsumMinus;
    event.rechitN = rn;
    event.digiN = dn;

    std::map<int, Channel> byId;
    for (int j = 0; j < rn && j < kMaxMod; ++j) {
      Channel c;
      c.zside = rzside[j];
      c.section = rsection[j];
      c.channel = rchannel[j];
      c.energy = renergy[j];
      byId[channelId(c)] = c;
      if (useForZdcSum(c.section, c.channel)) {
        if (c.zside > 0) event.energyPlus += c.energy;
        if (c.zside < 0) event.energyMinus += c.energy;
      }
    }
    for (int j = 0; j < dn && j < kMaxMod; ++j) {
      Channel key;
      key.zside = dzside[j];
      key.section = dsection[j];
      key.channel = dchannel[j];
      Channel &c = byId[channelId(key)];
      c.zside = key.zside;
      c.section = key.section;
      c.channel = key.channel;
      for (int ts = 0; ts < kNTs; ++ts) {
        c.charge[ts] = charge[ts][j];
        c.adc[ts] = adc[ts][j];
      }
      if (useForZdcSum(c.section, c.channel)) {
        const double delta = static_cast<double>(c.charge[2]) - c.charge[1];
        if (c.zside > 0) event.deltaChargePlus += delta;
        if (c.zside < 0) event.deltaChargeMinus += delta;
      }
    }
    for (const auto &item : byId) event.channels.push_back(item.second);
    sample.events[EventKey{run, lumi, evt}] = event;
  }
}

std::vector<int> ids(const ZdcEvent &event) {
  std::vector<int> out;
  for (const auto &c : event.channels) out.push_back(channelId(c));
  std::sort(out.begin(), out.end());
  return out;
}

}  // namespace

int diagnose_zdc_channel_mismatch(const char *recreatedPath,
                                  const char *officialPath,
                                  const char *outDir) {
  TString out(outDir);
  if (!ownedOutput(out)) {
    std::cerr << "Refusing to write outside Evan-owned CERN paths: " << out << "\n";
    return 2;
  }
  gSystem->mkdir(out, true);

  Sample recreated;
  Sample official;
  if (!openSample(recreatedPath, recreated, "recreated")) return 1;
  if (!openSample(officialPath, official, "official")) return 1;
  buildSample(recreated);
  buildSample(official);

  std::ofstream rows((out + "/zdc_channel_event_sums.tsv").Data());
  rows << "run\tlumi\tevt\trecEntry\toffEntry"
       << "\trecSumPlus\toffSumPlus\trecEnergyPlus\toffEnergyPlus\trecDeltaChargePlus\toffDeltaChargePlus"
       << "\trecSumMinus\toffSumMinus\trecEnergyMinus\toffEnergyMinus\trecDeltaChargeMinus\toffDeltaChargeMinus"
       << "\trecN\toffN\trecDigiN\toffDigiN\tchannelIdsMatch\n";

  std::ofstream chans((out + "/zdc_channel_rows_first200.tsv").Data());
  chans << "run\tlumi\tevt\tzside\tsection\tchannel"
        << "\trecEnergy\toffEnergy\trecQ1\toffQ1\trecQ2\toffQ2\trecQ2mQ1\toffQ2mQ1\trecAdc1\toffAdc1\trecAdc2\toffAdc2\n";

  long long matched = 0;
  long long channelIdsMatch = 0;
  long long recSumEqualsEnergy = 0;
  long long offSumEqualsEnergy = 0;
  SlopeStats plusSum;
  SlopeStats minusSum;
  SlopeStats plusEnergy;
  SlopeStats minusEnergy;
  SlopeStats plusDeltaCharge;
  SlopeStats minusDeltaCharge;

  for (const auto &item : recreated.events) {
    const auto found = official.events.find(item.first);
    if (found == official.events.end()) continue;
    const ZdcEvent &r = item.second;
    const ZdcEvent &o = found->second;
    ++matched;
    const bool idsMatch = ids(r) == ids(o);
    if (idsMatch) ++channelIdsMatch;
    if (std::abs(r.sumPlus - r.energyPlus) < 1e-3 && std::abs(r.sumMinus - r.energyMinus) < 1e-3) ++recSumEqualsEnergy;
    if (std::abs(o.sumPlus - o.energyPlus) < 1e-3 && std::abs(o.sumMinus - o.energyMinus) < 1e-3) ++offSumEqualsEnergy;
    plusSum.add(r.sumPlus, o.sumPlus);
    minusSum.add(r.sumMinus, o.sumMinus);
    plusEnergy.add(r.energyPlus, o.energyPlus);
    minusEnergy.add(r.energyMinus, o.energyMinus);
    plusDeltaCharge.add(r.deltaChargePlus, o.deltaChargePlus);
    minusDeltaCharge.add(r.deltaChargeMinus, o.deltaChargeMinus);

    rows << item.first.run << '\t' << item.first.lumi << '\t' << item.first.evt
         << '\t' << r.entry << '\t' << o.entry
         << '\t' << r.sumPlus << '\t' << o.sumPlus << '\t' << r.energyPlus << '\t' << o.energyPlus
         << '\t' << r.deltaChargePlus << '\t' << o.deltaChargePlus
         << '\t' << r.sumMinus << '\t' << o.sumMinus << '\t' << r.energyMinus << '\t' << o.energyMinus
         << '\t' << r.deltaChargeMinus << '\t' << o.deltaChargeMinus
         << '\t' << r.rechitN << '\t' << o.rechitN << '\t' << r.digiN << '\t' << o.digiN
         << '\t' << (idsMatch ? 1 : 0) << '\n';

    if (matched <= 200) {
      std::map<int, Channel> rr;
      std::map<int, Channel> oo;
      for (const auto &c : r.channels) rr[channelId(c)] = c;
      for (const auto &c : o.channels) oo[channelId(c)] = c;
      for (const auto &entry : rr) {
        const auto of = oo.find(entry.first);
        if (of == oo.end()) continue;
        const Channel &rc = entry.second;
        const Channel &oc = of->second;
        chans << item.first.run << '\t' << item.first.lumi << '\t' << item.first.evt
              << '\t' << rc.zside << '\t' << rc.section << '\t' << rc.channel
              << '\t' << rc.energy << '\t' << oc.energy
              << '\t' << rc.charge[1] << '\t' << oc.charge[1]
              << '\t' << rc.charge[2] << '\t' << oc.charge[2]
              << '\t' << (rc.charge[2] - rc.charge[1]) << '\t' << (oc.charge[2] - oc.charge[1])
              << '\t' << rc.adc[1] << '\t' << oc.adc[1]
              << '\t' << rc.adc[2] << '\t' << oc.adc[2] << '\n';
      }
    }
  }

  std::ofstream summary((out + "/zdc_channel_mismatch_summary.md").Data());
  summary << "# ZDC Channel Mismatch Diagnostic\n\n";
  summary << "- Recreated: `" << recreatedPath << "`\n";
  summary << "- Official: `" << officialPath << "`\n";
  summary << "- Matched events: `" << matched << "`\n";
  summary << "- Channel-ID set match: `" << channelIdsMatch << "/" << matched << "`\n";
  summary << "- Recreated branch sums equal per-channel energy sums: `" << recSumEqualsEnergy << "/" << matched << "`\n";
  summary << "- Official branch sums equal per-channel energy sums: `" << offSumEqualsEnergy << "/" << matched << "`\n";
  summary << "- Official/recreated sumPlus slope through origin: `" << plusSum.slope() << "`; mean ratio: `" << plusSum.meanRatio() << "`\n";
  summary << "- Official/recreated sumMinus slope through origin: `" << minusSum.slope() << "`; mean ratio: `" << minusSum.meanRatio() << "`\n";
  summary << "- Official/recreated energyPlus slope through origin: `" << plusEnergy.slope() << "`; mean ratio: `" << plusEnergy.meanRatio() << "`\n";
  summary << "- Official/recreated energyMinus slope through origin: `" << minusEnergy.slope() << "`; mean ratio: `" << minusEnergy.meanRatio() << "`\n";
  summary << "- Official/recreated raw TS2-TS1 charge plus slope: `" << plusDeltaCharge.slope() << "`; mean ratio: `" << plusDeltaCharge.meanRatio() << "`\n";
  summary << "- Official/recreated raw TS2-TS1 charge minus slope: `" << minusDeltaCharge.slope() << "`; mean ratio: `" << minusDeltaCharge.meanRatio() << "`\n";
  summary << "- Event-sum rows: `zdc_channel_event_sums.tsv`\n";
  summary << "- First 200 matched-event channel rows: `zdc_channel_rows_first200.tsv`\n";

  std::cout << "matched=" << matched << "\n";
  std::cout << "channel_ids_match=" << channelIdsMatch << "/" << matched << "\n";
  std::cout << "official_over_recreated_sum_plus_slope=" << plusSum.slope() << "\n";
  std::cout << "official_over_recreated_sum_minus_slope=" << minusSum.slope() << "\n";
  std::cout << "summary=" << (out + "/zdc_channel_mismatch_summary.md") << "\n";
  return matched > 0 ? 0 : 3;
}
