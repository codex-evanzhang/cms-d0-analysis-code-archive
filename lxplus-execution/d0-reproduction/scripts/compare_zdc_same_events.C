#include <TFile.h>
#include <TString.h>
#include <TSystem.h>
#include <TTree.h>

#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

namespace {

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

struct ZdcRow {
  Long64_t entry = -1;
  float rechitPlus = 0.0;
  float rechitMinus = 0.0;
  float digiPlus = 0.0;
  float digiMinus = 0.0;
  int digiN = 0;
  int rechitN = 0;
};

struct Sample {
  std::unique_ptr<TFile> file;
  TTree *hi = nullptr;
  TTree *rechit = nullptr;
  TTree *digi = nullptr;
  std::unordered_map<EventKey, ZdcRow, EventKeyHash> rows;
};

bool ownedOutput(const TString &path) {
  return path.BeginsWith("/afs/cern.ch/user/e/evzhang") ||
         path.BeginsWith("/afs/cern.ch/work/e/evzhang") ||
         path.BeginsWith("/eos/user/e/evzhang");
}

std::string keyString(const EventKey &key) {
  return std::to_string(key.run) + ":" + std::to_string(key.lumi) + ":" + std::to_string(key.evt);
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
    std::cerr << "Missing one of hiEvtAnalyzer/HiTree, zdcanalyzer/zdcrechit, zdcanalyzer/zdcdigi in " << label << "\n";
    return false;
  }
  if (sample.hi->GetEntries() != sample.rechit->GetEntries() || sample.hi->GetEntries() != sample.digi->GetEntries()) {
    std::cerr << "Entry-count mismatch inside " << label << ": hi=" << sample.hi->GetEntries()
              << " rechit=" << sample.rechit->GetEntries() << " digi=" << sample.digi->GetEntries() << "\n";
    return false;
  }
  return true;
}

void buildMap(Sample &sample) {
  unsigned int run = 0;
  unsigned int lumi = 0;
  unsigned long long evt = 0;
  float rechitPlus = 0.0;
  float rechitMinus = 0.0;
  float digiPlus = 0.0;
  float digiMinus = 0.0;
  int rechitN = 0;
  int digiN = 0;

  sample.hi->SetBranchStatus("*", 0);
  sample.hi->SetBranchStatus("run", 1);
  sample.hi->SetBranchStatus("lumi", 1);
  sample.hi->SetBranchStatus("evt", 1);
  sample.hi->SetBranchAddress("run", &run);
  sample.hi->SetBranchAddress("lumi", &lumi);
  sample.hi->SetBranchAddress("evt", &evt);

  sample.rechit->SetBranchStatus("*", 0);
  sample.rechit->SetBranchStatus("n", 1);
  sample.rechit->SetBranchStatus("sumPlus", 1);
  sample.rechit->SetBranchStatus("sumMinus", 1);
  sample.rechit->SetBranchAddress("n", &rechitN);
  sample.rechit->SetBranchAddress("sumPlus", &rechitPlus);
  sample.rechit->SetBranchAddress("sumMinus", &rechitMinus);

  sample.digi->SetBranchStatus("*", 0);
  sample.digi->SetBranchStatus("n", 1);
  sample.digi->SetBranchStatus("sumPlus", 1);
  sample.digi->SetBranchStatus("sumMinus", 1);
  sample.digi->SetBranchAddress("n", &digiN);
  sample.digi->SetBranchAddress("sumPlus", &digiPlus);
  sample.digi->SetBranchAddress("sumMinus", &digiMinus);

  const Long64_t entries = sample.hi->GetEntries();
  sample.rows.reserve(entries);
  for (Long64_t i = 0; i < entries; ++i) {
    sample.hi->GetEntry(i);
    sample.rechit->GetEntry(i);
    sample.digi->GetEntry(i);
    EventKey key{run, lumi, evt};
    sample.rows[key] = ZdcRow{i, rechitPlus, rechitMinus, digiPlus, digiMinus, digiN, rechitN};
  }
}

bool ltOr1500(const ZdcRow &row) {
  return row.rechitPlus < 1500.0 || row.rechitMinus < 1500.0;
}

bool xor0nXnNominal(const ZdcRow &row) {
  const bool gammaN = row.rechitMinus > 1000.0 && row.rechitPlus < 1100.0;
  const bool Ngamma = row.rechitPlus > 1100.0 && row.rechitMinus < 1000.0;
  return gammaN || Ngamma;
}

}  // namespace

int compare_zdc_same_events(const char *recreatedPath,
                            const char *officialPath,
                            const char *outDir) {
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
  buildMap(recreated);
  buildMap(official);

  std::ofstream rows((out + "/zdc_same_event_rows.tsv").Data());
  rows << "run\tlumi\tevt\trecreatedEntry\tofficialEntry"
       << "\trecreatedRechitPlus\tofficialRechitPlus\tdiffRechitPlus"
       << "\trecreatedRechitMinus\tofficialRechitMinus\tdiffRechitMinus"
       << "\trecreatedDigiPlus\tofficialDigiPlus\tdiffDigiPlus"
       << "\trecreatedDigiMinus\tofficialDigiMinus\tdiffDigiMinus"
       << "\trecreatedRechitN\tofficialRechitN\trecreatedDigiN\tofficialDigiN"
       << "\trecreatedLtOr1500\tofficialLtOr1500"
       << "\trecreatedNominal0nXn\tofficialNominal0nXn\n";

  Long64_t matched = 0;
  Long64_t missingOfficial = 0;
  Long64_t missingRecreated = 0;
  Long64_t exactRechit = 0;
  Long64_t exactDigi = 0;
  Long64_t ltOrAgree = 0;
  Long64_t nominalAgree = 0;
  double maxAbsRechitPlus = 0.0;
  double maxAbsRechitMinus = 0.0;
  double maxAbsDigiPlus = 0.0;
  double maxAbsDigiMinus = 0.0;

  for (const auto &item : recreated.rows) {
    const auto found = official.rows.find(item.first);
    if (found == official.rows.end()) {
      ++missingOfficial;
      continue;
    }
    const auto &r = item.second;
    const auto &o = found->second;
    ++matched;
    const double dRechitPlus = static_cast<double>(r.rechitPlus) - o.rechitPlus;
    const double dRechitMinus = static_cast<double>(r.rechitMinus) - o.rechitMinus;
    const double dDigiPlus = static_cast<double>(r.digiPlus) - o.digiPlus;
    const double dDigiMinus = static_cast<double>(r.digiMinus) - o.digiMinus;
    maxAbsRechitPlus = std::max(maxAbsRechitPlus, std::abs(dRechitPlus));
    maxAbsRechitMinus = std::max(maxAbsRechitMinus, std::abs(dRechitMinus));
    maxAbsDigiPlus = std::max(maxAbsDigiPlus, std::abs(dDigiPlus));
    maxAbsDigiMinus = std::max(maxAbsDigiMinus, std::abs(dDigiMinus));
    if (std::abs(dRechitPlus) < 1e-4 && std::abs(dRechitMinus) < 1e-4) ++exactRechit;
    if (std::abs(dDigiPlus) < 1e-4 && std::abs(dDigiMinus) < 1e-4) ++exactDigi;
    if (ltOr1500(r) == ltOr1500(o)) ++ltOrAgree;
    if (xor0nXnNominal(r) == xor0nXnNominal(o)) ++nominalAgree;
    rows << item.first.run << '\t' << item.first.lumi << '\t' << item.first.evt
         << '\t' << r.entry << '\t' << o.entry
         << '\t' << r.rechitPlus << '\t' << o.rechitPlus << '\t' << dRechitPlus
         << '\t' << r.rechitMinus << '\t' << o.rechitMinus << '\t' << dRechitMinus
         << '\t' << r.digiPlus << '\t' << o.digiPlus << '\t' << dDigiPlus
         << '\t' << r.digiMinus << '\t' << o.digiMinus << '\t' << dDigiMinus
         << '\t' << r.rechitN << '\t' << o.rechitN
         << '\t' << r.digiN << '\t' << o.digiN
         << '\t' << (ltOr1500(r) ? 1 : 0) << '\t' << (ltOr1500(o) ? 1 : 0)
         << '\t' << (xor0nXnNominal(r) ? 1 : 0) << '\t' << (xor0nXnNominal(o) ? 1 : 0)
         << '\n';
  }
  for (const auto &item : official.rows) {
    if (recreated.rows.find(item.first) == recreated.rows.end()) ++missingRecreated;
  }

  std::ofstream summary((out + "/zdc_same_event_summary.md").Data());
  summary << "# ZDC Same-Event Forest Comparison\n\n";
  summary << "- Recreated: `" << recreatedPath << "`\n";
  summary << "- Official: `" << officialPath << "`\n";
  summary << "- Recreated events: `" << recreated.rows.size() << "`\n";
  summary << "- Official events: `" << official.rows.size() << "`\n";
  summary << "- Matched events: `" << matched << "`\n";
  summary << "- Recreated events missing from official file: `" << missingOfficial << "`\n";
  summary << "- Official events missing from recreated file: `" << missingRecreated << "`\n";
  summary << "- Exact rechit sums on matched events: `" << exactRechit << "/" << matched << "`\n";
  summary << "- Exact digi sums on matched events: `" << exactDigi << "/" << matched << "`\n";
  summary << "- `lt OR 1500` filter agreement on matched events: `" << ltOrAgree << "/" << matched << "`\n";
  summary << "- nominal `0nXn` agreement on matched events: `" << nominalAgree << "/" << matched << "`\n";
  summary << "- Max |rechit plus diff|: `" << maxAbsRechitPlus << "`\n";
  summary << "- Max |rechit minus diff|: `" << maxAbsRechitMinus << "`\n";
  summary << "- Max |digi plus diff|: `" << maxAbsDigiPlus << "`\n";
  summary << "- Max |digi minus diff|: `" << maxAbsDigiMinus << "`\n";

  std::cout << "matched=" << matched << "\n";
  std::cout << "exact_rechit=" << exactRechit << "/" << matched << "\n";
  std::cout << "lt_or_1500_agree=" << ltOrAgree << "/" << matched << "\n";
  std::cout << "nominal_0nXn_agree=" << nominalAgree << "/" << matched << "\n";
  std::cout << "summary=" << (out + "/zdc_same_event_summary.md") << "\n";
  return (matched > 0 && exactRechit == matched && ltOrAgree == matched) ? 0 : 3;
}
