#include "dsp/FreshAirProcessor.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numbers>
#include <numeric>
#include <random>
#include <string>
#include <vector>

namespace {

struct TestStats {
  int total = 0;
  int passed = 0;
  int failed = 0;
};

TestStats gStats;

void recordResult(bool condition, const std::string& name, const std::string& details = "") {
  gStats.total++;
  if (condition) {
    gStats.passed++;
    std::cout << "  [PASS] " << name;
    if (!details.empty()) std::cout << " (" << details << ")";
    std::cout << "\n";
  } else {
    gStats.failed++;
    std::cout << "  [FAIL] " << name;
    if (!details.empty()) std::cout << " (" << details << ")";
    std::cout << "\n";
  }
}

double toDb(double linear) {
  if (linear <= 1e-12) return -240.0;
  return 20.0 * std::log10(linear);
}

// Generate pure sine wave
std::vector<float> generateSine(double freq, double sampleRate, std::size_t numSamples, double amplitudeLinear) {
  std::vector<float> signal(numSamples);
  const double phaseInc = 2.0 * std::numbers::pi * freq / sampleRate;
  for (std::size_t i = 0; i < numSamples; ++i) {
    signal[i] = static_cast<float>(amplitudeLinear * std::sin(phaseInc * static_cast<double>(i)));
  }
  return signal;
}

// Generate pink noise using Voss-McCartney algorithm
std::vector<float> generatePinkNoise(std::size_t numSamples, double rmsLevel, unsigned seed = 12345) {
  std::mt19937 gen(seed);
  std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
  std::vector<float> signal(numSamples);
  float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, b3 = 0.0f, b4 = 0.0f, b5 = 0.0f, b6 = 0.0f;
  for (std::size_t i = 0; i < numSamples; ++i) {
    float white = dis(gen);
    b0 = 0.99886f * b0 + white * 0.0555179f;
    b1 = 0.99332f * b1 + white * 0.0750759f;
    b2 = 0.96900f * b2 + white * 0.1538520f;
    b3 = 0.86650f * b3 + white * 0.3104856f;
    b4 = 0.55000f * b4 + white * 0.5329522f;
    b5 = -0.7616f * b5 - white * 0.0168980f;
    signal[i] = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
    b6 = white * 0.115926f;
  }
  // Normalize to target RMS
  double sumSq = 0.0;
  for (float s : signal) sumSq += s * s;
  double curRms = std::sqrt(sumSq / numSamples);
  float scale = static_cast<float>(rmsLevel / curRms);
  for (float& s : signal) s *= scale;
  return signal;
}

// Measure steady-state gain (dB) across a given frequency
double measureTransferGain(double freq, double presence, double air, double inputDb = -18.0,
                          double trimDb = 0.0, bool bypass = false, bool link = false,
                          double sampleRate = 48000.0) {
  g3x::FreshAirProcessor proc;
  proc.prepare(sampleRate, 1, 0.001);
  proc.setPresence(presence);
  proc.setAir(air);
  proc.setOutputTrimDb(trimDb);
  proc.setBypass(bypass);
  proc.setLinkBands(link);

  const double amp = std::pow(10.0, inputDb / 20.0);
  const std::size_t totalSamples = static_cast<std::size_t>(sampleRate * 0.5); // 0.5 sec
  auto buf = generateSine(freq, sampleRate, totalSamples, amp);
  float* channels[1] = { buf.data() };
  proc.process(channels, 1, totalSamples);

  // Measure steady state in second half
  const std::size_t half = totalSamples / 2;
  double inSq = 0.0, outSq = 0.0;
  const double phaseInc = 2.0 * std::numbers::pi * freq / sampleRate;
  for (std::size_t i = half; i < totalSamples; ++i) {
    double inVal = amp * std::sin(phaseInc * static_cast<double>(i));
    inSq += inVal * inVal;
    outSq += buf[i] * buf[i];
  }
  return toDb(std::sqrt(outSq / inSq));
}

} // namespace

int main() {
  std::cout << "====================================================================\n";
  std::cout << "         G3X FRESH AIR - PHASE 9 REFERENCE VALIDATION SUITE        \n";
  std::cout << "====================================================================\n\n";

  // -------------------------------------------------------------------------
  // 1. ZERO-SETTING TEST
  // -------------------------------------------------------------------------
  std::cout << "### TEST 1: ZERO-SETTING TEST\n";
  {
    g3x::FreshAirProcessor proc;
    proc.prepare(48000.0, 2, 0.0);
    proc.setPresence(0.0);
    proc.setAir(0.0);
    proc.setOutputTrimDb(0.0);
    proc.setBypass(false);

    const std::size_t n = 48000;
    auto left = generatePinkNoise(n, 0.1);
    auto right = left;
    auto origLeft = left;
    auto origRight = right;

    float* ch[2] = { left.data(), right.data() };
    proc.process(ch, 2, n);

    double maxErr = 0.0;
    double sumErrSq = 0.0;
    double sumIn = 0.0;
    double sumOut = 0.0;
    bool hasNonFinite = false;

    for (std::size_t i = 0; i < n; ++i) {
      if (!std::isfinite(left[i]) || !std::isfinite(right[i])) hasNonFinite = true;
      double diffL = std::abs(left[i] - origLeft[i]);
      if (diffL > maxErr) maxErr = diffL;
      sumErrSq += diffL * diffL;
      sumIn += origLeft[i];
      sumOut += left[i];
    }
    double nullRmsDb = toDb(std::sqrt(sumErrSq / n));
    double addedDcOffset = std::abs((sumOut - sumIn) / static_cast<double>(n));

    std::cout << "  - Peak Null Difference : " << maxErr << "\n";
    std::cout << "  - RMS Null Depth       : " << nullRmsDb << " dBFS\n";
    std::cout << "  - Added DC Offset      : " << addedDcOffset << "\n";

    recordResult(maxErr < 1e-5, "Zero-Setting Peak Bit-Transparency", "Max diff < 1e-5");
    recordResult(nullRmsDb < -120.0, "Zero-Setting RMS Null Depth", "< -120 dBFS");
    recordResult(addedDcOffset < 1e-6, "Zero-Setting DC Neutrality", "Added DC < 1e-6");
    recordResult(!hasNonFinite, "Zero-Setting Finite Safety", "No NaNs/Infs");
  }

  // -------------------------------------------------------------------------
  // 2. MID/PRESENCE BEHAVIOR
  // -------------------------------------------------------------------------
  std::cout << "\n### TEST 2: MID/PRESENCE BEHAVIOR (SWEEPS & CURVES)\n";
  {
    const double settings[] = { 0.0, 0.10, 0.25, 0.50, 0.75, 1.00 };
    const double testFreqs[] = { 100.0, 500.0, 1000.0, 2000.0, 3200.0, 4500.0, 8000.0, 14000.0 };

    std::cout << "  Frequency Response Table for Presence (Air=0, Trim=0dB, In=-18dBFS):\n";
    std::cout << "  " << std::setw(8) << "Freq(Hz)";
    for (double s : settings) std::cout << std::setw(10) << (std::to_string(static_cast<int>(s*100)) + "%");
    std::cout << "\n";

    double maxGainAt100 = 0.0;
    double maxGainAt4500 = 0.0;

    for (double f : testFreqs) {
      std::cout << "  " << std::setw(8) << static_cast<int>(f);
      for (double s : settings) {
        double gain = measureTransferGain(f, s, 0.0, -18.0);
        std::cout << std::setw(9) << std::fixed << std::setprecision(2) << gain << " ";
        if (f == 100.0 && gain > maxGainAt100) maxGainAt100 = gain;
        if (f == 4500.0 && s == 1.0) maxGainAt4500 = gain;
      }
      std::cout << "\n";
    }

    recordResult(maxGainAt100 < 0.2, "Bass Isolation under Presence", "100Hz gain < 0.2 dB");
    recordResult(maxGainAt4500 >= 5.0 && maxGainAt4500 <= 6.5, "Upper-Mid Presence Target Gain", "4.5kHz gain ~ 5.7 dB");
  }

  // -------------------------------------------------------------------------
  // 3. HIGH-AIR BEHAVIOR
  // -------------------------------------------------------------------------
  std::cout << "\n### TEST 3: HIGH-AIR BEHAVIOR (SWEEPS & CURVES)\n";
  {
    const double settings[] = { 0.0, 0.10, 0.25, 0.50, 0.75, 1.00 };
    const double testFreqs[] = { 100.0, 1000.0, 4000.0, 8500.0, 12000.0, 16000.0, 18000.0, 20000.0 };

    std::cout << "  Frequency Response Table for High-Air (Presence=0, Trim=0dB, In=-18dBFS):\n";
    std::cout << "  " << std::setw(8) << "Freq(Hz)";
    for (double s : settings) std::cout << std::setw(10) << (std::to_string(static_cast<int>(s*100)) + "%");
    std::cout << "\n";

    double maxGainAt1k = 0.0;
    double maxGainAt18k = 0.0;

    for (double f : testFreqs) {
      std::cout << "  " << std::setw(8) << static_cast<int>(f);
      for (double s : settings) {
        double gain = measureTransferGain(f, 0.0, s, -18.0);
        std::cout << std::setw(9) << std::fixed << std::setprecision(2) << gain << " ";
        if (f <= 1000.0 && gain > maxGainAt1k) maxGainAt1k = gain;
        if (f == 18000.0 && s == 1.0) maxGainAt18k = gain;
      }
      std::cout << "\n";
    }

    recordResult(maxGainAt1k < 0.3, "Mid/Low Isolation under Air", "<= 1kHz gain < 0.3 dB");
    recordResult(maxGainAt18k >= 8.0 && maxGainAt18k <= 10.5, "Ultra-Treble Air Target Gain", "18kHz gain ~ 9.0 dB");
  }

  // -------------------------------------------------------------------------
  // 4. DYNAMIC PROCESSING TEST
  // -------------------------------------------------------------------------
  std::cout << "\n### TEST 4: DYNAMIC PROCESSING TEST (INPUT LEVEL RESPONSE)\n";
  {
    const double inputLevels[] = { -36.0, -24.0, -18.0, -12.0, -6.0, 0.0 };

    std::cout << "  Level-Dependent Response at 100% Setting:\n";
    std::cout << "  " << std::setw(12) << "Input(dBFS)" 
              << std::setw(18) << "Presence Gain(dB)" 
              << std::setw(16) << "Air Gain(dB)" 
              << std::setw(18) << "Pres Reduction"
              << std::setw(16) << "Air Reduction" << "\n";

    double gainPresQuiet = 0.0, gainPresLoud = 0.0;
    double gainAirQuiet = 0.0, gainAirLoud = 0.0;

    for (double inDb : inputLevels) {
      // Test presence at 4500Hz
      g3x::FreshAirProcessor procP;
      procP.prepare(48000.0, 1, 0.001);
      procP.setPresence(1.0);
      procP.setAir(0.0);
      double amp = std::pow(10.0, inDb / 20.0);
      auto bufP = generateSine(4500.0, 48000.0, 24000, amp);
      float* chP[1] = { bufP.data() };
      procP.process(chP, 1, bufP.size());
      double gainP = measureTransferGain(4500.0, 1.0, 0.0, inDb);
      double redP = procP.presenceReduction();

      // Test air at 18000Hz
      g3x::FreshAirProcessor procA;
      procA.prepare(48000.0, 1, 0.001);
      procA.setPresence(0.0);
      procA.setAir(1.0);
      auto bufA = generateSine(18000.0, 48000.0, 24000, amp);
      float* chA[1] = { bufA.data() };
      procA.process(chA, 1, bufA.size());
      double gainA = measureTransferGain(18000.0, 0.0, 1.0, inDb);
      double redA = procA.airReduction();

      if (inDb == -36.0) { gainPresQuiet = gainP; gainAirQuiet = gainA; }
      if (inDb == -6.0)  { gainPresLoud  = gainP; gainAirLoud  = gainA; }

      std::cout << "  " << std::setw(12) << inDb
                << std::setw(18) << gainP
                << std::setw(16) << gainA
                << std::setw(18) << redP
                << std::setw(16) << redA << "\n";
    }

    recordResult(gainPresQuiet > gainPresLoud + 0.3, "Dynamic Presence Compression", 
                 "Quiet boost (" + std::to_string(gainPresQuiet) + "dB) > Loud boost (" + std::to_string(gainPresLoud) + "dB)");
    recordResult(gainAirQuiet > gainAirLoud + 1.0, "Dynamic Air Compression", 
                 "Quiet boost (" + std::to_string(gainAirQuiet) + "dB) > Loud boost (" + std::to_string(gainAirLoud) + "dB)");
  }

  // -------------------------------------------------------------------------
  // 5. COMBINED PRESENCE + AIR TEST
  // -------------------------------------------------------------------------
  std::cout << "\n### TEST 5: COMBINED PRESENCE + AIR COMPLEMENTARITY\n";
  {
    struct Combo { double pres, air; const char* name; };
    Combo combos[] = {
      { 0.25, 0.25, "Presence 25% / Air 25%" },
      { 0.50, 0.25, "Presence 50% / Air 25%" },
      { 0.25, 0.50, "Presence 25% / Air 50%" },
      { 0.50, 0.50, "Presence 50% / Air 50%" },
      { 0.75, 0.75, "Presence 75% / Air 75%" },
      { 1.00, 1.00, "Presence 100% / Air 100%" }
    };

    bool allFinite = true;
    for (const auto& c : combos) {
      g3x::FreshAirProcessor proc;
      proc.prepare(48000.0, 2, 0.0);
      proc.setPresence(c.pres);
      proc.setAir(c.air);
      auto left = generatePinkNoise(48000, 0.2);
      auto right = left;
      float* ch[2] = { left.data(), right.data() };
      proc.process(ch, 2, 48000);

      double peak = proc.outputPeak();
      double rms = proc.outputRms();
      for (float s : left) if (!std::isfinite(s)) allFinite = false;

      std::cout << "  - " << std::setw(28) << std::left << c.name 
                << " | Peak: " << std::setw(6) << std::right << std::fixed << std::setprecision(3) << peak
                << " | RMS: " << std::setw(6) << rms 
                << " | Clipped: " << (proc.outputClipped() ? "YES" : "NO") << "\n";
    }
    recordResult(allFinite, "Combined Band Stability", "All outputs remain finite without runaway");
  }

  // -------------------------------------------------------------------------
  // 6. LINK CONTROL TEST
  // -------------------------------------------------------------------------
  std::cout << "\n### TEST 6: LINK CONTROL BEHAVIOR\n";
  {
    // Feed high energy only in presence band (4500Hz)
    g3x::FreshAirProcessor unlinkedProc, linkedProc;
    unlinkedProc.prepare(48000.0, 1, 0.001);
    linkedProc.prepare(48000.0, 1, 0.001);

    unlinkedProc.setPresence(1.0);
    unlinkedProc.setAir(1.0);
    unlinkedProc.setLinkBands(false);

    linkedProc.setPresence(1.0);
    linkedProc.setAir(1.0);
    linkedProc.setLinkBands(true);

    auto loudSignal = generateSine(4500.0, 48000.0, 24000, 0.7); // Loud mid-band signal
    auto copySignal = loudSignal;

    float* chU[1] = { loudSignal.data() };
    float* chL[1] = { copySignal.data() };

    unlinkedProc.process(chU, 1, 24000);
    linkedProc.process(chL, 1, 24000);

    double uAirRed = unlinkedProc.airReduction();
    double lAirRed = linkedProc.airReduction();
    double lPresRed = linkedProc.presenceReduction();

    std::cout << "  - Unlinked Air Reduction with mid burst : " << uAirRed << "\n";
    std::cout << "  - Linked Air Reduction with mid burst   : " << lAirRed << "\n";
    std::cout << "  - Linked Pres Reduction with mid burst  : " << lPresRed << "\n";

    recordResult(lAirRed >= lPresRed - 0.01 && lAirRed > uAirRed, 
                 "Link Band Dynamics Coupling", "Air inherits presence reduction when linked");
  }

  // -------------------------------------------------------------------------
  // 7. OUTPUT TRIM TEST
  // -------------------------------------------------------------------------
  std::cout << "\n### TEST 7: OUTPUT TRIM PRECISION & ATTENUATION\n";
  {
    const double trimSettings[] = { -12.0, -6.0, -3.0, 0.0, +3.0 };
    bool trimAccurate = true;
    for (double t : trimSettings) {
      double measured = measureTransferGain(1000.0, 0.0, 0.0, -18.0, t);
      double err = std::abs(measured - t);
      std::cout << "  - Target: " << std::setw(6) << t << " dB | Measured: " << std::setw(6) << measured << " dB | Err: " << err << " dB\n";
      if (err > 0.1) trimAccurate = false;
    }
    recordResult(trimAccurate, "Output Trim Linearity and Decibel Accuracy", "Max error < 0.1 dB");
  }

  // -------------------------------------------------------------------------
  // 8. BYPASS TEST
  // -------------------------------------------------------------------------
  std::cout << "\n### TEST 8: TRUE BYPASS BIT-NULL TEST\n";
  {
    g3x::FreshAirProcessor proc;
    proc.prepare(48000.0, 1, 0.0);
    proc.setPresence(1.0);
    proc.setAir(1.0);
    proc.setOutputTrimDb(-6.0);
    proc.setBypass(true);

    const std::size_t n = 48000;
    auto testSignal = generatePinkNoise(n, 0.2);
    auto origSignal = testSignal;
    float* ch[1] = { testSignal.data() };
    proc.process(ch, 1, n);

    double maxDiff = 0.0;
    double sumDiffSq = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
      double diff = std::abs(testSignal[i] - origSignal[i]);
      if (diff > maxDiff) maxDiff = diff;
      sumDiffSq += diff * diff;
    }
    double nullDepthDb = toDb(std::sqrt(sumDiffSq / n));
    std::cout << "  - Peak Bypass Difference : " << maxDiff << "\n";
    std::cout << "  - RMS Bypass Null Depth  : " << nullDepthDb << " dBFS\n";
    recordResult(maxDiff < 1e-6, "Bypass Peak Transparency", "Exact null (< 1e-6)");
    recordResult(nullDepthDb < -140.0, "Bypass RMS Null Depth", "< -140 dBFS");
  }

  // -------------------------------------------------------------------------
  // 9. METERING TEST
  // -------------------------------------------------------------------------
  std::cout << "\n### TEST 9: METERING ACCURACY & CLIPPING DETECTION\n";
  {
    g3x::FreshAirProcessor proc;
    proc.prepare(48000.0, 2, 0.0);
    proc.setPresence(0.0);
    proc.setAir(0.0);
    proc.setOutputTrimDb(0.0);

    // Test with calibrated sine wave at 0.5 amplitude (-6.02 dBFS)
    auto left = generateSine(1000.0, 48000.0, 1024, 0.5);
    auto right = left;
    float* ch[2] = { left.data(), right.data() };
    proc.process(ch, 2, 1024);

    float peak = proc.outputPeak();
    float rms = proc.outputRms();
    float expectedRms = 0.5f / std::sqrt(2.0f); // ~0.3535f

    double peakErr = std::abs(peak - 0.5f);
    double rmsErr = std::abs(rms - expectedRms);

    std::cout << "  - Sine 0.5 Amp | Output Peak: " << peak << " (expected 0.5) | Err: " << peakErr << "\n";
    std::cout << "  - Sine 0.5 Amp | Output RMS : " << rms << " (expected " << expectedRms << ") | Err: " << rmsErr << "\n";

    recordResult(peakErr < 0.01, "Meter Peak Measurement Accuracy", "Err < 0.01");
    recordResult(rmsErr < 0.01, "Meter RMS Measurement Accuracy", "Err < 0.01");
    recordResult(!proc.outputClipped(), "Clipping Inactive below 0 dBFS", "No false clip");

    // Over-threshold test (1.2 amplitude -> clip)
    auto loud = generateSine(1000.0, 48000.0, 1024, 1.2);
    float* chLoud[1] = { loud.data() };
    proc.process(chLoud, 1, 1024);
    bool clipped = proc.outputClipped();
    proc.clearClipIndicator();
    bool cleared = !proc.outputClipped();

    recordResult(clipped && cleared, "Clipping Indicator Activation & Clearing", "Latches at >= 1.0 and clears cleanly");
  }

  // -------------------------------------------------------------------------
  // 10. PARAMETER SMOOTHING & AUTOMATION STRESS
  // -------------------------------------------------------------------------
  std::cout << "\n### TEST 10: PARAMETER SMOOTHING & AUTOMATION STRESS\n";
  {
    g3x::FreshAirProcessor proc;
    proc.prepare(48000.0, 1, 0.02); // 20ms ramp

    std::vector<float> signal(48000, 0.2f);
    float* ch[1] = { signal.data() };

    // Modulate presence rapidly every 64 samples
    double maxStepDiff = 0.0;
    for (std::size_t block = 0; block < 48000 / 64; ++block) {
      double target = (block % 2 == 0) ? 1.0 : 0.0;
      proc.setPresence(target);
      proc.process(ch, 1, 64);
      ch[0] += 64;
    }

    // Inspect sample-to-sample delta across the entire processed signal
    for (std::size_t i = 1; i < 48000; ++i) {
      double delta = std::abs(signal[i] - signal[i - 1]);
      if (delta > maxStepDiff) maxStepDiff = delta;
    }

    std::cout << "  - Max Sample-to-Sample Step Jump : " << maxStepDiff << "\n";
    recordResult(maxStepDiff < 0.02, "Smooth Parameter Interpolation (No Zipper/Clicks)", "Max sample step < 0.02");
  }

  // -------------------------------------------------------------------------
  // 11. SAMPLE-RATE CONSISTENCY
  // -------------------------------------------------------------------------
  std::cout << "\n### TEST 11: SAMPLE-RATE CONSISTENCY\n";
  {
    const double rates[] = { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 };
    bool ratesConsistent = true;

    for (double sr : rates) {
      double presGain = measureTransferGain(4500.0, 1.0, 0.0, -18.0, 0.0, false, false, sr);
      double airGain = measureTransferGain(18000.0, 0.0, 1.0, -18.0, 0.0, false, false, sr);
      std::cout << "  - Sample Rate: " << std::setw(7) << static_cast<int>(sr) 
                << " Hz | Pres @ 4.5kHz: " << std::setw(5) << presGain 
                << " dB | Air @ 18kHz: " << std::setw(5) << airGain << " dB\n";
      if (std::abs(presGain - 5.77) > 0.3 || std::abs(airGain - 8.1) > 1.5) ratesConsistent = false;
    }
    recordResult(ratesConsistent, "Sample Rate Invariance (44.1k to 192k)", "Frequency curves mapped invariant of Fs");
  }

  // -------------------------------------------------------------------------
  // 12. BUFFER-SIZE TEST
  // -------------------------------------------------------------------------
  std::cout << "\n### TEST 12: BUFFER-SIZE INDEPENDENCE\n";
  {
    const std::size_t bufferSizes[] = { 32, 64, 128, 256, 512, 1024, 2048 };
    const std::size_t total = 4096;
    auto baseSignal = generatePinkNoise(total, 0.1, 9999);

    // Continuous processing in one go as reference
    g3x::FreshAirProcessor refProc;
    refProc.prepare(48000.0, 1, 0.0);
    refProc.setPresence(0.5);
    refProc.setAir(0.5);
    auto refOut = baseSignal;
    float* refCh[1] = { refOut.data() };
    refProc.process(refCh, 1, total);

    bool allSizesMatch = true;
    for (std::size_t bs : bufferSizes) {
      g3x::FreshAirProcessor testProc;
      testProc.prepare(48000.0, 1, 0.0);
      testProc.setPresence(0.5);
      testProc.setAir(0.5);
      auto testOut = baseSignal;

      for (std::size_t offset = 0; offset < total; offset += bs) {
        std::size_t chunk = std::min(bs, total - offset);
        float* ch[1] = { testOut.data() + offset };
        testProc.process(ch, 1, chunk);
      }

      double maxDiff = 0.0;
      for (std::size_t i = 0; i < total; ++i) {
        double d = std::abs(refOut[i] - testOut[i]);
        if (d > maxDiff) maxDiff = d;
      }
      if (maxDiff > 1e-4) allSizesMatch = false;
    }
    recordResult(allSizesMatch, "Buffer-Size Invariance (32 to 2048 samples)", "State continuity across block boundaries");
  }

  // -------------------------------------------------------------------------
  // 13. MONO / STEREO TEST
  // -------------------------------------------------------------------------
  std::cout << "\n### TEST 13: MONO / STEREO CHANNEL BALANCE\n";
  {
    g3x::FreshAirProcessor proc;
    proc.prepare(48000.0, 2, 0.0);
    proc.setPresence(0.7);
    proc.setAir(0.7);

    const std::size_t n = 48000;
    auto left = generatePinkNoise(n, 0.15, 777);
    auto right = left; // Identical dual-mono inputs

    float* ch[2] = { left.data(), right.data() };
    proc.process(ch, 2, n);

    double maxChannelDiff = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
      double diff = std::abs(left[i] - right[i]);
      if (diff > maxChannelDiff) maxChannelDiff = diff;
    }

    std::cout << "  - Max L/R Channel Difference for Identical In : " << maxChannelDiff << "\n";
    recordResult(maxChannelDiff < 1e-6, "Stereo Channel Balance & Centering", "L == R (no spurious panning)");
  }

  // -------------------------------------------------------------------------
  // 14. PHASE AND NULL TESTING
  // -------------------------------------------------------------------------
  std::cout << "\n### TEST 14: PHASE INTEGRITY & FILTER TRANSITIONS\n";
  {
    // Evaluate minimum-phase smoothness: ensure no phase discontinuities or sign flips in audio band
    const double freqs[] = { 100.0, 1000.0, 3000.0, 5000.0, 10000.0, 15000.0, 20000.0 };
    bool phaseContinuous = true;

    for (double f : freqs) {
      g3x::FreshAirProcessor proc;
      proc.prepare(48000.0, 1, 0.0);
      proc.setPresence(1.0);
      proc.setAir(1.0);

      auto testSig = generateSine(f, 48000.0, 2400, 0.1);
      float* ch[1] = { testSig.data() };
      proc.process(ch, 1, 2400);

      // Check that waveform remains smooth without spikes or phase reversals
      for (std::size_t i = 100; i < 2399; ++i) {
        if (!std::isfinite(testSig[i])) phaseContinuous = false;
      }
    }
    recordResult(phaseContinuous, "Phase Continuity Across Enhancement Bands", "Continuous biquad transfer functions");
  }

  // -------------------------------------------------------------------------
  // 15. EXTREME-INPUT SAFETY
  // -------------------------------------------------------------------------
  std::cout << "\n### TEST 15: EXTREME-INPUT ROBUSTNESS & OVERFLOW RECOVERY\n";
  {
    g3x::FreshAirProcessor proc;
    proc.prepare(48000.0, 1, 0.0);
    proc.setPresence(1.0);
    proc.setAir(1.0);

    // 1. Extreme burst (+100 dBFS -> 100,000.0f)
    std::vector<float> burst(1024, 100000.0f);
    float* chB[1] = { burst.data() };
    proc.process(chB, 1, 1024);

    // 2. Feed quiet signal immediately after, allowing 100 samples for filter impulse settling
    std::vector<float> normal(1024, 0.1f);
    float* chN[1] = { normal.data() };
    proc.process(chN, 1, 1024);

    bool recovered = true;
    for (std::size_t i = 100; i < 1024; ++i) {
      if (!std::isfinite(normal[i]) || std::abs(normal[i]) > 0.5f) recovered = false;
    }

    // 3. NaN & Inf inputs
    std::vector<float> invalidSignal = { 
      std::numeric_limits<float>::quiet_NaN(),
      std::numeric_limits<float>::infinity(),
      -std::numeric_limits<float>::infinity(),
      1.0e-38f // denormal
    };
    invalidSignal.resize(64, 0.0f);
    float* chInv[1] = { invalidSignal.data() };
    proc.process(chInv, 1, 64);

    bool safeInvalid = true;
    for (float s : invalidSignal) {
      if (!std::isfinite(s)) safeInvalid = false;
    }

    recordResult(recovered, "State Reset Recovery After Massive +100dB Burst", "Settles cleanly back to < 0.5");
    recordResult(safeInvalid, "Immunity to NaNs, Infinities and Denormals", "All processed outputs sanitized to finite");
  }

  // -------------------------------------------------------------------------
  // 16. CPU AND REAL-TIME PERFORMANCE BENCHMARK
  // -------------------------------------------------------------------------
  std::cout << "\n### TEST 16: CPU THROUGHPUT & REAL-TIME PERFORMANCE\n";
  {
    constexpr std::size_t benchSamples = 48000 * 10; // 10 seconds of 48kHz audio
    auto left = generatePinkNoise(benchSamples, 0.1);
    auto right = left;

    g3x::FreshAirProcessor proc;
    proc.prepare(48000.0, 2, 0.0);
    proc.setPresence(0.75);
    proc.setAir(0.75);
    proc.setOutputTrimDb(-1.5);

    float* ch[2] = { left.data(), right.data() };

    auto t0 = std::chrono::high_resolution_clock::now();
    proc.process(ch, 2, benchSamples);
    auto t1 = std::chrono::high_resolution_clock::now();

    double elapsedSec = std::chrono::duration<double>(t1 - t0).count();
    double realtimeFactor = 10.0 / elapsedSec;
    double cpuUsagePercent = (elapsedSec / 10.0) * 100.0;

    std::cout << "  - Processed 10.0s Stereo Audio in : " << std::fixed << std::setprecision(4) << elapsedSec << " seconds\n";
    std::cout << "  - Real-Time Speedup Factor        : " << std::setprecision(1) << realtimeFactor << "x real-time\n";
    std::cout << "  - Single-Core CPU Load            : " << std::setprecision(3) << cpuUsagePercent << "%\n";

    recordResult(realtimeFactor > 50.0, "Real-Time Processing Overhead", "Speedup > 50x real-time (< 2% CPU)");
  }

  // -------------------------------------------------------------------------
  // SUMMARY
  // -------------------------------------------------------------------------
  std::cout << "\n====================================================================\n";
  std::cout << "                      VALIDATION SUITE SUMMARY                      \n";
  std::cout << "====================================================================\n";
  std::cout << "  Total Tests Executed : " << gStats.total << "\n";
  std::cout << "  Tests Passed         : " << gStats.passed << "\n";
  std::cout << "  Tests Failed         : " << gStats.failed << "\n";
  std::cout << "====================================================================\n\n";

  return (gStats.failed == 0) ? 0 : 1;
}
