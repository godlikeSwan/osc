#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <combaseapi.h>
#include <mmreg.h>
#include <ksmedia.h>
#include <functiondiscoverykeys_devpkey.h>
#include <comdef.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <limits.h>
#include <stdint.h>

enum Format {
    FORMAT_PCM,
    FORMAT_FLOAT
};

class Capture {
    private:
    IAudioClient *inAudioClient;
    IAudioCaptureClient *inCaptureClient;
    UINT32 inBufferFrameCount;
    HANDLE event;
    Format format;

    public:
    #define BUF_LEN 500000
    float buf[BUF_LEN];
    size_t inBufI;
    size_t outBufI;

    unsigned long sampleRate;

    Capture (IMMDevice *device, int inExclusive) {
        for (size_t i = 0; i < BUF_LEN; ++i) this->buf[i] = 0.0f;
        this->inBufI = 0;
        this->outBufI = 0;

        const IID IID_IAudioClient = __uuidof(IAudioClient);
        const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

        HRESULT hr;

        this->inAudioClient = NULL;
        hr = device->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&inAudioClient);
        assert(hr >= 0);

        WAVEFORMATEX *waveFormat = NULL;


        IPropertyStore *propertyStore = NULL;
        PROPVARIANT propVariant;
        if (inExclusive) {
            PropVariantInit(&propVariant);
            hr = device->OpenPropertyStore(STGM_READ, &propertyStore);
            assert(hr >= 0);
            hr = propertyStore->GetValue(PKEY_AudioEngine_DeviceFormat, &propVariant);
            assert(hr >= 0 && propVariant.vt == VT_BLOB);
            waveFormat = (WAVEFORMATEX *)propVariant.blob.pBlobData;
            assert(waveFormat != NULL);
        } else {
            hr = inAudioClient->GetMixFormat(&waveFormat);
            assert(hr >= 0);
            assert(waveFormat != NULL);
        }

        assert(waveFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE && waveFormat->cbSize >= 22);
        assert(((WAVEFORMATEXTENSIBLE *)waveFormat)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT
            || ((WAVEFORMATEXTENSIBLE *)waveFormat)->SubFormat == KSDATAFORMAT_SUBTYPE_PCM);

        this->format = ((WAVEFORMATEXTENSIBLE *)waveFormat)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT ? FORMAT_FLOAT : FORMAT_PCM;

        if (this->format == FORMAT_FLOAT) assert(((WAVEFORMATEXTENSIBLE *)waveFormat)->Samples.wValidBitsPerSample == 32);
        if (this->format == FORMAT_PCM) assert(((WAVEFORMATEXTENSIBLE *)waveFormat)->Samples.wValidBitsPerSample == 16);

        this->sampleRate = waveFormat->nSamplesPerSec;

        REFERENCE_TIME inRequestedDuration;
        hr = inAudioClient->GetDevicePeriod(NULL, &inRequestedDuration);
        assert(hr >= 0);

        hr = inAudioClient->Initialize(inExclusive ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
            inRequestedDuration, inRequestedDuration, waveFormat, NULL);
        assert(hr >= 0);

        if (inExclusive) {
            hr = PropVariantClear(&propVariant);
            assert(hr >= 0);
            if (propertyStore != NULL) propertyStore->Release();
        } else {
            if (waveFormat) CoTaskMemFree(waveFormat);
        }

        // assert(hr != AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED);

        this->event = NULL;
        this->event = CreateEvent(NULL, FALSE, FALSE, NULL);
        assert(this->event != NULL);

        hr = inAudioClient->SetEventHandle(event);
        assert(hr >= 0);

        hr = inAudioClient->GetBufferSize(&inBufferFrameCount);
        assert(hr >= 0);

        this->inCaptureClient = NULL;
        hr = inAudioClient->GetService(IID_IAudioCaptureClient, (void**)&this->inCaptureClient);
        assert(hr >= 0 && this->inCaptureClient != NULL);
    }

    void start () {
        HRESULT hr;
        hr = inAudioClient->Start();
        assert(hr >= 0);
    }

    void processOnce () {
        HRESULT hr;
        HANDLE h[1];
        h[0] = this->event;
        DWORD w = WaitForMultipleObjects(1, h, FALSE, 2000);
        assert(w >= WAIT_OBJECT_0 && w <= WAIT_OBJECT_0 + 1);
        if (w - WAIT_OBJECT_0 == 0) {
            // In event (the only event)
            BYTE *pData;
            UINT32 n;
            DWORD flags;
            hr = inCaptureClient->GetBuffer(&pData, &n, &flags, NULL, NULL);
            assert(hr >= 0);
            if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                n = 0;
            }
            for (UINT32 j = 0; j < n * 2; ++j) {
                if (this->format == FORMAT_PCM) {
                    union {
                        int16_t i;
                        BYTE b[2];
                    } u;
                    if ((this->inBufI + BUF_LEN - this->outBufI) % BUF_LEN < 500000) {
                        u.b[0] = pData[j * 2 + 0];
                        u.b[1] = pData[j * 2 + 1];
                        this->buf[this->inBufI++] = (float)u.i / INT16_MAX;
                        if (this->inBufI >= 500000) this->inBufI = 0;
                    }
                } else if (this->format == FORMAT_FLOAT) {
                    union {
                        float f;
                        BYTE b[4];
                    } u;
                    if ((this->inBufI + BUF_LEN - this->outBufI) % BUF_LEN < 500000) {
                        u.b[0] = pData[j * 4 + 0];
                        u.b[1] = pData[j * 4 + 1];
                        u.b[2] = pData[j * 4 + 2];
                        u.b[3] = pData[j * 4 + 3];
                        this->buf[this->inBufI++] = u.f;
                        if (this->inBufI >= 500000) this->inBufI = 0;
                    }
                }
            }
            hr = inCaptureClient->ReleaseBuffer(n);
            assert(hr >= 0);
        }
    }

    void stop () {
        HRESULT hr;
        hr = inAudioClient->Stop();
        assert(hr >= 0);
    }

    ~Capture () {
        this->inAudioClient->Release();
        this->inCaptureClient->Release();
    }

    int haveNewValues () {
        return this->inBufI != this->outBufI;
    }

    float popValue () {
        float value = this->buf[this->outBufI];
        this->outBufI += 1;
        this->outBufI %= BUF_LEN;
        return value;
    }
};
