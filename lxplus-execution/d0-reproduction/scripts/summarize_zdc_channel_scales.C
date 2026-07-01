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
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr int kMaxMod = 64;

struct EventKey {
  unsigned int run = 0;
  unsigned int lumi = 0;
  unsigned long long evt = 0;
  bool operator==(const EventKey& other) const {
    return run == other.run && lumi == other.lumi && evt == other.evt;
  }
};

struct EventKeyHash {
  std::size_t operator()(const EventKey& key) const {
    std::size_t seed = std::hash<unsigned int>{}(key.run);
    seed ^= std::hash<unsigned int>{}(key.lumi) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<unsigned long long>{}(key.evt) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
  }
};

struct ChannelEnergy {
  int zside = 0;
  int section = 0;
  int channel = 0;
  float energy = 0;
};

struct Event {
  std::map<int, ChannelEnergy> channels;
  float sumPlus = 0;
  float sumMinus = 0;
};

struct Sample {
  std::unique_ptr<TFile> file;
  TTree* hi = nullptr;
  TTree* rechit = nullptr;
  std::unordered_map<EventKey, Event, EventKeyHash> events;
};

struct Stats {
  double xy = 0;
  double xx = 0;
  double sumRatio = 0;
  double sumAbsDiff = 0;
  double maxAbsDiff = 0;
  long long n = 0;

  void add(double rec, double off) {
    if (std::abs(rec) < 1e-6) return;
    xy += rec * off;
    xx += rec * rec;
    sumRatio += off / rec;
    const double absDiff = std::abs(off - rec);
    sumAbsDiff += absDiff;
    maxAbsDiff = std::max(maxAbsDiff, absDiff);
    ++n;
  }

  double slope() const { return xx > 0 ? xy / xx : 0; }
  double meanRatio() const { return n > 0 ? sumRatio / n : 0; }
  double meanAbsDiff() const { return n > 0 ? sumAbsDiff / n : 0; }
};

bool ownedOutput(const TString& path) {
  return path.BeginsWith("/afs/cern.ch/user/e/evzhang") ||
         path.BeginsWith("/afs/cern.ch/work/e/evzhang") ||
         path.BeginsWith("/eos/user/e/evzhang");
}

int channelId(int zside, int section, int channel) {
  return (zside > 0 ? 1 : -1) * 1000 + section * 100 + channel;
}

std::string channelLabel(int id) {
  const int sign = id > 0 ? 1 : -1;
  const int rest = std::abs(id) - 1000;
  const int section = rest / 100;
  const int channel = rest % 100;
  return std::string(sign > 0 ? "plus" : "minus") + "_sec" + std::to_string(section) + "_ch" + std::to_string(channel);
}

bool useForSum(int section, int channel) {
  if (!(section == 1 || section == 2)) return false;
  if (section == 1 && channel > 5) return false;
  return true;
}

bool openSample(const char* path, Sample& sample, const char* label) {
  sample.file.reset(TFile::Open(path, "READ"));
  if (!sample.file || sample.file->IsZombie()) {
    std::cerr << "Could not open " << label << ": " << path << "\n";
    return false;
  }
  sample.hi = dynamic_cast<TTree*>(sample.file->Get("hiEvtAnalyzer/HiTree"));
  sample.rechit = dynamic_cast<TTree*>(sample.file->Get("zdcanalyzer/zdcrechit"));
  if (!sample.hi || !sample.rechit) {
    std::cerr << "Missing hiEvtAnalyzer/HiTree or zdcanalyzer/zdcrechit in " << label << "\n";
    return false;
  }
  return true;
}

void buildSample(Sample& sample) {
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

  int n = 0;
  int zside[kMaxMod] = {};
  int section[kMaxMod] = {};
  int channel[kMaxMod] = {};
  float energy[kMaxMod] = {};
  float sumPlus = 0;
  float sumMinus = 0;
  const char* energyBranch = sample.rechit->GetBranch("energy") ? "energy" : "e";
  sample.rechit->SetBranchStatus("*", 0);
  sample.rechit->SetBranchStatus("n", 1);
  sample.rechit->SetBranchStatus("zside", 1);
  sample.rechit->SetBranchStatus("section", 1);
  sample.rechit->SetBranchStatus("channel", 1);
  sample.rechit->SetBranchStatus(energyBranch, 1);
  sample.rechit->SetBranchStatus("sumPlus", 1);
  sample.rechit->SetBranchStatus("sumMinus", 1);
  sample.rechit->SetBranchAddress("n", &n);
  sample.rechit->SetBranchAddress("zside", zside);
  sample.rechit->SetBranchAddress("section", section);
  sample.rechit->SetBranchAddress("channel", channel);
  sample.rechit->SetBranchAddress(energyBranch, energy);
  sample.rechit->SetBranchAddress("sumPlus", &sumPlus);
  sample.rechit->SetBranchAddress("sumMinus", &sumMinus);

  const Long64_t entries = std::min(sample.hi->GetEntries(), sample.rechit->GetEntries());
  sample.events.reserve(entries);
  for (Long64_t i = 0; i < entries; ++i) {
    sample.hi->GetEntry(i);
    sample.rechit->GetEntry(i);
    Event event;
    event.sumPlus = sumPlus;
    event.sumMinus = sumMinus;
    for (int j = 0; j < n && j < kMaxMod; ++j) {
      if (!useForSum(section[j], channel[j])) continue;
      ChannelEnergy c{zside[j], section[j], channel[j], energy[j]};
      event.channels[channelId(c.zside, c.section, c.channel)] = c;
    }
    sample.events[EventKey{run, lumi, evt}] = event;
  }
}

}  // namespace

int summarize_zdc_channel_scales(const char* recreatedPath,
                                 const char* officialPath,
                                 const char* outDir) {
  const TString out(outDir);
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

  std::map<int, Stats> channelStats;
  Stats plusSum;
  Stats minusSum;
  Stats plusChannels;
  Stats minusChannels;
  long long matchedEvents = 0;
  long long matchedChannels = 0;

  for (const auto& item : recreated.events) {
    const auto foundEvent = official.events.find(item.first);
    if (foundEvent == official.events.end()) continue;
    ++matchedEvents;
    const Event& rec = item.second;
    const Event& off = foundEvent->second;
    plusSum.add(rec.sumPlus, off.sumPlus);
    minusSum.add(rec.sumMinus, off.sumMinus);
    for (const auto& recChannel : rec.channels) {
      const auto foundChannel = off.channels.find(recChannel.first);
      if (foundChannel == off.channels.end()) continue;
      const double r = recChannel.second.energy;
      const double o = foundChannel->second.energy;
      channelStats[recChannel.first].add(r, o);
      if (recChannel.second.zside > 0) plusChannels.add(r, o);
      if (recChannel.second.zside < 0) minusChannels.add(r, o);
      ++matchedChannels;
    }
  }

  std::ofstream table((out + "/zdc_channel_scale_summary.tsv").Data());
  table << "channel_id\tlabel\tn\tslope_official_over_recreated\tmean_ratio\tmean_abs_diff\tmax_abs_diff\n";
  for (const auto& item : channelStats) {
    table << item.first << '\t' << channelLabel(item.first) << '\t' << item.second.n << '\t'
          << item.second.slope() << '\t' << item.second.meanRatio() << '\t'
          << item.second.meanAbsDiff() << '\t' << item.second.maxAbsDiff << '\n';
  }

  std::ofstream summary((out + "/zdc_channel_scale_summary.md").Data());
  summary << "# ZDC Channel Scale Summary\n\n";
  summary << "- Recreated: `" << recreatedPath << "`\n";
  summary << "- Official: `" << officialPath << "`\n";
  summary << "- Matched events: `" << matchedEvents << "`\n";
  summary << "- Matched channels: `" << matchedChannels << "`\n";
  summary << "- Sum plus official/recreated slope: `" << plusSum.slope() << "` over `" << plusSum.n << "` events\n";
  summary << "- Sum minus official/recreated slope: `" << minusSum.slope() << "` over `" << minusSum.n << "` events\n";
  summary << "- Plus channel-energy official/recreated slope: `" << plusChannels.slope() << "` over `" << plusChannels.n << "` channels\n";
  summary << "- Minus channel-energy official/recreated slope: `" << minusChannels.slope() << "` over `" << minusChannels.n << "` channels\n";
  summary << "- Per-channel table: `zdc_channel_scale_summary.tsv`\n";

  std::cout << "matched_events=" << matchedEvents << "\n";
  std::cout << "matched_channels=" << matchedChannels << "\n";
  std::cout << "sum_plus_slope=" << plusSum.slope() << "\n";
  std::cout << "sum_minus_slope=" << minusSum.slope() << "\n";
  std::cout << "plus_channel_slope=" << plusChannels.slope() << "\n";
  std::cout << "minus_channel_slope=" << minusChannels.slope() << "\n";
  std::cout << "summary=" << (out + "/zdc_channel_scale_summary.md") << "\n";
  return matchedEvents > 0 ? 0 : 3;
}
