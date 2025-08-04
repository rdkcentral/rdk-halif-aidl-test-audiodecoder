/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
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

#include <ut.h>
#include <ut_log.h>
#include <ut_kvp_profile.h>
#include <ut_control_plane.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include <binder/Status.h>
#include <utils/String16.h>
#include <stdio.h>
#include <vector>
#include <optional>
#include <algorithm>
#include <unordered_set>
#include <tuple>
#include <map>

#include "vts_audiodecoder_helper_functions.h"
#include "vts_audiodecoder_parse_hfp.h"
#include "vts_audiodecoder_parse_testconfig.h"
#include "vts_audiodecoder.h"

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

    void checkCompleteStateTransition(sp<IAudioDecoder> decoder, sp<AudioDecoderEventListener> eventListener, State oldState, State intermediateState, State newState)
    {
        ::com::rdk::hal::State currentState;

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

std::multimap<Property, int32_t> validIntProperties =
{
    {Property::LOW_LATENCY_MODE, 0},
    {Property::LOW_LATENCY_MODE, 1},
    {Property::AV_SOURCE , int32_t(static_cast<int32_t>(AVSource::AUTO))},
    {Property::AV_SOURCE, int32_t(static_cast<int32_t>(AVSource::UNKNOWN))},
    {Property::AV_SOURCE, int32_t(static_cast<int32_t>(AVSource::IP))},
    {Property::AV_SOURCE, int32_t(static_cast<int32_t>(AVSource::TUNER))},
    {Property::AV_SOURCE, int32_t(static_cast<int32_t>(AVSource::SYSTEM))},
    {Property::AV_SOURCE, int32_t(static_cast<int32_t>(AVSource::HDMI_1))}
};
std::multimap<Property, int32_t> validAC4IntProperties =
{
    {Property::AC4_PRESENTATION_GROUP_INDEX , -1},
    {Property::AC4_PRESENTATION_GROUP_INDEX , 0},
    {Property::AC4_ASSOCIATED_TYPE, -1},
    {Property::AC4_ASSOCIATED_TYPE, 0},
    {Property::AC4_ASSOCIATED_TYPE, 1},
    {Property::AC4_ASSOCIATED_TYPE, 2},
    {Property::AC4_AUTO_SELECTION_PRIORITY, -1},
    {Property::AC4_AUTO_SELECTION_PRIORITY, 0},
    {Property::AC4_AUTO_SELECTION_PRIORITY, 1},
    {Property::AC4_MIXER_BALANCE, 255},
    {Property::AC4_MIXER_BALANCE, -32},
    {Property::AC4_MIXER_BALANCE, -12},
    {Property::AC4_MIXER_BALANCE, 0},
    {Property::AC4_MIXER_BALANCE, 12},
    {Property::AC4_MIXER_BALANCE, 32},
    {Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE, -1},
    {Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE, 0},
    {Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE, 1}
};

std::multimap<Property, android::String16> validAC4StringProperties =
{
    {Property::AC4_PREFERRED_LANG1, android::String16("")},
    {Property::AC4_PREFERRED_LANG1, android::String16("eng")},
    {Property::AC4_PREFERRED_LANG1, android::String16("fra")},
    {Property::AC4_PREFERRED_LANG2, android::String16("")},
    {Property::AC4_PREFERRED_LANG2, android::String16("eng")},
    {Property::AC4_PREFERRED_LANG1, android::String16("fra")}
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
                if (Codec::DOLBY_AC4 == _codec)
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
            Codec _codec = capabilities->supportedCodecs[0];
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
                          Codec::DOLBY_AC4) != capabilities->supportedCodecs.end())
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
                checkCompleteStateTransition(decoder, eventListener, State::CLOSED, State::OPENING, State::READY);

                /* Start the decoder */
                AUDIODECODER_START_SUCCESS(decoder, audioDecoderController, false);
                checkCompleteStateTransition(decoder, eventListener, State::READY, State::STARTING, State::STARTED);

                /* Flush the decoder with reset flag false*/
                AUDIODECODER_FLUSH_SUCCESS(decoder, audioDecoderController, false, false);
                checkCompleteStateTransition(decoder, eventListener, State::STARTED, State::FLUSHING, State::STARTED);

                /* Flush the decoder with reset flag true*/
                AUDIODECODER_FLUSH_SUCCESS(decoder, audioDecoderController, true, false);
                checkCompleteStateTransition(decoder, eventListener, State::STARTED, State::FLUSHING, State::READY);

                /* Start the decoder */
                AUDIODECODER_START_SUCCESS(decoder, audioDecoderController, false);
                checkCompleteStateTransition(decoder, eventListener, State::READY, State::STARTING, State::STARTED);

                /* Stop the decoder */
                AUDIODECODER_STOP_SUCCESS(decoder, audioDecoderController, false);
                checkCompleteStateTransition(decoder, eventListener, State::STARTED, State::STOPPING, State::READY);

                /* Close the decoder */
                AUDIODECODER_CLOSE_SUCCESS(decoder, audioDecoderController, false);
                checkCompleteStateTransition(decoder, eventListener, State::READY, State::CLOSING, State::CLOSED);
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
            Codec _codec = capabilities->supportedCodecs[0];
            sp<IAudioDecoderController> audioDecoderController;

            //Skip if the plafrom doesn't support secure decode
            if (secureDecode && !capabilities->supportsSecure)
            {
                audioDecoderControllersList.push_back(nullptr);
                continue;
            }

            /* Open the Decoder */
            AUDIODECODER_OPEN_SUCCESS(decoder, _codec, secureDecode, controlListener, audioDecoderController, false);
            checkCompleteStateTransition(decoder, eventListener, State::CLOSED, State::OPENING, State::READY);

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
            checkCompleteStateTransition(decoder, eventListener, State::READY, State::STARTING, State::STARTED);

            /* Flush the decoder with reset flag false*/
            AUDIODECODER_FLUSH_SUCCESS(decoder, audioDecoderController, false, false);
            checkCompleteStateTransition(decoder, eventListener, State::STARTED, State::FLUSHING, State::STARTED);

            /* Flush the decoder with reset flag true*/
            AUDIODECODER_FLUSH_SUCCESS(decoder, audioDecoderController, true, false);
            checkCompleteStateTransition(decoder, eventListener, State::STARTED, State::FLUSHING, State::READY);

            /* Start the decoder */
            AUDIODECODER_START_SUCCESS(decoder, audioDecoderController, false);
            checkCompleteStateTransition(decoder, eventListener, State::READY, State::STARTING, State::STARTED);

            /* Stop the decoder */
            AUDIODECODER_STOP_SUCCESS(decoder, audioDecoderController, false);
            checkCompleteStateTransition(decoder, eventListener, State::STARTED, State::STOPPING, State::READY);
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
            checkCompleteStateTransition(decoder, eventListener, State::READY, State::CLOSING, State::CLOSED);
        }

        audioDecoderControllersList.clear();
    }
}

/** @} */ // End of AUDIO_DECODER_HALTEST L2 File
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER Module
/** @} */ // End of HPK