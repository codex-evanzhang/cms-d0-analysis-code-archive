#include <TFile.h>
#include <TString.h>
#include <TSystem.h>
#include <TTree.h>

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

struct EventKey {
  int run = 0;
  int lumi = 0;
  int event = 0;

  bool operator==(const EventKey& other) const {
    return run == other.run && lumi == other.lumi && event == other.event;
  }
};

struct EventKeyHash {
  std::size_t operator()(const EventKey& key) const {
    std::size_t seed = std::hash<int>{}(key.run);
    seed ^= std::hash<int>{}(key.lumi) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<int>{}(key.event) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
  }
};

bool isOwnedOutput(const TString& path) {
  return path.BeginsWith("/afs/cern.ch/user/e/evzhang") ||
         path.BeginsWith("/afs/cern.ch/work/e/evzhang") ||
         path.BeginsWith("/eos/user/e/evzhang");
}

std::string keyString(const EventKey& key) {
  return std::to_string(key.run) + ":" + std::to_string(key.lumi) + ":" + std::to_string(key.event);
}

TString officialUrl(const TString& campaignBase, int fileNumber) {
  const char* subdir = fileNumber < 1000 ? "0000" : "0001";
  return Form("%s/%s/HiForest_2023PbPbUPC_Jan24Reco_%d.root",
              campaignBase.Data(), subdir, fileNumber);
}

std::vector<EventKey> readTargetKeys(const char* targetForest, const char* treeName) {
  std::vector<EventKey> keys;
  TFile* file = TFile::Open(targetForest, "READ");
  if (!file || file->IsZombie()) {
    std::cerr << "ERROR: could not open target forest " << targetForest << "\n";
    return keys;
  }

  TTree* tree = dynamic_cast<TTree*>(file->Get(treeName));
  if (!tree) {
    std::cerr << "ERROR: missing target tree " << treeName << "\n";
    file->Close();
    return keys;
  }

  int run = 0;
  int lumi = 0;
  int event = 0;
  tree->SetBranchStatus("*", 0);
  tree->SetBranchStatus("RunNo", 1);
  tree->SetBranchStatus("LumiNo", 1);
  tree->SetBranchStatus("EvtNo", 1);
  tree->SetBranchAddress("RunNo", &run);
  tree->SetBranchAddress("LumiNo", &lumi);
  tree->SetBranchAddress("EvtNo", &event);

  const Long64_t entries = tree->GetEntries();
  keys.reserve(entries);
  for (Long64_t i = 0; i < entries; ++i) {
    tree->GetEntry(i);
    keys.push_back({run, lumi, event});
  }
  file->Close();
  return keys;
}

}  // namespace

int find_official_forest_events(
    const char* targetForest =
        "/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-forest-2023/smoke/hiforward0_miniaod_100_d0_zdc_pf_track_hcalsev/HiForest_2023PbPbUPC_Jan24Reco_smoke.root",
    const char* campaignBase =
        "root://xrootd-vanderbilt.sites.opensciencegrid.org//store/user/jdlang/Run3_PbPbUPC/Forest_2023_Jan2024ReReco_2025Reforest/HIForward0/crab_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward0/260212_170406",
    int firstFile = 1,
    int lastFile = 1278,
    const char* outputCsv =
        "/eos/user/e/evzhang/codex-eos/outputs/d0-reproduction/recreated-forest-2023/official_match_search/matches.csv",
    const char* treeName = "Dfinder/ntDkpi") {
  if (!isOwnedOutput(outputCsv)) {
    std::cerr << "ERROR: refusing to write outside Evan-owned CERN paths: " << outputCsv << "\n";
    return 2;
  }

  TString outDir = gSystem->DirName(outputCsv);
  gSystem->mkdir(outDir, true);

  const std::vector<EventKey> targetKeys = readTargetKeys(targetForest, treeName);
  if (targetKeys.empty()) {
    std::cerr << "ERROR: no target keys read from " << targetForest << "\n";
    return 3;
  }

  std::unordered_set<EventKey, EventKeyHash> wanted;
  for (const auto& key : targetKeys) wanted.insert(key);

  std::ofstream csv(outputCsv);
  csv << "file_number,file_url,matched,run_lumi_rows,matched_keys\n";
  csv.flush();

  int bestFile = -1;
  int bestMatches = 0;
  TString bestUrl;

  for (int fileNumber = firstFile; fileNumber <= lastFile; ++fileNumber) {
    TString url = officialUrl(campaignBase, fileNumber);
    TFile* file = TFile::Open(url, "READ");
    if (!file || file->IsZombie()) {
      if (file) file->Close();
      continue;
    }

    TTree* tree = dynamic_cast<TTree*>(file->Get(treeName));
    if (!tree) {
      file->Close();
      continue;
    }

    int run = 0;
    int lumi = 0;
    int event = 0;
    tree->SetBranchStatus("*", 0);
    tree->SetBranchStatus("RunNo", 1);
    tree->SetBranchStatus("LumiNo", 1);
    tree->SetBranchStatus("EvtNo", 1);
    tree->SetBranchAddress("RunNo", &run);
    tree->SetBranchAddress("LumiNo", &lumi);
    tree->SetBranchAddress("EvtNo", &event);

    std::vector<EventKey> matchedKeys;
    int runLumiRows = 0;
    const Long64_t entries = tree->GetEntries();
    for (Long64_t i = 0; i < entries; ++i) {
      tree->GetEntry(i);
      EventKey key{run, lumi, event};
      if (run == targetKeys.front().run && lumi == targetKeys.front().lumi) ++runLumiRows;
      if (wanted.find(key) != wanted.end()) matchedKeys.push_back(key);
    }

    if (!matchedKeys.empty()) {
      csv << fileNumber << "," << url << "," << matchedKeys.size() << "," << runLumiRows << ",\"";
      for (std::size_t i = 0; i < matchedKeys.size(); ++i) {
        if (i) csv << " ";
        csv << keyString(matchedKeys[i]);
      }
      csv << "\"\n";
      csv.flush();

      std::cout << "MATCH file=" << fileNumber << " matched=" << matchedKeys.size()
                << " run_lumi_rows=" << runLumiRows << " url=" << url << "\n";
      if (static_cast<int>(matchedKeys.size()) > bestMatches) {
        bestMatches = matchedKeys.size();
        bestFile = fileNumber;
        bestUrl = url;
      }
      if (bestMatches == static_cast<int>(targetKeys.size())) {
        file->Close();
        break;
      }
    }

    if (fileNumber % 25 == 0) {
      std::cout << "scanned_through=" << fileNumber << " best_file=" << bestFile
                << " best_matches=" << bestMatches << "\n" << std::flush;
    }
    file->Close();
  }

  std::cout << "BEST file=" << bestFile << " matches=" << bestMatches
            << " target_events=" << targetKeys.size() << " url=" << bestUrl << "\n";
  std::cout << "csv=" << outputCsv << "\n";
  std::cout << std::flush;
  return bestMatches > 0 ? 0 : 1;
}
