#include "Pythia8/Pythia.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

struct Counts {
  int eventsAttempted = 0;
  int eventsAccepted = 0;
  int eventsWithCharmHadron = 0;
  int eventsWithD0 = 0;
  int d0Total = 0;
  int d0ToKPi = 0;
  int d0InPtRapidityWindow = 0;
};

bool isKPiPair(const Pythia8::Event& event, int firstDaughter, int lastDaughter) {
  if (firstDaughter <= 0 || lastDaughter <= 0 || firstDaughter > lastDaughter) return false;
  std::vector<int> ids;
  for (int i = firstDaughter; i <= lastDaughter; ++i) {
    const int idAbs = std::abs(event[i].id());
    if (idAbs == 211 || idAbs == 321) ids.push_back(event[i].id());
  }
  if (ids.size() != 2) return false;
  const bool hasK = std::any_of(ids.begin(), ids.end(), [](int id) { return std::abs(id) == 321; });
  const bool hasPi = std::any_of(ids.begin(), ids.end(), [](int id) { return std::abs(id) == 211; });
  const int chargeSum = (ids[0] > 0 ? 1 : -1) + (ids[1] > 0 ? 1 : -1);
  return hasK && hasPi && chargeSum == 0;
}

bool isCharmHadron(int idAbs) {
  if (idAbs == 421 || idAbs == 411 || idAbs == 431 || idAbs == 4122) return true;
  if (idAbs >= 400 && idAbs < 500) return true;
  if (idAbs >= 4000 && idAbs < 5000) return true;
  return false;
}

void writeCommands(const std::string& path, const std::vector<std::string>& commands) {
  std::ofstream out(path);
  for (const auto& command : commands) out << command << "\n";
}

}  // namespace

int main(int argc, char** argv) {
  const int nEvents = argc > 1 ? std::stoi(argv[1]) : 1000;
  const std::string summaryPath = argc > 2 ? argv[2] : "standalone_pythia_d0_summary.csv";
  const std::string commandsPath = argc > 3 ? argv[3] : "pythia_commands.cmnd";

  std::vector<std::string> commands = {
      "Beams:idA = 2212",
      "Beams:idB = 2212",
      "Beams:eCM = 5362.",
      "HardQCD:hardccbar = on",
      "PhaseSpace:pTHatMin = 2.",
      "421:onMode = off",
      "421:onIfMatch = -321 211",
      "421:onIfMatch = 321 -211",
      "ParticleDecays:limitTau0 = on",
      "ParticleDecays:tau0Max = 10",
      "Next:numberShowInfo = 0",
      "Next:numberShowProcess = 0",
      "Next:numberShowEvent = 0",
  };
  writeCommands(commandsPath, commands);

  Pythia8::Pythia pythia;
  for (const auto& command : commands) {
    if (!pythia.readString(command)) {
      std::cerr << "Failed Pythia command: " << command << "\n";
      return 2;
    }
  }
  if (!pythia.init()) {
    std::cerr << "Pythia initialization failed\n";
    return 2;
  }

  Counts counts;
  std::map<std::string, int> d0PtRapidityBins;
  const std::vector<double> ptEdges = {0.0, 2.0, 5.0, 8.0, 12.0, 30.0};
  const std::vector<double> yEdges = {-5.0, -2.0, -1.0, 0.0, 1.0, 2.0, 5.0};

  for (int iEvent = 0; iEvent < nEvents; ++iEvent) {
    ++counts.eventsAttempted;
    if (!pythia.next()) continue;
    ++counts.eventsAccepted;

    bool eventHasCharm = false;
    bool eventHasD0 = false;
    for (int i = 0; i < pythia.event.size(); ++i) {
      const auto& particle = pythia.event[i];
      const int idAbs = std::abs(particle.id());
      if (isCharmHadron(idAbs)) eventHasCharm = true;
      if (idAbs != 421) continue;

      eventHasD0 = true;
      ++counts.d0Total;
      if (std::abs(particle.y()) < 2.0 && particle.pT() > 2.0 && particle.pT() < 12.0) {
        ++counts.d0InPtRapidityWindow;
      }
      if (isKPiPair(pythia.event, particle.daughter1(), particle.daughter2())) ++counts.d0ToKPi;

      int iptBin = -1;
      int iyBin = -1;
      for (int ipt = 0; ipt + 1 < static_cast<int>(ptEdges.size()); ++ipt) {
        if (particle.pT() >= ptEdges[ipt] && particle.pT() < ptEdges[ipt + 1]) iptBin = ipt;
      }
      for (int iy = 0; iy + 1 < static_cast<int>(yEdges.size()); ++iy) {
        if (particle.y() >= yEdges[iy] && particle.y() < yEdges[iy + 1]) iyBin = iy;
      }
      if (iptBin >= 0 && iyBin >= 0) {
        const std::string key = std::to_string(ptEdges[iptBin]) + "<pt<" +
                                std::to_string(ptEdges[iptBin + 1]) + "," +
                                std::to_string(yEdges[iyBin]) + "<y<" +
                                std::to_string(yEdges[iyBin + 1]);
        ++d0PtRapidityBins[key];
      }
    }
    if (eventHasCharm) ++counts.eventsWithCharmHadron;
    if (eventHasD0) ++counts.eventsWithD0;
  }

  pythia.stat();

  std::ofstream summary(summaryPath);
  summary << "metric,value\n";
  summary << "events_attempted," << counts.eventsAttempted << "\n";
  summary << "events_accepted," << counts.eventsAccepted << "\n";
  summary << "events_with_charm_hadron," << counts.eventsWithCharmHadron << "\n";
  summary << "events_with_d0_or_antid0," << counts.eventsWithD0 << "\n";
  summary << "d0_or_antid0_total," << counts.d0Total << "\n";
  summary << "d0_or_antid0_to_kpi," << counts.d0ToKPi << "\n";
  summary << "d0_or_antid0_2pt12_absy2," << counts.d0InPtRapidityWindow << "\n";
  summary << "pythia_sigma_gen_mb," << std::setprecision(12) << pythia.info.sigmaGen() << "\n";
  summary << "pythia_sigma_err_mb," << std::setprecision(12) << pythia.info.sigmaErr() << "\n";
  summary << "\npt_y_bin,count\n";
  for (const auto& [key, count] : d0PtRapidityBins) summary << key << "," << count << "\n";

  std::cout << "summary=" << summaryPath << "\n";
  std::cout << "commands=" << commandsPath << "\n";
  return 0;
}
