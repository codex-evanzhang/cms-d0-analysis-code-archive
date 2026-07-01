# D0 UPC PbPb Analysis Roadmap

<!-- DOC_OWNER: cms-analysis-manager AN-derived execution roadmap. -->
<!-- TOKEN_NOTE: use this file instead of rereading the AN for section-level workflow. Exact values still need source verification before final claims. -->

Status: draft from the uploaded analysis note, no code inspection required.
Dataset locations updated from the CMS HiForest TWiki on 2026-06-14.

Private source:

- PDF: `private/cms-analysis/analysis-notes/D0_production_in_UPC_PbPb_in_0nXn_events_with_rapidity_gap_AN.pdf`
- Extracted text: `private/cms-analysis/extracted/D0_production_in_UPC_PbPb_in_0nXn_events_with_rapidity_gap_AN.txt`
- Source date in PDF metadata: 2026-06-07
- HiForest TWiki: `https://twiki.cern.ch/twiki/bin/view/CMS/HiForest2025PbPbHF`
  - Relevant anchor: `#2023_PbPb_UPC_Jan_2024_ReReco_Go`
  - Parsed inventory: `research/cms-d0-analysis/output/hiforest2025pbpbhf_datasets.md`
  - Machine-readable inventory: `research/cms-d0-analysis/output/hiforest2025pbpbhf_datasets.yaml`
  - TWiki revision observed: r67, 2026-06-10

Analysis target:

- Reproduce the \(D^0 \to K\pi\) measurement in ultraperipheral PbPb collisions at \(\sqrt{s_{NN}} = 5.36\) TeV.
- Event class: \(0nXn\) with a rapidity gap on the photon-going / \(0n\) side.
- Kinematic range: \(2 < p_T(D^0) < 12\) GeV and \(-2 < y(D^0) < 2\).
- Final observable: corrected differential cross section vs \(p_T\) and \(y\), with comparison to FONLL-based predictions.

## Build Philosophy

- Treat the AN as a specification, not as final code.
- Reproduce all plots and tables from data, MC, and documented corrections.
- Reproduce the analysis as if it had not already been done: every
  analysis-specific processing step, including forest production where it is
  analysis-owned, candidate production, skim/reduction, selections, fits,
  corrections, and plots, should be rerun or independently reimplemented.
- Treat existing skims, merged ROOT files, final plots, and tables as reference
  artifacts for validation only, not as the nominal starting point for the
  reproduced measurement.
- The practical starting point is the least-reduced official input available
  with stable provenance, usually MINIAOD or the documented forest production
  input. Do not attempt to redo central CMS detector reconstruction unless the
  analysis itself changed or owned that reconstruction step.
- Keep every assumption labeled as `from-note`, `from-code`, `from-data`, `from-supervisor`, `assumption`, or `unverified`.
- Do not make final physics claims until the relevant data, correction, and systematic cross-checks are complete.
- Use section gates: do not move a section to `validated` until its inputs, cutflow, plots, and uncertainty treatment are reproducible.
- Cut-derivation guidance from supervisor, recorded 2026-06-17:
  ZDC selection efficiency is data-driven; topological cuts should be optimized
  or validated with MC signal efficiency versus data-sideband background, with
  BDTs allowed if validated; HF cuts should be automated and cross-checked with
  data observables such as track multiplicity to verify hadronic-event
  rejection.

## Section 1: Physics Scope And Observable

Purpose:

- Define the physics question and final measurement before implementation.
- Keep the analysis limited to UPC charm production in the \(0nXn\) topology until the reproduction is complete.

Data needed:

- Final selected data yields in \(D^0\) \(p_T\) and \(y\) bins.
- Corrected \(Xn0n\), \(0nXn\), and symmetrized photon-nucleus views.

Software needed:

- Plotting and table generation tools for final-note outputs.
- Provenance tracking for every final number and plot.

Required decisions:

- Use \(D^0\) and \(\overline{D^0}\) together, then average with the factor in the cross-section formula.
- Keep the \(p_T\) bins aligned with the AN: 2-5, 5-8, and 8-12 GeV.
- Keep rapidity bins aligned with the AN target unless a supervisor-approved alternative is requested.

Outputs and checks:

- One-page conceptual map of the measurement.
- List of final plots and tables to reproduce.
- Explicit mapping from each final plot/table to its input section.

## Section 2: Datasets

Purpose:

- Build the certified data sample and normalization inputs.

Data needed:

- Primary official input from the TWiki Jan 2024 ReReco section:
  - Run range: 374804-375746.
  - MINIAOD datasets: `/HIForward[0-19]/HIRun2023A-16Jan2024-v1/MINIAOD`.
  - These MINIAOD datasets are the preferred starting point for a full-chain
    reproduction when resources and access allow.
- Forest production listed by the TWiki:
  - Forest directories:
    `root://xrootd-vanderbilt.sites.opensciencegrid.org//store/user/jdlang/Run3_PbPbUPC/Forest_2023_Jan2024ReReco_2025Reforest/HIForward[0-19]/crab_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward[0-19]/...`.
  - If these forests were produced as part of the analysis workflow, the
    reproduction should rerun or independently validate the foresting
    configuration from MINIAOD before using them for final claims.
- Existing D0 skims for validation/reference, not as nominal reproduced inputs:
  - Combined D0 skim:
    `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward_Drej-pasor_Trig-4_isL1ZDCOr_microt.root`.
  - Per-PD D0 skims:
    `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_2023PbPbUPC_Jan2024ReReco_20260212Forest_HIForward{0..19}_Drej-pasor_Trig-4.root`.
- Golden JSON:
  `/eos/user/c/cmsdqm/www/CAF/certification/Collisions23HI/Cert_Collisions2023HI_374288_375823_Good_ZDC_Golden.json`.
- Control data listed by the TWiki:
  - EmptyBX skim:
    `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2.root`.
  - EmptyBX forests:
    `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/forest/crab_HiForest_260218_HIEmptyBX_HIRun2023A_PromptReco_v2/260219_210631/0000`.
  - ZeroBias skim:
    `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/Dzero_260212-hfle_HiForest_260218_HIPhysicsRawPrime0-5_HIRun2023A_ZB_374970.root`.
  - ZeroBias forests:
    `/eos/cms/store/group/phys_heavyions/wangj/Forest2023PbPb/forest/crab_HiForest_260218_HIPhysicsRawPrime*_HIRun2023A_ZB_374970/*/0000`.
- Run list and run range from the AN appendix and the TWiki must be reconciled.
- Trigger path availability and prescales.
- OMS / BRIL luminosity inputs.
- ZDC calibration and any ZDC-specific good-run selection used for normalization.

Software needed:

- CMSSW environment matching the AN production chain:
  - Reconstruction context: CMSSW_13_2_4.
  - Foresting context: CMSSW_13_2_6_patch2.
- TWiki Jan 2024 ReReco provenance to reconcile before final claims:
  - CMSSW_13_2_15.
  - Forest 13_2_X.
  - Dfinder 14XX.
- `brilcalc` for luminosity.
- DAS / CMS data bookkeeping access.
- ROOT or compatible forest reader.
- JSON lumisection filter tooling.

Filters and selections:

- Certified lumisection selection from the Golden JSON.
- Trigger-specific run/lumisection filtering.
- TWiki skim/event-filter summary for the Jan 2024 ReReco D0 skim:
  - Forest filters include `pPrimaryVertexFilter`, `pZDCEnergyFilter0nOr`
    with `sumPlus <= 1500 || sumMinus <= 1500`, and HLT.
  - Overview event filter: `hltZDCOr && PVfilter && pZDCEnergyFilter0nOr`.
  - Skim HLT flag: `isZDCOr`.
  - D0 preselection includes `Dpt > 0`, low track-pT thresholds, and the
    documented `pasor` skim selection.
- The skim/reducer step is an analysis step to reproduce. A successful
  reproduction should regenerate the D0 skim from forests or MINIAOD and compare
  event counts, candidate counts, branch schema, cutflow, and basic mass plots
  against the existing skim.
- Later event-level filters should not be mixed into the dataset registry; keep raw sample certification separate from analysis cuts.

Outputs and checks:

- `DATASETS.md` update with the data paths, run list, file counts, and CMSSW/global-tag provenance.
- Validate the 20 per-PD Jan 2024 skim files and the combined skim as reference
  artifacts, then reproduce them before using skim-level quantities in final
  results.
- Validate the Vanderbilt XRootD forest directories with the right proxy/access
  before depending on raw forest-level branches; current local check hit an
  XRootD auth error, while CERN EOS skim paths and JSON paths were visible.
- Run-lumi table matching the AN.
- Luminosity table with command, normtag, JSON, and final value.
- Check that all final analysis files belong to the same certified run/lumi set.

## Section 3: Monte Carlo Samples

Purpose:

- Build all simulated inputs needed for efficiencies, templates, reweighting, and contamination estimates.

MC needed:

- Initial TWiki-listed official prompt signal MC:
  - Skim:
    `/eos/cms/store/group/phys_heavyions/wangj/Forest2024PbPb/Dzero_260212-hfle_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2.root`.
  - Forest directory listed by TWiki:
    `root://xrootd-vanderbilt.sites.opensciencegrid.org//store/user/wangj/prompt-GNucleusToD0-PhotonBeamA_Bin-Pthat0_Fil-Kpi_UPC_5p36TeV_pythia8-evtgen/crab_HiForest_260218_prompt_GNucleusToD0-PhotonBeamA_Bin-Pthat0_Kpi_t2/260218_200449`.
  - MINIAOD dataset label:
    `/prompt-GNucleusToD0-PhotonBeamA_Bin-Pthat0_Fil-Kpi_UPC_5p36TeV_pythia8-evtgen/HINPbPbWinter24MiniAOD-NoPU_UPC_UPC_141X_mcRun3_2024_realistic_HI_v14-v2/MINIAODSIM`.
  - Production context listed by TWiki: `CMSSW_15_1_0_patch4`,
    `forest_CMSSW_15_1_X`, Dfinder 14XX.
  - Existing MC skims are validation artifacts; the MC foresting, candidate
    production, matching, weighting, and efficiency extraction should be
    reproduced from the least-reduced available MC input before final use.
- Photonuclear PYTHIA8 samples with direct and resolved photon processes.
- Prompt and nonprompt \(D^0\) components.
- Beam-A and beam-B photon-direction samples.
- \(p_T\)-hat productions, including low-\(p_T\) samples and higher-\(p_T\) samples for coverage up to 12 GeV.
- Forced \(D^0 \to K\pi\) decay samples where used for signal templates or efficiency.
- Inclusive samples for background and event-property checks.
- HYDJET or equivalent peripheral PbPb samples for hadronic contamination estimates.
- SoftQCD-style samples if needed for low-\(p_T\) shape validation or reweighting.

Software needed:

- PYTHIA8 photonuclear generation configured consistently with the AN.
- EVTGEN for forced charm decays where applicable.
- CMSSW simulation, reconstruction, and foresting chain matching the AN.
- Sample-weight calculator using cross sections, filter efficiencies, and generated event counts.

Filters and classifications:

- \(D^0\)-decay filters for forced-decay samples.
- \(p_T\)-hat bin filters and stitching weights.
- Prompt vs nonprompt classification from generator ancestry.
- Direct vs resolved classification from generator process labels, with the AN caveat that process ID alone is not sufficient for every case.
- Beam direction classification for photon-from-plus vs photon-from-minus samples.

Outputs and checks:

- MC sample registry with path, generator setup, cross section, filter efficiency, event count, and intended use.
- Generator-level \(D^0\) \(p_T,y\) coverage plots.
- Direct/resolved and prompt/nonprompt composition plots.
- \(p_T\)-hat stitching closure plot.
- List of MC samples used by each later section.

## Section 4: Trigger Selection

Purpose:

- Apply the correct online trigger paths and derive trigger-efficiency corrections where needed.

Data needed:

- Trigger bits and prescales for the UPC trigger paths.
- L1 ZDCOR monitoring sample.
- L1 ZDCXOR plus Jet8 sample for higher-\(p_T\) regions.
- Trigger emulation outputs or inputs needed to emulate the L1 jet algorithm.
- Leading-track and leading-jet kinematics for trigger-efficiency maps.

Software needed:

- HLT path selector.
- L1 trigger-emulation or trigger-object reader.
- Efficiency-map builder in track \(p_T,\eta\), and alternative jet \(p_T,\eta\) maps for systematics.
- Trigger-weight applier for analysis histograms.

Filters and corrections:

- Low-\(p_T\) and high-\(p_T\) bins use different UPC trigger path combinations in the AN.
- Pixel-cluster requirements are part of the online path definitions and should be recorded with each trigger.
- L1 ZDC algorithms select OR/XOR topologies using online ZDC sums.
- L1 jet trigger correction is derived from a ZDC-triggered control sample using leading-track efficiency maps.
- Apply reciprocal-efficiency weights to triggered events.
- Flip rapidity convention for the opposite photon direction when constructing the photon-nucleus view.

Outputs and checks:

- Trigger path table by \(p_T\) bin.
- Trigger prescale table.
- L1 ZDC validation curves.
- L1 jet efficiency maps.
- Per-bin trigger correction factors.
- Closure test using MC or emulation.
- Systematic variations from alternative trigger maps.

Open clarification:

- The AN has wording that can be read as inconsistent for the low-\(p_T\) trigger sample naming. Resolve whether the final low-\(p_T\) cross section uses ZDCOR, ZDCXOR, or a combined convention before coding the final formula.

## Section 5: Offline Event Selection

Purpose:

- Select clean UPC \(0nXn\) events and suppress hadronic PbPb, beam background, cosmics, noise, and gamma-gamma contamination.

Data needed:

- Primary vertex information.
- Number of reconstructed vertices.
- MET filter flags, especially halo rejection.
- ZDC rechit or digi information with time-slice energies.
- PF candidates in HF acceptance.
- Empty-bunch or EBX data for HF noise studies.
- MC signal samples for direct/resolved gap efficiency.
- Peripheral HYDJET or equivalent samples for hadronic contamination.

Software needed:

- Event cutflow runner.
- ZDC offline energy builder.
- HF \(E_{\max}\) rapidity-gap calculator.
- EBX/noise validation plotter.
- Hadronic-contamination estimator.

Filters and selections:

- At least one good primary vertex.
- \(|v_z| < 15\) cm.
- Reconstructed vertex multiplicity requirement from the AN; verify whether the intended requirement is `nVtx <= 3` before finalizing.
- Halo-muon rejection using the relevant MET filter.
- Offline ZDC signal built with the AN Appendix E recipe: ADC-to-fC conversion,
  pedestal subtraction, gain correction, OOTPU subtraction, then offline
  `TS2 - TS1`.
- ZDC signal convention: `ZDCsignal = HADsum + 0.1 * EMsum`.
- \(0nXn\) selection from an XOR between the two ZDC sides: plus \(Xn\) and
  minus \(0n\), or minus \(Xn\) and plus \(0n\), but not both.
- ZDC one-neutron thresholds from the AN: plus `1100 GeV`, minus `1000 GeV`.
- ZDC selection efficiency must be established with data-driven control
  samples: EmptyBX for the 0n-side fake rate, fitted 1n/2n/3n spectra for the
  Xn-side efficiency, and an EM-pileup correction/uncertainty. The currently
  checked prompt D0 MC does not provide usable detector-level ZDC neutron
  energy.
- Rapidity-gap requirement using the maximum HF PF-candidate energy on the photon-going / \(0n\) side.
- Nominal HF gap thresholds from the AN, with low and high threshold variations for systematics.
- HF thresholds should be scanned automatically and validated against
  data-level hadronic-activity observables, especially track multiplicity, not
  only EmptyBX detector-noise fake rate.

Outputs and checks:

- Event cutflow table after each cut.
- ZDC one-neutron peak and threshold validation plots.
- \(0n\) inefficiency estimate from EBX.
- \(Xn\) inefficiency estimate from ZDC peak fits.
- HF \(E_{\max}\) noise and signal-shape plots.
- HF-threshold cross-check plots versus track multiplicity and other available
  activity proxies.
- Gap-threshold scan showing stability of corrected yield or cross section.
- Hadronic contamination estimate with explicit uncertainty.

## Section 6: Event Selection Efficiencies

Purpose:

- Correct for UPC event-level selection inefficiencies in \(D^0\) kinematic bins.

MC needed:

- Signal MC with direct/resolved and prompt/nonprompt components.
- \(p_T\)-hat stitched and reweighted MC.
- Generator-level \(D^0\) truth information.

Software needed:

- Gen-level \(D^0\) selector.
- Event-selection efficiency histogrammer.
- Efficiency uncertainty calculator.

Filters and corrections:

- Numerator: generated \(D^0\) events passing the full event-level UPC selection.
- Denominator: generated \(D^0\) events before event-level selection, in the same \(p_T,y\) bins.
- Keep event efficiency separate from detector acceptance and \(D^0\) reconstruction efficiency.

Outputs and checks:

- \(\epsilon_{\mathrm{evt}}(p_T,y)\) maps.
- Separate maps for direct/resolved and prompt/nonprompt subsets for validation.
- Statistical uncertainty from finite MC.
- Sensitivity to MC composition.

## Section 7: D-Meson Selection And Signal Extraction

Purpose:

- Reconstruct \(D^0\) candidates, select displaced decay topologies, and extract raw yields from invariant-mass fits.

Data and MC needed:

- Reconstructed tracks with quality, kinematics, and uncertainty information.
- Secondary-vertex fit inputs.
- Signal MC for shape and swapped-assignment templates.
- Inclusive charm-decay MC for reflection/background templates.
- Data sidebands for background validation.

Software needed:

- \(D^0\) candidate builder.
- ROOT `KinematicParticleVertexFitter` or equivalent secondary-vertex fitter.
- Selection-variable plotting tools.
- Mass-fit framework with constrained/fixed template components.
- Goodness-of-fit and residual plotting tools.

Track and candidate filters:

- Opposite-charge track pairs.
- Both \(K\pi\) and \(\pi K\) mass assignments considered.
- Track high-purity requirement.
- Track \(|\eta| < 2.4\).
- Track \(p_T > 1.0\) GeV.
- Track relative \(p_T\) uncertainty requirement from the AN.
- Candidate invariant-mass window around the PDG \(D^0\) mass.
- Rectangular topological cuts by \(p_T\) and \(|y|\):
  - pointing angle;
  - normalized 3D decay length;
  - secondary-vertex probability;
  - daughter opening angle.
- Alternative BDT-based topological selection may be used if it is trained on
  MC signal and validated against data-sideband background, with overtraining,
  mass-shape-bias, and bin-by-bin efficiency checks.

Signal extraction model:

- Signal: double Gaussian with common mean.
- Combinatorial background: exponential nominal model.
- Swapped mass assignment: Gaussian-like component constrained from MC.
- \(D^0 \to K^+K^-\) and \(D^0 \to \pi^+\pi^-\) reflections: fixed-shape components from dedicated MC.
- Use combined fits where the AN uses them to obtain resolution scale factors or template constraints.

Outputs and checks:

- Candidate cut table by \(p_T\) and \(|y|\).
- Signal-vs-background variable comparisons.
- Sideband definition and sideband validation.
- MC signal-efficiency versus data-sideband background-rejection scans for the
  chosen rectangular or BDT topological selection.
- Mass spectra for every \(p_T,y\) bin.
- Fit-result table: raw yield, uncertainty, mean, width, p-value, and fit status.
- Residual/pull plots.
- Reflection-yield and swapped-yield constraints recorded.

## Section 8: D-Meson Efficiency

Purpose:

- Measure acceptance times reconstruction times selection efficiency for \(D^0\) candidates.

MC needed:

- Inclusive UPC photoproduction \(D^0\) MC.
- Generator truth for \(D^0\), decay daughters, prompt/nonprompt ancestry, direct/resolved origin.
- Reconstructed candidate matching to truth.

Software needed:

- Gen-reco matcher.
- Efficiency-map builder.
- Reweighting-aware efficiency calculator.

Filters and corrections:

- Denominator: generated \(D^0\) in the analysis kinematic bin.
- Numerator: generated \(D^0\) that is accepted, reconstructed, selected, and matched.
- Keep this correction separate from event-selection efficiency and trigger correction.
- Study prompt/nonprompt and direct/resolved efficiency differences as systematic inputs.

Outputs and checks:

- \(\alpha\epsilon_{D^0}(p_T,y)\) nominal map.
- Separate prompt, nonprompt, direct, and resolved maps.
- MC statistical uncertainty.
- Data/MC validation plots for selection variables.

## Section 9: MC Reweighting

Purpose:

- Reduce bias from mismodeled generator \(D^0\) kinematics and event composition.

Inputs needed:

- PYTHIA generator-level \(D^0\) \(p_T,y\) spectra.
- FONLL-based \(D^0\) \(p_T,y\) predictions with the AN's nominal PDF choices.
- Optional data-derived or multiplicity-derived weights used as systematic alternatives.
- \(p_T\)-hat bin weights.

Software needed:

- 2D \(p_T,y\) weight-table builder.
- Weight applier for efficiency calculations.
- Weight validation and closure plots.

Weights and variations:

- Nominal \(p_T\)-hat stitching weights.
- Nominal FONLL/PYTHIA \(D^0\) kinematic weights.
- Alternative no-FONLL or data-driven weights for systematics.
- Optional multiplicity reweighting if needed to reproduce event-property distributions.

Outputs and checks:

- Weight maps and ratio plots.
- Efficiency comparison before and after reweighting.
- Systematic difference between nominal and alternative weights.
- Check for large or unstable weights in sparse bins.

## Section 10: Cross-Section Calculation

Purpose:

- Combine raw yields, luminosity, bin widths, branching fraction, prescales, efficiencies, and pileup corrections into the final differential cross section.

Inputs needed:

- Raw \(D^0+\overline{D^0}\) yields from Section 7.
- Integrated luminosity and normalization from Section 2 / Appendix I.
- \(D^0 \to K\pi\) branching fraction.
- Bin widths in \(p_T\) and \(y\).
- Trigger correction and prescale.
- Event-selection efficiency.
- \(D^0\) acceptance/reconstruction/selection efficiency.
- EMD pileup correction.

Software needed:

- Cross-section calculator.
- Uncertainty propagation script.
- Table and plot renderer.

Formula components:

- Particle/antiparticle averaging factor.
- Luminosity.
- Branching ratio.
- \(p_T\) and \(y\) bin widths.
- Raw yield.
- Event efficiency.
- Trigger correction and prescale.
- \(D^0\) efficiency.
- EMD pileup correction.

Outputs and checks:

- Cross-section table per \(p_T,y\) bin.
- Separate \(Xn0n\) and \(0nXn\) results.
- Symmetrized photon-nucleus result.
- Unit check.
- Zero/negative-yield and low-efficiency safeguards.
- Comparison to AN tables/plots before any new interpretation.

## Section 11: Systematic Uncertainties

Purpose:

- Build a reproducible variation pipeline for every uncertainty source.

Systematic sources to implement:

- Luminosity.
- Trigger efficiency correction.
- Event-selection efficiency.
- \(D^0\) selection efficiency.
- MC reweighting.
- Track reconstruction efficiency.
- Signal extraction model.
- \(D^0 \to K\pi\) branching fraction.
- HF rapidity-gap threshold.
- Prompt/nonprompt fraction.
- EMD pileup correction.
- Direct/resolved composition where applicable.

Software needed:

- Variation runner that can rerun only the affected analysis layer.
- Systematic table builder.
- Quadrature combiner with source grouping.
- Plotter for per-source and total uncertainty.

Required variations:

- Trigger: alternative leading-track and leading-jet efficiency maps, plus closure checks.
- Event selection: MC statistical uncertainty, EMD pileup variation, direct/resolved composition variation.
- \(D^0\) selection: loosen/tighten topological and track selections as specified by the AN.
- MC reweighting: nominal vs alternative kinematic or multiplicity weights.
- Signal extraction: fixed mean, fixed widths, alternate background model, and reflection treatment.
- Gap selection: low and high HF \(E_{\max}\) thresholds.
- Prompt fraction: DCA template fit and assigned prompt/nonprompt uncertainty.

Outputs and checks:

- One table per systematic source.
- Combined uncertainty table by \(p_T,y\) bin.
- Plot showing total and dominant uncertainty sources.
- Clear separation of bin-correlated vs bin-uncorrelated sources if needed for final interpretation.

## Section 12: Results

Purpose:

- Assemble the corrected measurement after all analysis sections are validated.

Inputs needed:

- Nominal cross sections.
- Full statistical and systematic uncertainties.
- \(Xn0n\), \(0nXn\), and symmetrized results.
- Trigger and normalization metadata.

Software needed:

- Final plotting script.
- Table exporter for the analysis note.
- Ratio and compatibility plot tools.

Outputs and checks:

- Final differential cross-section plots vs \(p_T\) and \(y\).
- \(Xn0n\) vs \(0nXn\) compatibility plots.
- Symmetrized photon-nucleus result.
- Ratio plots against the AN reference.
- Supervisor-facing summary with only verified statements.

## Sections 13-14: FONLL Predictions And Comparison

Purpose:

- Compare the measured \(D^0\) cross sections to perturbative-QCD predictions.

Theory inputs needed:

- FONLL calculation setup used by the AN.
- Photon PDF choice.
- Lead and proton PDF/nPDF choices.
- Charm fragmentation fraction and fragmentation-ratio conventions.
- Effective photon flux and EMD correction.
- Validation references for HERA ep and CMS pp comparisons.

Software needed:

- FONLL execution environment or imported official prediction tables.
- PDF set support, likely through LHAPDF or the AN's existing theory workflow.
- UPC photon-flux and EMD correction tools or tables.
- Theory/data comparison plotter.

Outputs and checks:

- FONLL validation against HERA and pp reference data.
- UPC theory prediction table in the same bins as the measurement.
- Data/theory overlay and ratio plots.
- Theory uncertainty band if available.
- Explicit note of what theory inputs are external and not reproduced locally.

## Section 15: Conclusions

Purpose:

- Write only the conclusions supported by the completed reproduction.

Required before drafting:

- All final plots pass section checks.
- Cross-section table is reproducible from raw inputs.
- Systematics table is reproducible from variation outputs.
- Theory comparison is either reproduced or clearly labeled as imported.
- All open ambiguities are resolved or explicitly listed.

Outputs:

- Short conclusion paragraph for the analysis note.
- List of limitations and possible future extensions.
- Clear statement of whether the reproduction passed.

## Appendices: Operational Roadmap

Appendix A, data sample run range:

- Build the canonical run-range table.
- Cross-check run list against Golden JSON and trigger availability.

Appendix B, \(p_T\)-hat reweighting:

- Record each \(p_T\)-hat sample, event count, cross section, filter efficiency, and stitching weight.
- Validate stitched generator-level spectra.

Appendix C, PYTHIA production:

- Archive generator settings, process switches, beam direction handling, and decay settings.
- Link each production configuration to the MC registry.

Appendix D, additional MC samples:

- Keep private and official sample lists separate.
- Record which samples are used for nominal corrections, validation, or systematic alternatives.

Appendix E, ZDC energy-sum construction:

- Implement the offline ZDC signal recipe exactly once in a reusable helper.
- Validate one-neutron peak positions and threshold choices.

Appendix F, ZDC trigger list:

- Build a trigger dictionary with HLT path, L1 seed, prescale, online threshold, and intended analysis bin.

Appendix G, MET filters:

- Record each filter used, its purpose, and whether it is applied to data, MC, or both.
- Keep beam-halo, noise, and cosmics rejection separate in the cutflow.

Appendix H, glossary:

- Maintain a compact variable glossary to prevent branch-name and physics-variable confusion.

Appendix I, global normalization:

- Save every `brilcalc` command, JSON, normtag, and calibration scale factor.
- Keep preliminary and final luminosity values distinct.

Appendix J, EMD pileup:

- Implement the EMD survival correction and its uncertainty.
- Record whether the correction is applied globally or bin-by-bin.

Appendix K, Bjorken-x relation:

- Generate generator-level maps relating \(D^0\) \(p_T,y\) to photon energy, \(x\), and \(Q^2\).
- Use these only for interpretation after the measured cross sections are validated.

## End-To-End Execution Order

1. Source registry:
   collect AN, paper draft, sample lists, trigger list, JSON, luminosity notes, MC production notes, and existing target plots.
2. Dataset inventory:
   verify data paths, run list, lumisections, trigger path availability, and forest branches.
3. MC inventory:
   verify MC paths, event counts, generator categories, cross sections, filter efficiencies, and truth branches.
4. Event-selection reproduction:
   implement certified data filtering, trigger selection, PV/MET filters, ZDC XOR, and HF gap; compare cutflows and event plots.
5. Candidate reconstruction:
   build \(D^0\) candidates, apply track/topology cuts, and compare candidate variable distributions.
6. Signal extraction:
   fit invariant-mass spectra in each analysis bin and reproduce raw yields.
7. Corrections:
   derive trigger, event-selection, \(D^0\) efficiency, MC-reweighting, prescale, luminosity, and EMD corrections.
8. Cross section:
   compute corrected cross sections and reproduce the AN result.
9. Systematics:
   run all variations and combine uncertainties.
10. Theory comparison:
    reproduce or import validated FONLL predictions and compare to data.
11. Note assembly:
    generate final plots/tables, write concise text, and mark all unsupported claims as open.

## Immediate Implementation Checklist

- [ ] Create `SOURCES.md` with the AN PDF, extracted text, paper draft, and external theory references.
- [ ] Create a machine-readable sample registry for 2023 data and MC.
- [ ] Extract all AN plot/table targets into `TARGET_OUTPUTS.md`.
- [ ] Build a trigger dictionary from Section 4 and Appendix F.
- [ ] Build a filter dictionary from Sections 5, 7, and Appendix G.
- [ ] Build a correction dictionary from Sections 6, 8, 9, 10, I, and J.
- [ ] Build a systematic-variation dictionary from Section 11.
- [ ] Define reproduction tolerances with Evan/supervisor before claiming success.

## Ambiguities To Resolve Early

- Confirm the exact vertex multiplicity cut; the prose and extracted text should be checked against the PDF around Section 5.
- Confirm low-\(p_T\) trigger naming and sample usage before implementing Section 10.
- Confirm the canonical final luminosity, JSON, normtag, and ZDC calibration scale factor.
- Confirm official MC sample paths and whether any private MC samples are allowed for final outputs.
- Confirm whether theory predictions must be rerun locally or imported from official validated tables.
- Confirm which plots/tables from the AN are mandatory reproduction targets and what tolerance defines agreement.
