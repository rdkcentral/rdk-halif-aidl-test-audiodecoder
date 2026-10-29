# AUDIO DECODER L3 Low Level Test Specification and Procedure Documentation

## Table of Contents

- [Overview](#overview)
- [Acronyms, Terms and Abbreviations](#acronyms-terms-and-abbreviations)
- [References](#references)
- [Test Configuration](#test-configuration)
- [Class Design](#class-design)
- [Common Validation Functions](#common-validation-functions)
- [L3 Tests](#l3-tests)


## Acronyms, Terms and Abbreviations

- `HAL` \- Hardware Abstraction Layer, may include some common components
- `UT`  \- Unit Test(s)
- `rms`  \- Root Mean Square
- `YAML` \- Yet Another Markup Language
- `PTS` \- Presentation Time Stamp
- `Hz` \- Hertz
- `AAC` \- Advanced Audio Coding
- `PCM` \- Pulse Code Modulation
- `EOS` \- End of Stream
- `URL` \- Uniform Resource Locator

## Overview

This document outlines the L3 test design for the Audio Decoder module.

## References

- High Level Test Specification - [audioDecoder_High-Level_TestSpecification.md](audioDecoder_High-Level_TestSpecification.md)
- Audio Decoder Interface Definitions - [Audio Decoder HAL](https://github.com/rdkcentral/rdk-halif-aidl/tree/v0.6/audiodecoder/current/com/rdk/hal/audiodecoder)
- Audio Decoder Overview - [Audio Decoder Overview](https://rdkcentral.github.io/rdk-halif-aidl/halif/audio_decoder/current/audio_decoder/)
- Audio test streams: [Audio TestStreams](audioDecoder_TestStreams.md)

## Test Configuration

The test configuration is defined in [vts_l3_test_config.yaml](../../test_configuration/vts_l3_test_config.yaml), which specifies the streams, `PTS` values, frame sizes, and other key parameters required for L3 validation. This configuration includes references to supporting `YAML` files such as [vts_audio_aac_streams.yaml](../../test_configuration/vts_audio_aac_streams.yaml) files required by the test.

### Example Test Configuration File

```yaml
include_0: test_configuration/vts_audio_aac_streams.yaml
include_1: test_configuration/vts_audio_mp3_streams.yaml

IAudioDecoderManager:
  stream_download_path: "http://<>/streams"
  input_streams_path: "assets/streams/input"
  reference_streams_path: "assets/streams/reference/"
  L3:
    ADecode_Sequential_NonTunnel_NonSecure:
      Controls:
        delay_between_decode_calls: 5 #Time in milli seconds
        stop_start_pts: 106666667
        signal_discontinuity_pts: 149333333
        flush_pts: 170666667
        flush_reset_pts: 192000000
        eos_pts: 192000000
      Inputs:
        Streams:
          - Name: tones_48k_2_256_aac
            PTS:
              - 0
              - 21333333
              - 42666667
              - 64000000
              - 85333333
              - 106666667
              - 128000000
              - 149333333
              - 170666667
              - 192000000
            trim:
              - 128000000 #Start in Nano seconds
              - 149333333 #End
            rms_diff_threshold: 0.5
            decode_time_threshold: 10000000 #Time in nano seconds
          - Name: tones_48k_1_256_aac
            PTS:
              - 0
              - 21333333
              - 42666667
              - 64000000
              - 85333333
              - 106666667
              - 128000000
              - 149333333
              - 170666667
              - 192000000
            trim:
              - 128000000 #Start in Nano seconds
              - 149333333 #End
            rms_diff_threshold: 0.5
            decode_time_threshold: 10000000 #Time in nano seconds
```

#### Key Parameters

- stream_download_path: `URL` path to download the audio test streams
- input_streams_path: Target path where input streams are stored
- reference_streams_path: Target path for reference PCM streams
- delay_between_decode_calls: insert delay in msec between each `decodeBufferWithMetadata()` call
- stop_start_pts: `pts` value in nano seconds at which the decoder should be stopped and restarted during the test. -1 indicates that decoder should not be restarted
- signal_discontinuity_pts: `pts` value in nano seconds at which the test should signal discontinuity via `signalDiscontinuity()`. -1 indicates that no discontinuity should be signalled
- flush_pts - `pts` value in nano seconds that determine when the test should flush the decoder. -1 indicates that test should not flush
- flush_reset_pts - `pts` value in nano seconds that determine when the test should flush with reset the decoder. -1 indicates that test should not flush
- eos_pts - `pts` value in nano seconds that determine when the test should sent `eos` signal to decoder. -1 indicates that test should not signal `eos`
- Name: Identifier for the stream, as defined in the stream configuration file
- PTS: Presentation Time Stamps (per frame) in nanoseconds
- trim: Start and end trim points in Nano seconds
- rms_diff_threshold: Maximum allowed `rms` difference for output validation
- decode_time_threshold: Time budget per frame for decoding, in nanoseconds

### Example Stream Configuration File

```yaml
AudioStreams:
  tones_48k_2_256_aac:
    input: tones_48k_2_256.aac
    reference: tones_48k_2_256_ref.pcm
    codec: AAC_LC
    sample_rate: 48000
    channels: 2
    format: S16LE
    num_of_frames: 10
    frame_size:
      - 288
      - 312
      - 372
      - 370
      - 344
      - 362
      - 360
      - 356
      - 355
      - 334
  tones_48k_1_256_aac:
    input: tones_48k_1_256.aac
    reference: tones_48k_1_256_ref.pcm
    codec: AAC_LC # refer enum Codec for more Codec details
    sample_rate: 48000
    channels: 1
    format: S16LE # refer enum PCMFormat for more format details
    num_of_frames: 10
    frame_size:
      - 203
      - 176
      - 223
      - 190
      - 204
      - 200
      - 194
      - 207
      - 185
      - 201
    #error_frames:
    #  - 0
    #  - 1
```

#### Key Parameters

- input: Stream name
- reference: Reference stream name
- codec: audio codec information, Refer `Codec` enum
- sample_rate: Sample rate in `Hz`
- channels: Number of audio channels
- format: `pcm` format of the reference stream. Refer `PCMFormat` enum
- num_of_frames: Total number of frames in the stream
- frame_size: Byte size for each frame, listed in order
- error_frames: List of invalid frame indices (starting from 0)
## Class Design

This test suite is designed using an object-oriented approach, leveraging the C++ variant of the `ut-core` testing framework. The following class structure and interaction model supports robust test orchestration, concurrent execution, and decoder validation.

### Primary Classes

|#|Class|Description|
|-|-----|-----------|
|1|VTS_L3_AudioDecoder|Test class which contains the test cases implementation|
|2|DecoderTest|Decoder test helper class|
|3|AudioDecoderControllerListener|AudioDecoder Controller listener class|
|4|AudioDecoderEventListener|AudioDecoder Event listener class|

### VTS_L3_AudioDecoder Class

The L3 audio decoder tests are implemented using the C++ variant of `ut-core`. For more information on the framework, refer to [ut-core](https://github.com/rdkcentral/ut-core/blob/4.7.0/README.md)

The following class diagram shows the inheritance structure and configuration dependencies for the test suite:

```mermaid
---
title: Audio Decoder Class Flow
---
classDiagram
    GTEST_API_ <|-- ut_core : inherits
    ut_core <|-- VTS_L3_AudioDecoder : inherits
    class VTS_L3_AudioDecoder{
      SetUp()
      TearDown()
      runSequentialDecode()
      runConcurrentDecode()
      runSingleProcessTest()
      isDecoderUsable()
    }
    VTS_L3_AudioDecoder <|-- L3_Testcases
    note for L3_Testcases "uses vts_l3_test_config.yaml"
```
#### VTS_L3_AudioDecoder Class Functions

The class overrides `SetUp()` and `TearDown()` from `ut_core` to handle initialization and clean-up common to all test cases

|#|Function|Arguments|Description|HAL API|
|-|--------|---------|-----------|-------|
|1|Setup|None|Initializes the test by acquiring service handles, supported decoder IDs, decoder capabilities, and registering event listeners|`getAudioDecoderIds()`<br>`getAudioDecoder()`<br>`registerEventListener()`<br>`getCapabilities()`|
|2|TearDown|None|Cleans up by unregistering event listeners|`unregisterEventListener()`|
|3|runSequentialDecode|testConfiguration_t& configuration<br>bool secureDecode<br>bool tunnelMode|Decodes a stream using supported decoders sequentially. If the first observed output handle indicates a different tunnel mode than requested, mode-specific validation is suppressed and the caller skips the test without running the remaining streams.|`getHeapMetrics()`|
|4|runConcurrentDecode|testConfiguration_t& configuration<br>bool secureDecode<br>bool tunnelMode|Launches decoder threads for one stream configuration. If the observed output handles indicate a different tunnel mode than requested, mode-specific validation is suppressed and the caller skips the test without running the remaining streams.|`getHeapMetrics()`|
|5|runSingleProcessTest|char *testName<br>bool secureMode<br>bool tunnelMode<br>bool concurrentMode|Invokes `runConcurrentDecode` or `runSequentialDecode`. After the first usable stream reveals the platform output mode, it immediately skips an unsupported requested mode instead of executing every configured stream.||

#### Setup

The following sequence diagram illustrates the flow of the `Setup` function:

```mermaid
sequenceDiagram
  participant VTS_L3_AudioDecoder(Setup)
  participant AudioDecoder HAL

  VTS_L3_AudioDecoder(Setup)->>AudioDecoder HAL: getAudioDecoderIds()
  AudioDecoder HAL-->>VTS_L3_AudioDecoder(Setup): list of decoder IDs
  loop For each decoder ID
    VTS_L3_AudioDecoder(Setup)->>AudioDecoder HAL: getAudioDecoder()
    AudioDecoder HAL-->>VTS_L3_AudioDecoder(Setup): decoder handle
    VTS_L3_AudioDecoder(Setup)->>AudioDecoder HAL: registerEventListener()
    VTS_L3_AudioDecoder(Setup)->>AudioDecoder HAL: getCapabilities()
    AudioDecoder HAL-->>VTS_L3_AudioDecoder(Setup): decoder capabilities
  end
```
#### TearDown

```mermaid
sequenceDiagram
  participant VTS_L3_AudioDecoder(TearDown)
  participant AudioDecoder HAL

  loop For each decoder ID
    VTS_L3_AudioDecoder(TearDown)->>AudioDecoder HAL: unregisterEventListener()
  end
```

#### runSequentialDecode

The following sequence diagram illustrates the flow of the `runSequentialDecode` function:

```mermaid
sequenceDiagram
  participant rundecoder as VTS_L3_AudioDecoder(runSequentialDecode)
  participant DecoderTest as DecoderTest
  participant buffer as AV Buffer HAL

  loop For each decoder
    rundecoder->>buffer : getHeapMetrics()
    buffer -->> rundecoder: AV Buffer metrics
    rundecoder->>DecoderTest: run()
    rundecoder->>rundecoder: wait for decode <br>to complete
    rundecoder->>buffer : getHeapMetrics()
    buffer -->> rundecoder: AV Buffer metrics
    rundecoder->>rundecoder: Validate AV buffer metrics
  end
```

#### runConcurrentDecode

The following sequence diagram illustrates the flow of the `runConcurrentDecode` function:

```mermaid
sequenceDiagram
  participant rundecoder as VTS_L3_AudioDecoder(runConcurrentDecode)
  participant thread_1 as decoderThread_1
  participant thread_N as decoderThread_2..N
  participant DecoderTest
  participant buffer as AV Buffer HAL

  loop For each decoder
    rundecoder->>buffer : getHeapMetrics()
    buffer -->> rundecoder: AV Buffer metrics
    rundecoder->>thread_1: launch thread
    rundecoder->>thread_N: launch threads
    par thread_1
      thread_1->>DecoderTest: run()
      thread_1->>thread_1: wait for decode <br>to complete
    and thread_N
      thread_N->>DecoderTest: run()
      thread_N->>thread_N: wait for decode <br>to complete
    end
    rundecoder->>rundecoder: join threads
    rundecoder->>buffer : getHeapMetrics()
    buffer -->> rundecoder: AV Buffer metrics
    rundecoder->>rundecoder: Validate AV buffer metrics
  end
```

### DecoderTest Class

This class includes methods for testing individual streams and frames.

|#|Function|Arguments|Description|HAL API|
|-|--------|---------|-----------|-------|
|1|decodeAndConsumeAudioStreams|bool validateMetrics|Decodes and consumes the audio frames sequentially|`createAudioPool()`<br>`alloc()`<br>`free()`<br>`destroyPool()`<br>`open()`<br>`start()`<br>`decodeBufferWithMetadata()`<br>`stop()`<br>`close()`|
|2|run|bool validateMetrics|calls decodeAndConsumeAudioStreams||
|3|getDecoderMetrics|int32_t &framesDecoded<br>int32_t &decodeErrors<br>int32_t &framesDropped|Gets the decoder metrics|`getProperty()`|

#### decodeAndConsumeAudioStreams

The following sequence diagram illustrates the flow of the `decodeAndConsumeAudioStreams` function:

```mermaid
sequenceDiagram
  participant rundecoder as DecoderTest(decodeAndConsumeAudioStreams)
  participant AudioDecoder HAL
  participant AV Buffer HAL

  rundecoder->>AV Buffer HAL: createAudioPool()
  AV Buffer HAL-->>rundecoder: pool handle
  loop For each stream
    rundecoder->>AudioDecoder HAL: open()
    AudioDecoder HAL-->>rundecoder: controller handle
    rundecoder->>AudioDecoder HAL: start()
    loop For each frame
      rundecoder->>AV Buffer HAL: alloc()
      AV Buffer HAL-->>rundecoder: buffer handle
      rundecoder->>rundecoder: read frame & copy to buffer
      rundecoder->>AudioDecoder HAL: decodeBufferWithMetadata()
      rundecoder->>rundecoder: wait for onFrameOutput()
      AudioDecoder HAL-->>rundecoder: onFrameOutput()
      rundecoder->>rundecoder: validate decode time
      rundecoder->>rundecoder: validate metadata and metrics
      rundecoder->>AV Buffer HAL: free()
    end
    rundecoder->>AudioDecoder HAL: stop()
    rundecoder->>AudioDecoder HAL: close()
  end
  rundecoder->>AV Buffer HAL: destroyPool()
```

### AudioDecoderControllerListener Class

This class is responsible for handling decoded frame output callbacks from the AudioDecoder `HAL`. The class collects decoded audio buffers along with their metadata and timestamps for later validation by the test framework

```mermaid
---
title: AudioDecoderControllerListener Class
---
classDiagram
    BnAudioDecoderControllerListener <|-- AudioDecoderControllerListener : inherits
    class AudioDecoderControllerListener{
        onFrameOutput()
        getNextFrame()
    }
```

#### Structures

##### DecodedAudioFrame_t

DecodedAudioFrame_t encapsulates information about each decoded audio frame, including the raw PCM data, number of samples, associated metadata, and the timestamp at which the test received the buffer. This is useful for time-based validation and synchronization with the reference stream.

```cpp
typedef struct DecodedAudioFrame
{
    vector<uint8_t> data;  /* Decoded pcm data */
    int64_t framePTS;      /* PTS of decoded frame */
    size_t numSamples;     /* Number of samples in the buffer */
    bool isMetaAvailable;  /* is metadata received from callback */
    struct AudioMetatData metadata; /* metadata */
    chrono::steady_clock::time_point receiveTime; /* Time at which test received the buffer */
}DecodedAudioFrame_t;
```
##### InputAudioFrame_t

This structure encapsulates information about each input audio frame

```cpp
typedef struct InputAudioFrame
{
    int64_t framePTS;
    bool isErrorFrame;
    chrono::steady_clock::time_point sendTime;
}InputAudioFrame_t;
```

##### ErrorDetails_t

The ErrorDetrails_t structure encapsulates information about an error,
including error codes, descriptions, and any relevant context needed for
error handling or debugging. This structure is typically used to provide
comprehensive error reporting within the application.

```cpp
typedef struct ErrorDetails
{
    ErrorCode vendorErrorCode;
    int32_t errorCode;
    chrono::steady_clock::time_point receiveTime;
}ErrorDetails_t;
```
#### AudioDecoderControllerListener Class Functions

|#|Function|Arguments|Description|
|-|--------|---------|-----------|
|1|onFrameOutput|int64_t nsPresentationTime<br>int64_t frameBufferHandle<br>const optional<FrameMetadata>& metadata|Override function from BnAudioDecoderControllerListener. Called by the `HAL` when a decoded frame is ready. It enqueues the decoded frame and metadata into an internal queue for test validation.|
|2|getNextFrame|DecodedAudioFrame_t &frameInfo|Pops the next available frame from the internal queue. Used by test logic to retrieve and validate decoded output|

### AudioDecoderEventListener Class

This class implements the BnAudioDecoderEventListener interface and is responsible for receiving and queuing events emitted by the AudioDecoder `HAL`. These events include decoding errors and state transitions, which are later validated by the test cases to ensure correct decoder behaviour.

```mermaid
---
title: AudioDecoderEventListener Class
---
classDiagram
    BnAudioDecoderEventListener <|-- AudioDecoderEventListener : inherits
    class AudioDecoderEventListener{
        onDecodeError()
        onStateChanged()
        waitForState()
        checkStateTransistion()
        getNextError()
    }
```

#### AudioDecoderEventListener Class Functions

|#|Function|Arguments|Description|
|-|--------|---------|-----------|
|1|onDecodeError|ErrorCode vendorErrorCode<br>int32_t errorCode|Callback invoked when the decoder reports an error. Captures the error information into an internal queue for further validation by the test case|
|2|onStateChanged|State oldState<br>State newState|Callback invoked when the decoder transitions between states. Logs the transition in a queue to allow the test case to verify correct state sequencing|
|3|waitForState|State desiredState<br>chrono::milliseconds timeout|Blocks execution until the decoder reaches the desired state or the timeout expires. Used to synchronize test flow with HAL state transitions|
|4|checkStateTransistion|State expOldState<br>State expNewState|Validates whether the expected transition from expOldState to expNewState has occurred by scanning the recorded state transition queue|
|5|getNextError|ErrorDetails_t &errorDetails|Pops the next available error from the internal queue|

## Helper Functions

These utility functions are shared across all VTS test cases

### Structures

#### testConfiguration_t

This structure holds per-test configuration details parsed from the `YAML` file (`vts_l3_test_config.yaml`). It defines all the input stream characteristics, reference comparison parameters, and frame-level metadata needed for test execution. This structure drives test behaviour and validation logic.

```cpp
typedef struct testConfiguration
{
    std::string input;           /* Path to input encoded stream file */
    std::string reference;       /* Path to reference PCM stream file */
    Codec codec;                 /* Codec type (e.g., AAC, OPUS) */
    int32_t sampleRate;          /* Sample rate of the stream */
    int32_t numChannels;         /* Number of audio channels */
    PCMFormat format;            /* PCM format of the reference stream */
    int32_t numOfFrames;         /* Total number of frames expected in the stream */
    double rms_diff_threashold;  /* Acceptable RMS difference threshold */
    int64_t trimStartNs;         /* Start trim position in nanoseconds */
    int64_t trimEndNs;           /* End trim position in nanoseconds */
    int32_t decoderDelay;        /* delay in msec between each decodeBufferWithMetadata() call */
    int64_t stopPts;             /*  pts value at which the decoder should be stopped and restarted */
    int64_t discontinuityPts;    /* pts value at which signalDiscontinuity() is called. -1 means no discontinuity */
    int64_t flushPts;            /* pts value that determine when the test should flush the decoder. -1 indicates that test should not flush */
    int64_t flushRestPts;        /* pts value that determine when the test should flush with reset the decoder. -1 indicates that test should not flush */
    int64_t eosPts;              /* pts value that determine when the test should signal eos via InputBufferMetadata.endOfStream. -1 indicates that test should not signal eos */
    int64_t sinkEosPts;          /* pts value at which sink eos should be signalled */
    int64_t sinkFlushPts;        /* pts value at which sink flush should be triggered */
    int32_t duration;            /* stream duration in milliseconds */
    std::vector<std::tuple<int32_t, int64_t, bool>> frameDetails; /* Tuple of frameSize, PTS, isCorrupted */
}testConfiguration_t;

```
### Functions

|#|Helper Functions|Arguments|Description|
|-|----------------|---------|-----------|
|1|vts_audiodeccoder_parse_testconfig|void* pTestInstance: ut kvp instance<br>char *module: Module name<br>char *testGroup: Test group L1, L2, L3<br>char *testNameL: Test name<br>vector\<testConfiguration_t\>& configuration|Parses the `vts_l3_test_config.yaml` for the given module, test group, and test name. Populates a list of testConfiguration_t structures for use in the test|
|2|vts_audiodecoder_parseAudioDecoderHFP|None|Parses the HAL feature profile (HFP) file to determine supported capabilities of the AudioDecoder HAL under test|

## Common Validation Functions

These functions provide standard mechanisms for validating the output and behaviour of the audio decoder under test

### Validate Decoder Metrics

The following decoder metrics are validated before and after decoding to ensure accurate tracking:

- `METRIC_FRAMES_DECODED`
- `METRIC_DECODE_ERRORS`
- `METRIC_FRAMES_DROPPED`

These are retrieved using the `getProperty()` API. The test confirms that the metrics increment correctly during the decoding process.

### Validate Decode Time

The time taken to decode each frame is calculated as the difference between `decodeBufferWithMetadata()` and the corresponding `onFrameOutput()` event. The result is validated against the `decode_time_threshold` specified in the configuration [vts_l3_test_config.yaml](../../test_configuration/vts_l3_test_config.yaml)

### Validate AV Buffers

This check ensures that both the test suite and the decoder HAL properly release allocated audio buffers. It compares memory usage at the beginning and end of each test using the `getHeapMetrics()` API to confirm no memory leaks or unreleased buffers remain.

## L3 Tests

The L3 tests validate the functional correctness and performance of the AudioDecoder HAL by simulating real-world decoding workflows. Each test calls `runSingleProcessTest` with different parameters. `Test Configuration Controls` describes the control parameters used from test configuration file `vts_l3_test_config.yaml`

|# | Test Name                                         | function             | Arguments                                      | Test Configuration Controls                |
|--|---------------------------------------------------|----------------------|------------------------------------------------|--------------------------------------------|
| 1| ADecode_Sequential_NonTunnel_NonSecure             | runSingleProcessTest | secureMode = false<br>concurrentMode = false   | Inputs.Streams                             |
| 2| ADecode_Sequential_NonTunnel_NonSecure_Error       | runSingleProcessTest | secureMode = false<br>concurrentMode = false   | Inputs.Streams                             |
| 3| ADecode_Sequential_NonTunnel_NonSecure_Trim        | runSingleProcessTest | secureMode = false<br>concurrentMode = false   | Inputs.Streams<br>Inputs.Streams.trim      |
| 4| ADecode_Sequential_NonTunnel_NonSecure_Discontinuity| runSingleProcessTest| secureMode = false<br>concurrentMode = false   | Inputs.Streams<br>Controls.signal_discontinuity_pts |
| 5| ADecode_Sequential_NonTunnel_NonSecure_EOS         | runSingleProcessTest | secureMode = false<br>concurrentMode = false   | Inputs.Streams<br>Controls.eos_pts         |
| 6| ADecode_Sequential_NonTunnel_NonSecure_Flush       | runSingleProcessTest | secureMode = false<br>concurrentMode = false   | Inputs.Streams<br>Controls.flush_pts       |
| 7| ADecode_Sequential_NonTunnel_NonSecure_FlushReset  | runSingleProcessTest | secureMode = false<br>concurrentMode = false   | Inputs.Streams<br>Controls.flush_reset_pts |
| 8| ADecode_Sequential_NonTunnel_NonSecure_StopStart   | runSingleProcessTest | secureMode = false<br>concurrentMode = false   | Inputs.Streams<br>Controls.stop_start_pts  |
| 9| ADecode_Sequential_NonTunnel_Secure                | runSingleProcessTest | secureMode = true<br>concurrentMode = false    | Inputs.Streams                             |
|10| ADecode_Sequential_NonTunnel_Secure_Error          | runSingleProcessTest | secureMode = true<br>concurrentMode = false    | Inputs.Streams                             |
|11| ADecode_Sequential_NonTunnel_Secure_Trim           | runSingleProcessTest | secureMode = true<br>concurrentMode = false    | Inputs.Streams<br>Inputs.Streams.trim      |
|12| ADecode_Sequential_NonTunnel_Secure_Discontinuity  | runSingleProcessTest | secureMode = true<br>concurrentMode = false    | Inputs.Streams<br>Controls.signal_discontinuity_pts |
|13| ADecode_Sequential_NonTunnel_Secure_EOS            | runSingleProcessTest | secureMode = true<br>concurrentMode = false    | Inputs.Streams<br>Controls.eos_pts         |
|14| ADecode_Sequential_NonTunnel_Secure_Flush          | runSingleProcessTest | secureMode = true<br>concurrentMode = false    | Inputs.Streams<br>Controls.flush_pts       |
|15| ADecode_Sequential_NonTunnel_Secure_FlushReset     | runSingleProcessTest | secureMode = true<br>concurrentMode = false    | Inputs.Streams<br>Controls.flush_reset_pts |
|16| ADecode_Sequential_NonTunnel_Secure_StopStart      | runSingleProcessTest | secureMode = true<br>concurrentMode = false    | Inputs.Streams<br>Controls.stop_start_pts  |
|17| ADecode_Sequential_Tunnel_NonSecure                | runSingleProcessTest | secureMode = false<br>concurrentMode = false   | Inputs.Streams                             |
|18| ADecode_Sequential_Tunnel_NonSecure_Error          | runSingleProcessTest | secureMode = false<br>concurrentMode = false   | Inputs.Streams                             |
|19| ADecode_Sequential_Tunnel_NonSecure_Trim           | runSingleProcessTest | secureMode = false<br>concurrentMode = false   | Inputs.Streams<br>Inputs.Streams.trim      |
|20| ADecode_Sequential_Tunnel_NonSecure_Discontinuity  | runSingleProcessTest | secureMode = false<br>concurrentMode = false   | Inputs.Streams<br>Controls.signal_discontinuity_pts |
|21| ADecode_Sequential_Tunnel_NonSecure_EOS            | runSingleProcessTest | secureMode = false<br>concurrentMode = false   | Inputs.Streams<br>Controls.eos_pts         |
|22| ADecode_Sequential_Tunnel_NonSecure_Flush          | runSingleProcessTest | secureMode = false<br>concurrentMode = false   | Inputs.Streams<br>Controls.flush_pts       |
|23| ADecode_Sequential_Tunnel_NonSecure_FlushReset     | runSingleProcessTest | secureMode = false<br>concurrentMode = false   | Inputs.Streams<br>Controls.flush_reset_pts |
|24| ADecode_Sequential_Tunnel_NonSecure_StopStart      | runSingleProcessTest | secureMode = false<br>concurrentMode = false   | Inputs.Streams<br>Controls.stop_start_pts  |
|25| ADecode_Sequential_Tunnel_Secure                   | runSingleProcessTest | secureMode = true<br>concurrentMode = false    | Inputs.Streams                             |
|26| ADecode_Sequential_Tunnel_Secure_Error             | runSingleProcessTest | secureMode = true<br>concurrentMode = false    | Inputs.Streams                             |
|27| ADecode_Sequential_Tunnel_Secure_Trim              | runSingleProcessTest | secureMode = true<br>concurrentMode = false    | Inputs.Streams<br>Inputs.Streams.trim      |
|28| ADecode_Sequential_Tunnel_Secure_Discontinuity     | runSingleProcessTest | secureMode = true<br>concurrentMode = false    | Inputs.Streams<br>Controls.signal_discontinuity_pts |
|29| ADecode_Sequential_Tunnel_Secure_EOS               | runSingleProcessTest | secureMode = true<br>concurrentMode = false    | Inputs.Streams<br>Controls.eos_pts         |
|30| ADecode_Sequential_Tunnel_Secure_Flush             | runSingleProcessTest | secureMode = true<br>concurrentMode = false    | Inputs.Streams<br>Controls.flush_pts       |
|31| ADecode_Sequential_Tunnel_Secure_FlushReset        | runSingleProcessTest | secureMode = true<br>concurrentMode = false    | Inputs.Streams<br>Controls.flush_reset_pts |
|32| ADecode_Sequential_Tunnel_Secure_StopStart         | runSingleProcessTest | secureMode = true<br>concurrentMode = false    | Inputs.Streams<br>Controls.stop_start_pts  |
|33| ADecode_Concurrent_NonTunnel_NonSecure             | runSingleProcessTest | secureMode = false<br>concurrentMode = true    | Inputs.Streams                             |
|34| ADecode_Concurrent_NonTunnel_NonSecure_Error       | runSingleProcessTest | secureMode = false<br>concurrentMode = true    | Inputs.Streams                             |
|35| ADecode_Concurrent_NonTunnel_NonSecure_Trim        | runSingleProcessTest | secureMode = false<br>concurrentMode = true    | Inputs.Streams<br>Inputs.Streams.trim      |
|36| ADecode_Concurrent_NonTunnel_NonSecure_Discontinuity| runSingleProcessTest| secureMode = false<br>concurrentMode = true    | Inputs.Streams<br>Controls.signal_discontinuity_pts |
|37| ADecode_Concurrent_NonTunnel_NonSecure_EOS         | runSingleProcessTest | secureMode = false<br>concurrentMode = true    | Inputs.Streams<br>Controls.eos_pts         |
|38| ADecode_Concurrent_NonTunnel_NonSecure_Flush       | runSingleProcessTest | secureMode = false<br>concurrentMode = true    | Inputs.Streams<br>Controls.flush_pts       |
|39| ADecode_Concurrent_NonTunnel_NonSecure_FlushReset  | runSingleProcessTest | secureMode = false<br>concurrentMode = true    | Inputs.Streams<br>Controls.flush_reset_pts |
|40| ADecode_Concurrent_NonTunnel_NonSecure_StopStart   | runSingleProcessTest | secureMode = false<br>concurrentMode = true    | Inputs.Streams<br>Controls.stop_start_pts  |
|41| ADecode_Concurrent_NonTunnel_Secure                | runSingleProcessTest | secureMode = true<br>concurrentMode = true     | Inputs.Streams                             |
|42| ADecode_Concurrent_NonTunnel_Secure_Error          | runSingleProcessTest | secureMode = true<br>concurrentMode = true     | Inputs.Streams                             |
|43| ADecode_Concurrent_NonTunnel_Secure_Trim           | runSingleProcessTest | secureMode = true<br>concurrentMode = true     | Inputs.Streams<br>Inputs.Streams.trim      |
|44| ADecode_Concurrent_NonTunnel_Secure_Discontinuity  | runSingleProcessTest | secureMode = true<br>concurrentMode = true     | Inputs.Streams<br>Controls.signal_discontinuity_pts |
|45| ADecode_Concurrent_NonTunnel_Secure_EOS            | runSingleProcessTest | secureMode = true<br>concurrentMode = true     | Inputs.Streams<br>Controls.eos_pts         |
|46| ADecode_Concurrent_NonTunnel_Secure_Flush          | runSingleProcessTest | secureMode = true<br>concurrentMode = true     | Inputs.Streams<br>Controls.flush_pts       |
|47| ADecode_Concurrent_NonTunnel_Secure_FlushReset     | runSingleProcessTest | secureMode = true<br>concurrentMode = true     | Inputs.Streams<br>Controls.flush_reset_pts |
|48| ADecode_Concurrent_NonTunnel_Secure_StopStart      | runSingleProcessTest | secureMode = true<br>concurrentMode = true     | Inputs.Streams<br>Controls.stop_start_pts  |
|49| ADecode_Concurrent_Tunnel_NonSecure                | runSingleProcessTest | secureMode = false<br>concurrentMode = true    | Inputs.Streams                             |
|50| ADecode_Concurrent_Tunnel_NonSecure_Error          | runSingleProcessTest | secureMode = false<br>concurrentMode = true    | Inputs.Streams                             |
|51| ADecode_Concurrent_Tunnel_NonSecure_Trim           | runSingleProcessTest | secureMode = false<br>concurrentMode = true    | Inputs.Streams<br>Inputs.Streams.trim      |
|52| ADecode_Concurrent_Tunnel_NonSecure_Discontinuity  | runSingleProcessTest | secureMode = false<br>concurrentMode = true    | Inputs.Streams<br>Controls.signal_discontinuity_pts |
|53| ADecode_Concurrent_Tunnel_NonSecure_EOS            | runSingleProcessTest | secureMode = false<br>concurrentMode = true    | Inputs.Streams<br>Controls.eos_pts         |
|54| ADecode_Concurrent_Tunnel_NonSecure_Flush          | runSingleProcessTest | secureMode = false<br>concurrentMode = true    | Inputs.Streams<br>Controls.flush_pts       |
|55| ADecode_Concurrent_Tunnel_NonSecure_FlushReset     | runSingleProcessTest | secureMode = false<br>concurrentMode = true    | Inputs.Streams<br>Controls.flush_reset_pts |
|56| ADecode_Concurrent_Tunnel_NonSecure_StopStart      | runSingleProcessTest | secureMode = false<br>concurrentMode = true    | Inputs.Streams<br>Controls.stop_start_pts  |
|57| ADecode_Concurrent_Tunnel_Secure                   | runSingleProcessTest | secureMode = true<br>concurrentMode = true     | Inputs.Streams                             |
|58| ADecode_Concurrent_Tunnel_Secure_Error             | runSingleProcessTest | secureMode = true<br>concurrentMode = true     | Inputs.Streams                             |
|59| ADecode_Concurrent_Tunnel_Secure_Trim              | runSingleProcessTest | secureMode = true<br>concurrentMode = true     | Inputs.Streams<br>Inputs.Streams.trim      |
|60| ADecode_Concurrent_Tunnel_Secure_Discontinuity     | runSingleProcessTest | secureMode = true<br>concurrentMode = true     | Inputs.Streams<br>Controls.signal_discontinuity_pts |
|61| ADecode_Concurrent_Tunnel_Secure_EOS               | runSingleProcessTest | secureMode = true<br>concurrentMode = true     | Inputs.Streams<br>Controls.eos_pts         |
|62| ADecode_Concurrent_Tunnel_Secure_Flush             | runSingleProcessTest | secureMode = true<br>concurrentMode = true     | Inputs.Streams<br>Controls.flush_pts       |
|63| ADecode_Concurrent_Tunnel_Secure_FlushReset        | runSingleProcessTest | secureMode = true<br>concurrentMode = true     | Inputs.Streams<br>Controls.flush_reset_pts |
|64| ADecode_Concurrent_Tunnel_Secure_StopStart         | runSingleProcessTest | secureMode = true<br>concurrentMode = true     | Inputs.Streams<br>Controls.stop_start_pts  |
