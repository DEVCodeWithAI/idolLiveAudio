# idolLiveAudio (v1.3.1)

**Lightweight, open-source Audio Plugin Host for creators, singers, and streamers.**

> Built with the assistance of AI (Gemini), OpenAI (ChatGPT), and FFAI Framework (Meta Llama 3)

---

## 🌐 Languages

- [🇺🇸 English (default)](README.md)
- [🇻🇳 Tiếng Việt](README.vi.md)

---

## 🖥️ Screenshot

![idolLiveAudio Main Interface](docs/images/screenshot_main.png)

*A lightweight, intuitive interface for managing plugins and your soundboard during live performances.*

---

## ✨ What's New in v1.3.1

Version 1.3.1 refines the core experience with a focus on stability and speed for live performers.

* **⚡️ Instant Preset Switching (Hot-Swap)**: Change between presets containing different plugin settings instantly, with no audio dropouts or glitches. This is perfect for switching vocal effects between songs during a live stream.
* **🧠 Smart Change Detection**: The app is now intelligent enough to ignore parameter changes from non-editable plugins (like key finders), preventing unnecessary "Do you want to save?" prompts.
* **🛡️ Robust State Management**: Preset saving and loading logic has been overhauled to correctly include the UI lock state, ensuring your setup is restored exactly as you left it.
* **🐛 Bug Fixes & Stability**: General improvements to plugin hosting and preset management to ensure a smoother, more reliable experience.

---

## 🚀 Key Features

✅ **Instant "Hot-Swap" Preset Switching** for seamless live performance
✅ Supports Waves, Antares Auto-Tune Pro, and all VST3 plugins
✅ Real-time audio processing with low latency
✅ Flexible plugin chain management per track
✅ Dedicated FX Chains for parallel processing (Reverb, Delay, etc.)
✅ Integrated Player & Recorder for each track (Post-FX)
✅ Multi-Track Project System for recording raw, unprocessed audio
✅ Safety Lock to prevent accidental changes to core configurations
✅ Integrated Soundboard for quick sound triggering
✅ Simple, user-friendly interface
✅ Developed with JUCE (C++20)
✅ Open-source under the GPLv3 license

---

## 💡 Pro-Tip: Setting Up Key Detection Plugins

For the best experience, it is highly recommended to place key detection plugins (like **Antares Auto-Key**, **Waves Key Detector**, etc.) in the **first plugin slot of the Music track**.

The application is specifically optimized to ignore state changes from this slot. This prevents the app from asking you to save your preset after the plugin automatically detects a new key, ensuring a truly seamless workflow.

---

## 📦 Installation

**Option 1: Download Prebuilt Release (Recommended)**

* Visit the [**Releases**](https://github.com/DEVCodeWithAI/idolLiveAudio/releases) section for the latest version.
* Download the appropriate `.zip` package.
* Extract and run the application.

**Option 2: Compile from Source**

* Requires a C++20 compatible compiler.
* Requires the [JUCE Framework](https://juce.com).
* Open the `idolLiveAudio.jucer` file with the Projucer.
* Export the project to your preferred IDE (Visual Studio, Xcode, etc.).
* Build and run.

---

## ⚠️ Important Notice for Waves Users

If you have a large collection of Waves plugins (e.g., Waves Ultimate), the initial plugin scan can take a significant amount of time. **This is normal!** Please do not close the application during the scan.

* **Estimated Scan Time:**
    * Small plugin set: A few seconds to 2 minutes.
    * Large plugin set (Waves Ultimate): Up to 10-15 minutes.

✅ The scan only runs once. After completion, idolLiveAudio saves the results to a local file for much faster startups in the future.

---

## 🎧 Recommended Audio Setup

For the best experience, a dedicated external sound card with low-latency ASIO drivers is highly recommended.

**If you do not have a professional sound card, we strongly suggest installing these free tools:**

* [**VB-Cable**](https://vb-audio.com/Cable/) – Virtual audio cable.
* [**ASIO4ALL**](https://www.asio4all.org/) – Universal ASIO driver for low-latency audio.
* [**Voicemeeter Banana**](https://vb-audio.com/Voicemeeter/banana.htm) – Virtual audio mixer and routing software.

---

## 💡 Contributing

We welcome community contributions! You can help by:

* Reporting bugs.
* Suggesting features.
* Submitting pull requests.
* Sharing presets and configurations.

See [CONTRIBUTING.md](CONTRIBUTING.md) for more details.

---

## ☕ Support the Project

This project is self-funded. If you find idolLiveAudio useful, please consider supporting its development by buying me a coffee:

👉 [**https://buymeacoffee.com/devcodewithai**](https://buymeacoffee.com/devcodewithai)

Your support helps cover development time and future plans, including a dedicated website, a community forum, and advanced AI-driven features.