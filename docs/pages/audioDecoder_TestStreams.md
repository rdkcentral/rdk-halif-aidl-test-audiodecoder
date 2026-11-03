# Audio Decoder Test Streams

Test streams are local data files for the test suite to inject directly into the `HAL`.

They are read by the test suite, written into buffers and passed into the audio decoder.

The naming convention for the test streams is as follows:

```bash

<Description>_<Samplerate>_<channel>_<Bitrate kbps>.<extension> (elementary stream file)
e.g.
tone_48k_2_256.ac3

```

The following test streams will be used for validation:

## AAC-LC

|#|Name|Frames|Description|
|-|----|------|-----------|
|1|tones_8k_2_64.aac|10|250Hz tone|
|2|tones_11.025k_2_64.aac|10|250Hz tone|
|3|tones_16k_2_64.aac|10|250Hz tone|
|4|tones_32k_2_64.aac|10|250Hz tone|
|5|tones_44.1k_2_128.aac|10|250Hz tone|
|6|tones_48k_1_256_10.aac|10|250Hz tone|
|7|tones_48k_2_256.aac|10|250Hz tone|
|8|tones_48k_5.1_384.aac|10|250Hz tone|
|9|tones_48k_7.1_384.aac|10|250Hz tone|
|10|tones_64k_2_256.aac|10|250Hz tone|
|11|tones_88.2k_2_256.aac|10|250Hz tone|
|12|tones_96k_2_384.aac|10|250Hz tone|
|13|tones_96k_5.1_512.aac|10|250Hz tone|
|14|tones_96k_7.1_1024.aac|10|250Hz tone|
|15|tones_48k_2_256_zero.aac|10|Random bits in the frames are reset to zero|
|16|tones_48k_2_256_random_bitflip.aac|10|Random bits in the frames are flipped|
|17|tones_48k_2_256_random.aac|10|Random bits in the frames are replaced with random values|
|18|tones_48k_2_256_sync.aac|10|invalid sync-word|
|19|tones_48k_2_256_header_fields.aac|10|Invalid sampling freq, invalid channel config, invalid layer|
|20|tones_48k_2_256_incompleframes.aac|10|Removed random bytes from each frames|

## MP3 (MPEG 1/2 Layer 3)

|#|Name|Frames|Description|
|-|----|------|-----------|
|1|tones_32k_1_256.mp3|10|250Hz tone|
|2|tones_32k_2_256.mp3|10|250Hz tone|
|3|tones_44.1k_1_128.mp3|10|250Hz tone|
|4|tones_44.1k_2_128.mp3|10|250Hz tone|
|5|tones_48k_1_256.mp3|10|250Hz tone|
|6|tones_48k_2_256.mp3|10|250Hz tone|
|7|tones_48k_2_256_sync|10|invalid syncword|
|8|tones_48k_2_256_zero.mp3|3|frame contains valid sync word with data replaced with zero|
|9|tones_48k_2_256_randombits.mp3|3|frame contains valid sync word with data replaced with ff|

## Error Streams Info

AV VTS L4 includes playback of error (corrupted) streams to evaluate the error recovery capability of the product.

As part of this verification, various audio error streams are generated and used during testing.

**Example:** Audio Error Stream File Name

```bash

    aac_48k_2ch_frame-skip-duplication_256k_a2814_v0_60sec.mp4

```

**Stream File Name Breakdown:**

|Codec|Sample Rate|Channel|Error Type|Bit Rate|Audio Frames|Video Frames|Duration|
|------|----------|------------|---------|----------|-------------|--------|------|
|AAC|48k | 2ch        |frame-skip-duplication|256k|2814|0|60sec|

### Supported Audio Error Types

Below is the list of supported audio error types used for AV VTS L4 recovery validation:

#### Generic Corruptions Modes

|**Audio Error Type**|**Description**|**Typical Effects**|**Applies to Codec Type**|
|--------------------|---------------|-------------------|-------------------------|
|**frame-skip-duplication**|Randomly drops or duplicates complete audio frames.|Audible pops, timing drift, “Invalid frame length” warnings.|AAC, AC3/EAC3, MP3, FLAC, PCM, Vorbis, Opus|
|**frame-truncation**|Cuts a frame before its natural end, leaving partial data.|Decoder “unexpected EOF” or silence gap.|AAC, AC3/EAC3, MP3, FLAC, PCM, Vorbis, Opus|
|**bitrate-field-corruption**|Alters header fields that signal bitrate or profile.|Decoder fallback to default profile; momentary mute.|AAC, AC3/EAC3, MP3, Vorbis, Opus|
|**channel-mode-corruption**|Flips channel config bits (stereo/mono/surround).|Channel imbalance, “Invalid channel configuration”.|AAC, AC3/EAC3, MP3, FLAC, PCM, Vorbis, Opus|
|**dynamic-range-corruption**|Modifies DRC metadata values.|Louder/quieter playback bursts.|AAC, AC3/EAC3, MP3, FLAC, PCM, Vorbis, Opus|
|**entropy-bit-flip**|Flips bits in Huffman-coded spectral data.|Crackling or “Invalid scalefactor” warnings.|AAC, AC3/EAC3, MP3, FLAC, PCM, Vorbis, Opus|
|**quantization-noise-injection**|Adds small random values to quantized coefficients.|Subtle distortion or fuzziness.|AAC, AC3/EAC3, MP3, FLAC, PCM, Vorbis, Opus|

#### AAC Specific Corruptions

|**Audio Error Type**|**Description**|**Typical Effects**|
|--------------------|---------------|-------------------|
|**adts-header-corruption**|Flips bits in ADTS headers (for AAC) but keeps syncword.|“Invalid sampling rate index”, playback mutes briefly.|
|**PCE-channel-map-tamper**|Corrupt Program Config Element for channel layout.|Channel misrouting or decode abort.|

#### AC3/EAC3 Specific Corruptions

|**Audio Error Type**|**Description**|**Typical Effects**|
|--------------------|---------------|-------------------|
|**syncframe-header-corruption**|Damage syncinfo or bitstream info fields (bitrate, sample rate).|Decoder mis-parses frame boundaries → jitter or mute.|
|**coupling-exponent-strategy-flip**|Alter coupling or exponent coding flags.|Severe distortion, banding artifacts.|

#### MP3 Specific Corruptions

|**Audio Error Type**|**Description**|**Typical Effects**|
|--------------------|---------------|-------------------|
|**frame-header-corruption-mp3**|Flip bits in 32-bit header (bitrate, mode, sample rate).|Wrong frame size → audible glitches or rapid header hunt.|
|**side-info-corruption**|Tamper bit reservoir pointers.|Metallic artifacts or short bursts of noise.|

#### FLAC Specific Corruptions

|**Audio Error Type**|**Description**|**Typical Effects**|
|--------------------|---------------|-------------------|
|**frame-header-corruption-flac**|Modify blocksize, sample rate, channel assignment.|Pops/dropouts; decoder may resync later.|
|**residual-partition-corruption**|Flip bits in residual coding partitions.|Harsh noise bursts without container failure.|

#### PCM Specific Corruptions

|**Audio Error Type**|**Description**|**Typical Effects**|
|--------------------|---------------|-------------------|
|**format-extensible-mismatch**|Corrupt channel mask or valid bits per sample.|Wrong channel routing or silence in some channels.|
|**bit-plane-fuzz**|Flip LSB/MSB in sample payload.|Audible hiss or clipping.|

#### Vorbis Specific Corruptions

|**Audio Error Type**|**Description**|**Typical Effects**|
|--------------------|---------------|-------------------|
|**identification-setup-header-corruption**|Tamper fields like audio_channels, audio_sample_rate, nominal bitrate, or blocksize in the Vorbis setup headers.|Decoder misinterprets stream parameters → decode rejection or severe mis-synthesis (wrong channel layout, timing errors).|
|**codebook-floor-residue-field-corruption**|Flip bits in packed codebooks, floor parameters, or residue configuration within the setup packet.|Entropy decode desynchronization → harsh noise bursts, tonal distortion, or frame drops while stream structure remains recoverable.|
|**mode-window-type-flag-corruption**|Alter packet type or mode/window flags so the decoder selects an incorrect transform size or mode.|Audible pre-echo, warbling artifacts, or hard decode error for that packet.|

#### Opus Specific Corruptions

|**Audio Error Type**|**Description**|**Typical Effects**|
|--------------------|---------------|-------------------|
|**TOC-byte-mutation**|Flip mode/bandwidth/frame size bits in TOC.|Wrong decode mode → distortion or frame loss.|
|**frame-packing-corruption**|Alter frame count or padding scheme.|Misaligned frames → one bad packet then recovery.|
