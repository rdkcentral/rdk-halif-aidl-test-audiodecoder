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
 * @defgroup AUDIO_DECODER_HALTEST_HEADER Audio Decoder HAL Tests helper functions Header File
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
 * @file vts_audiodecoder_test.h
 *
 */

#ifndef __VTS_AUDIODECODER_TEST_H__
#define __VTS_AUDIODECODER_TEST_H__

#include <stdio.h>
#include <vector>
#include <optional>
#include <algorithm>
#include <unordered_set>
#include <map>
#include <thread>
#include <chrono>
#include <deque>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <tuple>
#include <fstream>
#include <cmath>
#include <inttypes.h>

#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include <binder/Status.h>
#include <utils/String16.h>

#include <ut.h>
#include <ut_log.h>
#include <ut_kvp_profile.h>
#include <ut_control_plane.h>

#include "com/rdk/hal/OperationalMode.h"
#include "com/rdk/hal/audiodecoder/IAudioDecoderManager.h"
#include "com/rdk/hal/audiodecoder/BnAudioDecoderControllerListener.h"
#include "com/rdk/hal/audiodecoder/BnAudioDecoderEventListener.h"
#include "com/rdk/hal/audiodecoder/InputBufferMetadata.h"

#include "com/rdk/hal/avbuffer/IAVBuffer.h"
#include "com/rdk/hal/avbuffer/BnAVBufferSpaceListener.h"
#include "avbufferhelper.h"

#include "vts_audiodecoder_helper_functions.h"
#include "vts_audiodecoder_parse_hfp.h"
#include "vts_audiodecoder_parse_testconfig.h"
#include "vts_helper_functions.h"

using android::sp;
using android::String16;
using android::defaultServiceManager;
using android::binder::Status;
using namespace std;
using namespace std::chrono;

using namespace com::rdk::hal;
using namespace com::rdk::hal::audiodecoder;
using namespace com::rdk::hal::avbuffer;

// Type alias for cleaner code
using AudioDecoderState = ::com::rdk::hal::audiodecoder::State;

class AudioDecoderTest
{
public:
    AudioDecoderTest(IAudioDecoder::Id decoderId, sp<IAudioDecoder> decoderHandle,
                sp<AudioDecoderControllerListener> controlListener,
                sp<AudioDecoderEventListener> eventListener,
                sp<IAVBuffer> AVBufferManager, IAVBufferHelper* avbufferHelper,
                bool secureDecode, testConfiguration_t config, bool isSinkAttached);

    ~AudioDecoderTest();

    void run(bool validateMetrics);
    void runForOutputMode(bool validateMetrics);

    void getDecoderMetrics(int32_t &framesDecoded, int32_t &decodeErrors, int32_t &framesDropped);

private:
    IAudioDecoder::Id mDecoderId;
    sp<IAudioDecoder> mDecoderHandle;
    sp<AudioDecoderControllerListener> mControlListener;
    sp<AudioDecoderEventListener> mEventListener;
    sp<IAVBuffer> mAVBufferManager;
    bool mSecureDecode;
    testConfiguration_t mTestConfiguration;
    deque<InputAudioFrame_t> mInputFrame;
    IAVBufferHelper* mAVbufferHelper;
    bool mIsSinkAttached;
    Pool mAudioPool;
    sp<IAVBufferSpaceListener> mBufferListener;
    mutex mInputFrameMutex;
    sp<IAudioDecoderController> mAudioDecoderController;
    PropertyValue mAvSourcePropertyValue;
    ifstream mInputFile;
    int32_t mDecodeQueueFullCount = 0;
    bool    mDecoderSetup = false;
    OperationalMode mOperationMode;
    bool mTrimAppliedbyDecoder = false;

    struct FrameValidationState
    {
        int32_t  framesDecoded               = 0;
        int32_t  eosFramesReceived           = 0;
        int32_t  discontinuityFramesReceived = 0;
        bool     isPassThrough               = false;
        uint64_t accumulatedSize             = 0;
        int64_t  prevPTS                     = -1;
        int64_t  expectedNextPTS             = -1;
        int32_t  decodedSampleRate           = 0;
        int32_t  decodedChannels             = 0;
        audiodecoder::PCMFormat decodedFormat{};
        int32_t  errorFramesPushed           = 0;
        bool     validatePTS                 = false;
    };

    void setupAudioDecoder();
    void startDecoding();
    void teardownDecoder();
    void validateDecodedFrame();

    bool prepareInputBuffer(int32_t frameSize, int64_t& audioBufferHandle);
    void handleControlPoints(int64_t framePTS);
    void trackInputFrame(int64_t framePTS, bool errorFrame);
    audiodecoder::InputBufferMetadata buildMetadata(size_t frameIndex, int64_t framePTS);
    void submitFrame(int64_t audioBufferHandle, const audiodecoder::InputBufferMetadata& metadata, bool errorFrame);
    void handleMidStreamEOS(int64_t framePTS);

    bool checkPassThrough   (const DecodedAudioFrame_t& frame);
    void frameDecodeLatencyCheck(const DecodedAudioFrame_t& frame);
    void checkPTSSpacing    (const DecodedAudioFrame_t& frame, int64_t frameBytes, FrameValidationState& state);
    void checkFrameMetadata (const DecodedAudioFrame_t& frame, FrameValidationState& state);
    void validatePostLoopMetrics(const FrameValidationState& state);
};

#endif //__VTS_AUDIODECODER_TEST_H__

/** @} */ // End of AUDIO_DECODER_HALTEST_HEADER
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER
/** @} */ // End of HPK