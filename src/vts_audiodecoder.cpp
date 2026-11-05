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
 * @file vts_audiodecoder.cpp
 *
 */

#include "vts_audiodecoder.h"
#include "vts_avbuffer.h"

#define MAX_WAIT_TIME       200   /* ms — timeout for error event callbacks */

/* Controller listener Class Functions */

/**
 * Convert vector to comma-separated string
 *
 * @param channels Channel information vector
 * @return comma-separated string
 */
string AudioDecoderControllerListener::formatChannelVector(const std::vector<ChannelType>& channels)
{
    std::ostringstream oss;
    for (size_t i = 0; i < channels.size(); ++i) {
        oss << audiodecoder::toString(channels[i]);
        if (i != channels.size() - 1) oss << ", ";
    }
    return oss.str();
}

/**
 * Override Call-back funtion for decoder output
 *
 * @param nsPresentationTime PTS in nanoseconds
 * @param frameAVBufferHandle private decoded buffer handle
 * @param metadata stream metadata
 * @return Status
 */
Status AudioDecoderControllerListener::onFrameOutput(int64_t nsPresentationTime, int64_t frameAVBufferHandle, const ::optional<audiodecoder::FrameMetadata>& metadata)
{
    UT_LOG_INFO("[Audio DecoderID: %d] onFrameOutput: Received frameoutput [PTS:%lld, handle: %llX, metaAvail: %s, bitstreamEOS: %s]",
                mDecoderID, nsPresentationTime, frameAVBufferHandle,
                metadata.has_value() ? "true" : "false",
                metadata.has_value() ? UT_Control_GetMapString(bool_mapTable, metadata->bitstreamEOS) : "N/A");

    DecodedAudioFrame_t frame;
    frame.receiveTime = chrono::steady_clock::now();
    frame.framePTS = nsPresentationTime;

    frame.frameBufferHandle = frameAVBufferHandle;
    frame.isMetaAvailable = false;
    if (metadata.has_value())
    {
        if (metadata->metadata.has_value())
        {
            const auto& pcm = metadata->metadata.value();

            UT_LOG_INFO("[Audio DecoderID: %d] Stream Details [Codec: %s, SampleRate: %d, Channels: %d, channelType: [%s] format: %s planarFormat: %s, isDolbyAtmos: %s]",
                        mDecoderID,
                        audiodecoder::toString(metadata->sourceCodec).c_str(),
                        pcm.sampleRate,
                        pcm.numChannels,
                        formatChannelVector(pcm.channelTypes).c_str(),
                        ::com::rdk::hal::audiodecoder::toString(pcm.format).c_str(),
                        UT_Control_GetMapString(bool_mapTable, pcm.planarFormat),
                        UT_Control_GetMapString(bool_mapTable, metadata->isDolbyAtmos));
        }
        else
        {
            UT_LOG_INFO("[Audio DecoderID: %d] Stream Details [Codec: %s, PCMMetadata: not available, isDolbyAtmos: %s]",
                        mDecoderID,
                        audiodecoder::toString(metadata->sourceCodec).c_str(),
                        UT_Control_GetMapString(bool_mapTable, metadata->isDolbyAtmos));
        }

        UT_LOG_INFO("[Audio DecoderID: %d] Control Parameters [PTS: %lld, discontinuity: %d, bitstreamEOS: %d, LowLatency: %d, Source: %d, Trim(%lld-%lld)]",
                    mDecoderID, nsPresentationTime, metadata->discontinuity, metadata->bitstreamEOS, metadata->lowLatency,
                    metadata->source, metadata->trimStartNs, metadata->trimEndNs);

        frame.frameMetadata = std::make_shared<audiodecoder::FrameMetadata>(*metadata);
        frame.isMetaAvailable = true;

        frame.frameType = metadata->type;

        if (metadata->type == com::rdk::hal::audiodecoder::FrameType::SOC_PROPRIETARY)
        {
            UT_LOG_INFO("[Audio DecoderID: %d] FrameType SOC_PROPRIETARY: pass-through output; decoded PCM may not be produced.",
                        mDecoderID);
            frame.socPrivate = metadata->SoCPrivate;
        }
    }
    {
        lock_guard<mutex> lock(mQueueMutex);
        mFrameQueue.push(frame);
    }

    mFrameAvailable.notify_one();

    return ::android::binder::Status::ok();
}

/**
 * Returns the queued frames from onFrameOutput
 *
 * @param frameInfo frame information
 * @param popFrame if true function reads and pop the frame from queue
 *                 otherwise function just reads the frame info
 * @return True: if frame reads successfull, False: if frame is not available
 */
bool AudioDecoderControllerListener::getNextFrame(DecodedAudioFrame_t &frameInfo, int32_t waitTime, bool popFrame)
{
    std::unique_lock<std::mutex> lock(mQueueMutex);

    // Wait until queue is not empty or timeout occurs
    bool gotFrame = mFrameAvailable.wait_for(
        lock,
        std::chrono::milliseconds(waitTime),
        [this]() { return !mFrameQueue.empty(); }
    );

    if (!gotFrame)
    {
        UT_LOG_WARNING("[Audio DecoderID: %d] Decoded frame not received after %d msec", mDecoderID, waitTime);
        return false;
    }

    frameInfo = mFrameQueue.front();
    if (popFrame) {
        mFrameQueue.pop();
    }

    return true;
}

/**
 * Clears the frame queue
 *
 * @param None.
 * @return None
 */
void AudioDecoderControllerListener::resetFrames()
{
    lock_guard<mutex> lock(mQueueMutex);
    while (!mFrameQueue.empty())
    {
        mFrameQueue.pop();
    }
}

/**
 * Clears flags and notifies all mutexes
 *
 * @param None
 * @return None
 */
void AudioDecoderControllerListener::shutdown()
{
    mShutdownFlag = true;
    mFrameAvailable.notify_all();
    mDecodeBufferCV.notify_all();
    mEndOfStreamCV.notify_all();
}

/**
 * Sets the Decoder ID
 *
 * @param id: Decoder ID
 * @return None
 */
void AudioDecoderControllerListener::setDecoderID(int32_t id)
{
    mDecoderID = id;
}

/**
 * onDecodeBufferAvailable — fires once when input queue transitions from full to available.
 * Sets mDecodeBufferAvailableFlag and unblocks any thread waiting in waitForDecodeBufferAvailable().
 *
 * @return Status
 */
Status AudioDecoderControllerListener::onDecodeBufferAvailable()
{
    UT_LOG_INFO("[Audio DecoderID: %d] onDecodeBufferAvailable: input queue has space", mDecoderID);
    mDecodeBufferAvailableCount++;
    {
        lock_guard<mutex> lock(mDecodeBufferMutex);
        mDecodeBufferAvailableFlag = true;
    }
    mDecodeBufferCV.notify_one();
    return Status::ok();
}

Status AudioDecoderControllerListener::onEndOfStream()
{
    UT_LOG_INFO("[Audio DecoderID: %d] onEndOfStream callback received", mDecoderID);
    mEndOfStreamCount++;
    {
        lock_guard<mutex> lock(mEndOfStreamMutex);
        mEndOfStreamFlag = true;
    }
    mEndOfStreamCV.notify_one();
    mFrameAvailable.notify_all();
    return Status::ok();
}

/**
 * Wait for back-pressure relief. Returns true only if onDecodeBufferAvailable() fired within
 * waitTimeMs. Returns false on timeout or shutdown.
 *
 * @param waitTimeMs maximum time to wait in milliseconds
 * @return true if callback received, false on timeout or shutdown
 */
bool AudioDecoderControllerListener::waitForDecodeBufferAvailable(int32_t waitTimeMs)
{
    unique_lock<mutex> lock(mDecodeBufferMutex);
    mDecodeBufferCV.wait_for(
        lock,
        chrono::milliseconds(waitTimeMs),
        [this]() { return mDecodeBufferAvailableFlag.load() || mShutdownFlag.load(); }
    );
    return mDecodeBufferAvailableFlag.load();
}

/**
 * Reset the decode-buffer-available flag before a decode call that may block.
 *
 * @return None
 */
void AudioDecoderControllerListener::resetDecodeBufferAvailable()
{
    lock_guard<mutex> lock(mDecodeBufferMutex);
    mDecodeBufferAvailableFlag = false;
}

bool AudioDecoderControllerListener::waitForEndOfStream(int32_t waitTimeMs)
{
    unique_lock<mutex> lock(mEndOfStreamMutex);
    mEndOfStreamCV.wait_for(
        lock,
        chrono::milliseconds(waitTimeMs),
        [this]() { return mEndOfStreamFlag.load() || mShutdownFlag.load(); }
    );
    return mEndOfStreamFlag.load();
}

void AudioDecoderControllerListener::resetEndOfStream()
{
    lock_guard<mutex> lock(mEndOfStreamMutex);
    mEndOfStreamFlag = false;
}

/* Event listener Class Functions */

/**
 * Override Call-back funtion for decoder errors
 *
 * @param errorCode General error code
 * @param vendorErrorCode vendor error code
 * @return Status
 */
Status AudioDecoderEventListener::onDecodeError(audiodecoder::ErrorCode errorCode, int32_t vendorErrorCode)
{
    ErrorDetails_t errorDetails;
    errorDetails.receiveTime = chrono::steady_clock::now();

    UT_LOG_INFO("[Audio DecoderID: %d] Error Callback: Error Code: %s, Vendor Error Code: %d", mDecoderID, audiodecoder::toString(errorCode).c_str(), vendorErrorCode);

    errorDetails.errorCode = vendorErrorCode;
    errorDetails.vendorErrorCode = errorCode;

    {
        lock_guard<mutex> lock(mErrorMutex);
        mErrors.push(errorDetails);
    }
    mErrorAvailable.notify_one();
    return Status::ok();
}

/**
 * Override Call-back funtion for decoder state changes
 *
 * @param oldState Old state
 * @param newState New state
 * @return Status
 */
Status AudioDecoderEventListener::onStateChanged(AudioDecoderState oldState,AudioDecoderState newState)
{
    UT_LOG_INFO("[Audio DecoderID: %d] State Change received: %s -> %s", mDecoderID, audiodecoder::toString(oldState).c_str(), audiodecoder::toString(newState).c_str());

    {
        lock_guard<mutex> lock(mMutex);
        mStateChanges.emplace_back(oldState, newState);
        mCurrentState = newState;
    }
    mCV.notify_all(); // Unblock any waiting test code
    return Status::ok();
}

/**
 * Returns the queued eror info from onDecodeError
 *
 * @param errorDetails error information
 * @return True: if frame reads successfull, False: if frame is not available
 */
bool AudioDecoderEventListener::getNextError(ErrorDetails_t &errorDetails)
{
    std::unique_lock<std::mutex> lock(mErrorMutex);

    bool gotError = mErrorAvailable.wait_for(
        lock,
        std::chrono::milliseconds(MAX_WAIT_TIME),
        [this]() { return !mErrors.empty(); }
    );

    if (!gotError) {
        UT_LOG_ERROR("Error event not received after %d msec", MAX_WAIT_TIME);
        return false;
    }

    errorDetails = mErrors.front();
    mErrors.pop();

    return true;
}

/**
 * Wait for the decoder to reach a particular State within a given timeout.
 *
 * @param desiredState The state we want to observe.
 * @param timeout      How long to wait, e.g. chrono::milliseconds(500).
 * @return true if the state was reached; false if we timed out waiting.
 */
bool AudioDecoderEventListener::waitForState(AudioDecoderState desiredState, chrono::milliseconds timeout)
{
    unique_lock<mutex> lock(mMutex);
    // Wait until mCurrentState == desiredState or until timeout
    return mCV.wait_for(lock, timeout, [this, desiredState]() {
        return (mCurrentState == desiredState);
    });
}

/**
 * Clears the state queue
 *
 * @param None.
 * @return None
 */
void AudioDecoderEventListener::resetStateChanges()
{
    lock_guard<mutex> lock(mMutex);
    mStateChanges.clear();
}

/**
 * Checks if the expected states received from the call-back
 *
 * @param expOldState Expected old state
 * @param expNewState Expected new state
 * @return returns if the expected states received or not
 */

bool AudioDecoderEventListener::checkStateTransistion(AudioDecoderState expOldState, AudioDecoderState expNewState)
{
    vector<pair<AudioDecoderState, AudioDecoderState>> stateChange;
    bool foundClosedToOpening = false;

    {
        lock_guard<mutex> lock(mMutex);
        stateChange = mStateChanges;
    }

    for (const auto &[oldState, newState] : stateChange)
    {
        if (oldState == expOldState && newState == expNewState)
        {
            foundClosedToOpening = true;
            break;
        }
    }

    return foundClosedToOpening;
}

void AudioDecoderEventListener::resetErrors()
{
    lock_guard<mutex> lock(mErrorMutex);
    while (!mErrors.empty())
    {
        mErrors.pop();
    }
}

void resetDecodeSession(sp<AudioDecoderEventListener> eventListener,
                        sp<AudioDecoderControllerListener> controlListener)
{
    eventListener->resetErrors();
    controlListener->resetFrames();
    controlListener->resetDecodeBufferAvailable();
    controlListener->resetDecodeBufferAvailableCount();
    controlListener->resetEndOfStream();
    controlListener->resetEndOfStreamCount();
}

/**
 * Sets the Decoder ID
 *
 * @param id: Decoder ID
 * @return None
 */
void AudioDecoderEventListener::setDecoderID(int32_t id)
{
    mDecoderID = id;
}

/** @} */ // End of AUDIO_DECODER_HALTEST_HEADER
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER
/** @} */ // End of HPK
