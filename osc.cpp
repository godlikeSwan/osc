#include <mmdeviceapi.h>

#include "capture.cpp"

extern char debugString[128];

class Osc {
    private:
    Capture *capture;
    float timeDiff;

    public:
    UINT presicion;
    UINT sampleRate;
    float freq;
    float *frame;
    size_t offset;
    Osc (IMMDevice *device, float freq, UINT presicion) {
        this->capture = new Capture(device, 0);
        this->freq = freq;
        this->presicion = presicion;
        this->sampleRate = this->capture->sampleRate;

        this->frame = new float[this->presicion];
        for (size_t i = 0; i < this->presicion; ++i) this->frame[i] = .0f;
        this->offset = 0;

        this->timeDiff = .0f;
        // assert((float)this->sampleRate > this->freq * (float)this->presicion);
    }
    ~Osc () {
        delete[] this->frame;
        delete this->capture;
    }

    void start () {
        this->capture->start();
    }

    void stop () {
        this->capture->stop();
    }

    void processOnce () {
        this->capture->processOnce();
        float ratio = (float)this->sampleRate / (this->freq);

        // TODO: Don't know how to detect base frequency =(
        //
        // float ratio = (float)this->sampleRate / (this->freq * (float)this->presicion);
        // this->timeDiff += ratio;

        // sprintf_s(debugString, 128, "%f : %f : %f", (float)this->sampleRate, this->freq, (float)this->presicion);
        // size_t max1i = this->capture->outBufI;
        // size_t max2i = this->capture->outBufI;
        // for (size_t i = this->capture->outBufI + 1; i != this->capture->inBufI; i = (i + 1) % 500000) {
        //     float v = this->capture->buf[i];
        //     float max1v = this->capture->buf[max1i];
        //     float max2v = this->capture->buf[max2i];
        //     if (v > max1v) {
        //         max2i = max1i;
        //         max1i = i;
        //     } else if (v > max2v) {
        //         max2i = i;
        //     }
        // }
        // this->freq = (max1i + 500000 - max2i) % 500000;

        // float maxSum = .0f;
        // unsigned int maxN = 1;
        // for (unsigned int i = 1; i < this->sampleRate; ++i) {
        //         // FILE *f = NULL;
        //         // fopen_s(&f, "log.txt", "w+");
        //     for (unsigned int j = 0; j * i * 2 < this->sampleRate; ++j) {
        //         // fprintf(f, "j: %u; i * j * 2: %u; condition: %i\n", j, i * j * 2, j * i * 2 < this->sampleRate);
        //         // fflush(f);
        //         float sum = .0f;
        //         for (size_t k = 0 + j; k < 5000; k += i) {
        //         // for (size_t k = this->capture->outBufI + i; k != this->capture->inBufI; k = (k + i) % 500000) {
        //             float v = this->capture->buf[k];
        //             sum += v;
        //         }
        //         sum = sum * (float)i / (float)this->sampleRate;
        //         if (sum > maxSum) {
        //             maxSum = sum;
        //             maxN = i;
        //         }
        //     }
        //         // assert(0);
        //         // fclose(f);
        // }

        // this->freq = (float)this->sampleRate / (float)maxN;


        sprintf_s(debugString, 128, "freq: %f; sr: %f", this->freq, (float)this->sampleRate);

        while (this->capture->haveNewValues()) {
            float sum = .0f;
            unsigned int count = 0;
            unsigned int sr = this->sampleRate;
            while ((float)count * this->freq < (float)this->sampleRate && this->capture->haveNewValues()) {
            // while (this->timeDiff > 1.0f && this->capture->haveNewValues()) {
                // this->timeDiff -= ratio;
                sum += this->capture->popValue();
                ++count;
            }
            this->frame[this->offset] = sum / (float)count;
            this->offset += 1;
            this->offset %= this->presicion;
        //     this->timeDiff += ratio;
        }
    }
};
