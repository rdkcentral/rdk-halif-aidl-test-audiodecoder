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
 *
 */

/**
 * @addtogroup AUDIO_DECODER Audio Decoder Module
 * @{
 *
 */

/**
 * @defgroup AUDIO_DECODER_HALTEST Audio Decoder HAL Tests
 * @{
 *
 */

/**
 * @defgroup AUDIO_DECODER_HALTEST_L2 Audio Decoder HAL Tests L2 File
 * @{
 * @parblock
 *
 * ### L2 Test Cases for Audio Decoder HAL :
 *
 * This module includes Level 2 functional tests (success and failure scenarios).
 * This is to ensure that the API meets the operational requirements of the module across all vendors.
 *
 * **Pre-Conditions:** @n
 * **Dependencies:** None@n
 *
 * @endparblock
 *
 */

/**
 * @file vts_audiodecoder_l2_tests.c
 *
 */

#include "vts_audiodecoder.h"
#include "vts_avbuffer.h"

class L2AVBufferSpaceListener : public BnAVBufferSpaceListener
{
public:
    ::android::binder::Status onSpaceAvailable() override { return Status::ok(); }
};

static sp<IAVBuffer> vts_audiodecoder_l2_getAvBufferService()
{
    return android::interface_cast<IAVBuffer>(
        android::defaultServiceManager()->getService(
            android::String16(IAVBuffer::serviceName().c_str())
        )
    );
}

/* Test class registration with UT_Core */
UT_ADD_TEST_TO_GROUP(VTS_L2_AudioDecoder, UT_TESTS_L2)

class VTS_L2_AudioDecoder : public UTCore
{
public:
VTS_L2_AudioDecoder() : UTCore() {}

    ~VTS_L2_AudioDecoder() override = default;

    public:
        std::vector<std::tuple<sp<IAudioDecoder>, sp<AudioDecoderControllerListener>, sp<AudioDecoderEventListener>, audiodecoder::Capabilities *>> mAudioDecoderList;

    void SetUp() override
    {
        if (gAudioDecoderManager == nullptr) {
                UT_FAIL_FATAL("AudioDecoder Service not available");
        }

        std::vector<IAudioDecoder::Id> decoderIds;
        /* Get the Audio decoder IDs */
        AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

        android::ProcessState::self()->startThreadPool();

        for(const auto& _decoderId: decoderIds)
        {
            sp<IAudioDecoder> audioDecoder;
            audiodecoder::Capabilities *capabilities = new audiodecoder::Capabilities();

            sp<AudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

            sp<AudioDecoderControllerListener> controlListener = new AudioDecoderControllerListener();

            controlListener->setDecoderID(_decoderId.value);
            eventListener->setDecoderID(_decoderId.value);

            /* Get Audio decoder handle */
            AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

            /* Register events */
            AUDIODECODER_REGISTER_EVENT_SUCCESS(audioDecoder, eventListener);

            /* Get Capabilities of Audio Decoder */
            AUDIODECODER_GETCAPABILITIES(audioDecoder, capabilities);

            mAudioDecoderList.push_back({audioDecoder, controlListener, eventListener, capabilities});
        }
    }

    void checkCompleteStateTransition(sp<IAudioDecoder> decoder, sp<AudioDecoderEventListener> eventListener, AudioDecoderState oldState, AudioDecoderState intermediateState, AudioDecoderState newState)
    {
        AudioDecoderState currentState;

        eventListener->waitForState(intermediateState, std::chrono::milliseconds(MAX_TRANSITORY_STATE_WAIT_MS));

        UT_ASSERT_TRUE(eventListener->checkStateTransistion(oldState, intermediateState));

        eventListener->waitForState(newState, std::chrono::milliseconds(MAX_FINAL_STATE_WAIT_MS));

        AUDIODECODER_GET_STATE(decoder, currentState);

        UT_ASSERT_EQUAL(currentState, newState);

        UT_ASSERT_TRUE(eventListener->checkStateTransistion(intermediateState, newState));

        eventListener->resetStateChanges();
    }

    void TearDown() override
    {
        for (const auto& [decoder, controlListener, eventListener, capabilities] : mAudioDecoderList)
        {
            /* Un-register the events */
            AUDIODECODER_UNREGISTER_EVENT_SUCCESS(decoder, eventListener);
            delete capabilities;
        }
        mAudioDecoderList.clear();
        //TODO: check right place to call joinThreadPool
        //android::IPCThreadState::self()->joinThreadPool();
    }
};

std::multimap<audiodecoder::Property, int32_t> validIntProperties =
{
    {audiodecoder::Property::LOW_LATENCY_MODE, 0},
    {audiodecoder::Property::LOW_LATENCY_MODE, 1},
    {audiodecoder::Property::AV_SOURCE , int32_t(static_cast<int32_t>(AVSource::AUTO))},
    {audiodecoder::Property::AV_SOURCE, int32_t(static_cast<int32_t>(AVSource::UNKNOWN))},
    {audiodecoder::Property::AV_SOURCE, int32_t(static_cast<int32_t>(AVSource::IP))},
    {audiodecoder::Property::AV_SOURCE, int32_t(static_cast<int32_t>(AVSource::TUNER))},
    {audiodecoder::Property::AV_SOURCE, int32_t(static_cast<int32_t>(AVSource::SYSTEM))},
    {audiodecoder::Property::AV_SOURCE, int32_t(static_cast<int32_t>(AVSource::HDMI_1))}
};
std::multimap<audiodecoder::Property, int32_t> validAC4IntProperties =
{
    {audiodecoder::Property::AC4_PRESENTATION_GROUP_INDEX , -1},
    {audiodecoder::Property::AC4_PRESENTATION_GROUP_INDEX , 0},
    {audiodecoder::Property::AC4_ASSOCIATED_TYPE, -1},
    {audiodecoder::Property::AC4_ASSOCIATED_TYPE, 0},
    {audiodecoder::Property::AC4_ASSOCIATED_TYPE, 1},
    {audiodecoder::Property::AC4_ASSOCIATED_TYPE, 2},
    {audiodecoder::Property::AC4_AUTO_SELECTION_PRIORITY, -1},
    {audiodecoder::Property::AC4_AUTO_SELECTION_PRIORITY, 0},
    {audiodecoder::Property::AC4_AUTO_SELECTION_PRIORITY, 1},
    {audiodecoder::Property::AC4_MIXER_BALANCE, 255},
    {audiodecoder::Property::AC4_MIXER_BALANCE, -32},
    {audiodecoder::Property::AC4_MIXER_BALANCE, -12},
    {audiodecoder::Property::AC4_MIXER_BALANCE, 0},
    {audiodecoder::Property::AC4_MIXER_BALANCE, 12},
    {audiodecoder::Property::AC4_MIXER_BALANCE, 32},
    {audiodecoder::Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE, -1},
    {audiodecoder::Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE, 0},
    {audiodecoder::Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE, 1}
};

std::multimap<audiodecoder::Property, android::String16> validAC4StringProperties =
{
    {audiodecoder::Property::AC4_PREFERRED_LANG1, android::String16("")},
    {audiodecoder::Property::AC4_PREFERRED_LANG1, android::String16("eng")},
    {audiodecoder::Property::AC4_PREFERRED_LANG1, android::String16("fra")},
    {audiodecoder::Property::AC4_PREFERRED_LANG2, android::String16("")},
    {audiodecoder::Property::AC4_PREFERRED_LANG2, android::String16("eng")},
    {audiodecoder::Property::AC4_PREFERRED_LANG2, android::String16("fra")}
};

/**
 * @brief Test the ability to read and set properties on audio decoders
 *
 * This test verifies that properties can be set and retrieved correctly on audio decoders.
 * It ensures that the property LOW_LATENCY_MODE can be set to a specific value and then retrieved
 * to confirm the setting was applied correctly.
 *
 * **Test Group ID:** 02@n
 * **Test Case ID:** 001@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L2_AudioDecoder, SetandGetProperties)
{
    for (const auto& [decoder, controlListener, eventListener, capabilities] : mAudioDecoderList)
    {
        for (bool secureDecode : {true, false})
        {
            //Skip if the plafrom doesn't support secure decode
            if (secureDecode && !capabilities->supportsSecure)
            {
                continue;
            }

            for(const auto& _codec: capabilities->supportedCodecs)
            {
                sp<IAudioDecoderController> audioDecoderController;

                /* Open the Decoder */
                AUDIODECODER_OPEN_SUCCESS(decoder, _codec, secureDecode, controlListener, audioDecoderController, true);

                /* Test int property values */
                for (const auto& [property, value] : validIntProperties)
                {
                    PropertyValue setPropertyValue;
                    std::optional<PropertyValue> getPropertyValue;

                    setPropertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(value);

                    /* set the property */
                    AUDIODECODER_SETPROPERTY_SUCCESS(audioDecoderController, property, setPropertyValue);

                    /* get the property */
                    AUDIODECODER_GET_PROPERTY_SUCCESS(decoder, property, getPropertyValue);

                    int32_t getValue = getPropertyValue->value->get<PropertyValue::Value::Tag::intValue>();

                    UT_ASSERT_EQUAL(value, getValue);
                }
                /*Check for AC4 Property only if the decoder support AC4 codec*/
                if (audiodecoder::Codec::DOLBY_AC4 == _codec)
                {
                    /* Test int property values */
                    for (const auto &[property, value] : validAC4IntProperties)
                    {
                        PropertyValue setPropertyValue;
                        std::optional<PropertyValue> getPropertyValue;

                        setPropertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(value);

                        /* set the property */
                        AUDIODECODER_SETPROPERTY_SUCCESS(audioDecoderController, property, setPropertyValue);

                        /* get the property */
                        AUDIODECODER_GET_PROPERTY_SUCCESS(decoder, property, getPropertyValue);

                        int32_t getValue = getPropertyValue->value->get<PropertyValue::Value::Tag::intValue>();

                        UT_ASSERT_EQUAL(value, getValue);
                    }

                    /* Test String property values */
                    for (const auto &[property, value] : validAC4StringProperties)
                    {
                        PropertyValue setPropertyValue;
                        std::optional<PropertyValue> getPropertyValue;

                        setPropertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::stringValue>(value);

                        /* set the property */
                        AUDIODECODER_SETPROPERTY_SUCCESS(audioDecoderController, property, setPropertyValue);

                        /* get the property */
                        AUDIODECODER_GET_PROPERTY_SUCCESS(decoder, property, getPropertyValue);

                        android::String16 getValue = getPropertyValue->value->get<PropertyValue::Value::Tag::stringValue>();

                        UT_ASSERT_EQUAL(value, getValue);
                    }
                }
                /* Close the decoder */
                AUDIODECODER_CLOSE_SUCCESS(decoder, audioDecoderController, true);
            }
        }
    }
}

/**
 * @brief Test the ability to read and set properties on multi instances of audio decoder
 *
 * This test verifies that properties can be set and retrieved correctly on audio decoders.
 * It ensures that the property LOW_LATENCY_MODE can be set to a specific value and then
 * retrieved to confirm the setting was applied correctly.
 *
 * **Test Group ID:** 02@n
 * **Test Case ID:** 002@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L2_AudioDecoder, SetandGetProperties_multipleInstance)
{
    std::vector<sp<IAudioDecoderController>> audioDecoderControllersList;

    for (bool secureDecode : {true, false})
    {
        for (const auto& [decoder, controlListener, eventListener, capabilities] : mAudioDecoderList)
        {
            audiodecoder::Codec _codec = capabilities->supportedCodecs[0];
            sp<IAudioDecoderController> audioDecoderController;

            //Skip if the plafrom doesn't support secure decode
            if (secureDecode && !capabilities->supportsSecure)
            {
                audioDecoderControllersList.push_back(nullptr);
                continue;
            }
            /* Open the Decoder */
            AUDIODECODER_OPEN_SUCCESS(decoder, _codec, secureDecode, controlListener, audioDecoderController, true);

            audioDecoderControllersList.push_back(audioDecoderController);
        }

        for (size_t i = 0; i < mAudioDecoderList.size(); ++i)
        {
            const auto& [decoder, controlListener, eventListener, capabilities] = mAudioDecoderList[i];
            const auto& audioDecoderController = audioDecoderControllersList[i];

            //Skip if the plafrom doesn't support secure decode
            if (secureDecode && !capabilities->supportsSecure)
            {
                continue;
            }

            /* Test int property values */
            for (const auto& [property, value] : validIntProperties)
            {
                PropertyValue setPropertyValue;
                std::optional<PropertyValue> getPropertyValue;

                setPropertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(value);

                /* set the property */
                AUDIODECODER_SETPROPERTY_SUCCESS(audioDecoderController, property, setPropertyValue);

                /* get the property */
                AUDIODECODER_GET_PROPERTY_SUCCESS(decoder, property, getPropertyValue);

                int32_t getValue = getPropertyValue->value->get<PropertyValue::Value::Tag::intValue>();

                UT_ASSERT_EQUAL(value, getValue);
            }
            /*Check if decoder supports AC4 codec*/
            if (std::find(capabilities->supportedCodecs.begin(), capabilities->supportedCodecs.end(),
                          audiodecoder::Codec::DOLBY_AC4) != capabilities->supportedCodecs.end())
            {
                for (const auto &[property, value] : validAC4IntProperties)
                {
                    PropertyValue setPropertyValue;
                    std::optional<PropertyValue> getPropertyValue;

                    setPropertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(value);

                    /* set the property */
                    AUDIODECODER_SETPROPERTY_SUCCESS(audioDecoderController, property, setPropertyValue);

                    /* get the property */
                    AUDIODECODER_GET_PROPERTY_SUCCESS(decoder, property, getPropertyValue);

                    int32_t getValue = getPropertyValue->value->get<PropertyValue::Value::Tag::intValue>();

                    UT_ASSERT_EQUAL(value, getValue);
                }

                /* Test String property values */
                for (const auto &[property, value] : validAC4StringProperties)
                {
                    PropertyValue setPropertyValue;
                    std::optional<PropertyValue> getPropertyValue;

                    setPropertyValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::stringValue>(value);

                    /* set the property */
                    AUDIODECODER_SETPROPERTY_SUCCESS(audioDecoderController, property, setPropertyValue);

                    /* get the property */
                    AUDIODECODER_GET_PROPERTY_SUCCESS(decoder, property, getPropertyValue);

                    android::String16 getValue = getPropertyValue->value->get<PropertyValue::Value::Tag::stringValue>();

                    UT_ASSERT_EQUAL(value, getValue);
                }
            }
        }

        for (size_t i = 0; i < mAudioDecoderList.size(); ++i)
        {
            const auto& [decoder, controlListener, eventListener, capabilities] = mAudioDecoderList[i];
            const auto& audioDecoderController = audioDecoderControllersList[i];

            //Skip if the plafrom doesn't support secure decode
            if (secureDecode && !capabilities->supportsSecure)
            {
                continue;
            }

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(decoder, audioDecoderController, true);
        }

        audioDecoderControllersList.clear();
    }
}

/**
 * @brief Test the state transitions of the audio decoder
 *
 * This test verifies the correct state transitions of the audio decoder,
 * including registration, opening, starting, flushing, stopping, closing, and unregistering.
 * It ensures that each operation returns a successful status and the expected state changes occur.
 *
 * **Test Group ID:** 02@n
 * **Test Case ID:** 003@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L2_AudioDecoder, StateTransitions)
{
    for (const auto& [decoder, controlListener, eventListener, capabilities] : mAudioDecoderList)
    {
        for (bool secureDecode : {true, false})
        {
            //Skip if the plafrom doesn't support secure decode
            if (secureDecode && !capabilities->supportsSecure)
            {
                continue;
            }

            for(const auto& _codec: capabilities->supportedCodecs)
            {
                sp<IAudioDecoderController> audioDecoderController;

                /* Open the Decoder */
                AUDIODECODER_OPEN_SUCCESS(decoder, _codec, secureDecode, controlListener, audioDecoderController, false);
                checkCompleteStateTransition(decoder, eventListener, AudioDecoderState::CLOSED, AudioDecoderState::OPENING, AudioDecoderState::READY);

                /* Start the decoder */
                AUDIODECODER_START_SUCCESS(decoder, audioDecoderController, false);
                checkCompleteStateTransition(decoder, eventListener, AudioDecoderState::READY, AudioDecoderState::STARTING, AudioDecoderState::STARTED);

                /* Flush the decoder with reset flag false*/
                AUDIODECODER_FLUSH_SUCCESS(decoder, audioDecoderController, false, false);
                checkCompleteStateTransition(decoder, eventListener, AudioDecoderState::STARTED, AudioDecoderState::FLUSHING, AudioDecoderState::STARTED);

                /* Flush the decoder with reset flag true*/
                AUDIODECODER_FLUSH_SUCCESS(decoder, audioDecoderController, true, false);
                checkCompleteStateTransition(decoder, eventListener, AudioDecoderState::STARTED, AudioDecoderState::FLUSHING, AudioDecoderState::READY);

                /* Start the decoder */
                AUDIODECODER_START_SUCCESS(decoder, audioDecoderController, false);
                checkCompleteStateTransition(decoder, eventListener, AudioDecoderState::READY, AudioDecoderState::STARTING, AudioDecoderState::STARTED);

                /* Stop the decoder */
                AUDIODECODER_STOP_SUCCESS(decoder, audioDecoderController, false);
                checkCompleteStateTransition(decoder, eventListener, AudioDecoderState::STARTED, AudioDecoderState::STOPPING, AudioDecoderState::READY);

                /* Close the decoder */
                AUDIODECODER_CLOSE_SUCCESS(decoder, audioDecoderController, false);
                checkCompleteStateTransition(decoder, eventListener, AudioDecoderState::READY, AudioDecoderState::CLOSING, AudioDecoderState::CLOSED);
            }
        }
    }
}

/**
 * @brief Test the state transitions of multiple instances of audio decoders.
 *
 * This test verifies the correct state transitions of the audio decoder,
 * including registration, opening, starting, flushing, stopping, closing, and unregistering.
 * It ensures that each operation returns a successful status and the expected state changes occur.
 *
 * **Test Group ID:** 02@n
 * **Test Case ID:** 004@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L2_AudioDecoder, StateTransitions_mutipleInstance)
{
    std::vector<sp<IAudioDecoderController>> audioDecoderControllersList;

    for (bool secureDecode : {true, false})
    {
        for (const auto& [decoder, controlListener, eventListener, capabilities] : mAudioDecoderList)
        {
            audiodecoder::Codec _codec = capabilities->supportedCodecs[0];
            sp<IAudioDecoderController> audioDecoderController;

            //Skip if the plafrom doesn't support secure decode
            if (secureDecode && !capabilities->supportsSecure)
            {
                audioDecoderControllersList.push_back(nullptr);
                continue;
            }

            /* Open the Decoder */
            AUDIODECODER_OPEN_SUCCESS(decoder, _codec, secureDecode, controlListener, audioDecoderController, false);
            checkCompleteStateTransition(decoder, eventListener,AudioDecoderState::CLOSED,AudioDecoderState::OPENING,AudioDecoderState::READY);

            audioDecoderControllersList.push_back(audioDecoderController);
        }

        for (size_t i = 0; i < mAudioDecoderList.size(); ++i)
        {
            const auto& [decoder, controlListener, eventListener, capabilities] = mAudioDecoderList[i];
            const auto& audioDecoderController = audioDecoderControllersList[i];

            //Skip if the plafrom doesn't support secure decode
            if (secureDecode && !capabilities->supportsSecure)
            {
                continue;
            }

            /* Start the decoder */
            AUDIODECODER_START_SUCCESS(decoder, audioDecoderController, false);
            checkCompleteStateTransition(decoder, eventListener,AudioDecoderState::READY,AudioDecoderState::STARTING,AudioDecoderState::STARTED);

            /* Flush the decoder with reset flag false*/
            AUDIODECODER_FLUSH_SUCCESS(decoder, audioDecoderController, false, false);
            checkCompleteStateTransition(decoder, eventListener,AudioDecoderState::STARTED,AudioDecoderState::FLUSHING,AudioDecoderState::STARTED);

            /* Flush the decoder with reset flag true*/
            AUDIODECODER_FLUSH_SUCCESS(decoder, audioDecoderController, true, false);
            checkCompleteStateTransition(decoder, eventListener,AudioDecoderState::STARTED,AudioDecoderState::FLUSHING,AudioDecoderState::READY);

            /* Start the decoder */
            AUDIODECODER_START_SUCCESS(decoder, audioDecoderController, false);
            checkCompleteStateTransition(decoder, eventListener,AudioDecoderState::READY,AudioDecoderState::STARTING,AudioDecoderState::STARTED);

            /* Stop the decoder */
            AUDIODECODER_STOP_SUCCESS(decoder, audioDecoderController, false);
            checkCompleteStateTransition(decoder, eventListener,AudioDecoderState::STARTED,AudioDecoderState::STOPPING,AudioDecoderState::READY);
        }

        for (size_t i = 0; i < mAudioDecoderList.size(); ++i)
        {
            const auto& [decoder, controlListener, eventListener, capabilities] = mAudioDecoderList[i];
            const auto& audioDecoderController = audioDecoderControllersList[i];

            //Skip if the plafrom doesn't support secure decode
            if (secureDecode && !capabilities->supportsSecure)
            {
                continue;
            }

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(decoder, audioDecoderController, false);
            checkCompleteStateTransition(decoder, eventListener,AudioDecoderState::READY,AudioDecoderState::CLOSING,AudioDecoderState::CLOSED);
        }

        audioDecoderControllersList.clear();
    }
}

/**
 * @brief Validate signalEndOfStream lifecycle and recovery.
 *
 * Verifies EOS callback delivery, STARTED->STARTED drained behavior,
 * post-EOS decode rejection, second call no-op, and recovery via flush(reset=true).
 */
UT_ADD_TEST(VTS_L2_AudioDecoder, SignalEndOfStreamLifecycle)
{
    sp<IAVBuffer> avBufferManager = vts_audiodecoder_l2_getAvBufferService();
    UT_ASSERT_TRUE_FATAL(avBufferManager != nullptr);

    IAVBufferHelper* avBufferHelper = getAVBufferHelperInstance();
    UT_ASSERT_TRUE_FATAL(avBufferHelper != nullptr);

    for (const auto& [decoder, controlListener, eventListener, capabilities] : mAudioDecoderList)
    {
        for (bool secureDecode : {true, false})
        {
            if (secureDecode && !capabilities->supportsSecure)
            {
                continue;
            }

            for (const auto& _codec : capabilities->supportedCodecs)
            {
                sp<IAudioDecoderController> audioDecoderController;
                Pool audioPool;
                int64_t bufferHandle = -1;
                int64_t postEosBufferHandle = -1;
                sp<IAVBufferSpaceListener> bufferListener = new L2AVBufferSpaceListener();

                AUDIODECODER_OPEN_SUCCESS(decoder, _codec, secureDecode, controlListener, audioDecoderController, false);
                checkCompleteStateTransition(decoder, eventListener, AudioDecoderState::CLOSED, AudioDecoderState::OPENING, AudioDecoderState::READY);

                /* Get resource ID and set up pool in READY state before start */
                IAudioDecoder::Id decoderId;
                std::optional<PropertyValue> decoderIdValue;
                AUDIODECODER_GET_PROPERTY_SUCCESS(decoder, audiodecoder::Property::RESOURCE_ID, decoderIdValue);
                decoderId.value = decoderIdValue->value->get<PropertyValue::Value::Tag::intValue>();

                AVBUFFER_CREATE_AUDIOPOOL_SUCCESS(avBufferManager, secureDecode, decoderId, bufferListener, audioPool);

                /* Set AV source and audio format in READY state (required by HAL before decode) */
                PropertyValue avSourceValue;
                avSourceValue.value = PropertyValue::Value::make<PropertyValue::Value::Tag::intValue>(int32_t(static_cast<int32_t>(AVSource::IP)));
                AUDIODECODER_SETPROPERTY_SUCCESS(audioDecoderController, audiodecoder::Property::AV_SOURCE, avSourceValue);
                AUDIODECODER_SETAUDIOFORMAT_SUCCESS(audioDecoderController, 2, 48000);

                AUDIODECODER_START_SUCCESS(decoder, audioDecoderController, false);
                checkCompleteStateTransition(decoder, eventListener, AudioDecoderState::READY, AudioDecoderState::STARTING, AudioDecoderState::STARTED);

                AVBUFFER_ALLOC_SUCCESS(avBufferManager, audioPool, 1, bufferHandle);

                /* Write a fixed invalid byte; left unwritten, the byte is random. 0x02 is an Opus TOC
                 * with code 2, which needs a frame length byte that is missing (RFC 6716 sec 3.2.4),
                 * and it carries no sync word or header for any other codec, so every compressed
                 * decoder must reject it (verified with libopus and both ffmpeg Opus decoders). */
                uint8_t invalidByte = 0x02;
                if (secureDecode)
                {
                    UT_ASSERT_TRUE(avBufferHelper->writeSecureHandle(bufferHandle, &invalidByte, 1));
                }
                else
                {
                    uint32_t mapSize = 0;
                    void* mapBuffer = avBufferHelper->mapHandle(bufferHandle, &mapSize);
                    UT_ASSERT_TRUE(mapBuffer != nullptr && mapSize >= 1);
                    if (mapBuffer != nullptr)
                    {
                        if (mapSize >= 1)
                        {
                            memcpy(mapBuffer, &invalidByte, 1);
                        }
                        avBufferHelper->unmapHandle(bufferHandle);
                    }
                }

                audiodecoder::InputBufferMetadata metadata;
                metadata.nsPresentationTime = 1000000;
                metadata.discontinuity = false;
                metadata.trimStartNs = 0;
                metadata.trimEndNs = 0;

                eventListener->resetErrors();

                AUDIODECODER_DECODE_WITH_METADATA_SUCCESS(decoderId.value, audioDecoderController, bufferHandle, metadata);

                controlListener->resetEndOfStream();
                AUDIODECODER_SIGNAL_EOS_SUCCESS(decoder, audioDecoderController, false);
                UT_ASSERT_TRUE(controlListener->waitForEndOfStream(MAX_FINAL_STATE_WAIT_MS));

                AudioDecoderState state;
                AUDIODECODER_GET_STATE(decoder, state);
                UT_ASSERT_EQUAL(state, AudioDecoderState::STARTED);

                /* Second signalEndOfStream() must be a no-op — spec mandates no exception */
                AUDIODECODER_SIGNAL_EOS_SUCCESS(decoder, audioDecoderController, false);

                AVBUFFER_ALLOC_SUCCESS(avBufferManager, audioPool, 1, postEosBufferHandle);
                AUDIODECODER_DECODE_WITH_METADATA_FAILURE(audioDecoderController, postEosBufferHandle, metadata, Status::EX_ILLEGAL_STATE);

                AUDIODECODER_FLUSH_SUCCESS(decoder, audioDecoderController, true, false);
                checkCompleteStateTransition(decoder, eventListener, AudioDecoderState::STARTED, AudioDecoderState::FLUSHING, AudioDecoderState::READY);

                AUDIODECODER_START_SUCCESS(decoder, audioDecoderController, false);
                checkCompleteStateTransition(decoder, eventListener, AudioDecoderState::READY, AudioDecoderState::STARTING, AudioDecoderState::STARTED);

                AUDIODECODER_STOP_SUCCESS(decoder, audioDecoderController, false);
                checkCompleteStateTransition(decoder, eventListener, AudioDecoderState::STARTED, AudioDecoderState::STOPPING, AudioDecoderState::READY);

                AUDIODECODER_CLOSE_SUCCESS(decoder, audioDecoderController, false);
                checkCompleteStateTransition(decoder, eventListener, AudioDecoderState::READY, AudioDecoderState::CLOSING, AudioDecoderState::CLOSED);

                /* Free every buffer VTS still holds, so destroyPool() succeeds. */

                // Output frames: onEndOfStream() followed the last onFrameOutput(), so all are queued.
                // Every callback counts as output; a -1 handle is tunnelled output with no buffer to free.
                int32_t outputFramesReceived = 0;
                DecodedAudioFrame_t decodedFrame;
                while (controlListener->getNextFrame(decodedFrame, 0, true))
                {
                    outputFramesReceived++;
                    UT_LOG_WARNING("[Codec: %s] onFrameOutput received for invalid 1-byte input: PTS=%lld, handle=0x%llx",
                                   audiodecoder::toString(_codec).c_str(), static_cast<long long>(decodedFrame.framePTS),
                                   static_cast<unsigned long long>(decodedFrame.frameBufferHandle));
                    if (decodedFrame.frameBufferHandle == -1)
                    {
                        continue;
                    }
                    bool isOutputBufferValid = false;
                    AVBUFFER_ISVALID_SUCCESS(avBufferManager, decodedFrame.frameBufferHandle, isOutputBufferValid);
                    if (isOutputBufferValid)
                    {
                        AVBUFFER_FREE_SUCCESS(avBufferManager, decodedFrame.frameBufferHandle);
                    }
                }

                // Post-EOS buffer: decode rejected it, so ownership stayed with VTS.
                AVBUFFER_FREE_SUCCESS(avBufferManager, postEosBufferHandle);

                // 1-byte input buffer: the HAL owns it once decode accepts it, so it must be freed by close().
                // Still valid here means the HAL leaked it.
                bool isInputBufferValid = false;
                AVBUFFER_ISVALID_SUCCESS(avBufferManager, bufferHandle, isInputBufferValid);
                if (isInputBufferValid)
                {
                    UT_LOG_ERROR("[Codec: %s] HAL leaked accepted input buffer 0x%llx: still valid after close",
                                 audiodecoder::toString(_codec).c_str(), static_cast<unsigned long long>(bufferHandle));
                }
                UT_ASSERT_TRUE(!isInputBufferValid);
                if (isInputBufferValid)
                {
                    // Cleanup only, after the failed assert, so destroyPool() can succeed
                    AVBUFFER_FREE_SUCCESS(avBufferManager, bufferHandle);
                }

                // The 1-byte input is not a valid frame: a compressed decoder must report an error and
                // must not output a frame. PCM has no decode step, so it is excluded.
                if (_codec != audiodecoder::Codec::PCM)
                {
                    auto errorWaitEnd = chrono::steady_clock::now() + chrono::milliseconds(MAX_FINAL_STATE_WAIT_MS);
                    while (eventListener->getErrorCount() == 0 && chrono::steady_clock::now() < errorWaitEnd)
                    {
                        this_thread::sleep_for(chrono::milliseconds(10));
                    }
                    UT_ASSERT_EQUAL(outputFramesReceived, 0)
                        << "Codec " << audiodecoder::toString(_codec) << " output a frame for invalid 1-byte input";
                    UT_ASSERT_TRUE(eventListener->getErrorCount() > 0)
                        << "Codec " << audiodecoder::toString(_codec) << " did not report onDecodeError for invalid 1-byte input";
                }

                AVBUFFER_DESTROY_POOL_SUCCESS(avBufferManager, audioPool);
            }
        }
    }
}

/** @} */ // End of AUDIO_DECODER_HALTEST L2 File
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER Module
/** @} */ // End of HPK
