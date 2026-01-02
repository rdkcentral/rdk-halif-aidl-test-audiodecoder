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
#include <tuple>
#include <fstream>
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

extern sp<IAudioDecoderManager> gAudioDecoderManager;
extern void *gAudioDecoderTestConfiguration;
extern sp<IAudioDecoderManager> gAudioDecoderManager;

#define MAX_TRANSITORY_STATE_WAIT_MS  10
#define MAX_FINAL_STATE_WAIT_MS  500

#ifndef ENABLE_UT_INFO_LOGGING
#ifdef UT_LOG_DEBUG
#undef UT_LOG_DEBUG
#define UT_LOG_DEBUG(...)
#endif
#ifdef UT_LOG_INFO
#undef UT_LOG_INFO
#define UT_LOG_INFO(...)
#endif
#endif

#define AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_DEBUG("Calling getAudioDecoderIds(OUT:decoderIds:[])"); \
        status = gAudioDecoderManager->getAudioDecoderIds(&decoderIds); \
        UT_LOG_DEBUG("Result getAudioDecoderIds(OUT:decoderIds:[size: %d]), status:[isOK:%s, exception:%s]", \
                                decoderIds.size(), UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE_FATAL(status.isOk()); \
        UT_ASSERT_FALSE_FATAL(decoderIds.empty()); \
        UT_ASSERT_EQUAL_FATAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_DEBUG("Calling getAudioDecoder(IN:_decoderId:[%d], OUT:audioDecoder[])", _decoderId.value); \
        status = gAudioDecoderManager->getAudioDecoder(_decoderId, &audioDecoder); \
        UT_LOG_DEBUG("Result getAudioDecoder(IN:_decoderId:[%d], OUT:audioDecoder[0x%X]), status:[isOK:%s, exception:%s]", \
                                _decoderId.value, audioDecoder.get(), UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE_FATAL(status.isOk()); \
        UT_ASSERT_TRUE_FATAL(audioDecoder.get() != nullptr); \
        UT_ASSERT_EQUAL_FATAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AUDIODECODER_MANAGER_GETAUDIODECODER_FAILURE(_decoderId, audioDecoder, expectedException) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_DEBUG("Calling getAudioDecoder(IN:_decoderId:[%d], OUT:audioDecoder[])", _decoderId.value); \
        status = gAudioDecoderManager->getAudioDecoder(_decoderId, &audioDecoder); \
        UT_LOG_DEBUG("Result getAudioDecoder(IN:_decoderId:[%d]), status:[isOK:%s, exception:%s]", \
                                _decoderId.value, UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()) << "getAudioDecoder succeeded unexpectedly"; \
        UT_ASSERT_EQUAL(status.exceptionCode(), (expectedException)) << "Unexpected exception for getAudioDecoder"; \
    } while (0)

#define AUDIODECODER_GETCAPABILITIES(audioDecoder, capabilities) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_DEBUG("Calling getCapabilities(OUT:capabilities)"); \
        status = audioDecoder->getCapabilities(capabilities); \
        UT_LOG_DEBUG("Result getCapabilities(OUT:capabilities), status:[isOK:%s, exception:%s]", \
                                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()) << "getCapabilities failed unexpectedly"; \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE) << "Unexpected exception for getCapabilities"; \
    } while (0)

#define AUDIODECODER_GET_PROPERTY_SUCCESS(audioDecoder, property, propertyValue) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_DEBUG("Calling getProperty(IN:property[%s], OUT:propertyValue[]", audiodecoder::toString(property).c_str()); \
        status = (audioDecoder)->getProperty(property, &propertyValue); \
        if(propertyValue.has_value()) \
        { \
            switch (propertyValue->value.value().getTag()) \
            { \
                case PropertyValue::Value::Tag::intValue: \
                    UT_LOG_DEBUG("Result getProperty(IN:property[%s], OUT:propertyValue[%d]), status:[isOK:%s, exception:%s]", audiodecoder::toString(property).c_str(), \
                    propertyValue->value->get<PropertyValue::Value::Tag::intValue>(), \
                    UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                    UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
                    break; \
                case PropertyValue::Value::Tag::stringValue: \
                    UT_LOG_DEBUG("Result getProperty(IN:property[%s], OUT:propertyValue[%s]), status:[isOK:%s, exception:%s]", audiodecoder::toString(property).c_str(), \
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
            UT_LOG_DEBUG("Result getProperty(IN:property[%s], OUT:propertyValue[]), status:[isOK:%s, exception:%s]", audiodecoder::toString(property).c_str(), \
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
        UT_LOG_DEBUG("Calling getProperty(IN:property[%s], OUT:propertyValue[]", audiodecoder::toString(property).c_str()); \
        status = (audioDecoder)->getProperty(property, &propertyValue); \
        UT_LOG_DEBUG("Result getProperty(IN:property[%s], OUT:propertyValue[]), status:[isOK:%s, exception:%s]", audiodecoder::toString(property).c_str(), \
                UT_Control_GetMapString(bool_mapTable, status.isOk()), \
                UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
    } while (0)

#define AUDIODECODER_GET_STATE(audioDecoder, state) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_DEBUG("Calling getState(OUT:state[]"); \
        status = (audioDecoder)->getState(&state); \
        UT_LOG_DEBUG("Result getState(OUT:state[%s]), status:[isOK:%s, exception:%s]", audiodecoder::toString(state).c_str(), \
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
        UT_LOG_DEBUG("Calling open(IN:codec[%d], IN:secure[%s], IN:controllerListener[%p], OUT:audioDecoderController[])", \
            codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener); \
        status = (audioDecoder)->open(codec, secure, controllerListener, &audioDecoderController); \
        UT_LOG_DEBUG("Result open(IN:codec[%d], IN:secure[%s], IN:controllerListener[%p], OUT:audioDecoderController[0x%X]), status:[isOK:%s, exception:%s]", \
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
        UT_LOG_DEBUG("Calling open(IN:codec[%d], IN:secure[%s], IN:controllerListener[%p], OUT:audioDecoderController[])", \
            codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener); \
        status = audioDecoder->open(codec, secure, controllerListener, &audioDecoderController); \
        UT_LOG_DEBUG("Result open(IN:codec[%d], IN:secure[%s], IN:controllerListener[%p]), status:[isOK:%s, exception:%s]", \
            codec, UT_Control_GetMapString(bool_mapTable, secure), controllerListener, \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
    } while (0)

#define AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, stateCheck) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_DEBUG("Calling close(IN:audioDecoderController[%p], OUT:_aidl_return[])", audioDecoderController.get()); \
        status = audioDecoder->close(audioDecoderController, &_aidl_return); \
        UT_LOG_DEBUG("Result close(IN:audioDecoderController[%p], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
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
        UT_LOG_DEBUG("Calling close(IN:audioDecoderController[%p], OUT:_aidl_return[])", audioDecoderController.get()); \
        status = audioDecoder->close(audioDecoderController, &_aidl_return); \
        UT_LOG_DEBUG("Result close(IN:audioDecoderController[%p]), status:[isOK:%s, exception:%s]", \
            audioDecoderController.get(), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
    } while (0)

#define AUDIODECODER_REGISTER_EVENT_SUCCESS(audioDecoder, eventListener) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_DEBUG("Calling registerEventListener(IN:eventListener[%p], OUT:_aidl_return[])", eventListener.get()); \
        status = audioDecoder->registerEventListener(eventListener, &_aidl_return); \
        UT_LOG_DEBUG("Result registerEventListener(IN:eventListener[%p], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
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
        UT_LOG_DEBUG("Calling registerEventListener(IN:eventListener[%p], OUT:_aidl_return[])", eventListener.get()); \
        status = audioDecoder->registerEventListener(eventListener, &_aidl_return); \
        UT_LOG_DEBUG("Result registerEventListener(IN:eventListener[%p]), status:[isOK:%s, exception:%s]", \
            eventListener.get(), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        if (exception == Status::EX_NONE) \
            UT_ASSERT_TRUE(status.isOk()); \
        else \
            UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
    } while (0)

#define AUDIODECODER_UNREGISTER_EVENT_SUCCESS(audioDecoder, eventListener) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_DEBUG("Calling unregisterEventListener(IN:eventListener[%p], OUT:_aidl_return[])", eventListener.get()); \
        status = audioDecoder->unregisterEventListener(eventListener, &_aidl_return); \
        UT_LOG_DEBUG("Result unregisterEventListener(IN:eventListener[%p], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
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
        UT_LOG_DEBUG("Calling unregisterEventListener(IN:eventListener[%p], OUT:_aidl_return[])", eventListener.get()); \
        status = audioDecoder->unregisterEventListener(eventListener, &_aidl_return); \
        UT_LOG_DEBUG("Result unregisterEventListener(IN:eventListener[%p]), status:[isOK:%s, exception:%s]", \
            eventListener.get(), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        if(exception == Status::EX_NONE) \
        { \
            UT_ASSERT_TRUE(status.isOk()); \
        } else { \
            UT_ASSERT_FALSE(status.isOk()); \
        } \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
    } while (0)

#define AUDIODECODER_START_SUCCESS(audioDecoder, audioDecoderController, stateCheck) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_DEBUG("Calling start()"); \
        status = audioDecoderController->start(); \
        UT_LOG_DEBUG("Result start(), status:[isOK:%s, exception:%s]", \
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
        UT_LOG_DEBUG("Calling start()"); \
        status = audioDecoderController->start(); \
        UT_LOG_DEBUG("Result start(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(status.isOk()); \
    } while (0)

#define AUDIODECODER_STOP_SUCCESS(audioDecoder, audioDecoderController, stateCheck) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_DEBUG("Calling stop()"); \
        status = audioDecoderController->stop(); \
        UT_LOG_DEBUG("Result stop(), status:[isOK:%s, exception:%s]", \
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
        UT_LOG_DEBUG("Calling stop()"); \
        status = audioDecoderController->stop(); \
        UT_LOG_DEBUG("Result stop(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(status.isOk()); \
    } while (0)

#define AUDIODECODER_DECODEBUFFER_SUCCESS(decoderID, audioDecoderController, nsPresentationTime, bufferHandle, trimStartNs, trimEndNs) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_DEBUG("[DecoderID: %d] Calling decodeBuffer(IN:nsPresentationTime[%ld], IN:bufferHandle[0x%016" PRIx64 "], trimStartNs[%d], trimEndNs[%d], OUT:_aidl_return[])", \
            decoderID, nsPresentationTime, bufferHandle, trimStartNs, trimEndNs); \
        status = audioDecoderController->decodeBuffer(nsPresentationTime, bufferHandle, trimStartNs, trimEndNs, &_aidl_return); \
        UT_LOG_DEBUG("[DecoderID: %d] Result decodeBuffer(IN:nsPresentationTime[%ld], IN:bufferHandle[0x%016" PRIx64 "], trimStartNs[%d], trimEndNs[%d], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", \
            decoderID, nsPresentationTime, bufferHandle, trimStartNs, trimEndNs, UT_Control_GetMapString(bool_mapTable, _aidl_return), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_TRUE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), Status::EX_NONE); \
    } while (0)

#define AUDIODECODER_DECODEBUFFER_FAILURE(audioDecoderController, nsPresentationTime, bufferHandle, trimStartNs, trimEndNs, exception) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_DEBUG("Calling decodeBuffer(IN:nsPresentationTime[%ld], IN:bufferHandle[0x%016" PRIx64 "], trimStartNs[%d], trimEndNs[%d], OUT:_aidl_return[])", \
            nsPresentationTime, bufferHandle, trimStartNs, trimEndNs); \
        status = audioDecoderController->decodeBuffer(nsPresentationTime, bufferHandle, trimStartNs, trimEndNs, &_aidl_return); \
        UT_LOG_DEBUG("Result decodeBuffer(IN:nsPresentationTime[%ld], IN:bufferHandle[0x%016" PRIx64 "], trimStartNs[%d], trimEndNs[%d]), status:[isOK:%s, exception:%s]", \
            nsPresentationTime, bufferHandle, trimStartNs, trimEndNs, \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_FALSE(status.isOk()); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
    } while (0)

#define AUDIODECODER_FLUSH_SUCCESS(audioDecoder, audioDecoderController, reset, stateCheck) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_DEBUG("Calling flush(IN: reset[%s])", UT_Control_GetMapString(bool_mapTable, reset)); \
        status = audioDecoderController->flush(reset); \
        UT_LOG_DEBUG("Result flush(IN: reset[%s]), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, reset), \
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
        UT_LOG_DEBUG("Calling flush(IN: reset[%s])", UT_Control_GetMapString(bool_mapTable, reset)); \
        status = audioDecoderController->flush(reset); \
        UT_LOG_DEBUG("Result flush(IN: reset[%s]), status:[isOK:%s, exception:%s]", UT_Control_GetMapString(bool_mapTable, reset), \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(status.isOk()); \
    } while (0)

#define AUDIODECODER_SIGNAL_DISCONTINUITY_SUCCESS(audioDecoder, audioDecoderController, stateCheck) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_DEBUG("Calling signalDiscontinuity()"); \
        status = audioDecoderController->signalDiscontinuity(); \
        UT_LOG_DEBUG("Result signalDiscontinuity(), status:[isOK:%s, exception:%s]", \
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
        UT_LOG_DEBUG("Calling signalDiscontinuity()"); \
        status = audioDecoderController->signalDiscontinuity(); \
        UT_LOG_DEBUG("Result signalDiscontinuity(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(status.isOk()); \
    } while (0)

#define AUDIODECODER_SIGNAL_EOS_SUCCESS(audioDecoder, audioDecoderController, stateCheck) \
    do { \
        ::android::binder::Status status; \
        UT_LOG_DEBUG("Calling signalEOS()"); \
        status = audioDecoderController->signalEOS(); \
        UT_LOG_DEBUG("Result signalEOS(), status:[isOK:%s, exception:%s]", \
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
        UT_LOG_DEBUG("Calling signalEOS()"); \
        status = audioDecoderController->signalEOS(); \
        UT_LOG_DEBUG("Result signalEOS(), status:[isOK:%s, exception:%s]", \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(status.isOk()); \
    } while (0)

#define AUDIODECODER_PARSE_CSD_SUCCESS(audioDecoder, audioDecoderController, csdAudioFormat, codecData, stateCheck) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_DEBUG("Calling parseCodecSpecificData(IN:csdAudioFormat[%d], IN:codecData[0x%X], OUT:_aidl_return[])", csdAudioFormat, codecData); \
        status = audioDecoderController->parseCodecSpecificData(csdAudioFormat, codecData, &_aidl_return); \
        UT_LOG_DEBUG("Result parseCodecSpecificData(IN:csdAudioFormat[%d], IN:codecData[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", csdAudioFormat, codecData, \
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
        UT_LOG_DEBUG("Calling parseCodecSpecificData(IN:csdAudioFormat[%d], IN:codecData[0x%X], OUT:_aidl_return[])", csdAudioFormat, codecData); \
        status = audioDecoderController->parseCodecSpecificData(csdAudioFormat, codecData, &_aidl_return); \
        UT_LOG_DEBUG("Result parseCodecSpecificData(IN:csdAudioFormat[%d], IN:codecData[0x%X]), status:[isOK:%s, exception:%s]", csdAudioFormat, codecData, \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(status.isOk()); \
    } while (0)

#define AUDIODECODER_SETPROPERTY_SUCCESS(audioDecoderController, property, propertyValue) \
    do { \
        ::android::binder::Status status; \
        bool _aidl_return; \
        UT_LOG_DEBUG("Calling setProperty(IN:property[%s], IN:propertyValue[0x%X], OUT:_aidl_return[])", com::rdk::hal::audiodecoder::toString(property).c_str(), propertyValue.value); \
        status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return); \
        UT_LOG_DEBUG("Result setProperty(IN:property[%s], IN:propertyValue[0x%X], OUT:_aidl_return[%s]), status:[isOK:%s, exception:%s]", com::rdk::hal::audiodecoder::toString(property).c_str(), propertyValue.value, \
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
        UT_LOG_DEBUG("Calling setProperty(IN:property[%s], IN:propertyValue[0x%X], OUT:_aidl_return[])", com::rdk::hal::audiodecoder::toString(property).c_str(), propertyValue.value); \
        status = audioDecoderController->setProperty(property, propertyValue, &_aidl_return); \
        UT_LOG_DEBUG("Result setProperty(IN:property[%s], IN:propertyValue[0x%X]), status:[isOK:%s, exception:%s]", com::rdk::hal::audiodecoder::toString(property).c_str(), propertyValue.value, \
            UT_Control_GetMapString(bool_mapTable, status.isOk()), \
            UT_Control_GetMapString(statusException_mapTable, status.exceptionCode())); \
        UT_ASSERT_EQUAL(status.exceptionCode(), exception); \
        UT_ASSERT_FALSE(status.isOk()); \
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

/* Stream metadata */
struct AudioMetatData
{
    Codec sourceCodec;
    bool isDolbyAtmos;
    int32_t numChannels;
    int32_t sampleRate;
    PCMFormat format;
    bool planarFormat;
    int32_t trimStartNs;
    int32_t trimEndNs;
    bool lowLatency;
    bool endOfStream;
    bool discontinuity;
    vector<audiodecoder::ChannelType> channelTypes;
    AVSource source;
};

/* Decoded Frame details */
typedef struct DecodedAudioFrame
{
    int64_t framePTS;
    bool isMetaAvailable;
    bool isTunnel;
    int64_t frameBufferHandle;
    struct AudioMetatData metadata;
    chrono::steady_clock::time_point receiveTime;
}DecodedAudioFrame_t;

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
    ErrorCode vendorErrorCode;
    int32_t errorCode;
    chrono::steady_clock::time_point receiveTime;
}ErrorDetails_t;

/* Contoller Listener Class */
class AudioDecoderControllerListener : public BnAudioDecoderControllerListener
{
public:
    AudioDecoderControllerListener() : BnAudioDecoderControllerListener(), mShutdownFlag(false){}
    virtual ~AudioDecoderControllerListener() = default;

    string formatChannelVector(const std::vector<ChannelType>& channels);

    Status onFrameOutput(int64_t nsPresentationTime, int64_t frameBufferHandle, const ::optional<FrameMetadata>& metadata) override;

    bool getNextFrame(DecodedAudioFrame_t &frameInfo, int32_t waitTime, bool popFrame);

    void shutdown();
    void setDecoderID(int32_t id);
private:
    queue<DecodedAudioFrame_t> mFrameQueue;
    mutex mQueueMutex;
    condition_variable mFrameAvailable;
    atomic<bool> mShutdownFlag;
    int32_t mDecoderID;
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

    Status onDecodeError(::com::rdk::hal::audiodecoder::ErrorCode vendorErrorCode, int32_t errorCode) override;

    Status onStateChanged(AudioDecoderState oldState, AudioDecoderState newState) override;

    bool getNextError(ErrorDetails_t &errorDetails);

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
    int32_t mDecoderID;
};

class AudioDecoderTest
{
public:
    AudioDecoderTest(IAudioDecoder::Id decoderId, sp<IAudioDecoder> decoderHandle,
                sp<AudioDecoderControllerListener> controlListener,
                sp<AudioDecoderEventListener> eventListener,
                sp<IAVBuffer> AVBufferManager, IAVBufferHelper* avbufferHelper,
                bool secureDecode, testConfiguration_t config)
    {
        mDecoderId = decoderId;
        mDecoderHandle = decoderHandle;
        mControlListener = controlListener;
        mSecureDecode = secureDecode;
        mEventListener = eventListener;
        mTestConfiguration = config;
        mAVBufferManager = AVBufferManager;
        mAVbufferHelper = avbufferHelper;
    }

    void run(bool validateMetrics);

    bool getDecoderMetrics(int32_t &framesDecoded, int32_t &decodeErrors, int32_t &framesDropped);

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

    void decodeAndConsumeAudioStreams(bool validateMetrics);
};

#endif //__VTS_AUDIODECODER_H__

/** @} */ // End of AUDIO_DECODER_HALTEST_HEADER
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER
/** @} */ // End of HPK
