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
 * @file vts_audiodecoder.h
 *
 */

#ifndef __VTS_AUDIODECODER_H__
#define __VTS_AUDIODECODER_H__

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

extern void *gAudioDecoderTestConfiguration;
extern sp<IAudioDecoderManager> gAudioDecoderManager;

#ifdef MAX_TRANSITORY_STATE_WAIT_MS
#undef MAX_TRANSITORY_STATE_WAIT_MS
#endif
#define MAX_TRANSITORY_STATE_WAIT_MS 10
#define MAX_FINAL_STATE_WAIT_MS  500

/*#ifndef ENABLE_UT_INFO_LOGGING
    // Only WARNING and ERROR logs are enabled; DEBUG and INFO are disabled
    #ifdef UT_LOG_DEBUG
        #undef UT_LOG_DEBUG
    #endif
    #define UT_LOG_DEBUG(...)
    #ifdef UT_LOG_INFO
        #undef UT_LOG_INFO
    #endif
    #define UT_LOG_INFO(...)
#else //ENABLE_UT_INFO_LOGGING
    // ENABLE_UT_INFO_LOGGING is defined: use UT_LOG_LEVEL to control log macros
    #define UT_LOG_LEVEL_NONE    0
    #define UT_LOG_LEVEL_ERROR   1
    #define UT_LOG_LEVEL_WARNING 2
    #define UT_LOG_LEVEL_INFO    3
    #define UT_LOG_LEVEL_DEBUG   4

    #ifndef UT_LOG_LEVEL
        #define UT_LOG_LEVEL UT_LOG_LEVEL_INFO
    #endif

    // DEBUG
    #if UT_LOG_LEVEL < UT_LOG_LEVEL_DEBUG
        #ifdef UT_LOG_DEBUG
            #undef UT_LOG_DEBUG
        #endif
        #define UT_LOG_DEBUG(...)
    #endif
    // INFO
    #if UT_LOG_LEVEL < UT_LOG_LEVEL_INFO
        #ifdef UT_LOG_INFO
            #undef UT_LOG_INFO
        #endif
        #define UT_LOG_INFO(...)
    #endif
    // WARNING
    #if UT_LOG_LEVEL < UT_LOG_LEVEL_WARNING
        #ifdef UT_LOG_WARNING
            #undef UT_LOG_WARNING
        #endif
        #define UT_LOG_WARNING(...)
    #endif
    // ERROR
    #if UT_LOG_LEVEL < UT_LOG_LEVEL_ERROR
        #ifdef UT_LOG_ERROR
            #undef UT_LOG_ERROR
        #endif
        #define UT_LOG_ERROR(...)
    #endif
#endif //ENABLE_UT_INFO_LOGGING*/

#define AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling getAudioDecoderIds(OUT:decoderIds:[])"); \
        status = gAudioDecoderManager->getAudioDecoderIds(&decoderIds); \
        UT_LOG_INFO("Result getAudioDecoderIds(OUT:decoderIds:[size: %d]), status:[isOK:%s, exception:%s]", \
                                decoderIds.size(), UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE_FATAL(status.isOk()); \
        UT_ASSERT_FALSE_FATAL(decoderIds.empty()); \
        UT_ASSERT_EQUAL_FATAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling getAudioDecoder(IN:_decoderId:[%d], OUT:audioDecoder[])", _decoderId.value); \
        status = gAudioDecoderManager->getAudioDecoder(_decoderId, &audioDecoder); \
        UT_LOG_INFO("Result getAudioDecoder(IN:_decoderId:[%d], OUT:audioDecoder[0x%X]), status:[isOK:%s, exception:%s]", \
                                _decoderId.value, audioDecoder.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE_FATAL(status.isOk()); \
        UT_ASSERT_TRUE_FATAL(audioDecoder.get() != nullptr); \
        UT_ASSERT_EQUAL_FATAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AUDIODECODER_MANAGER_GETAUDIODECODER_FAILURE(_decoderId, audioDecoder, expectedException) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling getAudioDecoder(IN:_decoderId:[%d], OUT:audioDecoder[])", _decoderId.value); \
        status = gAudioDecoderManager->getAudioDecoder(_decoderId, &audioDecoder); \
        UT_LOG_INFO("Result getAudioDecoder(IN:_decoderId:[%d], OUT:audioDecoder[0x%X]), status:[isOK:%s, exception:%s]", \
                                _decoderId.value, audioDecoder.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()) << "getAudioDecoder succeeded "; \
        UT_ASSERT_EQUAL(status.exceptionCode(), (expectedException)) << "Unexpected exception for getAudioDecoder"; \
        UT_ASSERT_TRUE(audioDecoder.get() == nullptr) << "audioDecoder should be null on failure"; \
    } while (0)

#define AUDIODECODER_GETCAPABILITIES(audioDecoder, capabilities) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling getCapabilities(OUT:capabilities)"); \
        status = audioDecoder->getCapabilities(capabilities); \
        UT_LOG_INFO("Result getCapabilities(OUT:capabilities), status:[isOK:%s, exception:%s]", \
                                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()) << "getCapabilities failed unexpectedly"; \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE) << "Unexpected exception for getCapabilities"; \
    } while (0)

#define AUDIODECODER_GET_PROPERTY_SUCCESS(audioDecoder, property, propertyValue) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling getProperty(IN:property[%s], OUT:propertyValue[]", audiodecoder::toString(property).c_str()); \
        status = (audioDecoder)->getProperty(property, &propertyValue); \
        if(propertyValue.has_value()) \
        { \
            switch (propertyValue->value.value().getTag()) \
            { \
                case PropertyValue::Value::Tag::intValue: \
                    UT_LOG_INFO("Result getProperty(IN:property[%s], OUT:propertyValue[%d]), status:[isOK:%s, exception:%s]", audiodecoder::toString(property).c_str(), \
                    propertyValue->value->get<PropertyValue::Value::Tag::intValue>(), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
                    break; \
                case PropertyValue::Value::Tag::stringValue: \
                    UT_LOG_INFO("Result getProperty(IN:property[%s], OUT:propertyValue[%s]), status:[isOK:%s, exception:%s]", audiodecoder::toString(property).c_str(), \
                    propertyValue->value->get<PropertyValue::Value::Tag::stringValue>().string(), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
                    break; \
                default: \
                    break; \
            } \
        } \
        else \
        { \
            UT_LOG_INFO("Result getProperty(IN:property[%s], OUT:propertyValue[]), status:[isOK:%s, exception:%s]", audiodecoder::toString(property).c_str(), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
            UT_ASSERT_TRUE(false); \
        } \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AUDIODECODER_GET_PROPERTY_FAILURE(audioDecoder, property, propertyValue, exception) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling getProperty(IN:property[%s], OUT:propertyValue[]", audiodecoder::toString(property).c_str()); \
        status = (audioDecoder)->getProperty(property, &propertyValue); \
        UT_LOG_INFO("Result getProperty(IN:property[%s], OUT:propertyValue[]), status:[isOK:%s, exception:%s]", audiodecoder::toString(property).c_str(), \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
    } while (0)

#define AUDIODECODER_GET_STATE(audioDecoder, state) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling getState(OUT:state[]"); \
        status = (audioDecoder)->getState(&state); \
        UT_LOG_INFO("Result getState(OUT:state[%s]), status:[isOK:%s, exception:%s]", audiodecoder::toString(state).c_str(), \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AUDIODECODER_WAIT_FOR_STATE(audioDecoder, desiredState, lastState) \
    do { \
        int32_t sleep_time = 0; \
        do \
        { \
            this_thread::sleep_for(chrono::milliseconds(MAX_TRANSITORY_STATE_WAIT_MS)); \
            sleep_time += MAX_TRANSITORY_STATE_WAIT_MS; \
            /* Check the state of the decoder */ \
            AUDIODECODER_GET_STATE((audioDecoder), lastState); \
        } while (sleep_time < MAX_FINAL_STATE_WAIT_MS && lastState != desiredState); \
    } while (0)

#define AUDIODECODER_OPEN_SUCCESS(audioDecoder, codec, secure, controllerListener, audioDecoderController, stateCheck) \
    do { \
        ::android::binder::Status status; \
        \
        /* Open the Decoder */ \
        UT_LOG_INFO("Calling open(IN:codec[%d], IN:secure[%s], IN:controllerListener[%p], OUT:audioDecoderController[])", \
            codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener); \
        status = (audioDecoder)->open(codec, secure, controllerListener, &audioDecoderController); \
        UT_LOG_INFO("Result open(IN:codec[%d], IN:secure[%s], IN:controllerListener[%p], OUT:audioDecoderController[0x%X]), status:[isOK:%s, exception:%s]", \
            codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener, audioDecoderController.get(), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE_FATAL(status.isOk()); \
        UT_ASSERT_NOT_EQUAL_FATAL(audioDecoderController.get(), nullptr); \
        UT_ASSERT_EQUAL_FATAL(status.exceptionCode(), Status::EX_NONE); \
        \
        if (stateCheck) \
        { \
            AudioDecoderState lastState; \
            AUDIODECODER_WAIT_FOR_STATE(audioDecoder, AudioDecoderState::READY, lastState); \
            UT_ASSERT_EQUAL_FATAL(lastState, AudioDecoderState::READY); \
        } \
    } while (0)

#define AUDIODECODER_OPEN_FAILURE(audioDecoder, codec, secure, controllerListener, audioDecoderController, exception) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling open(IN:codec[%d], IN:secure[%s], IN:controllerListener[%p], OUT:audioDecoderController[])", \
            codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener); \
        status = audioDecoder->open(codec, secure, controllerListener, &audioDecoderController); \
        UT_LOG_INFO("Result open(IN:codec[%d], IN:secure[%s], IN:controllerListener[%p], OUT:audioDecoderController[0x%X]), status:[isOK:%s, exception:%s]", \
            codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener, audioDecoderController.get(), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        if ((exception) == Status::EX_NONE) { \
            UT_ASSERT_TRUE(status.isOk()); \
            UT_ASSERT_TRUE(audioDecoderController.get() == nullptr); \
        } else { \
            UT_ASSERT_FALSE(status.isOk()); \
        } \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
    } while (0)

#define AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, stateCheck) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling close(IN:audioDecoderController[%p], OUT:_aidl_return[])", audioDecoderController.get()); \
        status = audioDecoder->close(audioDecoderController, &_aidl_return); \
        UT_LOG_INFO("Result close(IN:audioDecoderController[%p], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
            audioDecoderController.get(), UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE_FATAL(status.isOk()); \
        UT_ASSERT_TRUE_FATAL(_aidl_return); \
        UT_ASSERT_EQUAL_FATAL(status.exceptionCode(), Status::EX_NONE); \
        if (stateCheck) \
        { \
            AudioDecoderState lastState; \
            AUDIODECODER_WAIT_FOR_STATE(audioDecoder, AudioDecoderState::CLOSED, lastState); \
            UT_ASSERT_EQUAL_FATAL(lastState, AudioDecoderState::CLOSED); \
        } \
    } while (0)

#define AUDIODECODER_CLOSE_FAILURE(audioDecoder, audioDecoderController, exception) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling close(IN:audioDecoderController[%p], OUT:_aidl_return[])", audioDecoderController.get()); \
        status = audioDecoder->close(audioDecoderController, &_aidl_return); \
        UT_LOG_INFO("Result close(IN:audioDecoderController[%p]), status:[isOK:%s, exception:%s]", \
            audioDecoderController.get(), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(_aidl_return); \
    } while (0)

#define AUDIODECODER_REGISTER_EVENT_SUCCESS(audioDecoder, eventListener) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling registerEventListener(IN:eventListener[%p], OUT:_aidl_return[])", eventListener.get()); \
        status = audioDecoder->registerEventListener(eventListener, &_aidl_return); \
        UT_LOG_INFO("Result registerEventListener(IN:eventListener[%p], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
            eventListener.get(), UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        UT_ASSERT_TRUE(_aidl_return); \
    } while (0)

#define AUDIODECODER_REGISTER_EVENT_FAILURE(audioDecoder, eventListener, exception) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling registerEventListener(IN:eventListener[%p], OUT:_aidl_return[])", eventListener.get()); \
        status = audioDecoder->registerEventListener(eventListener, &_aidl_return); \
        UT_LOG_INFO("Result registerEventListener(IN:eventListener[%p]), status:[isOK:%s, exception:%s]", \
            eventListener.get(), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        if (exception == Status::EX_NONE) \
        { \
            UT_ASSERT_TRUE(status.isOk()); \
            UT_ASSERT_FALSE(_aidl_return); \
        } else { \
            UT_ASSERT_FALSE(status.isOk()); \
        } \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
    } while (0)

#define AUDIODECODER_UNREGISTER_EVENT_SUCCESS(audioDecoder, eventListener) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling unregisterEventListener(IN:eventListener[%p], OUT:_aidl_return[])", eventListener.get()); \
        status = audioDecoder->unregisterEventListener(eventListener, &_aidl_return); \
        UT_LOG_INFO("Result unregisterEventListener(IN:eventListener[%p], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
            eventListener.get(), UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_TRUE(_aidl_return); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AUDIODECODER_UNREGISTER_EVENT_FAILURE(audioDecoder, eventListener, exception) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling unregisterEventListener(IN:eventListener[%p], OUT:_aidl_return[])", eventListener.get()); \
        status = audioDecoder->unregisterEventListener(eventListener, &_aidl_return); \
        UT_LOG_INFO("Result unregisterEventListener(IN:eventListener[%p]), status:[isOK:%s, exception:%s]", \
            eventListener.get(), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        if(exception == Status::EX_NONE) \
        { \
            UT_ASSERT_TRUE(status.isOk()); \
            UT_ASSERT_FALSE(_aidl_return); \
        } else { \
            UT_ASSERT_FALSE(status.isOk()); \
        } \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
    } while (0)

#define AUDIODECODER_START_SUCCESS(audioDecoder, audioDecoderController, stateCheck) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling start()"); \
        status = audioDecoderController->start(); \
        UT_LOG_INFO("Result start(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        if (stateCheck) \
        { \
            AudioDecoderState lastState; \
            AUDIODECODER_WAIT_FOR_STATE(audioDecoder, AudioDecoderState::STARTED, lastState); \
            UT_ASSERT_EQUAL(lastState, AudioDecoderState::STARTED); \
        } \
    } while (0)

#define AUDIODECODER_START_FAILURE(audioDecoderController, exception) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling start()"); \
        status = audioDecoderController->start(); \
        UT_LOG_INFO("Result start(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(status.isOk()); \
    } while (0)

#define AUDIODECODER_STOP_SUCCESS(audioDecoder, audioDecoderController, stateCheck) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling stop()"); \
        status = audioDecoderController->stop(); \
        UT_LOG_INFO("Result stop(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        if (stateCheck) \
        { \
            AudioDecoderState lastState; \
            AUDIODECODER_WAIT_FOR_STATE(audioDecoder, AudioDecoderState::READY, lastState); \
            UT_ASSERT_EQUAL(lastState, AudioDecoderState::READY); \
        } \
    } while (0)

#define AUDIODECODER_STOP_FAILURE(audioDecoderController, exception) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling stop()"); \
        status = audioDecoderController->stop(); \
        UT_LOG_INFO("Result stop(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(status.isOk()); \
    } while (0)

/* decodeBufferWithMetadata only submits data frames; EOS is driven via signalEndOfStream(). */
#define AUDIODECODER_DECODE_WITH_METADATA_SUCCESS(decoderID, audioDecoderController, bufferHandle, metadata) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("[DecoderID: %d] Calling decodeBufferWithMetadata(IN:bufferHandle[0x%016" PRIx64 "], IN:PTS[%" PRId64"], IN:trimStartNs[%d], IN:trimEndNs[%d], IN:discontinuity[%s])", \
            decoderID, (uint64_t)(bufferHandle), (int64_t)(metadata).nsPresentationTime, \
            (metadata).trimStartNs, (metadata).trimEndNs, \
            UT_Control_GetMapString(bool_mapTable, (metadata).discontinuity)); \
        status = (audioDecoderController)->decodeBufferWithMetadata(bufferHandle, metadata, &_aidl_return); \
        UT_LOG_INFO("[DecoderID: %d] Result decodeBufferWithMetadata(OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
            decoderID, UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        UT_ASSERT_TRUE(_aidl_return); \
    } while (0)


/* Failure: EX_NONE means soft rejection (status ok, _aidl_return false); non-EX_NONE means hard error */
#define AUDIODECODER_DECODE_WITH_METADATA_FAILURE(audioDecoderController, bufferHandle, metadata, exception) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return = false; \
        UT_LOG_INFO("Calling decodeBufferWithMetadata(IN:bufferHandle[0x%016" PRIx64 "], IN:PTS[%" PRId64 "], IN:trimStartNs[%d], IN:trimEndNs[%d], IN:discontinuity[%s])", \
            (uint64_t)(bufferHandle), (int64_t)(metadata).nsPresentationTime, \
            (metadata).trimStartNs, (metadata).trimEndNs, \
            UT_Control_GetMapString(bool_mapTable, (metadata).discontinuity)); \
        status = (audioDecoderController)->decodeBufferWithMetadata(bufferHandle, metadata, &_aidl_return); \
        UT_LOG_INFO("Result decodeBufferWithMetadata(OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        if ((exception) == Status::EX_NONE) { \
            UT_ASSERT_TRUE(status.isOk()); \
            UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
            UT_ASSERT_FALSE(_aidl_return); \
        } else { \
            UT_ASSERT_FALSE(status.isOk()); \
            UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        } \
    } while (0)

#define AUDIODECODER_FLUSH_SUCCESS(audioDecoder, audioDecoderController, reset, stateCheck) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling flush(IN: reset[%s])", UT_Control_GetMapString(bool_mapTable, reset)); \
        status = audioDecoderController->flush(reset); \
        UT_LOG_INFO("Result flush(IN: reset[%s]), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, reset), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        if (stateCheck) \
        { \
            this_thread::sleep_for(chrono::milliseconds(MAX_FINAL_STATE_WAIT_MS)); \
            if (reset) \
            { \
                AudioDecoderState lastState; \
                AUDIODECODER_WAIT_FOR_STATE(audioDecoder, AudioDecoderState::READY, lastState); \
                UT_ASSERT_EQUAL(lastState, AudioDecoderState::READY); \
            } \
            else \
            { \
                AudioDecoderState lastState; \
                AUDIODECODER_WAIT_FOR_STATE(audioDecoder, AudioDecoderState::STARTED, lastState); \
                UT_ASSERT_EQUAL(lastState, AudioDecoderState::STARTED); \
            } \
        } \
    } while (0)

#define AUDIODECODER_FLUSH_FAILURE(audioDecoderController, reset, exception) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling flush(IN: reset[%s])", UT_Control_GetMapString(bool_mapTable, reset)); \
        status = audioDecoderController->flush(reset); \
        UT_LOG_INFO("Result flush(IN: reset[%s]), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, reset), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(status.isOk()); \
    } while (0)

#define AUDIODECODER_SIGNAL_DISCONTINUITY_SUCCESS(audioDecoder, audioDecoderController, stateCheck) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling signalDiscontinuity()"); \
        status = audioDecoderController->signalDiscontinuity(); \
        UT_LOG_INFO("Result signalDiscontinuity(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        if (stateCheck) \
        { \
            AudioDecoderState lastState; \
            AUDIODECODER_WAIT_FOR_STATE(audioDecoder, AudioDecoderState::STARTED, lastState); \
            UT_ASSERT_EQUAL(lastState, AudioDecoderState::STARTED); \
        } \
    } while (0)

#define AUDIODECODER_SIGNAL_DISCONTINUITY_FAILURE(audioDecoderController, exception) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling signalDiscontinuity()"); \
        status = audioDecoderController->signalDiscontinuity(); \
        UT_LOG_INFO("Result signalDiscontinuity(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(status.isOk()); \
    } while (0)

#define AUDIODECODER_SIGNAL_EOS_SUCCESS(audioDecoder, audioDecoderController, stateCheck) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling signalEndOfStream()"); \
        status = audioDecoderController->signalEndOfStream(); \
        UT_LOG_INFO("Result signalEndOfStream(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        if (stateCheck) \
        { \
            AudioDecoderState lastState; \
            AUDIODECODER_WAIT_FOR_STATE(audioDecoder, AudioDecoderState::STARTED, lastState); \
            UT_ASSERT_EQUAL(lastState, AudioDecoderState::STARTED); \
        } \
    } while (0)

#define AUDIODECODER_SIGNAL_EOS_FAILURE(audioDecoderController, exception) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling signalEndOfStream()"); \
        status = audioDecoderController->signalEndOfStream(); \
        UT_LOG_INFO("Result signalEndOfStream(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(status.isOk()); \
    } while (0)

#define AUDIODECODER_PARSE_CSD_SUCCESS(audioDecoder, audioDecoderController, csdAudioFormat, codecData, stateCheck) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling parseCodecSpecificData(IN:csdAudioFormat[%d], IN:codecData[0x%X], OUT:_aidl_return[])", csdAudioFormat, codecData); \
        status = audioDecoderController->parseCodecSpecificData(csdAudioFormat, codecData, &_aidl_return); \
        UT_LOG_INFO("Result parseCodecSpecificData(IN:csdAudioFormat[%d], IN:codecData[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", csdAudioFormat, codecData, \
            UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_TRUE(_aidl_return); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
        if (stateCheck) \
        { \
            AudioDecoderState lastState; \
            AUDIODECODER_WAIT_FOR_STATE(audioDecoder, AudioDecoderState::STARTED, lastState); \
            UT_ASSERT_EQUAL(lastState, AudioDecoderState::STARTED); \
        } \
    } while (0)

#define AUDIODECODER_PARSE_CSD_FAILURE(audioDecoderController, csdAudioFormat, codecData, exception) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling parseCodecSpecificData(IN:csdAudioFormat[%d], IN:codecData[0x%X], OUT:_aidl_return[])", csdAudioFormat, codecData); \
        status = audioDecoderController->parseCodecSpecificData(csdAudioFormat, codecData, &_aidl_return); \
        UT_LOG_INFO("Result parseCodecSpecificData(IN:csdAudioFormat[%d], IN:codecData[0x%X]), status:[isOK:%s, exception:%s]", csdAudioFormat, codecData, \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(status.isOk()); \
    } while (0)

#define AUDIODECODER_SETPROPERTY_SUCCESS(audioDecoderController, property, propertyValue) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling setProperty(IN:property[%s], IN:propertyValue[0x%X], OUT:_aidl_return[])", com::rdk::hal::audiodecoder::toString(property).c_str(), propertyValue.value); \
        status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return); \
        UT_LOG_INFO("Result setProperty(IN:property[%s], IN:propertyValue[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", com::rdk::hal::audiodecoder::toString(property).c_str(), propertyValue.value, \
            UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_TRUE(_aidl_return); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AUDIODECODER_SETPROPERTY_FAILURE(audioDecoderController, property, propertyValue, exception) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_INFO("Calling setProperty(IN:property[%s], IN:propertyValue[0x%X], OUT:_aidl_return[])", com::rdk::hal::audiodecoder::toString(property).c_str(), propertyValue.value); \
        status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return); \
        UT_LOG_INFO("Result setProperty(IN:property[%s], IN:propertyValue[0x%X]), status:[isOK:%s, exception:%s]", com::rdk::hal::audiodecoder::toString(property).c_str(), propertyValue.value, \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(status.isOk()); \
    } while (0)

#define AUDIODECODER_SETAUDIOFORMAT_SUCCESS(audioDecoderController, channel, sampleRate) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling setAudioFormat(IN:channel[%d], IN:sampleRate[%d])", channel , sampleRate); \
        status = audioDecoderController->setAudioFormat(channel, sampleRate); \
        UT_LOG_INFO("Result setAudioFormat(IN:channel[%d], IN:sampleRate[%d], status:[isOK:%s, exception:%s]", channel, sampleRate, \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AUDIODECODER_SETAUDIOFORMAT_FAILURE(audioDecoderController, channel, sampleRate, exception) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_INFO("Calling setAudioFormat(IN:channel[%d], IN:sampleRate[%d])", channel , sampleRate); \
        status = audioDecoderController->setAudioFormat(channel, sampleRate); \
        UT_LOG_INFO("Result setAudioFormat(IN:channel[%d], IN:sampleRate[%d]), status:[isOK:%s, exception:%s]", channel, sampleRate, \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
    } while (0)

#define AUDIODECODER_SETPROPERTY_LIST_SUCCESS(audioDecoderController, properyList, type) \
    do { \
        for (const auto& [property, value] : properyList) \
        { \
            PropertyValue propertyValue; \
            propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::type>(value); \
            AUDIODECODER_SETPROPERTY_SUCCESS(audioDecoderController, property, propertyValue); \
        } \
    } while (0)

#define AUDIODECODER_SETPROPERTY_STRING_LIST_SUCCESS(audioDecoderController, properyList) \
    do { \
        for (const auto& [property, value] : properyList) \
        { \
            PropertyValue propertyValue; \
            propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::stringValue>(value); \
            AUDIODECODER_SETPROPERTY_SUCCESS(audioDecoderController, property, propertyValue); \
        } \
    } while (0)

#define AUDIODECODER_SETPROPERTY_LIST_FAILURE(audioDecoderController, properyList, type, exception) \
    do { \
        for (const auto& [property, value] : properyList) \
        { \
            PropertyValue propertyValue; \
            propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::type>(value); \
            AUDIODECODER_SETPROPERTY_FAILURE(audioDecoderController, property, propertyValue, exception); \
        } \
    } while (0)

#define AUDIODECODER_SETPROPERTY_STRING_LIST_FAILURE(audioDecoderController, properyList, exception) \
    do { \
        for (const auto& [property, value] : properyList) \
        { \
            PropertyValue propertyValue; \
            propertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::stringValue>(value); \
            AUDIODECODER_SETPROPERTY_FAILURE(audioDecoderController, property, propertyValue, exception); \
        } \
    } while (0)



/* Decoded Frame details */
typedef struct DecodedAudioFrame
{
    int64_t framePTS;
    bool isMetaAvailable;
    bool isTunnel;
    com::rdk::hal::audiodecoder::FrameType frameType { com::rdk::hal::audiodecoder::FrameType::PCM };
    int64_t frameBufferHandle;
    std::shared_ptr<const audiodecoder::FrameMetadata> frameMetadata;
    std::vector<uint8_t> socPrivate;
    chrono::steady_clock::time_point receiveTime;
}DecodedAudioFrame_t;

enum class FrameOutputMode
{
    UNKNOWN,
    TUNNEL,
    NON_TUNNEL
};

/* Input frame to decoder */
typedef struct InputAudioFrame
{
    int64_t framePTS;
    bool isErrorFrame;
    chrono::steady_clock::time_point sendTime;
}InputAudioFrame_t;

/* Error frame details */
typedef struct ErrorDetails
{
    com::rdk::hal::audiodecoder::ErrorCode vendorErrorCode;
    int32_t errorCode;
    chrono::steady_clock::time_point receiveTime;
}ErrorDetails_t;

/* Contoller Listener Class */
class AudioDecoderControllerListener : public BnAudioDecoderControllerListener
{
public:
    AudioDecoderControllerListener()
        : BnAudioDecoderControllerListener(),
          mShutdownFlag(false),
                    mDecodeBufferAvailableFlag(false),
                    mEndOfStreamFlag(false) {}
    virtual ~AudioDecoderControllerListener() = default;

    string formatChannelVector(const std::vector<ChannelType>& channels);

    /* Caller owns frameAVBufferHandle and must free it after consuming the decoded output */
    ::android::binder::Status onFrameOutput(
        int64_t nsPresentationTime,
        int64_t frameAVBufferHandle,
        const std::optional<com::rdk::hal::audiodecoder::FrameMetadata>& metadata
    ) override;

    ::android::binder::Status onEndOfStream() override;

    /* Fires once when the HAL input queue transitions from full to available */
    ::android::binder::Status onDecodeBufferAvailable() override;

    bool getNextFrame(DecodedAudioFrame_t &frameInfo, int32_t waitTime, bool popFrame);
    void resetFrames();
    FrameOutputMode getFrameOutputMode() const { return mFrameOutputMode.load(); }

    /* Returns true if onDecodeBufferAvailable fired within waitTimeMs, false on timeout */
    bool waitForDecodeBufferAvailable(int32_t waitTimeMs);
    void resetDecodeBufferAvailable();

    int32_t getDecodeBufferAvailableCount() const { return mDecodeBufferAvailableCount.load(); }
    void resetDecodeBufferAvailableCount()         { mDecodeBufferAvailableCount = 0; }

    bool waitForEndOfStream(int32_t waitTimeMs);
    void resetEndOfStream();
    int32_t getEndOfStreamCount() const { return mEndOfStreamCount.load(); }
    void resetEndOfStreamCount()        { mEndOfStreamCount = 0; }

    void shutdown();
    void setDecoderID(int32_t id);
private:
    queue<DecodedAudioFrame_t> mFrameQueue;
    mutex mQueueMutex;
    condition_variable mFrameAvailable;
    atomic<FrameOutputMode> mFrameOutputMode{FrameOutputMode::UNKNOWN};
    atomic<bool> mShutdownFlag;
    int32_t mDecoderID = -1;

    atomic<bool>    mDecodeBufferAvailableFlag;
    atomic<int32_t> mDecodeBufferAvailableCount{0};
    mutex mDecodeBufferMutex;
    condition_variable mDecodeBufferCV;

    atomic<bool>    mEndOfStreamFlag;
    atomic<int32_t> mEndOfStreamCount{0};
    mutex mEndOfStreamMutex;
    condition_variable mEndOfStreamCV;
};

/* Event Listener Class */
class AudioDecoderEventListener : public BnAudioDecoderEventListener
{
public:
    static int32_t mUsedDecoderID;
    AudioDecoderEventListener(): mCurrentState(AudioDecoderState::CLOSED)
    {
        AudioDecoderEventListener::mUsedDecoderID = -1;
    }

    static int32_t getUsedDecoder()
    {
        return AudioDecoderEventListener::mUsedDecoderID;
    }
    virtual ~AudioDecoderEventListener() = default;

    ::android::binder::Status onDecodeError(::com::rdk::hal::audiodecoder::ErrorCode errorCode, int32_t vendorErrorCode) override;

    Status onStateChanged(AudioDecoderState oldState, AudioDecoderState newState) override;

    bool getNextError(ErrorDetails_t &errorDetails);

    int32_t getErrorCount()
    {
        lock_guard<mutex> lock(mErrorMutex);
        return static_cast<int32_t>(mErrors.size());
    }

    /**
     * Wait for the decoder to reach a particular State within a given timeout.
     *
     * @param desiredState The state we want to observe.
     * @param timeout      How long to wait, e.g. chrono::milliseconds(500).
     * @return true if the state was reached; false if we timed out waiting.
     */
    bool waitForState(AudioDecoderState desiredState, chrono::milliseconds timeout);

    void resetStateChanges();

    bool checkStateTransistion(AudioDecoderState expOldState, AudioDecoderState expNewState);

    void resetErrors();
    void setDecoderID(int32_t id);

private:
    mutable mutex mMutex;
    mutable mutex mErrorMutex;
    condition_variable mCV;

    AudioDecoderState mCurrentState;
    vector<pair<AudioDecoderState, AudioDecoderState>> mStateChanges;
    condition_variable mErrorAvailable;
    queue<ErrorDetails_t> mErrors;
    int32_t mDecoderID = -1;
};

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
    bool    mEosSubmitted         = false;
    bool    mSubmittedInSession   = false;
    // Set false if setupAudioDecoder() bails out before opening the decoder
    // (e.g. input file missing) — run() checks this to skip startDecoding()/
    // teardownDecoder() rather than using a null mAudioDecoderController.
    bool    mSetupSucceeded       = false;

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
        int32_t  decodeErrors                = 0;
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
    void submitFrame(int64_t audioBufferHandle, const audiodecoder::InputBufferMetadata& metadata);
    void handleMidStreamEOS(int64_t framePTS);

    bool checkPassThrough   (const DecodedAudioFrame_t& frame, int32_t framesDecoded, bool& isPassThrough);
    void checkPTSSpacing    (const DecodedAudioFrame_t& frame, int64_t frameBytes, FrameValidationState& state);
    void checkFrameMetadata (const DecodedAudioFrame_t& frame, FrameValidationState& state);
    void validatePostLoopMetrics(const FrameValidationState& state);
};

/**
 * Resets both listeners before each stream decode run so that
 * errors and frames from a previous stream do not affect validation.
 */
void resetDecodeSession(sp<AudioDecoderEventListener> eventListener,
                        sp<AudioDecoderControllerListener> controlListener);

#endif //__VTS_AUDIODECODER_H__

/** @} */ // End of AUDIO_DECODER_HALTEST_HEADER
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER
/** @} */ // End of HPK
