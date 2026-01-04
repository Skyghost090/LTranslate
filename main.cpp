#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <future>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <iostream>
#include <rapidjson/document.h>
#include <bits/stdc++.h>
#include <signal.h>
#include <fstream>
#include <cmath>
#include <math.h>
#include <iostream>
#include <unicode/utypes.h>
#include <unicode/unistr.h>
#include <unicode/translit.h>
#include <vlc/libvlc.h>
#include <vlc/vlc.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <boost/algorithm/string/replace.hpp>
#include <pulse/error.h>
#include <pulse/gccmacro.h>
#include <pulse/simple.h>
#include <pulse/rtclock.h>
#include <fcntl.h>
#include "main.h"
#include "audio.h"
#include "curl_flyweight.h"

#define SAMPLE_RATE 22050
#define BIT_DEPTH 16
#define BUF_SIZE (SAMPLE_RATE) / 2

class SineOscillator {
    float frequency, amplitude, angle = 0.0f, offset = 0.0f;
public:
    SineOscillator(float freq, float amp) : frequency(freq), amplitude(amp) {
        offset = 2 * M_PI * frequency / SAMPLE_RATE;
    }
    float process() {
        auto sample = amplitude * sin(angle);
        angle += offset;
        return sample;
        // Asin(2pif/sr)
    }
};


static void finish(pa_simple *s) {
    if (s) pa_simple_free(s);
}
static bool running = true;

/* Signals handling */
static void handle_sigterm(int signo) { running = false; }

static void init_signal() {
    struct sigaction sa;
    sa.sa_flags = 0;

    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handle_sigterm;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
}

static void writeToFile(std::ofstream &file, int value, int size) {
    file.write(reinterpret_cast<const char*> (&value), size);
}

static void wav_init(std::ofstream &file){
    file.open("waveform-pa.wav", std::ios::binary);

    //Header chunk
    file << "RIFF";
    file << "----";
    file << "WAVE";

    // Format chunk
    file << "fmt ";
    writeToFile(file, 16, 4); // Size
    writeToFile(file, 1, 2); // Compression code
    writeToFile(file, 1, 2); // Number of channels
    writeToFile(file, SAMPLE_RATE, 4); // Sample rate
    writeToFile(file, SAMPLE_RATE * BIT_DEPTH / 8, 4 ); // Byte rate
    writeToFile(file, BIT_DEPTH / 8, 2); // Block align
    writeToFile(file, BIT_DEPTH, 2); // Bit depth

    //Data chunk
    file << "data";
    file << "----";
}

static void wav_close(std::ofstream &file){
    file.close();
}

class Server : public audio {
public:
    void play() override {
        stat("./voice.mp3", &voice_file_stat);
        float filesize = voice_file_stat.st_size;
        float timeseconds = filesize / 8000;
        const char* argv[] = { "--rate", "2.5" };
        libvlc_instance_t * inst;
        libvlc_media_player_t *mp;
        libvlc_media_t *m;
        inst = libvlc_new (2, argv);
        m = libvlc_media_new_path (inst, "./voice.mp3");
        // libvlc_media_add_option(m, param);
        printf("audio time: %f\n", timeseconds);
        mp = libvlc_media_player_new_from_media (m);
        libvlc_media_release (m);
        libvlc_media_player_play (mp);
        usleep(int(timeseconds * 1000000) / 2.5);
        libvlc_media_player_stop (mp);
        libvlc_media_player_release (mp);
        libvlc_release (inst);
    }

    int record(char *argv) override {
        int error;
        static pa_sample_spec ss;
        ss.format = PA_SAMPLE_S16LE;  // May vary based on your system
        ss.rate = SAMPLE_RATE;
        ss.channels = 1;

        init_signal();

        pa_simple *s = NULL;
        static pa_buffer_attr buf_attr;
        buf_attr.maxlength = (uint32_t) 2000; // 0.0...%
        buf_attr.fragsize = (uint32_t) 0;
        buf_attr.minreq = 0;
        buf_attr.prebuf = (uint32_t) -1;
        buf_attr.tlength = 0;

        // Create the recording stream
        if (!(s = pa_simple_new(NULL, "ltranslate", PA_STREAM_RECORD, argv, "record", &ss, NULL, &buf_attr, &error))) {
            fprintf(stderr, __FILE__ ": pa_simple_new() failed: %s\n",
                    pa_strerror(error));
            finish(s);
            return -1;
        }

        std::ofstream audio_file;
        wav_init(audio_file);
        int pre_audio_pos = audio_file.tellp();

        SineOscillator sineOscillator(440,0.5);
        auto maxAmplitude = pow(2, BIT_DEPTH - 1) - 1;

        int16_t* buffer = (int16_t*) malloc(BUF_SIZE*sizeof(int16_t));
        int j = 0;
        while (running) {
            if (j == 6){
                running = false;
                break;
            }
            if (pa_simple_read(s, (int16_t*) buffer, BUF_SIZE*sizeof(int16_t), &error) < 0) {
                fprintf(stderr, __FILE__ ": pa_simple_read() failed: %s\n",
                        pa_strerror(error));
                finish(s);
                return -1;
            }
            fprintf(stdout, "read %d done\n", buffer[0]);

            // Write to file
            for(int i=0; i<BUF_SIZE; i++) writeToFile(audio_file, buffer[i], 2);
            j++;
        }
        printf("finishing...\n");
        int post_audio_pos = audio_file.tellp();
        audio_file.seekp(pre_audio_pos - 4);
        writeToFile(audio_file, post_audio_pos - pre_audio_pos, 4);
        audio_file.seekp(4, std::ios::beg);
        writeToFile(audio_file, post_audio_pos - 8, 4);
        wav_close(audio_file);
        running = true;
        free(buffer);
        finish(s);
        return 0;
    }
};

static size_t
WriteCallback(
    char *receivedData,
    size_t dataSize,
    size_t dataBlocks,
    void *outputBuffer
) {
    std::string *strBuffer = static_cast<std::string*>(outputBuffer);
    strBuffer->append(receivedData, dataSize * dataBlocks);
    return dataSize * dataBlocks;
}

auto curl_flyweight = new curl_data();

static std::string init_translate(){
    std::string responseBuffer;
    rapidjson::Document doc;
    curl_flyweight->curl = curl_easy_init();
    if(curl_flyweight->curl) {
        curl_easy_setopt(curl_flyweight->curl, CURLOPT_URL, "127.0.0.1:8080/inference");
        curl_easy_setopt(curl_flyweight->curl, CURLOPT_DEFAULT_PROTOCOL, "http");
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: multipart/form-data");
        curl_easy_setopt(curl_flyweight->curl, CURLOPT_HTTPHEADER, headers);
        curl_mime *mime;
        curl_mimepart *part;
        mime = curl_mime_init(curl_flyweight->curl);
        part = curl_mime_addpart(mime);
        curl_mime_name(part, "file");
        curl_mime_filedata(part, "./waveform-pa.wav");
        curl_easy_setopt(curl_flyweight->curl, CURLOPT_MIMEPOST, mime);
        curl_easy_setopt(curl_flyweight->curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl_flyweight->curl, CURLOPT_WRITEDATA, &responseBuffer);
        curl_flyweight->res = curl_easy_perform(curl_flyweight->curl);
        curl_mime_free(mime);
    }
    curl_easy_cleanup(curl_flyweight->curl);
    doc.Parse(responseBuffer.c_str());
    std::string return_text = doc["text"].GetString();
    return return_text;
}

static void shell_translate(std::string text, char *translatedText){
    std::replace(text.begin(), text.end(), '\n', ' ');
    boost::replace_all(text, "'", " ");
    char command[10000];
    printf("foo: %d bar: %d\n", foo, bar);
    sprintf(command, "trans -brief '%s' | head -n 1", text.c_str());
    printf("command: %s\n", command);
    FILE *translate_cmd = popen(command, "r");
    char buf[256];
    while (fgets(buf, sizeof(buf), translate_cmd) != 0) {}
    int remove_index = strlen(buf) - 1;
    memmove(&buf[remove_index], &buf[remove_index + 1], strlen(buf) - remove_index);
    strcpy(translatedText, buf);
    pclose(translate_cmd);
}

static void speak_text(std::string text, char *language){
    FILE *audiofile = fopen("voice.mp3","wb");
    std::replace(text.begin(),text.end(), '\n', ' ');
    std::replace(text.begin(), text.end(), '>', ' ');
    curl_flyweight->curl = curl_easy_init();
    char adress[10000];
    char *escaped_str = curl_easy_escape(curl_flyweight->curl, text.c_str(), 0);
    std::string *content;

    if(curl_flyweight->curl) {
        sprintf(adress,"https://translate.google.com/translate_tts?ie=UTF-8&q=%s&tl=%s&client=tw-ob", escaped_str, language);
        printf("%s\n", adress);
        curl_easy_setopt(curl_flyweight->curl, CURLOPT_URL, adress);
        curl_easy_setopt(curl_flyweight->curl, CURLOPT_DEFAULT_PROTOCOL, "https");
        struct curl_slist *headers = NULL;
        curl_easy_setopt(curl_flyweight->curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl_flyweight->curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl_flyweight->curl, CURLOPT_WRITEDATA, audiofile);
        curl_flyweight->res = curl_easy_perform(curl_flyweight->curl);
    }
    curl_easy_cleanup(curl_flyweight->curl);
}

static void action_latency(void *ptr()){
    auto start = std::chrono::high_resolution_clock::now();
    auto end = std::chrono::high_resolution_clock::now();
    ptr();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    fprintf(stdout, "record done %ld ms \n", duration);
}

static void init_transcript(){
    record_audio.get();
    transcriptedText = init_translate();
    if (strcmp(transcriptedText.c_str(), " [BLANK_AUDIO]") !=  10){
        shell_translate(transcriptedText, translatedText);
        printf("tranlated text: %s\n", translatedText);
        speak_text(translatedText, lang);
        audio_play.get();
    }
}

void whisper_reboot(){
    execl("/bin/sh", "reboot_whisper.sh");
}

audio* audio_server = new Server;

void audio_record(){
    audio_server->record(arg);
}

void play_audio(){
    audio_server->play();
}

static void start(char *arg){
    lang = arg;
    while (true){
        audio* audio_server = new Server;
        record_audio = std::async(std::launch::async, audio_record);
        audio_play = std::async(std::launch::async, play_audio);
        init_transcript();
        usleep(250000);
    }
}

int main(int argc, char **argv){
    if (argc == 1){
        int fd = open("./help.txt", O_RDONLY);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        char *buf = (char*)malloc(sizeof(char*) * 800000);
        ssize_t file = read(fd, buf, sizeof(char*) * 800000);

        write(STDOUT_FILENO, buf, sizeof(char*) * 400000-1);
        return 0;
    }
    if (argc < 4){
        printf("please type a parameter, use --help for more information\n");
        return 1;
    } else {
        if (strcmp(argv[1], "--start") == 0){
            arg = argv[3];
            start(argv[2]);
        }
    }
    return 0;
}
