/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @addtogroup HPK Hardware Porting Kit
 * @{
 * @par The Hardware Porting Kit
 * HPK is the next evolution of the well-defined Hardware Abstraction Layer
 * (HAL), but augmented with more comprehensive documentation and test suites
 * that OEM or SOC vendors can use to self-certify their ports before taking
 * them to RDKM for validation or to an operator for final integration and
 * deployment. The Hardware Porting Kit effectively enables an OEM and/or SOC
 * vendor to self-certify their own Video Accelerator devices, with minimal RDKM
 * assistance.
 *
 */

/**
 * @addtogroup AUDIO_DECODER Audio Decoder Module
 * @{
 */

/**
 * @addtogroup AUDIO_DECODER_HALTEST Audio Decoder HAL Tests
 * @{
 */

/**
 * @defgroup AUDIO_DECODER_HALTEST_CLASS Audio Decoder HAL Tests helper functions Header File
 * @{
 * @parblock
 *
 * ### Common functions/macros required for the audio module :
 *
 * Common functions/macros required for the audio module .
 *
 * **Pre-Conditions:**  None @n
 * **Dependencies:** None @n
 *
 * @endparblock
 *
 */

/**
 * @file vts_audiodecoder_test.cpp
 *
 */

#include "vts_audiodecoder.h"
#include "vts_avbuffer.h"
#include "vts_audiodecoder_test.h"

#define PTS_DELTA_NS        10    /* ns — acceptable tolerance for PTS spacing between decoded frames */
#define MAX_FRAME_WAIT_TIME 500  /* ms — timeout for decoded frame output (must survive flush gaps) */
#define MAX_RETRY_COUNT 5        /* Maximum number of retries for waiting frames */

class AVBufferSpaceListener : public BnAVBufferSpaceListener
{
public:

    AVBufferSpaceListener() {};
    virtual ~AVBufferSpaceListener() = default;

    Status onSpaceAvailable() override
    {
        return Status::ok();
    }
};

AudioDecoderTest::AudioDecoderTest(IAudioDecoder::Id decoderId, sp<IAudioDecoder> decoderHandle,
    sp<AudioDecoderControllerListener> controlListener,
    sp<AudioDecoderEventListener> eventListener,
    sp<IAVBuffer> AVBufferManager, IAVBufferHelper* avbufferHelper,
    bool secureDecode, testConfiguration_t config, bool isSinkAttached)
        : mDecoderId(decoderId),
          mDecoderHandle(decoderHandle),
          mControlListener(controlListener),
          mEventListener(eventListener),
          mAVBufferManager(AVBufferManager),
          mSecureDecode(secureDecode),
          mTestConfiguration(std::move(config)),
          mAVbufferHelper(avbufferHelper),
          mIsSinkAttached(isSinkAttached)
{
    mBufferListener = new AVBufferSpaceListener();
    AVBUFFER_CREATE_AUDIOPOOL_SUCCESS(mAVBufferManager, mSecureDecode, mDecoderId, mBufferListener, mAudioPool);
}

AudioDecoderTest::~AudioDecoderTest()
{
    AVBUFFER_DESTROY_POOL_SUCCESS(mAVBufferManager, mAudioPool);
}

void AudioDecoderTest::getDecoderMetrics(int32_t &framesDecoded,
                                         int32_t &decodeErrors,
                                         int32_t &framesDropped)
{
    optional<PropertyValue> propertyValue;

    framesDecoded = decodeErrors = framesDropped = -1;

    AUDIODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, audiodecoder::Property::METRIC_FRAMES_DECODED, propertyValue);
    if (propertyValue.has_value() && propertyValue->value.has_value())
    {
        framesDecoded = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();
    }

    AUDIODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, audiodecoder::Property::METRIC_DECODE_ERRORS, propertyValue);
    if (propertyValue.has_value() && propertyValue->value.has_value())
    {
        decodeErrors = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();
    }

    AUDIODECODER_GET_PROPERTY_SUCCESS(mDecoderHandle, audiodecoder::Property::METRIC_FRAMES_DROPPED, propertyValue);
    if (propertyValue.has_value() && propertyValue->value.has_value())
    {
        framesDropped = propertyValue->value->get<PropertyValue::Value::Tag::intValue>();
    }
}

bool AudioDecoderTest::prepareInputBuffer(int32_t frameSize, int64_t& audioBufferHandle)
{
    vector<uint8_t> inputBuffer(frameSize);
    void* mapBuffer = nullptr;
    uint32_t mapSize = 0;

    AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, mAudioPool, frameSize, audioBufferHandle);

    mInputFile.read(reinterpret_cast<char*>(inputBuffer.data()), frameSize);
    if (mInputFile.eof() || mInputFile.gcount() < frameSize)
    {
        mInputFile.clear();
        mInputFile.seekg(0, std::ios::beg);
        mInputFile.read(reinterpret_cast<char*>(inputBuffer.data()), frameSize);
        if (mInputFile.gcount() < frameSize)
        {
            UT_LOG_ERROR("[Audio DecoderID: %d] Input file too small for frame (need %d, got %zd)",
                         mDecoderId.value, frameSize, (size_t)mInputFile.gcount());
            AVBUFFER_FREE_SUCCESS(mAVBufferManager, audioBufferHandle);
            return false;
        }
    }

    if (mSecureDecode)
    {
        if (!mAVbufferHelper->writeSecureHandle(audioBufferHandle, inputBuffer.data(), frameSize))
        {
            UT_LOG_ERROR("[Audio DecoderID: %d] Failed to copy the data to secure buffer", mDecoderId.value);
            AVBUFFER_FREE_SUCCESS(mAVBufferManager, audioBufferHandle);
            return false;
        }
    }
    else
    {
        mapBuffer = mAVbufferHelper->mapHandle(audioBufferHandle, &mapSize);
        if (mapBuffer && mapSize >= (uint32_t)frameSize)
        {
            memcpy(mapBuffer, inputBuffer.data(), frameSize);
            mAVbufferHelper->unmapHandle(audioBufferHandle);
        }
        else
        {
            UT_LOG_ERROR("[Audio DecoderID: %d] Failed to map buffer or insufficient size (mapBuffer:%p, mapSize:%u, required:%u)",
                         mDecoderId.value, mapBuffer, mapSize, frameSize);
            if (mapBuffer)
                mAVbufferHelper->unmapHandle(audioBufferHandle);
            AVBUFFER_FREE_SUCCESS(mAVBufferManager, audioBufferHandle);
            return false;
        }
    }
    return true;
}

void AudioDecoderTest::handleControlPoints(int64_t framePTS)
{
    if (mTestConfiguration.stopPts == framePTS)
    {
        AUDIODECODER_STOP_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
        AUDIODECODER_START_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
    }

    if (mTestConfiguration.discontinuityPts == framePTS)
    {
        AUDIODECODER_SIGNAL_DISCONTINUITY_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
    }

    if (mTestConfiguration.flushPts == framePTS)
    {
        AUDIODECODER_FLUSH_SUCCESS(mDecoderHandle, mAudioDecoderController, false, true);
        AUDIODECODER_SIGNAL_DISCONTINUITY_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
    }

    if (mTestConfiguration.flushRestPts == framePTS)
    {
        AUDIODECODER_FLUSH_SUCCESS(mDecoderHandle, mAudioDecoderController, true, true);
        AUDIODECODER_START_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
    }
}

void AudioDecoderTest::trackInputFrame(int64_t framePTS, bool errorFrame)
{
    InputAudioFrame_t inputFrame;
    inputFrame.sendTime    = chrono::steady_clock::now();
    inputFrame.framePTS    = framePTS;
    inputFrame.isErrorFrame = errorFrame;
    lock_guard<mutex> lock(mInputFrameMutex);
    mInputFrame.push_back(inputFrame);
}

audiodecoder::InputBufferMetadata AudioDecoderTest::buildMetadata(size_t frameIndex, int64_t framePTS)
{
    audiodecoder::InputBufferMetadata metadata;
    metadata.nsPresentationTime = framePTS;

    if(mTestConfiguration.trimPts == framePTS)
    {
        metadata.trimStartNs = mTestConfiguration.trimStartNs;
        metadata.trimEndNs   = mTestConfiguration.trimEndNs;
    }
    return metadata;
}

void AudioDecoderTest::submitFrame(int64_t audioBufferHandle, const audiodecoder::InputBufferMetadata& metadata, bool errorFrame)
{
    ::android::binder::Status status;
    bool decodeResult = false;

    mControlListener->resetDecodeBufferAvailable();

    trackInputFrame(metadata.nsPresentationTime, errorFrame);

    AUDIODECODER_DECODE_WITH_METADATA_SUCCESS(mDecoderId.value, mAudioDecoderController, audioBufferHandle, metadata, decodeResult);

    if (!decodeResult)
    {
        decodeResult = false;

        mDecodeQueueFullCount++;
        UT_LOG_WARNING("[Audio DecoderID: %d] decodeBufferWithMetadata returned false at PTS %lld "
                       "(queue full #%d) — waiting for onDecodeBufferAvailable before retry",
                       mDecoderId.value, metadata.nsPresentationTime, mDecodeQueueFullCount);
        UT_ASSERT_TRUE(mControlListener->waitForDecodeBufferAvailable(MAX_FRAME_WAIT_TIME));

        AUDIODECODER_DECODE_WITH_METADATA_SUCCESS(mDecoderId.value, mAudioDecoderController, audioBufferHandle, metadata, decodeResult);
        UT_ASSERT_TRUE(decodeResult);
        if (!decodeResult)
        {
            AVBUFFER_FREE_SUCCESS(mAVBufferManager, audioBufferHandle);
            return;
        }
    }
}

void AudioDecoderTest::handleMidStreamEOS(int64_t framePTS)
{
    /* Allocate a fresh handle for the post-EOS negative test. The EOS buffer was
     * already accepted (and freed) by the HAL, so reusing it would be a spec
     * violation. HAL rejects this call with EX_ILLEGAL_STATE because EOS was
     * already submitted; ownership stays with the caller, so we free it. */
    int64_t tempHandle;
    AVBUFFER_ALLOC_SUCCESS(mAVBufferManager, mAudioPool, 1, tempHandle);

    audiodecoder::InputBufferMetadata dummyMeta;
    dummyMeta.nsPresentationTime = framePTS;
    dummyMeta.trimStartNs  = 0;
    dummyMeta.trimEndNs    = 0;
    AUDIODECODER_DECODE_WITH_METADATA_FAILURE(mAudioDecoderController, tempHandle, dummyMeta, Status::EX_ILLEGAL_STATE);
    AVBUFFER_FREE_SUCCESS(mAVBufferManager, tempHandle);

    UT_LOG_INFO("[Audio DecoderID: %d] Restarting decoder after mid-stream EOS at PTS:%lld",
                mDecoderId.value, framePTS);


    mControlListener->resetEndOfStream();

    /* TODO: stop will not reset the metrics as per the spec, so any accumulated metrics before this point will persist
       If in future spec changes to reset metrics on stop(), this section may need to be updated accordingly */
    AUDIODECODER_STOP_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
    AUDIODECODER_START_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
}

void AudioDecoderTest::frameDecodeLatencyCheck(const DecodedAudioFrame_t& frame)
{
    lock_guard<mutex> lock(mInputFrameMutex);
    auto inputIt = find_if(mInputFrame.begin(), mInputFrame.end(),
        [&frame](const InputAudioFrame_t& f) {
            return f.framePTS == frame.framePTS;
        });
    if (inputIt == mInputFrame.end())
    {
        UT_LOG_ERROR("[Audio DecoderID: %d] Out Frame [PTS: %lld] does not match any submitted Input Frame PTS",
                        mDecoderId.value, frame.framePTS);
        UT_ASSERT_TRUE(false);
    }
    else
    {
        auto duration = chrono::duration_cast<chrono::microseconds>(
            frame.receiveTime - inputIt->sendTime);
        UT_LOG_INFO("[Audio DecoderID: %d] Decode Time in micro second [PTS: %lld]: %lld",
                    mDecoderId.value, frame.framePTS, duration.count());
        UT_ASSERT_TRUE(duration.count() <= mTestConfiguration.decodeTime);
    }
}

bool AudioDecoderTest::checkPassThrough(const DecodedAudioFrame_t& frame)
{
    /* SOC_PROPRIETARY (pass-through): log once, skip decoded-frame validation */
    if (frame.isMetaAvailable &&
        frame.frameType == com::rdk::hal::audiodecoder::FrameType::SOC_PROPRIETARY)
    {
        UT_LOG_WARNING("[Audio DecoderID: %d] SOC_PROPRIETARY(pass-through) output detected. "
                           "Skipping decoded-frame validation.", mDecoderId.value);
        return true;
    }
    return false;
}

void AudioDecoderTest::checkPTSSpacing(const DecodedAudioFrame_t& frame, int64_t frameBytes, FrameValidationState& state)
{
    const auto& pcm = (frame.frameMetadata && frame.frameMetadata->metadata.has_value())
                      ? frame.frameMetadata->metadata : std::optional<audiodecoder::PCMMetadata>{};
    int32_t frameSampleRate = (pcm.has_value() && pcm->sampleRate > 0)
                              ? pcm->sampleRate : state.decodedSampleRate;
    int32_t frameChannels   = (pcm.has_value() && pcm->numChannels > 0)
                              ? pcm->numChannels : state.decodedChannels;
    audiodecoder::PCMFormat frameFormat = pcm.has_value()
                              ? pcm->format : state.decodedFormat;
    int64_t frameDurationNs = vts_audio_frameDurationNs(frameBytes, frameSampleRate, frameChannels, frameFormat);

    if (state.prevPTS != -1)
    {
        int64_t delta = frame.framePTS - state.expectedNextPTS;
        UT_LOG_INFO("[Audio DecoderID: %d] PTS spacing [frame: %d, expectedPTS: %lld, actualPTS: %lld, delta: %lld]",
                    mDecoderId.value, state.framesDecoded, (long long)state.expectedNextPTS, (long long)frame.framePTS, (long long)delta);
        UT_ASSERT_TRUE(delta >= -PTS_DELTA_NS && delta <= PTS_DELTA_NS);
    }
    state.prevPTS         = frame.framePTS;
    state.expectedNextPTS = frame.framePTS + frameDurationNs;
}

void AudioDecoderTest::checkFrameMetadata(const DecodedAudioFrame_t& frame, FrameValidationState& state)
{
    const auto& fm = *frame.frameMetadata;
    UT_LOG_INFO("[Audio DecoderID: %d] Frame metadata: sourceCodec=%d (expected=%d) source=%d (expected=%d) bitstreamEOS=%s",
                mDecoderId.value,
                static_cast<int32_t>(fm.sourceCodec), static_cast<int32_t>(mTestConfiguration.codec),
                static_cast<int32_t>(fm.source),
                mAvSourcePropertyValue.value->get<PropertyValue::Value::Tag::intValue>(),
                fm.bitstreamEOS ? "true" : "false");

    if (frame.framePTS == -1LL && frame.frameBufferHandle == (int64_t)0xFFFFFFFFFFFFFFFF)
    {
        return;
    }

    UT_ASSERT_EQUAL(fm.sourceCodec, mTestConfiguration.codec);
    UT_ASSERT_EQUAL(int32_t(static_cast<int32_t>(fm.source)),
                    mAvSourcePropertyValue.value->get<PropertyValue::Value::Tag::intValue>());

    if (fm.discontinuity)
    {
        state.discontinuityFramesReceived++;
    }

    if (mTestConfiguration.trimStartNs > 0 || mTestConfiguration.trimEndNs > 0)
    {
        if(fm.trimStartNs == 0 && fm.trimEndNs == 0)
        {
            mTrimAppliedbyDecoder = true;
        }
    }

    if (state.decodedSampleRate == 0 && fm.metadata.has_value() && fm.metadata->sampleRate > 0)
    {
        state.decodedSampleRate = fm.metadata->sampleRate;
        state.decodedChannels   = fm.metadata->numChannels;
        state.decodedFormat     = fm.metadata->format;
        UT_LOG_INFO("[Audio DecoderID: %d] PCM metadata: sampleRate=%d channels=%d format=%s",
                    mDecoderId.value, state.decodedSampleRate, state.decodedChannels,
                    audiodecoder::toString(state.decodedFormat).c_str());
    }
}

void AudioDecoderTest::validatePostLoopMetrics(const FrameValidationState& state)
{
    UT_LOG_INFO("[Audio DecoderID: %d] duration=%lld decodedSampleRate=%d decodedChannels=%d",
                    mDecoderId.value, (long long)mTestConfiguration.duration, state.decodedSampleRate, state.decodedChannels);

    if (mTestConfiguration.duration > 0 && state.decodedSampleRate > 0 &&
        state.decodedChannels > 0 && mOperationMode == OperationalMode::NON_TUNNELLED)
    {
        int64_t expectedSamples     = vts_audio_expectedSamples(mTestConfiguration.duration, state.decodedSampleRate);
        int64_t totalDecodedSamples = vts_audio_totalSamples(state.accumulatedSize, state.decodedChannels, state.decodedFormat);
        int64_t trimRemovedSamples = 0;

        if (mTrimAppliedbyDecoder)
        {
            trimRemovedSamples = vts_audio_expectedSamples(mTestConfiguration.trimRemoved, state.decodedSampleRate);
            expectedSamples -= trimRemovedSamples;
        }

        UT_LOG_INFO("[Audio DecoderID: %d] Resampling validation: totalDecodedSamples=%lld expectedSamples=%lld (-trimed: %lld) decodeErrors=%d",
                    mDecoderId.value, (long long)totalDecodedSamples, (long long)expectedSamples, (long long)trimRemovedSamples, state.errorFramesPushed);

        if (state.errorFramesPushed > 0)
        {
            UT_ASSERT_TRUE(totalDecodedSamples <= expectedSamples);
        }
        else
        {
            if(mTrimAppliedbyDecoder)
            {
                // Allow a margin of 10 samples for trimming inaccuracies
                UT_ASSERT_FALSE((totalDecodedSamples <= expectedSamples - 10) || (totalDecodedSamples >= expectedSamples + 10));
            }
            else
            {
                UT_ASSERT_EQUAL(totalDecodedSamples, expectedSamples);
            }
        }
    }

    int32_t totalReceivedErrors = mEventListener->getErrorCount();

    /* expectedFrames: passthrough counts all frames; non-passthrough subtracts error frames */
    int32_t expectedFrames = state.isPassThrough ? mTestConfiguration.numOfFrames
                                                 : mTestConfiguration.numOfFrames - state.errorFramesPushed;

    /* Frame count: METRIC_FRAMES_DECODED compared against yaml num_of_frames is the authoritative
     * check. Applies to all codec types including SOC_PROPRIETARY pass-through.
     * When flush is configured use relaxed (<=) checks.
     * Per Property.aidl, these metrics reset only on open()/flush(), not stop()/start(), so a
     * mid-stream EOS restart (handleMidStreamEOS() uses stop()+start()) keeps counting cumulatively. */
    int32_t metricsFrameDecoded, metricsDecodeErrors, metricsFramesDropped;

    getDecoderMetrics(metricsFrameDecoded, metricsDecodeErrors, metricsFramesDropped);

    UT_LOG_DEBUG("Final Decoder Metrics: Codec: %d, Frames Decoded=%d, Decode Errors=%d, Frames Dropped=%d",
        mTestConfiguration.codec, metricsFrameDecoded, metricsDecodeErrors, metricsFramesDropped);

    int32_t metricsExpectedDecoded = expectedFrames;
    int32_t metricsExpectedErrors  = state.errorFramesPushed;

    if (metricsFrameDecoded != -1)
    {
        //TODO: in future if stop resets the metrics stoppts, eosPts should be validated accordingly
        if ((mTestConfiguration.flushPts == -1) && (mTestConfiguration.flushRestPts == -1))
        {
            UT_ASSERT_EQUAL(metricsFrameDecoded, metricsExpectedDecoded);
        }
        else
        {
            UT_ASSERT_TRUE(metricsFrameDecoded <= metricsExpectedDecoded);
        }
    }


    /* Discontinuity/flush validation: at least one but not all frames should carry
     * the discontinuity flag when a discontinuity or flush point is configured */
    if ((mTestConfiguration.discontinuityPts != -1) || (mTestConfiguration.flushPts != -1))
    {
        UT_ASSERT_TRUE((state.discontinuityFramesReceived > 0 && state.discontinuityFramesReceived < expectedFrames));
        //TODO: why is state.discontinuityFramesReceived < expectedFrames, it shoould be ideally equal to 1
    }

    /* SOC_PROPRIETARY pass-through: no further checks needed */
    if (state.isPassThrough)
    {
        return;
    }

    /* Error count validation */
    UT_ASSERT_EQUAL(totalReceivedErrors, state.errorFramesPushed);

    if ((mTestConfiguration.flushPts == -1) && (mTestConfiguration.flushRestPts == -1))
    {
        if (metricsDecodeErrors != -1)
        {
            UT_ASSERT_EQUAL(metricsDecodeErrors, metricsExpectedErrors);
        }
        if (metricsFramesDropped != -1)
        {
            UT_ASSERT_EQUAL(metricsFramesDropped, 0);
        }
    }
    else
    {
        if (metricsDecodeErrors != -1)
        {
            UT_ASSERT_TRUE(metricsDecodeErrors <= metricsExpectedErrors);
        }
        if (metricsFramesDropped != -1)
        {
            UT_ASSERT_TRUE(metricsFramesDropped >= 0);
        }
    }
}

void AudioDecoderTest::setupAudioDecoder()
{
    mAvSourcePropertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(int32_t(static_cast<int32_t>(AVSource::IP)));

    UT_LOG_DEBUG("[Audio DecoderID: %d] Start of Testing Stream: %s", mDecoderId.value, mTestConfiguration.input.c_str());

    mInputFile.open(mTestConfiguration.input, ios::binary);
    if (!mInputFile)
    {
        UT_LOG_ERROR("[Audio DecoderID: %d] Failed to open file: %s", mDecoderId.value, mTestConfiguration.input.c_str());
        UT_FAIL("Failed to open input file");
        mDecoderSetup = false;
        return;
    }

    /* Open the Decoder */
    AUDIODECODER_OPEN_SUCCESS(mDecoderHandle, mTestConfiguration.codec, mSecureDecode, mControlListener, mAudioDecoderController, true);

    /* Set AV Source */
    AUDIODECODER_SETPROPERTY_SUCCESS(mAudioDecoderController, audiodecoder::Property::AV_SOURCE, mAvSourcePropertyValue);

    /* Set audio format */
    AUDIODECODER_SETAUDIOFORMAT_SUCCESS(mAudioDecoderController,
                                        mTestConfiguration.numChannels,
                                        mTestConfiguration.sampleRate);

    /* Start the decoder */
    AUDIODECODER_START_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
    mDecoderSetup       = true;

    /* Reset shared input frame queue */
    {
        lock_guard<mutex> lock(mInputFrameMutex);
        mInputFrame.clear();
    }

    mOperationMode = OperationalMode::NON_TUNNELLED;
    AUDIODECODER_GET_OPERATIONALMODE_SUCCESS(mDecoderHandle, mOperationMode);

    return;
}

void AudioDecoderTest::teardownDecoder()
{
    /* Stop the decoder */
    AUDIODECODER_STOP_SUCCESS(mDecoderHandle, mAudioDecoderController, true);

    /* Close the decoder */
    AUDIODECODER_CLOSE_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
}

void AudioDecoderTest::cleanupAudioDecoder()
{
    DecodedAudioFrame_t decodedFrame;
    while (mControlListener->getNextFrame(decodedFrame, 2000, true))
    {
        if(decodedFrame.frameBufferHandle != -1)
        {
            bool isBufferValid;

            UT_LOG_ERROR("[Audio DecoderID: %d] Frames received after the audio decoder has been stopped", mDecoderId.value);
            UT_ASSERT_TRUE(false);

            AVBUFFER_ISVALID_SUCCESS(mAVBufferManager, decodedFrame.frameBufferHandle, isBufferValid);

            if (isBufferValid)
            {
                AVBUFFER_FREE_SUCCESS(mAVBufferManager, decodedFrame.frameBufferHandle);
            }
        }
    }
}

void AudioDecoderTest::startDecoding()
{
    const auto& frames = mTestConfiguration.frameDetails;

    for (size_t frameIndex = 0; frameIndex < frames.size(); ++frameIndex)
    {
        const auto& [frameSize, framePTS, errorFrame] = frames[frameIndex];
        int64_t audioBufferHandle;

        if (!prepareInputBuffer(frameSize, audioBufferHandle))
            continue;

        handleControlPoints(framePTS);

        bool isEosPtsFrame = (mTestConfiguration.eosPts != -1 && mTestConfiguration.eosPts == framePTS);
        auto metadata = buildMetadata(frameIndex, framePTS);

        submitFrame(audioBufferHandle, metadata, errorFrame);

        if (isEosPtsFrame)
        {
            mControlListener->resetEndOfStream();
            AUDIODECODER_SIGNAL_EOS_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
            UT_ASSERT_TRUE(mControlListener->waitForEndOfStream(MAX_FRAME_WAIT_TIME));
        }

        if (mTestConfiguration.decoderDelay > 0)
            this_thread::sleep_for(chrono::milliseconds(mTestConfiguration.decoderDelay));

        if (isEosPtsFrame)
            handleMidStreamEOS(framePTS);
    }

    mControlListener->resetEndOfStream();
    AUDIODECODER_SIGNAL_EOS_SUCCESS(mDecoderHandle, mAudioDecoderController, true);
    UT_ASSERT_TRUE(mControlListener->waitForEndOfStream(MAX_FRAME_WAIT_TIME));

    UT_LOG_INFO("[Audio DecoderID: %d] End of Testing Stream: %s",
                mDecoderId.value, mTestConfiguration.input.c_str());
}

void AudioDecoderTest::validateDecodedFrame()
{
    DecodedAudioFrame_t decodedFrame;
    int32_t retryCount = 0;
    FrameValidationState state;

    state.errorFramesPushed = static_cast<int32_t>(count_if(
        mTestConfiguration.frameDetails.begin(),
        mTestConfiguration.frameDetails.end(),
        [](const auto& f) { return get<2>(f); }));

    int32_t samplesPerFrame = vts_audioDecoder_helper_getSamplesPerFrame(mTestConfiguration.codec);
    state.validatePTS = (samplesPerFrame > 0 && state.errorFramesPushed == 0 &&
                         mTestConfiguration.flushPts == -1 &&
                         mTestConfiguration.flushRestPts == -1);
    state.isPassThrough = false;

    while (true)
    {
        if(!mControlListener->getNextFrame(decodedFrame, MAX_FRAME_WAIT_TIME, true))
        {
            /* If frame not received within the wait time, check if EOS has been reached */
            if (mControlListener->waitForEndOfStream(MAX_FRAME_WAIT_TIME))
            {
                /* EOS has been reached, no more frames expected */
                break;
            }

            /* EOS has not been reached, continue waiting for frames */
            retryCount++;
            UT_LOG_ERROR("[Audio DecoderID: %d] decoded frame not received within %d ms", mDecoderId.value, MAX_FRAME_WAIT_TIME);
            UT_ASSERT_TRUE(false);

            if(retryCount < MAX_RETRY_COUNT)
            {
                continue;
            }
            else
            {
                UT_LOG_ERROR("[Audio DecoderID: %d] Max retry count reached, exiting", mDecoderId.value);
                break;
            }
        }

        /* Reset retry count for the next frame */
        retryCount = 0;

        /* check frame decode latency for the current decoded frame */
        frameDecodeLatencyCheck(decodedFrame);

        /* validate buffer only if it is valid */
        if (decodedFrame.frameBufferHandle != -1)
        {
            state.framesDecoded++;

            /* additonal validation for non-tunnelled mode,
             ensure operation mode is NON_TUNNELLED when buffer is valid */
            UT_ASSERT_EQUAL(mOperationMode, OperationalMode::NON_TUNNELLED);

            int64_t frameBytes = mAVbufferHelper->getAllocationSize(decodedFrame.frameBufferHandle);

            if (mTestConfiguration.duration > 0)
            {
                state.accumulatedSize += frameBytes;
            }

            state.isPassThrough = checkPassThrough(decodedFrame);
            if (!state.isPassThrough && state.validatePTS)
            {
                checkPTSSpacing(decodedFrame, frameBytes, state);
            }

            /* Free decoded output buffer — both SOC_PROPRIETARY and normal path */
            bool isBufferValid;
            AVBUFFER_ISVALID_SUCCESS(mAVBufferManager, decodedFrame.frameBufferHandle, isBufferValid);

            if (isBufferValid)
            {
                AVBUFFER_FREE_SUCCESS(mAVBufferManager, decodedFrame.frameBufferHandle);
            }
        }
        else
        {
            UT_LOG_INFO("[Audio DecoderID: %d] Received invalid frame buffer handle", mDecoderId.value);
            /* additonal validation for tunnelled mode,
             ensure operation mode is TUNNELLED when buffer is invalid */
            UT_ASSERT_EQUAL(mOperationMode, OperationalMode::TUNNELLED);
        }

        /* metadata — separate from buffer handling */
        if (decodedFrame.isMetaAvailable)
        {
            checkFrameMetadata(decodedFrame, state);
        }
    }
    validatePostLoopMetrics(state);

    cleanupAudioDecoder();
}

void AudioDecoderTest::run(bool validateMetrics)
{
    setupAudioDecoder();
    if (!mDecoderSetup)
    {
        /* Decoder was never opened/started — nothing valid to decode or tear
         * down. UT_FAIL() in setupAudioDecoder() already recorded the
         * failure. */
        return;
    }

    thread validateThread;

    if (validateMetrics && !mIsSinkAttached)
        validateThread = thread(&AudioDecoderTest::validateDecodedFrame, this);

    startDecoding();

    if (validateThread.joinable())
        validateThread.join();

    /* onDecodeBufferAvailable and queue-full compliance check (all tests).
     *
     * Per spec (IAudioDecoderControllerListener): "Not fired in steady-state
     * operation — only after a refused buffer."  Both counts must be zero for
     * a compliant HAL.  A non-zero count means decodeBufferWithMetadata() returned
     * false during decoding, indicating the HAL's internal input queue filled up —
     * the decoder is too slow relative to the submission rate.
     */
    int32_t decodeBufferAvailCount = mControlListener->getDecodeBufferAvailableCount();

    if (mDecodeQueueFullCount > 0 || decodeBufferAvailCount > 0)
    {
        UT_LOG_ERROR("[Audio DecoderID: %d] HAL input queue filled during decode: "
                     "decodeBufferWithMetadata returned false %d time(s), "
                     "onDecodeBufferAvailable fired %d time(s) — "
                     "decoder is behaving slow relative to submission rate",
                     mDecoderId.value, mDecodeQueueFullCount, decodeBufferAvailCount);
    }

    UT_ASSERT_EQUAL(mDecodeQueueFullCount, 0);
    UT_ASSERT_EQUAL(decodeBufferAvailCount, 0);

    /* Reset decodebufferavailable events */
    mControlListener->resetDecodeBufferAvailable();
    mControlListener->resetEndOfStream();

    teardownDecoder();
}

/** @} */ // End of AUDIO_DECODER_HALTEST_HEADER
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER
/** @} */ // End of HPK