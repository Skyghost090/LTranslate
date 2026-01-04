# LTranslate

LTranslate is a desktop application that captures **English audio played on your system**, transcribes it in real time, and **translates it into any target language**. It is designed to be lightweight, low-latency, and suitable for developers who want system-wide audio translation without relying on heavy cloud services.

The tool is especially useful for:

* Translating videos, podcasts, or meetings played on the desktop
* Accessibility and language learning
* Integration into Linux-based desktop environments

---

## Features

* 🎧 **Desktop Audio Capture** – Captures system output audio (not microphone-only)
* 🗣️ **English Speech Recognition** – Accurate transcription of English audio
* 🌍 **Multi-language Translation** – Translate to any supported target language
* ⚡ **Low Latency** – Optimized for near real-time processing
* 🖥️ **Desktop-Friendly** – Designed for Linux desktop environments

---

## Architecture Overview

LTranslate is composed of four main stages:

1. **Audio Capture**
   Captures desktop/system audio (e.g., via PulseAudio or PipeWire).

2. **Speech-to-Text (STT)**
   Converts English audio into text using a local or remote STT engine.

3. **Translation Engine**
   Translates the transcribed text into the target language.

4. **Output Layer**
   Displays, streams, or exports translated text (console, GUI, subtitles, API, etc.).

```
Desktop Audio → STT (English) → Translation → Output
```

---

## Supported Platforms

* **Linux** (primary target)

  * Works with PulseAudio and PipeWire
* Other platforms may be supported in the future

---

## Dependencies

Depending on your build configuration, LTranslate may use:

* Audio backend:

  * PulseAudio

  * libcurl
 
  * libvlc

> The application is designed to work fully offline when local models are used.

---

## Installation

### Build from Source

```bash
git clone https://github.com/yourusername/ltranslate.git
cd ltranslate
g++ main.cpp -lcurl -lm -std=c++17 -ldl -lstdc++ -lpulse -lpulse-simple -lvlc -o ltranslate
```

---

## Usage

### Basic Example

```bash
ltranslate --target-lang pt
```

This command:

* Captures English audio from the desktop
* Transcribes it
* Translates it to Portuguese

## Use Cases

* 🎬 Translating movies or videos in real time
* 🎓 Language learning and immersion
* 🧑‍💻 Developers needing live translation pipelines
* ♿ Accessibility for non-English speakers

---

## Contributing

Contributions are welcome!

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Open a pull request

Please ensure your code follows the project style and includes documentation where applicable.

---

## License

This project is licensed under the MIT License.

---

## Disclaimer

LTranslate is intended for educational and accessibility purposes. Ensure you have the right to capture and translate audio content according to local laws and service terms.

---

## Name Origin

**LTranslate** stands for **Live / Linux Translate**, emphasizing real-time desktop audio translation.
