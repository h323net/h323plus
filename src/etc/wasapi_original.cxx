/*
 * wasapi.cxx
 *
 * Copyright (c) 2015 Spranto International Pte Ltd. All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * Contributor(s): ______________________________________.
 *
 * $Id: wasapi_original.cxx,v 1.1.2.1 2015/10/10 08:54:36 shorne Exp $
 * 
 */


#include <ptlib.h>
#include <ptlib/sound.h>
#include <ptclib/delaychan.h>

// Windows MFT Includes
#include <Wmcodecdsp.h>
#include <mferror.h>

#include <Mftransform.h>
#pragma comment(lib, "Mfplat.lib")

#include <Mfapi.h>
#pragma comment(lib, "Mfuuid.lib")

EXTERN_GUID(CLSID_CResamplerMediaObject, 0xf447b69e, 0x1884, 0x4a7e, 0x80, 0x55, 0x34, 0x6f, 0x74, 0xd6, 0xed, 0xb3);


// Windows WASAPI Includes
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>


#include <avrt.h>
#pragma comment(lib, "avrt.lib")

///////////////////////////////////////////////////////////////////////

#define _WASAPI_SHAREDEXCLUSIVE 1   // In Exclusive mode

#define SAMPLE_TIME               60     // 60ms (3x20ms or 2x30ms)
#define SAMPLE_COUNT               2     // 2 samples in the buffer
#define MILLISEC_PER_100NANO   10000     // 100 nanosec units
#define BUFFER_DURATION        SAMPLE_TIME * MILLISEC_PER_100NANO * SAMPLE_COUNT
#define PERIODICITY            SAMPLE_TIME * MILLISEC_PER_100NANO

///////////////////////////////////////////////////////////////////////

    
template <class T> class PComPtr
{
    T * pointer;
  public:
    PComPtr() : pointer(NULL) { }
    ~PComPtr() { Release(); }

    PComPtr & operator=(T * p)
    {
      Release();
      if (p != NULL)
        p->AddRef();
      pointer = p;
      return *this;
    }

    operator T *()        const { return  pointer; }
    T & operator*()       const { return *pointer; }
    T** operator&()             { return &pointer; }
    T* operator->()       const { return  pointer; }
    bool operator!()      const { return  pointer == NULL; }
    bool operator<(T* p)  const { return  pointer < p; }
    bool operator==(T* p) const { return  pointer == p; }
    bool operator!=(T* p) const { return  pointer != p; }

    void Release()
    {
      T * p = pointer;
      if (p != NULL) {
        pointer = NULL;
        p->Release();
      }
    }

  private:
    PComPtr(const PComPtr &) {}
    void operator=(const PComPtr &) { }
};


template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

//#################################################################################


#if PTRACING

static PString COMErrorMsg(HRESULT hr)
{
    PString errMsg = "Unknown Error";

    switch (hr) {
        // PWASAPIDevice::Read
        case AUDCLNT_E_BUFFER_ERROR: errMsg = "Windows 7: GetBuffer failed to retrieve a data buffer and *ppData points to NULL."; break;
        case AUDCLNT_E_OUT_OF_ORDER: errMsg = "A previous IAudioCaptureClient::GetBuffer call is still in effect."; break;
        case AUDCLNT_E_DEVICE_INVALIDATED: errMsg = "The audio endpoint device has been unplugged"; break;
        case AUDCLNT_E_BUFFER_OPERATION_PENDING: errMsg = "Buffer cannot be accessed because a stream reset is in progress."; break;
        case AUDCLNT_E_SERVICE_NOT_RUNNING: errMsg = "The Windows audio service is not running."; break;
        case E_POINTER: errMsg = "Parameter ppData, pNumFramesToRead, or pdwFlags is NULL."; break;

        // MFTAudioResampler::Resample
        case E_INVALIDARG: errMsg = "Invalid argument."; break;
        case MF_E_INVALIDSTREAMNUMBER: errMsg = "Invalid stream identifier"; break;
        case MF_E_NO_SAMPLE_DURATION: errMsg = "The input sample requires a valid sample duration."; break;
        case MF_E_NO_SAMPLE_TIMESTAMP: errMsg = "The input sample requires a time stamp.To set the time stamp, call IMFSample::SetSampleTime."; break;
        case MF_E_NOTACCEPTING: errMsg = "The transform cannot process more input at this time."; break;
        case MF_E_TRANSFORM_TYPE_NOT_SET: errMsg = "The media type is not set on one or more streams."; break;
        case MF_E_UNSUPPORTED_D3D_TYPE: errMsg = "The media type is not supported for DirectX Video Acceleration(DXVA)."; break;
        case E_UNEXPECTED: errMsg = "The ProcessOutput method was called on an asynchronous MFT that was not expecting this method call."; break;
        case MF_E_TRANSFORM_STREAM_CHANGE: errMsg = "The format has changed on an output stream, or there is a new preferred format, or there is a new output stream."; break;
    }

    return errMsg;
}

static PString MMErrorMessage(HRESULT hr)
{
  PString msg;
  DWORD dwMsgLen = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                                 FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL,
                                 hr,
                                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
#if PTLIB_MAJOR == 2 && PTLIB_MINOR >= 12
                                 msg.GetPointerAndSetLength(256), 255,
#else
                                 msg.GetPointer(1000), 999,
#endif
                                 NULL);
  if (dwMsgLen > 0)
    return msg;

#pragma warning(disable:4995)
  char hex[20];
  snprintf(hex, sizeof(hex), "0x%08x", hr);
  return hex;
}

static bool CheckMMError(HRESULT hr, const char * fn)
{
  if (SUCCEEDED(hr))
    return false;

  PTRACE(1,"WASAPI\tFunction \"" << fn << "\" failed : " << MMErrorMessage(hr));
  return true;
}

#define CHECK_ERROR(fn, action) if (CheckMMError(fn, #fn)) action
#define CHECK_NO_ERROR(fn) (!CheckMMError(fn, #fn))


static void PrintWave(const WAVEFORMATEX & m_Format, ostream & strm) {

    int indent = (int)strm.precision() + 2;
    strm << "WAVEFORMATEX:\n"
         << setw(indent+2) << "Format : " << m_Format.wFormatTag << "\n"
         << setw(indent+2) << "nChannels      : " << m_Format.nChannels << "\n"
         << setw(indent+2) << "wBitsPerSample : " << m_Format.wBitsPerSample << "\n"
         << setw(indent+2) << "nSamplesPerSec : " << m_Format.nSamplesPerSec << "\n"
         << setw(indent+2) << "nAvgBytesPerSec: " << m_Format.nAvgBytesPerSec << "\n"
         << setw(indent+2) << "nBlockAlign    : " << m_Format.nBlockAlign << "\n"
         << setw(indent+2) << "cbSize         : " << m_Format.cbSize << "\n";
}

static PString PrintWaveFormatEx(const WAVEFORMATEX & m_Format) {
    PStringStream strm;
    PrintWave(m_Format,strm);
    return strm;
}

#else
#define CHECK_ERROR(fn, action) if (FAILED(fn)) action
#define CHECK_NO_ERROR(fn) !FAILED(fn)
#endif

#define CHECK_ERROR_RETURN(fn) CHECK_ERROR(fn, return false;)

////////////////////////////////////////////////////////////////////////////////////////////////////////////


/* MFTAudioResampler
    Windows MFT (Media Foundation Transform) wrapper
    for converting WASAPI audio output to usable format
*/
class MFTAudioResampler
{

public:

    /*  Class Constructor
        Generic Class constructor. 
        Call Initialise to Instance the MFT COM libraries
    */
    MFTAudioResampler();

    /*  Class Deconstructor
        Generic Class deconstructor. 
    */
    ~MFTAudioResampler();

    /*  Initialise
        Initialise the MFT COM libraries for the given audio transform
    */
    PBoolean Initialise(const WAVEFORMATEX & in, const WAVEFORMATEX & out);

    /*  IsInitialised
        Initialise the MFT COM libraries for the given audio transform
    */
    PBoolean IsInitialised();

    /*  Close
        Close the resampler
    */
    void Close();

    /*  Resample
        Resample Media
        NOTE: The MFT engine must be initialised before calling Resample
    */
    PBoolean Resample(const BYTE * inMedia, PINDEX inSize, BYTE * outMedia, PINDEX & outSize, PBoolean & moreDataNeeded);

protected:

    /*  ConvertMediaType
        Convert from WAVEFORMATEX to IMFMediaType
    */
    PBoolean ConvertMediaType(const WAVEFORMATEX & fmt, IMFMediaType * pMediaType);

    /*  CreateMediaSample
        Create IMF sample from raw PCM input
    */
    PBoolean CreateMediaSample(const BYTE * data, PINDEX bytes, IMFSample * pSample);

    /*  GetMediaSample
        retrieve raw PCM out from IMF Sample
    */
    PBoolean GetMediaSample(IMFSample * pSample, BYTE * data, PINDEX & bytes);

private:

    /*  MFT COM classes
    */
    PComPtr<IUnknown>                       m_spTransformUnk;         // Resampler media object     
    PComPtr<IMFTransform>                   m_pTransform;             // Resampler transform
    PComPtr<IWMResamplerProps>              m_spResamplerProps;       // Resampler properties

    PBoolean                                m_isInitialised;          // Whether the Resampler has been initialised

};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MFTAudioResampler::MFTAudioResampler()
    : m_isInitialised(false)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
}

MFTAudioResampler::~MFTAudioResampler()
{
    Close();
}

PBoolean MFTAudioResampler::ConvertMediaType(const WAVEFORMATEX & fmt, IMFMediaType * pMediaType)
{
    CHECK_ERROR_RETURN(pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio))
    CHECK_ERROR_RETURN(pMediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM))
    CHECK_ERROR_RETURN(pMediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, fmt.nChannels))
    CHECK_ERROR_RETURN(pMediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, fmt.nSamplesPerSec))
    CHECK_ERROR_RETURN(pMediaType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, fmt.nBlockAlign))
    CHECK_ERROR_RETURN(pMediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, fmt.nAvgBytesPerSec))
    CHECK_ERROR_RETURN(pMediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, fmt.wBitsPerSample))
    CHECK_ERROR_RETURN(pMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE))

    CHECK_ERROR_RETURN(pMediaType->SetUINT32(MF_MT_AUDIO_CHANNEL_MASK,
        (fmt.nChannels == 1 ? SPEAKER_FRONT_CENTER : SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT)))

    CHECK_ERROR_RETURN(pMediaType->SetUINT32(MF_MT_AUDIO_VALID_BITS_PER_SAMPLE, fmt.wBitsPerSample))

    return true;
}

PBoolean MFTAudioResampler::Initialise(const WAVEFORMATEX & in, const WAVEFORMATEX & out)
{
    CHECK_ERROR_RETURN(CoCreateInstance(CLSID_CResamplerMediaObject, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&m_spTransformUnk))

    CHECK_ERROR_RETURN(m_spTransformUnk->QueryInterface(IID_PPV_ARGS(&m_pTransform)))

    CHECK_ERROR_RETURN(m_spTransformUnk->QueryInterface(IID_PPV_ARGS(&m_spResamplerProps)))

    CHECK_ERROR(m_spResamplerProps->SetHalfFilterLength(60),
                        PTRACE(2, "MFT\tERROR setting quality level");)  //< best conversion quality */

    PComPtr<IMFMediaType> inFormat, outFormat;
    MFCreateMediaType(&inFormat);
    MFCreateMediaType(&outFormat);

    if (ConvertMediaType(in, inFormat) && ConvertMediaType(out, outFormat)) {
        m_pTransform->SetInputType(0, inFormat, 0);
        m_pTransform->SetOutputType(0, outFormat, 0);
        m_isInitialised = true;
    }
    else {
        PTRACE(2, "MFT\tERROR setting media Format");
    }

    return m_isInitialised;
}

PBoolean MFTAudioResampler::IsInitialised()
{
    return m_isInitialised;
}

void MFTAudioResampler::Close()
{
    if (!m_isInitialised)
        return;

    m_spTransformUnk.Release();  
    m_pTransform.Release();
    m_spResamplerProps.Release();
    m_isInitialised = false;
}

PBoolean MFTAudioResampler::CreateMediaSample(const BYTE * data, PINDEX bytes, IMFSample * pSample)
{
    PComPtr<IMFMediaBuffer> pBuffer;
    CHECK_ERROR_RETURN(MFCreateMemoryBuffer((DWORD)bytes, &pBuffer))

    BYTE  *pByteBufferTo = NULL;
    CHECK_ERROR_RETURN(pBuffer->Lock(&pByteBufferTo, NULL, NULL))
    memcpy(pByteBufferTo, data, bytes);
    pBuffer->Unlock();
    pByteBufferTo = NULL;

    CHECK_ERROR_RETURN(pBuffer->SetCurrentLength((DWORD)bytes))

    CHECK_ERROR_RETURN(pSample->AddBuffer(pBuffer))

    return true;
}

PBoolean MFTAudioResampler::GetMediaSample(IMFSample * pSample, BYTE * data, PINDEX & bytes)
{
    PComPtr<IMFMediaBuffer> spBuffer;
    CHECK_ERROR_RETURN(pSample->ConvertToContiguousBuffer(&spBuffer))
    DWORD cbBytes = 0;
    CHECK_ERROR_RETURN(spBuffer->GetCurrentLength(&cbBytes))

    BYTE  *pByteBuffer = NULL;
    CHECK_ERROR_RETURN(spBuffer->Lock(&pByteBuffer, NULL, NULL))

    BYTE *to = new BYTE[cbBytes]; //< output PCM data
    bytes = cbBytes;              //< output PCM data size
    memcpy(to, pByteBuffer, cbBytes);

    spBuffer->Unlock();

    return true;
}

PBoolean MFTAudioResampler::Resample(const BYTE * inMedia, PINDEX inSize, BYTE * outMedia, PINDEX & outSize, PBoolean & moreDataNeeded)
{
    if (!m_isInitialised)
        return false;

    PComPtr<IMFSample> pSample;
    MFCreateSample(&pSample);

    if (!CreateMediaSample(inMedia, inSize, pSample)) {
        PTRACE(2, "MFT\tERROR setting media sample");
        return false;
    }

    HRESULT hr = m_pTransform->ProcessInput(0, pSample, 0);
    if (hr != S_OK) {
        PTRACE(4, "MFT\tERROR Process Input: " << COMErrorMsg(hr));
        return false;
    }

    MFT_OUTPUT_DATA_BUFFER outputDataBuffer;
    DWORD dwStatus;
    PComPtr<IMFMediaBuffer> pOutput;
    moreDataNeeded = false;
    memset(&outputDataBuffer, 0, sizeof outputDataBuffer);

    MFCreateSample(&(outputDataBuffer.pSample));
    MFCreateMemoryBuffer((DWORD)outSize, &pOutput);
    outputDataBuffer.pSample->AddBuffer(pOutput);
    outputDataBuffer.dwStreamID = 0;
    outputDataBuffer.dwStatus = 0;
    outputDataBuffer.pEvents = NULL;

        hr = m_pTransform->ProcessOutput(0, 1, &outputDataBuffer, &dwStatus);
        if (hr == S_OK) 
        {
            IMFSample *pSample = outputDataBuffer.pSample;
            return GetMediaSample(pSample, outMedia, outSize);
        }
        else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
        { // More input data needed.
            moreDataNeeded = true;
            return true;
        }  else {
            PTRACE(4, "MFT\tOutput Error: " << COMErrorMsg(hr));
            return false;
        }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/* PWASAPIDevice Class
    A WASAPI COM wrapper for a Audio device.
    Note the class is UniDirectional (Player/Recorder) 
    Use Initialise to instance the COM classes for a given direction
*/
class PWASAPIDevice
{

public:
    
    /*  Class Constructor
        Generic Class constructor. Must call Initialise to instance the COM libraries and 
        assign a direction for the class
    */
    PWASAPIDevice();

    /*  Class Deconstructor
    */
    ~PWASAPIDevice();

    /*  Roles Enumerator
        Windows supports in the settings the allocation of audio
        devices for different roles. There are three defined roles.
     */
    enum Roles {
        e_Console,              // Default Console device
        e_Communications,       // Default Communications device
        e_Multimedia,           // Default Multimedia device
        e_Unknown               // Unknown or Undefined (DEFAULT)
    };

    /*  Initialise
        Initialise the WASAPI COM classes for a given direction (Player/Recorder).
        NOTE: MUST called first after instancing class.
    */
    void Initialise(
        const PSoundChannel::Directions direction   ///< Direction of Device
        );

    /*  GetDeviceNames
        Get a list of audio devices for a given direction (Player/Recorder).
        NOTE: Initialise must be called first.
    */
    bool GetDeviceNames(PStringArray & devices);

    /*  OpenDeviceByNames
        Open the audio device by name for a given direction (Player/Recorder).
        NOTE: Initialise must be called first.
    */
    bool OpenDeviceByName(const PString & devName);

    /*  OpenDeviceByRole
        Open the audio device by given role for a given direction (Player/Recorder).
        NOTE: Initialise must be called first.
    */
    bool OpenDeviceByRole(PWASAPIDevice::Roles role);


    /*  InitialiseDevice
        Open the audio device by given role  for a given direction (Player/Recorder).
        NOTE: Must call Initialise and OpenDeviceBy* before calling InitialiseDevice.
    */
    bool InitialiseDevice(unsigned numChannels, unsigned sampleRate, unsigned bitsPerSample);

    /*  Start
        Instruct the WASAPI engine to start capturing or rendering.
        NOTE: InitialiseDevice must be called first.
    */
    bool Start();

    /*  Stop
        Instruct the WASAPI engine to stop capturing or rendering.
        NOTE: Engine must be running i.e. call Start to have effect.
    */
    bool Stop();

    /*  Close
        Close the Initialised WASAPI audio device.
    */
    void Close();

    /*  GetVolume
        Get the volume of the opened device
    */
    PBoolean GetVolume(unsigned & vol);

    /*  SetVolume
        Set the volume of the opened device
    */
    PBoolean SetVolume(unsigned vol);

    /*  SetMute
        Mute the device
    */
    PBoolean SetMute(PBoolean mute);

    /*  GetMute
        GetMute status
    */
    PBoolean GetMute(PBoolean & mute);

    /*  Read
        Read from the audio buffer with a given size
        NOTE: Must call Start before being able to read from the buffer.
    */
    bool Read(void * buffer, PINDEX amount);

    /*  Write
        Write to the audio buffer with a given size
        NOTE: Must call Start before being able to write the buffer.
    */
    bool Write(const void * buffer, PINDEX amount);

    /*  GetSampleRate
        Get the current sample rate for media read/write pacing
    */
    unsigned GetSampleRate() const;

    /*  IsInitialised
        Is the WASAPI engine initialised for a given direction
    */
    bool IsInitialised();

    /*  IsOpen
        Is a WASAPI device for a given direction open
    */
    bool IsOpen() const;

protected:

    /*  GetDeviceName
        Get the device name for the given device enumerator
        default enumerates and instances IMMDevice and calls GetDeviceName
    */
    PString GetDeviceName(UINT index);

    /*  GetDeviceName
        Get the device name for the supplied IMMDevice
    */
    PString GetDeviceName(IMMDevice * device);

    /*  OpenDevice
        Open the device with provided enumerator index
    */
    bool OpenDevice(UINT index);

private:

    /*  WASAPI COM classes
    */
    PComPtr<IMMDeviceEnumerator> m_pDevEnum;
    PComPtr<IMMDeviceCollection> m_pDevCol;
    PComPtr<IMMDevice>           m_pDevice;
    PComPtr<IAudioClient>        m_pClient;
    PComPtr<IAudioCaptureClient> m_pCapture;
    PComPtr<IAudioRenderClient>  m_pRender;

    /*  Private class elements
    */
    MFTAudioResampler            m_pResampler;          // Audio resampler

    WAVEFORMATEX                 m_waveFormat;          // Output required Waveform

    std::queue<PBYTEArray>       m_frameBuffer;         // Audio sample buffer
    UINT32                       m_mediaBufferSize;     // sample size for output
    PBYTEArray                   m_resampleBuffer;      // Resample Buffer

    PString                      m_deviceName;          // Device friendly name
    bool                         m_isInitialised;       // Is class initialised for a given direction
    EDataFlow                    m_direction;           // Direction of the class (capture(Recorder) or render(Player))
    PINDEX                       m_devCount;            // Number of devices detected for a given direction
    bool                         m_isOpen;              // Is a particular device initialised and ready
    bool                         m_isRunning;           // Is Capturing or Rendering running.

};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PWASAPIDevice::PWASAPIDevice()
    : m_mediaBufferSize(0), m_resampleBuffer(0), m_isInitialised(false), m_direction(eAll),
    m_devCount(0), m_isOpen(false), m_isRunning(false)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
}

PWASAPIDevice::~PWASAPIDevice()
{
    if (IsOpen())
        Close();
}


void PWASAPIDevice::Initialise(const PSoundChannel::Directions direction)
{
    m_direction = (direction == PSoundChannel::Player) ? eRender : eCapture;

    // Create the system device enumerator
    CHECK_ERROR(CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pDevEnum)), return;)

    // Create an enumerator for the Audio Endpoints
    CHECK_ERROR(m_pDevEnum->EnumAudioEndpoints(m_direction, DEVICE_STATE_ACTIVE, &m_pDevCol), return;)

    UINT devs = 0;
    CHECK_ERROR(m_pDevCol && m_pDevCol->GetCount(&devs), return);
    m_devCount = devs;

    PTRACE(6, "WASAPI\t" << m_devCount << " Audio " << ((direction == PSoundChannel::Player) ? "Play" : "Record") << " devices detected.");

    m_isInitialised = (m_devCount > 0);
}

PString PWASAPIDevice::GetDeviceName(UINT index)
{
    PComPtr<IMMDevice> device;

    CHECK_ERROR(m_pDevCol->Item(index, &device), return PString());

    return GetDeviceName(device);

}

PString PWASAPIDevice::GetDeviceName(IMMDevice * device)
{
    LPWSTR deviceId;
    CHECK_ERROR(device->GetId(&deviceId), return PString());
    PComPtr<IPropertyStore> propertyStore;
    CHECK_ERROR(device->OpenPropertyStore(STGM_READ, &propertyStore), return PString());

    PROPVARIANT friendlyName;
    PropVariantInit(&friendlyName);
    CHECK_ERROR(propertyStore->GetValue(PKEY_Device_FriendlyName, &friendlyName), return PString());

    PString name = friendlyName.bstrVal;
    PropVariantClear(&friendlyName);
    return name;
}

bool PWASAPIDevice::GetDeviceNames(PStringArray & devices)
{
    if (!m_devCount)
        return false;

    for (int i = 0; i < m_devCount; i += 1) {
        devices.AppendString(GetDeviceName(i));
    }
    return true;
}

bool PWASAPIDevice::OpenDevice(UINT index)
{
    CHECK_ERROR_RETURN(m_pDevCol->Item(index, &m_pDevice));

    return true;
}

bool PWASAPIDevice::OpenDeviceByName(const PString & devName)
{
    if (!m_devCount)
        return false;

    int id = -1;
    for (int i = 0; i < m_devCount; i += 1) {
        if (GetDeviceName(i) == devName) {
            id = i;
            break;
        }
    }
    if (id < 0)
        return false;

    if (!OpenDevice(id))
        return false;

    m_deviceName = devName;

    return true;
}

bool PWASAPIDevice::OpenDeviceByRole(PWASAPIDevice::Roles role)
{
    if (IsOpen())
        return true;

    ERole deviceRole;
    switch (role) {
    case e_Communications:
        deviceRole = eCommunications;
        break;
    case e_Multimedia:
        deviceRole = eMultimedia;
        break;
    case e_Console:
    default:
        deviceRole = eConsole;
    }

    CHECK_ERROR_RETURN(m_pDevEnum->GetDefaultAudioEndpoint(m_direction, deviceRole, &m_pDevice))

    m_deviceName = GetDeviceName(m_pDevice);

    return true;
}

bool PWASAPIDevice::InitialiseDevice(unsigned numChannels, unsigned sampleRate, unsigned bitsPerSample)
{
    PAssert(numChannels == 1 || numChannels == 2, PInvalidParameter);
    PAssert(bitsPerSample == 8 || bitsPerSample == 16, PInvalidParameter);

    //  Now activate an IAudioClient object on our preferred endpoint and retrieve the mix format for that endpoint.
    CHECK_ERROR_RETURN(m_pDevice->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER, NULL, reinterpret_cast<void **>(&m_pClient)))

    REFERENCE_TIME DefaultDevicePeriod;
    REFERENCE_TIME MinimumDevicePeriod;
    CHECK_ERROR_RETURN(m_pClient->GetDevicePeriod(&DefaultDevicePeriod, &MinimumDevicePeriod))

    // Output/Input Format
    m_waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    m_waveFormat.wBitsPerSample = bitsPerSample;
    m_waveFormat.nChannels = numChannels;
    m_waveFormat.nSamplesPerSec = sampleRate;
    m_waveFormat.cbSize = 0;
    m_waveFormat.nBlockAlign = m_waveFormat.nChannels * m_waveFormat.wBitsPerSample / 8;
    m_waveFormat.nAvgBytesPerSec = m_waveFormat.nSamplesPerSec * m_waveFormat.nBlockAlign;

    PTRACE(6, "WASAPI\tNeeded Format " << PrintWaveFormatEx(m_waveFormat));

    UINT32 samplesPerSec = 1;

#ifdef _WASAPI_SHAREDEXCLUSIVE

    WAVEFORMATEXTENSIBLE wasFormat;
    wasFormat.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wasFormat.Format.nChannels = 2;
    wasFormat.Format.nSamplesPerSec = 44100;
    wasFormat.Format.wBitsPerSample = 16;
    wasFormat.Format.nBlockAlign = wasFormat.Format.wBitsPerSample / 8 * wasFormat.Format.nChannels;
    wasFormat.Format.nAvgBytesPerSec = wasFormat.Format.nSamplesPerSec * wasFormat.Format.nBlockAlign;
    wasFormat.Format.cbSize = 22;
    wasFormat.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    wasFormat.Samples.wValidBitsPerSample = wasFormat.Format.wBitsPerSample;
    wasFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    samplesPerSec = wasFormat.Format.nSamplesPerSec;

    PTRACE(6, "WASAPI\t" << (m_direction == eCapture ? "Capture" : "Render") << "ing with " << PrintWaveFormatEx(wasFormat.Format));

    // Initialise the resampler
    if ((m_direction == eCapture && !m_pResampler.Initialise(wasFormat.Format, m_waveFormat)) ||
        (m_direction == eRender && !m_pResampler.Initialise(m_waveFormat, wasFormat.Format))) {
        PTRACE(2, "WASAPI\tERROR Initialising Resampler for " << (m_direction == eCapture ? "Capture" : "Render") << "ing.");
        return false;
    }

    DefaultDevicePeriod = BUFFER_DURATION;
    MinimumDevicePeriod = PERIODICITY;

    CHECK_ERROR_RETURN(m_pClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_NOPERSIST, DefaultDevicePeriod, MinimumDevicePeriod, (WAVEFORMATEX*)&wasFormat, NULL))

#else

    WAVEFORMATEX * mixClosestMatch = NULL;
    HRESULT hr = m_pClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &m_waveFormat, &mixClosestMatch);
    if (hr == S_OK) {
        PTRACE(6, "WASAPI\tAccepted " << PrintWaveFormatEx(m_waveFormat));
        mixClosestMatch = &m_waveFormat;
    }
    else if (hr == S_FALSE) {
        PTRACE(6, "WASAPI\t" << (m_direction == eCapture ? "Capture." : "Render") << " need to resample. Closest " << PrintWaveFormatEx(*mixClosestMatch));
        // Initialise the resampler
        if ((m_direction == eCapture && !m_pResampler.Initialise(*mixClosestMatch, m_waveFormat)) ||
            (m_direction == eRender && !m_pResampler.Initialise(m_waveFormat, *mixClosestMatch))) {
                PTRACE(2, "WASAPI\tERROR Initialising Resampler for " << (m_direction == eCapture ? "Capture" : "Render") << "ing.");
                CoTaskMemFree(mixClosestMatch);
                return false;
        }
        else {
            PTRACE(4, "PSound\tWASAPI Resampler for " << (m_direction == eCapture ? "Captur" : "Render") << "ing initialised.");
        }
    }
    else {
        PTRACE(2, "WASAPI\tERROR Format Not Accepted " << PrintWaveFormatEx(m_waveFormat));
        return false;
    }

    CHECK_ERROR(m_pClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_NOPERSIST,
        DefaultDevicePeriod, 0, mixClosestMatch, NULL), { CoTaskMemFree(mixClosestMatch); return false; })

     samplesPerSec = mixClosestMatch->nSamplesPerSec;
     CoTaskMemFree(mixClosestMatch);

#endif

    CHECK_ERROR(m_pClient->GetBufferSize(&m_mediaBufferSize), { PTRACE(4,"WASAPI\tUnable to get audio client buffer:"); return false; })

    if (m_direction == eCapture) {
        CHECK_ERROR(m_pClient->GetService(IID_PPV_ARGS(&m_pCapture)), { PTRACE(4,"WASAPI\tUnable to get new capture client: "); return false; })

        DWORD mmcssTaskIndex = 0;
        if (NULL == AvSetMmThreadCharacteristics(PString("Audio"), &mmcssTaskIndex)) {
            PTRACE(4, "WASAPI\tUnable to enable MMCSS on capture thread: " << GetLastError());
        }
    }
    else if (m_direction == eRender) {
        CHECK_ERROR(m_pClient->GetService(IID_PPV_ARGS(&m_pRender)), { PTRACE(4,"WASAPI\tUnable to get new render client: "); return false; })
    }

    PTRACE(2, "WASAPI\tDevice " << m_deviceName << " initialised for " << (m_direction == eRender ? "play" : "record") << "ing" 
             << " with buffer of " << m_mediaBufferSize << " bytes / " << (1000 * m_mediaBufferSize / samplesPerSec) << "ms");

    m_isOpen = true;
    return m_isOpen;
}

PBoolean PWASAPIDevice::GetVolume(unsigned & vol)
{
    if (!m_isOpen)
        return false;

    PComPtr<IAudioEndpointVolume> endpointVolume;
    CHECK_ERROR_RETURN(m_pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume))

    float epVol = 0.0;
    CHECK_ERROR_RETURN(endpointVolume->GetMasterVolumeLevelScalar(&epVol));
    vol = (unsigned)(epVol * 100L);
    return true;
}


PBoolean PWASAPIDevice::SetVolume(unsigned vol)
{
    if (!m_isOpen)
        return false;

    if (vol > 100 || vol < 0)
        return false;

    PComPtr<IAudioEndpointVolume> endpointVolume;
    CHECK_ERROR_RETURN(m_pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume))

    float epVol = ((float)vol)/100L;
    CHECK_ERROR_RETURN(endpointVolume->SetMasterVolumeLevelScalar(epVol,NULL));

    return true;
}


PBoolean PWASAPIDevice::SetMute(PBoolean mute)
{
    if (!m_isOpen)
        return false;

    PComPtr<IAudioEndpointVolume> endpointVolume;
    CHECK_ERROR_RETURN(m_pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume))

    BOOL pMute = mute;
    CHECK_ERROR_RETURN(endpointVolume->SetMute(pMute, NULL));

    return true;
}


PBoolean PWASAPIDevice::GetMute(PBoolean & mute)
{
    if (!m_isOpen)
        return false;

    PComPtr<IAudioEndpointVolume> endpointVolume;
    CHECK_ERROR_RETURN(m_pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume))

    BOOL pMute = false;
    CHECK_ERROR_RETURN(endpointVolume->GetMute(&pMute));

    mute = pMute;
    return true;
}

bool PWASAPIDevice::Read(void * buffer, PINDEX amount)
{
    if (!m_isRunning) return false;

    UINT32 pLength = 0;
    CHECK_ERROR_RETURN(m_pCapture->GetNextPacketSize(&pLength))

    while (pLength > 0) {  
        short *pData;
        UINT32 framesAvailable;
        DWORD  flags;
        HRESULT hr = m_pCapture->GetBuffer((BYTE**)&pData, &framesAvailable, &flags, NULL, NULL);
        if (hr == S_OK) {
            if (flags == AUDCLNT_BUFFERFLAGS_SILENT) {
                // Error getting audio. http://http://weblab.se/83/
                PTRACE(4, "WASAPI\tERROR: SILENT FLAG.");
                break;
            }

            PINDEX sampleSize = (int)((amount * 1000) / (2.0 * m_waveFormat.nSamplesPerSec));
            PINDEX noSamples = SAMPLE_TIME / sampleSize;

            BYTE * oData = NULL;
            PINDEX oLength;

            if (m_pResampler.IsInitialised()) {

                oLength = noSamples * amount;
                if (m_resampleBuffer.GetSize() != amount)
                    m_resampleBuffer.SetSize(oLength);

                PBoolean moreData = false;
                m_pResampler.Resample((BYTE *)pData, pLength, m_resampleBuffer.GetPointer(), oLength, moreData);

                if (moreData) {
                    PTRACE(2, "WASAPI\tERROR: Resampler logic. Need more data.");
                    CHECK_ERROR(m_pCapture->ReleaseBuffer(framesAvailable), PTRACE(4, "WASAPI\tUnable to release capture buffer:");)
                    CHECK_ERROR_RETURN(m_pCapture->GetNextPacketSize(&pLength));
                    continue;
                }

                oData = m_resampleBuffer.GetPointer();
            }
            else
            {
                oData = (BYTE*)pData;
                oLength = pLength;
            }

            PINDEX sLength = oLength / noSamples;
            for (PINDEX i=0; i<noSamples; ++i)
                m_frameBuffer.push(PBYTEArray(oData+ i*sLength, sLength));

            CHECK_ERROR(m_pCapture->ReleaseBuffer(framesAvailable), PTRACE(4, "WASAPI\tUnable to release capture buffer:");)
            CHECK_ERROR_RETURN(m_pCapture->GetNextPacketSize(&pLength));
        }
        else if (hr != AUDCLNT_S_BUFFER_EMPTY) {
            PTRACE(2, "WASAPI\tERROR: Audio GetBuffer: " << COMErrorMsg(hr));
            return false;
        }
    }

    // Write the frame
    if (!m_frameBuffer.empty()) {
        PBYTEArray & frame = m_frameBuffer.front();
        memcpy(buffer, frame.GetPointer(), frame.GetSize());
        m_frameBuffer.pop();
    } else {
        PTRACE(2, "WASAPI\tNo audio frames ready, writing silence frame.");
        memset(buffer, 0, amount);
    }

    return true;
}

bool PWASAPIDevice::Write(const void * buffer, PINDEX amount)
{
    if (!m_isRunning) return false;

    return true;
}

bool PWASAPIDevice::Start()
{
    if (!m_isOpen) return false;
    if (m_isRunning) return true;

    CHECK_ERROR_RETURN(m_pClient->Start())
        m_isRunning = true;
    return true;
}

bool PWASAPIDevice::Stop()
{
    if (!m_isOpen) return false;
    if (!m_isRunning) return true;

    CHECK_ERROR_RETURN(m_pClient->Stop())

    while (!m_frameBuffer.empty())
        m_frameBuffer.pop();

    m_isRunning = false;
    return true;
}

void PWASAPIDevice::Close()
{
    if (!m_isOpen)
        return;

    Stop();

    while (!m_frameBuffer.empty())
        m_frameBuffer.pop();

    m_pResampler.Close();
    m_resampleBuffer.SetSize(0);

    m_pDevEnum.Release();
    m_pDevCol.Release();
    m_pDevice.Release();
    m_pClient.Release();
    m_pCapture.Release();
    m_pRender.Release();
    m_isOpen = false;
}

bool PWASAPIDevice::IsInitialised()
{
    return m_isInitialised;
}

bool PWASAPIDevice::IsOpen() const
{
    return m_isOpen;
}

unsigned PWASAPIDevice::GetSampleRate() const
{
    if (!m_isOpen)
        return 0;

    return m_waveFormat.nSamplesPerSec;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class PSoundChannel_WASAPI : public PSoundChannel
{
    PCLASSINFO(PSoundChannel_WASAPI, PSoundChannel);

public:
    PSoundChannel_WASAPI();
    ~PSoundChannel_WASAPI();

    PBoolean Open(const PString & device,
      Directions dir, unsigned numChannels,
      unsigned sampleRate, unsigned bitsPerSample
    );

    PBoolean IsOpen() const;

    PBoolean Close();

    PBoolean SetVolume(unsigned volume);

    PBoolean GetVolume(unsigned & volume);

    PBoolean SetMute(PBoolean mute);

    PBoolean GetMute(PBoolean & mute);

    PBoolean Read(void * buffer, PINDEX amount);

    PBoolean Write(const void * buffer, PINDEX amount);

    unsigned GetSampleRate() const;

    bool SourceEncoded(bool & lastFrame, unsigned & amount);

    PBoolean DisableDecode();

    static PStringArray GetNames()  { return PStringArray("WASAPI"); }

    static PStringArray GetDeviceNames(
      Directions direction,               ///< Direction for device (record or play)
      PPluginManager * pluginMgr = NULL   ///< Plug in manager, use default if NULL
      );

    PStringArray GetAudioDeviceNames(
      Directions direction                ///< Direction for device (record or play)
      );

private:

    PWASAPIDevice       m_pMMDevice;
    PAdaptiveDelay      m_pacing;

};

//#################################################################################

PSoundChannel_WASAPI::PSoundChannel_WASAPI() 
{
  
}

PSoundChannel_WASAPI::~PSoundChannel_WASAPI()
{
    if (IsOpen())
        Close();
}

PStringArray PSoundChannel_WASAPI::GetDeviceNames(Directions direction, PPluginManager * /*pluginMgr*/) 
{
    PSoundChannel_WASAPI instance;
    return instance.GetAudioDeviceNames(direction);
}

PStringArray PSoundChannel_WASAPI::GetAudioDeviceNames(Directions direction) 
{
    if (!m_pMMDevice.IsInitialised())
        m_pMMDevice.Initialise(direction);

    PStringArray devices;
    devices.AppendString("Default Console");
    devices.AppendString("Default Communication");
    devices.AppendString("Default Multimedia");

    m_pMMDevice.GetDeviceNames(devices);

    if (direction == PSoundChannel::Recorder)
        devices.AppendString("Console Capture");

    return devices;
}

PBoolean PSoundChannel_WASAPI::Open(const PString & device,
      Directions dir, unsigned numChannels, unsigned sampleRate, unsigned bitsPerSample )
{
    PTRACE(4, "PSound\tWASAPI Opening " << device << " for " << (dir == PSoundChannel::Player ? "play" : "record") << "ing");

    if (device == "Console Capture") {
        if (!m_pMMDevice.IsInitialised())
            m_pMMDevice.Initialise(PSoundChannel::Player);

        if (!m_pMMDevice.OpenDeviceByRole(PWASAPIDevice::e_Console) ||
            !m_pMMDevice.InitialiseDevice(numChannels, sampleRate, bitsPerSample))
                return false;

        return m_pMMDevice.Start();
    }

    if (!m_pMMDevice.IsInitialised())
        m_pMMDevice.Initialise(dir);

    PWASAPIDevice::Roles role = PWASAPIDevice::e_Unknown;
    if (device == "Default Console")
        role = PWASAPIDevice::e_Console;
    else if (device == "Default Communication")
        role = PWASAPIDevice::e_Communications;
    else if (device == "Default Multimedia")
        role = PWASAPIDevice::e_Multimedia;

    if (role == PWASAPIDevice::e_Unknown) {
        if (!m_pMMDevice.OpenDeviceByName(device)) {
            PTRACE(2, "PSound\tWASAPI ERROR opening by name " << device 
                << " for " << (dir == PSoundChannel::Player ? "play" : "record") << "ing");
            return false;
        }
    } else { 
        if (!m_pMMDevice.OpenDeviceByRole(role)) {
            PTRACE(2, "PSound\tWASAPI ERROR opening by role " << device 
                << " for " << (dir == PSoundChannel::Player ? "play" : "record") << "ing");
            return false;
        }
    }
        
    if (!m_pMMDevice.InitialiseDevice(numChannels, sampleRate, bitsPerSample)) {
        PTRACE(2, "PSound\tWASAPI ERROR Initialising " << device 
            << " for " << (dir == PSoundChannel::Player ? "play" : "record") << "ing");
        return false;
    }

    return m_pMMDevice.Start();
}

PBoolean PSoundChannel_WASAPI::IsOpen() const 
{ 
    return m_pMMDevice.IsOpen(); 
}

PBoolean PSoundChannel_WASAPI::Close() 
{ 
    m_pMMDevice.Close();
    return true; 
}

PBoolean PSoundChannel_WASAPI::SetVolume(unsigned volume)
{
    return m_pMMDevice.SetVolume(volume);
}

PBoolean PSoundChannel_WASAPI::GetVolume(unsigned & volume)
{
    return m_pMMDevice.GetVolume(volume);
}

PBoolean PSoundChannel_WASAPI::SetMute(PBoolean mute)
{
    return m_pMMDevice.SetMute(mute);
}

PBoolean  PSoundChannel_WASAPI::GetMute(PBoolean & mute)
{
    return m_pMMDevice.GetMute(mute);
}

PBoolean PSoundChannel_WASAPI::Read(void * buffer, PINDEX amount)
{
    lastReadCount = 0;
    m_pacing.Delay((int)amount / 2 * 1000 / m_pMMDevice.GetSampleRate());

    if (!m_pMMDevice.Read(buffer, amount))
        return false;

    lastReadCount = amount;
    return true;

}


PBoolean PSoundChannel_WASAPI::Write(const void * buf, PINDEX len)
{
    m_pacing.Delay((int)len / 2 * 1000 / m_pMMDevice.GetSampleRate());

    if (!m_pMMDevice.Write(buf, len))
        return false;

    return true;
}

unsigned PSoundChannel_WASAPI::GetSampleRate() const
{
    return m_pMMDevice.GetSampleRate();
}

bool PSoundChannel_WASAPI::SourceEncoded(bool & /*lastFrame*/, unsigned & /*amount*/)
{
   return false;
}

PBoolean PSoundChannel_WASAPI::DisableDecode()
{
   return false;
}

///////////////////////////////////////////////////////////////////////////////

PCREATE_SOUND_PLUGIN(WASAPI, PSoundChannel_WASAPI)

///////////////////////////////////////////////////////////////////////////////////////////////
