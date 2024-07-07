#include <mmdeviceapi.h>
#include <audioclient.h>
#include <combaseapi.h>
#include <mmreg.h>
#include <ksmedia.h>
#include <functiondiscoverykeys_devpkey.h>
#include <wchar.h>
#include <assert.h>

class DeviceEnum {
    private:
    IMMDeviceEnumerator *enumerator;
    IMMDevice *defaultDevice;
    IMMDeviceCollection *devices;

    public:
    DeviceEnum () {
        this->enumerator = NULL;
        this->defaultDevice = NULL;
        this->devices = NULL;

        HRESULT hr;
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&this->enumerator);
        assert(hr >= 0 && this->enumerator != NULL);
    }

    ~DeviceEnum () {
        if (this->enumerator != NULL) {
            this->enumerator->Release();
            this->enumerator = NULL;
        }
        if (this->defaultDevice != NULL) {
            this->defaultDevice->Release();
            this->defaultDevice = NULL;
        }
        if (this->devices != NULL) {
            this->devices->Release();
            this->devices = NULL;
        }
    }

    void enumerate () {
        if (this->defaultDevice != NULL) {
            this->defaultDevice->Release();
            this->defaultDevice = NULL;
        }
        if (this->devices != NULL) {
            this->devices->Release();
            this->devices = NULL;
        }

        HRESULT hr;
        hr = this->enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &this->defaultDevice);
        assert(hr >= 0 && this->defaultDevice != NULL);
        hr = this->enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &this->devices);
        assert(hr >= 0 && this->devices != NULL);
    }

    UINT getCount () {
        // assert(this->defaultDevice != NULL && this->devices != NULL && "Enumerate devices first");
        if (this->defaultDevice == NULL || this->devices == NULL) this->enumerate();
        HRESULT hr;
        UINT n;
        hr = this->devices->GetCount(&n);
        assert(hr >= 0);
        return n;
    }

    void getDeviceName (UINT i, LPSTR buf, size_t maxLen) {
        // assert(this->defaultDevice != NULL && this->devices != NULL && "Enumerate devices first");
        if (this->defaultDevice == NULL || this->devices == NULL) this->enumerate();
        assert(i >= 0 && i < this->getCount() && "Device index out of range");
        PROPVARIANT propVariant;
        PropVariantInit(&propVariant);
        IMMDevice *device;
        HRESULT hr;
        hr = this->devices->Item(i, &device);
        assert(hr >= 0 && device != NULL);
        IPropertyStore *propertyStore = NULL;
        device->OpenPropertyStore(STGM_READ, &propertyStore);
        propertyStore->GetValue(PKEY_Device_FriendlyName, &propVariant);

        LPWSTR id = NULL;
        hr = device->GetId(&id);
        assert(hr >= 0 && id != NULL);

        LPWSTR defaultDeviceId = NULL;
        hr = this->defaultDevice->GetId(&defaultDeviceId);
        assert(hr >= 0 && defaultDeviceId != NULL);

        sprintf_s(buf, maxLen, "%.5S%s", propVariant.pwszVal, (!wcscmp(defaultDeviceId, id) ? " [default]" : ""));

        PropVariantClear(&propVariant);
        propertyStore->Release();

        CoTaskMemFree(id);
        CoTaskMemFree(defaultDeviceId);
    }

    IMMDevice *getDevice (UINT i) {
        assert(this->defaultDevice != NULL && this->devices != NULL && "Enumerate devices first");
        assert(i >= 0 && i < this->getCount() && "Device index out of range");
        IMMDevice *device;
        HRESULT hr;
        hr = this->devices->Item(i, &device);
        assert(hr >= 0 && device != NULL);
        return device;
    }
};
