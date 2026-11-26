# Audio Decoder High Level Test Specification Document

## Table of Contents

- [Acronyms, Terms and Abbreviations](#acronyms-terms-and-abbreviations)
- [Introduction](#introduction)
- [Module Description](#module-description)
- [L2 Test Specification](#l2-test-specification)
- [L3 Test Specification](#l3-test-specification)

## Acronyms, Terms and Abbreviations

- `HAL`    - Hardware Abstraction Layer
- `API`    - Application Programming Interface
- `AIDL`   - Android Interface Definition Language
- `L2`     - Level 2 Testing
- `L3`     - Level 3 Testing
- `NA`     - Not Applicable
- `Caller` - Any user of the interface via the `APIs`
- `CB`     - Call-back function (suffix)
- `MS12`   - MultiStream 12
- `PCM`    - Pulse Code Modulation
- `AC3`    - Audio Codec 3
- `EAC3`   - Enhanced `AC3`
- `WMA`    - Windows Media Audio
- `AAC`    - Advanced Audio coding
- `DD`     - DOLBY Digital
- `DDPLUS` - DOLBY Digital Plus
- `RDK`    - Reference Design Kit
- `EOS`    - End of Stream
- `rms`    - Root mean Square
- `PTS`    - Presentation Time Stamp
- `rms`    - Root Mean Square

## Introduction

This document provides an overview of the testing requirements for the Audio Decoder module. It outlines the scope of testing, objectives, testing levels and approaches, specific test requirements.

- Audio Decoder Interface Definitions - [Audio Decoder HAL](https://github.com/rdkcentral/rdk-halif-aidl/tree/v0.6/audiodecoder/current/com/rdk/hal/audiodecoder)

- Audio Decoder Overview - [Audio Decoder Overview](https://wiki.rdkcentral.com/display/RDKHAL/Audio+Decoder)

## Module Description

The Audio Decoder `HAL` service provides the interfaces for compressed audio to be passed to the vendor layer for decoding.  It may be passed secure buffers if it indicates support for secure audio processing.

The output of an audio decoder can take two routes:

1. **Non-tunnel mode** - Decoded `PCM` audio frames with metadata are sent back to the `RDK` media pipeline.
2. **Tunnel mode** - Audio is passed directly to the vendor layer mixer.

## Test Streams Specification

- List of audio test streams: [Audio TestStreams](audioDecoder_TestStreams.md)

- Each Test should refer [vts_l3_test_config.yaml](../../test_configuration/vts_l3_test_config.yaml) for the streams and information like `PTS`, frame sizes for the test.

## L2 Test Specification

|Test|Description|HAL APIs|
|----|-----------|--------|
|Set and Get Audio Properties|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>Loop through supported audio decoders</li><li>set read-write properties (`Property`) of decoder and verify using get function</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getProperty, close </li><li>IAudioDecoderController - setProperty</li></ul>|
|Set and Get Audio Properties with multi instance of audio decoder|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>If platform supports multiple decoder, open all the instances of the decoders</li><li>set read-write properties (`Property`) of all decoders and verify using get function</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getProperty, close </li><li>IAudioDecoderController - setProperty</li></ul>|
|States of Audio Decoder|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>Loop through supported audio decoders</li><li>Register event listeners using `registerEventListener`</li><li>Ensure state transitions (`open -> start -> flush -> stop -> close`) work as expected using `getState`. Validate event listener notifications using `onStateChanged`.</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getState, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, stop, flush</li></ul>|
|States of Audio Decoder with multi instance|<ul><li>Read supported audio decoders using `IAudioDecoderManager` class</li><li>If platform supports multiple decoder, open all the instances of the decoders</li><li>Register event listeners using `registerEventListener`</li><li>Ensure state transitions of all decoder instances (`open -> start -> flush -> stop -> close`) work as expected using `getState`. Validate event listener notifications using `onStateChanged`.</li></ul>|<ul><li>IAudioDecoderManager - getAudioDecoderIds, getAudioDecoder</li><li>IAudioDecoder - open, getState, registerEventListener, unregisterEventListener, close </li><li>IAudioDecoderController - start, stop, flush</li></ul>|

## L3 Test Specification

### Prerequisites

#### AV Buffer

Audio frames are supplied to the Audio Decoder during testing, which necessitates a pre-validated AVBuffer module. Therefore, the AVBuffer must successfully pass VTS L1, L2, and L3 testing before being used in the Audio Decoder tests

### Feature List

VTS L3 validates all the Audio Decoder features in both sequential and concurrent decoding modes (if the platform supports multiple decoders) as represented below.

#### Sequential Decoding

The Sequential Decoding test evaluates each audio decoder individually, one after another, to ensure functional correctness across all supported features. This approach helps isolate issues and verify that every decoder instance performs as expected in a standalone environment.

```mermaid
block-beta
    block:1
        AudioStream("Audio Stream") space AVBuffer("AVBuffer") space Decoder("Decoder_1") space Output("Output")
        AudioStream --> AVBuffer
        AVBuffer --> Decoder
        Decoder --> Output
    end
```

#### Concurrent Decoding

The Concurrent Decoding test assesses the system's ability to handle multiple audio decoders operating simultaneously, each configured with different supported features. This test is designed to validate the robustness, resource management, and feature compatibility of the platform under concurrent load, and is applicable only if the platform supports multiple decoder instances.

```mermaid
block-beta
columns 1
    block:1
        AudioStream_1("Audio Stream") space AVBuffer_1("AVBuffer") space Decoder_1("Decoder_1") space Output_1("Output")
        AudioStream_1 --> AVBuffer_1
        AVBuffer_1 --> Decoder_1
        Decoder_1 --> Output_1
    end
    space
    block:2
        AudioStream_2("Audio Stream") space AVBuffer_2("AVBuffer") space Decoder_2("Decoder_2") space Output_2("Output")
        AudioStream_2 --> AVBuffer_2
        AVBuffer_2 --> Decoder_2
        Decoder_2 --> Output_2
    end
    space
    block:N
        AudioStream_N("Audio Stream") space AVBuffer_N("AVBuffer") space Decoder_N("Decoder_N") space Output_N("Output")
        AudioStream_N --> AVBuffer_N
        AVBuffer_N --> Decoder_N
        Decoder_N --> Output_N
    end
```

The following table outlines the core features validated in the L3 test phase

|#|Testing Feature|Description|
|-|---------------|-----------|
|1|Tunnel/Non-Tunnel|Executes the decoder in tunnel mode. Based on stream format, the decoder chooses between tunnel and non-tunnel operation. In tunnel mode, the test validates only metrics and metadata. In non-tunnel mode, test also validates decoded buffers by comparing their `rms` difference against reference data.|
|2|Secure and Non-Secure|Initializes the decoder in secure mode, ensuring all buffer operations occur in secure memory|
|3|Error Streams|Verifies decoder behaviour with error streams. Ensures appropriate events are raised and frame metadata is updated accurately|
|4|Trim|Sends trim parameters to the decoder and checks whether the output buffer is correctly trimmed|
|5|Discontinuity|This test verifies that discontinuity is correctly signalled to the decoder via `signalDiscontinuity()`. Setting `InputBufferMetadata.discontinuity = true` on `decodeBufferWithMetadata()` is illegal and must return `EX_ILLEGAL_ARGUMENT`. The decoder must continue decoding subsequent frames without errors, and the metadata of the decoded frames should indicate that the discontinuity was processed|
|6|EOS|Test the decoder's handling of end-of-stream, signalled via `InputBufferMetadata.endOfStream = true` on the final `decodeBufferWithMetadata()` call. The decoder should correctly decode all frames up to EOS and update relevant metadata fields. It must ignore any buffers received after EOS and resume normal decoding when restarted.|
|7|Flush without reset|Test the decoder's flush functionality. The decoder should discard any buffers already queued and resume normal decoding of subsequent frames|
|8|Flush with reset|Test the decoder’s flush functionality combined with reset. The decoder should discard all queued buffers and transition to the READY state. After restarting, the decoder should properly process incoming frames|
|9|Concurrent decoding|If platform supports multiple decoders, test executes multiple decoders simultaneously, validating decoded outputs and their respective metrics.|

### Test Cases

The table below summarizes the L3 test cases executed using various audio stream formats and configurations. Each test runs against every decoder that supports the stream codec (and secure decode where applicable); tests are skipped if no decoder qualifies. Tunnel mode is not covered by the current implementation. These tests are also executed in a multi-process environment:

|#|Test|Secure|Error streams|Trim|Discontinuity|EOS|Flush|Flush+Reset|Stop+Start|Concurrency|
|-|----|------|-------------|----|-------------|---|-----|-----------|----------|-----------|
|1|ADecode_Sequential_NonSecure|✘|✘|✘|✘|✘|✘|✘|✘|✘|
|2|ADecode_Sequential_NonSecure_Error|✘|✔|✘|✘|✘|✘|✘|✘|✘|
|3|ADecode_Sequential_NonSecure_Trim|✘|✘|✔|✘|✘|✘|✘|✘|✘|
|4|ADecode_Sequential_NonSecure_Discontinuity|✘|✘|✘|✔|✘|✘|✘|✘|✘|
|5|ADecode_Sequential_NonSecure_EOS|✘|✘|✘|✘|✔|✘|✘|✘|✘|
|6|ADecode_Sequential_NonSecure_Flush|✘|✘|✘|✘|✘|✔|✘|✘|✘|
|7|ADecode_Sequential_NonSecure_FlushReset|✘|✘|✘|✘|✘|✘|✔|✘|✘|
|8|ADecode_Sequential_NonSecure_StopStart|✘|✘|✘|✘|✘|✘|✘|✔|✘|
|9|ADecode_Sequential_NonSecure_OnDecodeBufAvail|✘|✘|✘|✘|✘|✘|✘|✘|✘|
|10|ADecode_Sequential_Secure|✔|✘|✘|✘|✘|✘|✘|✘|✘|
|11|ADecode_Sequential_Secure_Error|✔|✔|✘|✘|✘|✘|✘|✘|✘|
|12|ADecode_Sequential_Secure_Trim|✔|✘|✔|✘|✘|✘|✘|✘|✘|
|13|ADecode_Sequential_Secure_Discontinuity|✔|✘|✘|✔|✘|✘|✘|✘|✘|
|14|ADecode_Sequential_Secure_EOS|✔|✘|✘|✘|✔|✘|✘|✘|✘|
|15|ADecode_Sequential_Secure_Flush|✔|✘|✘|✘|✘|✔|✘|✘|✘|
|16|ADecode_Sequential_Secure_FlushReset|✔|✘|✘|✘|✘|✘|✔|✘|✘|
|17|ADecode_Sequential_Secure_StopStart|✔|✘|✘|✘|✘|✘|✘|✔|✘|
|18|ADecode_Concurrent_NonSecure|✘|✘|✘|✘|✘|✘|✘|✘|✔|
|19|ADecode_Concurrent_NonSecure_Error|✘|✔|✘|✘|✘|✘|✘|✘|✔|
|20|ADecode_Concurrent_NonSecure_Trim|✘|✘|✔|✘|✘|✘|✘|✘|✔|
|21|ADecode_Concurrent_NonSecure_Discontinuity|✘|✘|✘|✔|✘|✘|✘|✘|✔|
|22|ADecode_Concurrent_NonSecure_EOS|✘|✘|✘|✘|✔|✘|✘|✘|✔|
|23|ADecode_Concurrent_NonSecure_Flush|✘|✘|✘|✘|✘|✔|✘|✘|✔|
|24|ADecode_Concurrent_NonSecure_FlushReset|✘|✘|✘|✘|✘|✘|✔|✘|✔|
|25|ADecode_Concurrent_NonSecure_StopStart|✘|✘|✘|✘|✘|✘|✘|✔|✔|
|26|ADecode_Concurrent_Secure|✔|✘|✘|✘|✘|✘|✘|✘|✔|
|27|ADecode_Concurrent_Secure_Error|✔|✔|✘|✘|✘|✘|✘|✘|✔|
|28|ADecode_Concurrent_Secure_Trim|✔|✘|✔|✘|✘|✘|✘|✘|✔|
|29|ADecode_Concurrent_Secure_Discontinuity|✔|✘|✘|✔|✘|✘|✘|✘|✔|
|30|ADecode_Concurrent_Secure_EOS|✔|✘|✘|✘|✔|✘|✘|✘|✔|
|31|ADecode_Concurrent_Secure_Flush|✔|✘|✘|✘|✘|✔|✘|✘|✔|
|32|ADecode_Concurrent_Secure_FlushReset|✔|✘|✘|✘|✘|✘|✔|✘|✔|
|33|ADecode_Concurrent_Secure_StopStart|✔|✘|✘|✘|✘|✘|✘|✔|✔|

`ADecode_Sequential_NonSecure_OnDecodeBufAvail` stresses the HAL input queue (larger stream, no delay between decode calls) and verifies that neither `queueFullCount` nor `decodeBufferAvailCount` becomes non-zero.
