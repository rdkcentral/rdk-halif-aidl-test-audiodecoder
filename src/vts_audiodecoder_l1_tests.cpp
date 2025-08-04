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
 * @defgroup AUDIO_DECODER_HALTEST_L1 Audio Decoder HAL Tests L1 File
 * @{
 * @parblock
 *
 * ### L1 Test Cases for Audio Decoder HAL :
 *
 * This module includes Level 1 functional tests (success and failure scenarios).
 * This is to ensure that the API meets the operational requirements of the module across all vendors.
 *
 * **Pre-Conditions:** @n
 * **Dependencies:** None@n
 *
 * @endparblock
 *
 */

/**
 * @file vts_audiodecoder_l1_tests.c
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
#include <map>

#include "vts_audiodecoder_helper_functions.h"
#include "vts_audiodecoder_parse_hfp.h"
#include "vts_audiodecoder_parse_testconfig.h"
#include "vts_audiodecoder.h"

/* Test class registration with UT_Core */
UT_ADD_TEST_TO_GROUP(VTS_L1_AudioDecoder, UT_TESTS_L1)

class VTS_L1_AudioDecoder : public UTCore
{
public:
    VTS_L1_AudioDecoder() : UTCore() {}

    ~VTS_L1_AudioDecoder() override = default;

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

UT_status_t vts_audiodecoder_getAudioDecoderService()
{
    gAudioDecoderManager = android::interface_cast<IAudioDecoderManager>(
            defaultServiceManager()->getService(String16(IAudioDecoderManager::serviceName().c_str())));
    if (gAudioDecoderManager == nullptr)
    {
        UT_LOG_ERROR("AudioDecoder Service not available");
        return UT_STATUS_FAILURE;
    }
    return UT_STATUS_OK;
}

/**
* @brief Tests the retrieval of audio decoder IDs to ensure the function fetches valid and unique IDs.
*
* This test verifies that the service's getAudioDecoderIds function properly retrieves a list of audio decoder IDs,
* ensuring they are unique and correspond to expected values defined in the HAL FEATURE PROFILE. Also verifies
* they are consistent across retrievals.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 001@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Retrieve the initial audio decoder IDs|None|Should retrieve a non-empty list of IDs|Should Pass|
* |02|Retrieve the second set of audio IDs|None|Should retrieve a non-empty list of IDs|Should Pass|
* |03|Ensure the sizes and values of both lists are equal|initialDecoderIds.size(), secondDecoderIDs.size(), initialDecoderIds[i].value, secondDecoderIds[i].value|Sizes and values should match|Should Pass|
* |04|Get the supported decoder IDs from the profile file|Profile file|Profile should contain supported decoder IDs|Should Pass|
* |05|Ensure the sizes and values of profile file and inital set should match|initialDecoderIds.size(), initialDecoderIds[i].value|Sizes and values should match|Should Pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, getAudioDecoderIds_pos)
{
    int i = 0;
    std::vector<IAudioDecoder::Id> initialDecoderIds;
    std::vector<IAudioDecoder::Id> secondDecoderIds;
    std::vector<int32_t> decoderId;
    std::unordered_set<int> decoderId_from_profile;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(initialDecoderIds);

    for (const auto& obj : initialDecoderIds)
    {
        decoderId.push_back(obj.value);
    }

    UT_ASSERT_TRUE(vts_audiodecoder_helper_areElementsUnique(decoderId));

    /* Get the Audio decoder IDs second time */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(secondDecoderIds);

    UT_ASSERT_EQUAL(initialDecoderIds.size(), secondDecoderIds.size());

    for (size_t i = 0; i < initialDecoderIds.size(); ++i)
    {
        UT_ASSERT_EQUAL(initialDecoderIds[i].value, secondDecoderIds[i].value);
    }

    /* checking the number of audio decoder ids from the api getAudioDecoderIds()
       equal to the number of audio decoder ids from HAL FEATURE PROFILE  */
    UT_ASSERT_EQUAL(initialDecoderIds.size(), gAudiodecoders_hfp.size());

    for (const auto& obj : gAudiodecoders_hfp)
    {
        decoderId_from_profile.insert(obj.id);
    }

    /* checking whether retrieved audio decoder ids from the api getAudioDecoderIds()
       is listed in the HAL FEATURE PROFILE */
    for (int num : decoderId)
    {
        UT_ASSERT_NOT_EQUAL(decoderId_from_profile.find(num), decoderId_from_profile.end());
    }
}

/**
* @brief Test case for verifying the positive scenario of getAudioDecoder functionality
*
* This test case checks the getAudioDecoder method of the IAudioDecoder service for all available decoder
* id and ensures that the returned decoder object is valid and the status is OK. Also verifies
* they are consistent across retrievals
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 002@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Retrieve the initial audio decoder IDs|None|Should retrieve a non-empty list of IDs|Should Pass|
* |02|Loop through each decoder in the decoder list and get deoder instance|id, audiodecoder|Status is OK and audiodecoder is not NULL|Should Pass|
* |03|Invoke getAudioDecoder again with same id; check status|id, audiodecoder|Status is OK and audiodecoder is not NULL|Should Pass|
* |04|Compare RESOURCE_ID from both invocations|resource_id_1, resource_id_2|RESOURCE_IDs are equal|Should Pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, getAudioDecoder_pos)
{
    sp<IAudioDecoder> audiodecoder_1;
    sp<IAudioDecoder> audiodecoder_2;
    IAudioDecoder::Id id ;
    std::optional<PropertyValue> resource_id_1;
    std::optional<PropertyValue> resource_id_2;
    std::vector<IAudioDecoder::Id> decoderIds;
    Property property = Property::RESOURCE_ID;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        Status status;

        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audiodecoder_1);
        /* Get the resource ID */
        AUDIODECODER_GET_PROPERTY_SUCCESS(audiodecoder_1, property, resource_id_1);

        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audiodecoder_2);
        /* Get the resource ID */
        AUDIODECODER_GET_PROPERTY_SUCCESS(audiodecoder_2, property, resource_id_2);

        int32_t _resource_id_1 = resource_id_1->value->get<PropertyValue::Value::Tag::intValue>();
        int32_t _resource_id_2 = resource_id_2->value->get<PropertyValue::Value::Tag::intValue>();

        UT_ASSERT_EQUAL(_resource_id_1, _resource_id_2);
    }
}

/**
* @brief Test to verify behavior of getAudioDecoder with invalid ID
*
* This test checks if getAudioDecoder returns the appropriate error when provided with an invalid decoder ID
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 003@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Retrieve the initial audio decoder IDs|None|Should retrieve a non-empty list of IDs|Should Pass|
* |02|Get audio decoder with invalid id (min id value - 1)|(min id value - 1), audiodecoder|status.isOk() should be false and audiodecoder should be nullptr|Should Pass|
* |03|Get audio decoder with invalid id (max id value + 1)|(max id value + 1), audiodecoder|status.isOk() should be false and audiodecoder should be nullptr|Should Pass|
* |04|Get audio decoder for UNDEFINED ID|UNDEFINED, audiodecoder|status.isOk() should be false and audiodecoder should be nullptr|Should Pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, getAudioDecoder_neg)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    std::vector<int> decoderId_from_profile;
    IAudioDecoder::Id invalidID;
    sp<IAudioDecoder> audiodecoder;
    ::android::binder::Status status;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    if(decoderIds.size() > 1)
        std::sort(decoderIds.begin(), decoderIds.end());

    invalidID = decoderIds.front();
    invalidID.value -= 1;
    AUDIODECODER_MANAGER_GETAUDIODECODER_FAILURE(invalidID, audiodecoder);

    invalidID = decoderIds.back();
    invalidID.value += 1;
    AUDIODECODER_MANAGER_GETAUDIODECODER_FAILURE(invalidID, audiodecoder);

    invalidID.value = IAudioDecoder::Id::UNDEFINED;
    AUDIODECODER_MANAGER_GETAUDIODECODER_FAILURE(invalidID, audiodecoder);
}

/**
* @brief Test positive case for getting audio decoder capabilities
*
* This test verifies the successful retrieval of capabilities from the audio decoder interface.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 004@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Invoke getCapabilities() with the Capabilities instance for all supported audiodecoders|Capabilities instance|Status should be ok|Should Pass|
* |02|Validate the capabilities obtained with HFP|Capabilities instance|Successful validation of capabilities|Should Pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, getCapabilities_pos)
{
    Capabilities capabilities;
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Get Capabilities of Audio Decoder */
        AUDIODECODER_GETCAPABILITIES(audioDecoder, &capabilities);

        vts_audiodecoder_helper_validateCapabilities(_decoderId.value, capabilities);
    }
}

/**
* @brief Test to check the positive scenarios for retrieving various audio decoder properties.
*
* This test iterates over a list of audio decoder properties and verifies that the getProperty
* method of the audioDecoder object correctly retrieves the property values. It ensures that
* the status is OK and the property value matches the expected criteria.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 005@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Initialize audioDecoder object and iterate through the properties list for supported audio decoders|decoder, property list[RESOURCE_ID, SECURE_AUDIO, METRIC_FRAMES_DECODED, METRIC_DECODE_ERRORS, METRIC_FRAMES_DROPPED]|Status should be ok|Should Pass|
* |02|Validate the property obtained with HFP|property value|Successful validation of property|Should Pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, getProperty_pos)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    std::vector<Property> testProperties = {Property::RESOURCE_ID, Property::LOW_LATENCY_MODE, Property::AV_SOURCE, Property::SECURE_AUDIO,
                                            Property::AC4_PRESENTATION_GROUP_INDEX, Property::AC4_PREFERRED_LANG1, Property::AC4_PREFERRED_LANG2,
                                            Property::AC4_ASSOCIATED_TYPE, Property::AC4_AUTO_SELECTION_PRIORITY, Property::AC4_MIXER_BALANCE,
                                            Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE, Property::METRIC_FRAMES_DECODED,
                                            Property::METRIC_DECODE_ERRORS, Property::METRIC_FRAMES_DROPPED};
    ::android::binder::Status status;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Get RESOURCE_ID */
        for (auto property : testProperties)
        {
            std::optional<PropertyValue> propertyValue;
            AUDIODECODER_GET_PROPERTY_SUCCESS(audioDecoder, property, propertyValue);
            UT_ASSERT_TRUE(vts_audiodecoder_helper_validatePropertyValue(_decoderId.value, propertyValue, property));
        }
    }
}

/**
* @brief Tests the behavior of the audioDecoder's getProperty function with an invalid property.
*
* This test validates that the getProperty function of the audioDecoder handles invalid properties
* correctly by passing a non-existent property ID.
* This ensures robustness by verifying the function’s response to unexpected or erroneous input.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 006@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Call getProperty function with invalid property value for all supported decoders|decoder, invalid property|Status should be invalid|Should Pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, getProperty_neg)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    std::optional<PropertyValue> propertyValue;
    int32_t firstValue = static_cast<int32_t>(Property::RESOURCE_ID);
    Property Invalidproperty = static_cast<Property>(firstValue - 1);
    ::android::binder::Status status;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        AUDIODECODER_GET_PROPERTY_FAILURE(audioDecoder, Invalidproperty, propertyValue, Status::EX_ILLEGAL_ARGUMENT);
    }
}

/**
* @brief Test the audio decoder's state retrieval function.
*
* This test verifies if the getState function of the audio decoder retrieves the state correctly and ensures the state is valid.@n
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 007@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Invoke getState to retrieve the state of all available decoders|status.isOk()|status.isOk() should be true|Should Pass|
* |02|Validate the retrieved state|state|State should be valid|Should Pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, getState_pos)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    Capabilities capabilities;
    ::android::binder::Status status;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Get Capabilities of Audio Decoder */
        AUDIODECODER_GETCAPABILITIES(audioDecoder, &capabilities);

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            sp<IAudioDecoderController> audioDecoderController = nullptr;
            sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();

            /* Open the decoder */
            AUDIODECODER_OPEN_SUCCESS(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, audioDecoderController, true);

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, true);
        }
    }
}

/**
* @brief This test verifies the ability to open  audio decoders in various codecs.
*
* This test checks the initial state of the audio decoder, ensures there are available decoders,
* and iteratively opens the decoders for supported codecs and then closes them.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 008@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, open_pos)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    Capabilities capabilities;
    ::android::binder::Status status;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Get Capabilities of Audio Decoder */
        AUDIODECODER_GETCAPABILITIES(audioDecoder, &capabilities);

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            sp<IAudioDecoderController> audioDecoderController;
            sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();

            /* Open the decoder */
            AUDIODECODER_OPEN_SUCCESS(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, audioDecoderController, true);

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, true);
        }
    }
}

/**
* @brief This test case is designed to check the error handling of the audioDecoder's open function
*
* The function tests the behavior of the audioDecoder API when attempting to open an audio decoder with
* 1. Invalid Parameters
* 2. Open decoder again.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 009@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Loop throug supported audio decoders|getAudioDecoderIds|None|Should be successful|
* |02|Get audio decoder and capabilites|audioDecoder, capabilities|status should bo ok|Should be successful|
* |03|Send invalid codec to open|invalidCodec|status.isOk() = false and audioDecoderController = nullptr|Should be successful|
* |04|Try to open as secure decoding if decoder doesn't supprot secure|supportSecure=True|status.isOk() = false and audioDecoderController = nullptr|Should be successful|
* |05|send invalid listener|listener=nullptr|status.isOk() = false and audioDecoderController = nullptr|Should be successful|
* |06|Open the decoder with valid parameters|valid parameters to open|status.isOk() = true and audioDecoderController = valid pointer|Should be successful|
* |07|Open the decoder again with valid parameters|valid parameters to open|status.isOk() = false and audioDecoderController = nullptr|Should be successful|
* |08|Close the decoder|None|status.isOk() = true|Should be successful|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, open_neg)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    Codec invalidCodec;
    Capabilities capabilities;
    ::android::binder::Status status;
    sp<IAudioDecoderController> audioDecoderController = nullptr;
    sp<IAudioDecoderController> audioDecoderController_nullptr = nullptr;
    sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();

    bool _aidl_return;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Get Capabilities of Audio Decoder */
        AUDIODECODER_GETCAPABILITIES(audioDecoder, &capabilities);

        if(capabilities.supportedCodecs.size() > 1)
            std::sort(capabilities.supportedCodecs.begin(), capabilities.supportedCodecs.end());

        invalidCodec = static_cast<Codec>(static_cast<int32_t>(capabilities.supportedCodecs.front()) - 1);

        /* Send invalid audio codec value */
        AUDIODECODER_OPEN_FAILURE(audioDecoder, invalidCodec, capabilities.supportsSecure, controllerListener, audioDecoderController_nullptr, Status::EX_ILLEGAL_ARGUMENT);

        /* If decoder doesn't support secure, request for secure decoder */
        if (capabilities.supportsSecure == false)
        {
            AUDIODECODER_OPEN_FAILURE(audioDecoder, capabilities.supportedCodecs.front(), true, controllerListener, audioDecoderController_nullptr, Status::EX_ILLEGAL_ARGUMENT);
        }

        /* Send invalid listener */
        AUDIODECODER_OPEN_FAILURE(audioDecoder, capabilities.supportedCodecs.front(), capabilities.supportsSecure, nullptr, audioDecoderController_nullptr, Status::EX_NULL_POINTER);

        /* Open the decoder */
        AUDIODECODER_OPEN_SUCCESS(audioDecoder, capabilities.supportedCodecs.front(), capabilities.supportsSecure, controllerListener, audioDecoderController, true);

        /* Open the Decoder again */
        AUDIODECODER_OPEN_FAILURE(audioDecoder, capabilities.supportedCodecs.front(), capabilities.supportsSecure, controllerListener, audioDecoderController_nullptr, Status::EX_ILLEGAL_STATE);

        AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, true);
    }
}

/**
* @brief Test to verify the successful closing of an audio decoder
*
* This test ensures that the audio decoder closes correctly when it is in the READY state.
* The objective is to test the audio decoder's state transition handling and verify that the `close` API functions as expected.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 010@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, close_pos)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    Capabilities capabilities;
    ::android::binder::Status status;
    bool _aidl_return;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Get Capabilities of Audio Decoder */
        AUDIODECODER_GETCAPABILITIES(audioDecoder, &capabilities);

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            sp<IAudioDecoderController> audioDecoderController;
            sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();

            /* Open the decoder */
            AUDIODECODER_OPEN_SUCCESS(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, audioDecoderController, true);

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, true);
        }
    }
}

/**
* @brief Test for closing the audio decoder in different states
*
* This test checks the behavior of the audio decoder when trying to close it
* from the READY state and then attempts to close it again from the CLOSED state.
* Also test checks the behavious when decoder is closed again@n
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 011@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|Close the decoder with invalid controller|audioDecoderController=nullptr|status should be not ok|Should pass|
* |05|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
* |06|Close the audio decoder again|audioDecoderController, _aidl_return|status.isOk() should be false, _aidl_return = false|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, close_neg)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    Capabilities capabilities;
    ::android::binder::Status status;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Get Capabilities of Audio Decoder */
        AUDIODECODER_GETCAPABILITIES(audioDecoder, &capabilities);

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            sp<IAudioDecoderController> audioDecoderController, audioDecoderController_nullptr = nullptr;
            sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();

            /* Open the decoder */
            AUDIODECODER_OPEN_SUCCESS(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, audioDecoderController, true);

            /* Close with invalid controller */
            AUDIODECODER_CLOSE_FAILURE(audioDecoder, audioDecoderController_nullptr, Status::EX_NULL_POINTER);

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Close the decoder again*/
            AUDIODECODER_CLOSE_FAILURE(audioDecoder, audioDecoderController, Status::EX_ILLEGAL_STATE);
        }
    }
}

/**
* @brief Test to register an audio decoder event listener and verify its successful registration
*
* This test checks if an audio decoder event listener can be registered successfully.
* It ensures that the function registering the listener returns true and completes without errors.
* It also cleans up by unregistering the listener.
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 012 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None @n
* **Dependencies:** None @n
* **User Interaction:** None @n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|register decoder events|listener|status should be ok|Should pass|
* |03|un-register decoder events|listener|status should be ok|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, registerEventListener_pos)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    ::android::binder::Status status;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Register events */
        AUDIODECODER_REGISTER_EVENT_SUCCESS(audioDecoder, eventListener);

        /* Un-register the events */
        AUDIODECODER_UNREGISTER_EVENT_SUCCESS(audioDecoder, eventListener);
    }
}

/**
* @brief Test the negative case scenario for registering an audio decoder event listener.
*
* This test attempts to register an audio decoder event listener that has already been registered
* in the SetUp method. It ensures that a listener cannot be registered more than once and validates
* the appropriate negative responses from the registration function. This test also checks the behaviour
* if invalid listener is passed
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 013@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|register decoder events with invalid listener|listener=nullptr|status should not be ok|Should pass|
* |03|register decoder events|listener|status should be ok|Should pass|
* |04|register decoder events again|listener|status should not be ok|Should pass|
* |05|un-register decoder events|listener|status should be ok|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, registerEventListener_neg)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    ::android::binder::Status status;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();
        sp<IAudioDecoderEventListener> eventListener_nullptr = nullptr;

        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Register events with invalid listener */
        AUDIODECODER_REGISTER_EVENT_FAILURE(audioDecoder, eventListener_nullptr, Status::EX_NULL_POINTER);

        /* Register events */
        AUDIODECODER_REGISTER_EVENT_SUCCESS(audioDecoder, eventListener);

        /* Register events again */
        AUDIODECODER_REGISTER_EVENT_FAILURE(audioDecoder, eventListener, Status::EX_NONE);

        /* Un-register the events */
        AUDIODECODER_UNREGISTER_EVENT_SUCCESS(audioDecoder, eventListener);
    }
}

/**
* @brief Test to register an audio decoder event listener and verify its successful registration and un-registration
*
* This test checks if an audio decoder event listener can be registered successfully.
* It ensures that the function registering the listener returns true and completes without errors.
* It also cleans up by unregistering the listener.
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 014 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None @n
* **Dependencies:** None @n
* **User Interaction:** None @n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|register decoder events|listener|status should be ok|Should pass|
* |03|un-register decoder events|listener|status should be ok|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, unregisterEventListener_pos)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    ::android::binder::Status status;
    bool _aidl_return;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Register events */
        AUDIODECODER_REGISTER_EVENT_SUCCESS(audioDecoder, eventListener);

        /* Un-register the events */
        AUDIODECODER_UNREGISTER_EVENT_SUCCESS(audioDecoder, eventListener);

    }
}

/**
* @brief Test to check the negative scenarios of unregister event listener
*
* This test verifies the behavior of the audio decoder when trying to unregister an event listener
* that was never registered. Also verifies with invalid parameters.
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 015 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None @n
* **Dependencies:** None @n
* **User Interaction:** None @n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|un-register decoder events without register|listener|status should be ok|Should pass|
* |03|register decoder events|listener|status should be ok|Should pass|
* |04|un-register decoder events with nullptr|listener=nullptr|status should not be ok|Should pass|
* |05|un-register decoder events|listener|status should be ok|Should pass|
* |06|un-register decoder events again|listener|status should not be ok|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, unregisterEventListener_neg)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    ::android::binder::Status status;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();
        sp<IAudioDecoderEventListener> eventListener_nullptr = nullptr;

        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Un-register the events */
        AUDIODECODER_UNREGISTER_EVENT_FAILURE(audioDecoder, eventListener, Status::EX_NONE);

        /* Register events */
        AUDIODECODER_REGISTER_EVENT_SUCCESS(audioDecoder, eventListener);

        /* Un-register the events with nullptr*/
        AUDIODECODER_UNREGISTER_EVENT_FAILURE(audioDecoder, eventListener_nullptr, Status::EX_NULL_POINTER);

        /* Un-register the events */
        AUDIODECODER_UNREGISTER_EVENT_SUCCESS(audioDecoder, eventListener);

        /* Un-register the events again */
        AUDIODECODER_UNREGISTER_EVENT_FAILURE(audioDecoder, eventListener, Status::EX_NONE);
    }
}

/**
* @brief Test the positive start functionality of the audio decoder.
*
* This test verifies that the audio decoder can be successfully started and stopped,
* ensuring that the start method works as expected without error.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 016@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|start the decoder|audioDecoderController|status should be ok|Should pass|
* |05|Check the state of the decoder|None|state should be STARTED, status should be ok|Should pass|
* |06|Stop the decoder|None|state should be READY, status should be ok|Should pass|
* |07|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, start_pos)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    audiodecoder::Capabilities capabilities;
    ::android::binder::Status status;
    bool _aidl_return;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Get Capabilities of Audio Decoder */
        AUDIODECODER_GETCAPABILITIES(audioDecoder, &capabilities);

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            sp<IAudioDecoderController> audioDecoderController;
            sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();

            /* Open the Decoder */
            AUDIODECODER_OPEN_SUCCESS(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, audioDecoderController, true);

            /* Start the decoder */
            AUDIODECODER_START_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Stop the decoder */
            AUDIODECODER_STOP_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, true);
        }
    }
}

/**
* @brief Test for negative scenario when starting the audio decoder that is already started
*
* This test checks the behavior of the audio decoder when trying to start it again
* after it has already been started. The expected behavior is to throw an illegal state exception.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 017@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|start the decoder|audioDecoderController|status should be ok|Should pass|
* |05|Check the state of the decoder|None|state should be STARTED, status should be ok|Should pass|
* |06|start the decoder again|None|status should not be ok|Should pass|
* |07|Stop the decoder|None|state should be READY, status should be ok|Should pass|
* |08|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
* |09|start the decoder again|None|status should not be ok|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, start_neg)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    Capabilities capabilities;
    ::android::binder::Status status;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Get Capabilities of Audio Decoder */
        AUDIODECODER_GETCAPABILITIES(audioDecoder, &capabilities);

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            sp<IAudioDecoderController> audioDecoderController;
            sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();
            sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

            /* Open the decoder */
            AUDIODECODER_OPEN_SUCCESS(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, audioDecoderController, true);

            /* Start the decoder */
            AUDIODECODER_START_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Start the decoder again */
            AUDIODECODER_START_FAILURE(audioDecoderController, Status::EX_ILLEGAL_STATE);

            /* Stop the decoder */
            AUDIODECODER_STOP_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Start the decoder after close */
            //AUDIODECODER_START_FAILURE(audioDecoderController, Status::EX_ILLEGAL_STATE);
        }
    }
}

/**
* @brief Test the functionality of stopping the audio decoder controller.
*
* This test validates the correct behavior of the stop method on the audio decoder controller,
* ensuring that the decoder can be stopped from the STARTED state.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 018@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|start the decoder|audioDecoderController|status should be ok|Should pass|
* |05|Check the state of the decoder|None|state should be STARTED, status should be ok|Should pass|
* |06|Stop the decoder|None|state should be READY, status should be ok|Should pass|
* |07|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, stop_pos)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    audiodecoder::Capabilities capabilities;
    ::android::binder::Status status;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Get Capabilities of Audio Decoder */
        AUDIODECODER_GETCAPABILITIES(audioDecoder, &capabilities);

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            sp<IAudioDecoderController> audioDecoderController;
            sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();
            sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

            /* Open the Decoder */
            AUDIODECODER_OPEN_SUCCESS(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, audioDecoderController, true);

            /* Start the decoder */
            AUDIODECODER_START_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Stop the decoder */
            AUDIODECODER_STOP_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, true);
        }
    }
}

/**
* @brief Test the stop function of the audio decoder when called in an invalid state
*
* This test validates whether the audio decoder controller handles
* 1. Illegal state transitions correctly by invoking the stop function twice consecutively
* 2. Stop after closing the decoder
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 019@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|start the decoder|audioDecoderController|status should be ok|Should pass|
* |05|Check the state of the decoder|None|state should be STARTED, status should be ok|Should pass|
* |06|Stop the decoder|None|state should be READY, status should be ok|Should pass|
* |07|Stop the decoder again|None|status should not be ok|Should pass|
* |08|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
* |09|stop the decoder again|None|status should not be ok|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, stop_neg)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    Capabilities capabilities;
    ::com::rdk::hal::State state;
    ::android::binder::Status status;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Get Capabilities of Audio Decoder */
        AUDIODECODER_GETCAPABILITIES(audioDecoder, &capabilities);

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            sp<IAudioDecoderController> audioDecoderController;
            sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();
            sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

            /* Open the decoder */
            AUDIODECODER_OPEN_SUCCESS(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, audioDecoderController, true);

            /* Start the decoder */
            AUDIODECODER_START_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Stop the decoder */
            AUDIODECODER_STOP_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Stop the decoder again */
            AUDIODECODER_STOP_FAILURE(audioDecoderController, Status::EX_ILLEGAL_STATE);

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, true);

            /* stop the decoder again */
            AUDIODECODER_STOP_FAILURE(audioDecoderController, Status::EX_ILLEGAL_STATE);
        }
    }
}

/**
* @brief This test validates if the `decodeBuffer` method handles invalid input arguments correctly.
*
* The objective of this test is to provide an invalid arguments.
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 020@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|start the decoder|audioDecoderController|status should be ok|Should pass|
* |05|Check the state of the decoder|None|state should be STARTED, status should be ok|Should pass|
* |06|decodeBuffer with invalid parameters|invalid arguments|status should not be ok|Should pass|
* |07|Stop the decoder|None|state should be READY, status should be ok|Should pass|
* |08|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, decodeBuffer_neg)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    audiodecoder::Capabilities capabilities;
    ::android::binder::Status status;

    int64_t invalid_nsPresentationTime = -1000000;
    int64_t valid_nsPresentationTime   =  1000000;
    int64_t invalid_bufferHandle = -1;
    int32_t valid_trimStartNs    = 1000;
    int32_t invalid_trimStartNs  = -1000;
    int32_t valid_trimEndNs      = 500;
    int32_t invalid_trimEndNs    = -500;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Get Capabilities of Audio Decoder */
        AUDIODECODER_GETCAPABILITIES(audioDecoder, &capabilities);

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            sp<IAudioDecoderController> audioDecoderController;
            sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();
            sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

            /* Open the Decoder */
            AUDIODECODER_OPEN_SUCCESS(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, audioDecoderController, true);

            /* Start the decoder */
            AUDIODECODER_START_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Decode buffer with invalid Presentation time */
            AUDIODECODER_DECODEBUFFER_FAILURE(audioDecoderController, invalid_nsPresentationTime,
                                              invalid_bufferHandle, valid_trimStartNs, valid_trimEndNs, Status::EX_ILLEGAL_ARGUMENT);

            /* Decode buffer with invalid buffer handle */
            AUDIODECODER_DECODEBUFFER_FAILURE(audioDecoderController, valid_nsPresentationTime,
                                              invalid_bufferHandle, valid_trimStartNs, valid_trimEndNs, Status::EX_ILLEGAL_ARGUMENT);

            /* Decode buffer with invalid trimStart */
            AUDIODECODER_DECODEBUFFER_FAILURE(audioDecoderController, valid_nsPresentationTime,
                                              invalid_bufferHandle, invalid_trimStartNs, valid_trimEndNs, Status::EX_ILLEGAL_ARGUMENT);

            /* Decode buffer with invalid trimEnd */
            AUDIODECODER_DECODEBUFFER_FAILURE(audioDecoderController, valid_nsPresentationTime,
                                              invalid_bufferHandle, valid_trimStartNs, invalid_trimEndNs, Status::EX_ILLEGAL_ARGUMENT);

            /* Stop the decoder */
            AUDIODECODER_STOP_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, true);
        }
    }
}

/**
* @brief Test for positive scenario of audio decoder's flush function
*
* This test validates the functionality of the `flush` function of the audio decoder controller when reset parameter
* reset false: decoder changes to STARTED
*       true: decoder changes to READY
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 021 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|start the decoder|audioDecoderController|status should be ok|Should pass|
* |05|Check the state of the decoder|None|state should be STARTED, status should be ok|Should pass|
* |06|flush the decoder with reset false|reset=false|status should be ok and state should be STARTED|Should pass|
* |07|flush the decoder with reset true|reset=true|status should be ok and state should be READY|Should pass|
* |08|start the decoder|audioDecoderController|status should be ok|Should pass|
* |09|Stop the decoder|None|state should be READY, status should be ok|Should pass|
* |10|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, flush_pos)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    audiodecoder::Capabilities capabilities;
    ::android::binder::Status status;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Get Capabilities of Audio Decoder */
        AUDIODECODER_GETCAPABILITIES(audioDecoder, &capabilities);

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            sp<IAudioDecoderController> audioDecoderController;
            sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();
            sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

            /* Open the Decoder */
            AUDIODECODER_OPEN_SUCCESS(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, audioDecoderController, true);

            /* Start the decoder */
            AUDIODECODER_START_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Flush the decoder with reset flag false*/
            AUDIODECODER_FLUSH_SUCCESS(audioDecoder, audioDecoderController, false, true);

            /* Flush the decoder with reset flag false*/
            AUDIODECODER_FLUSH_SUCCESS(audioDecoder, audioDecoderController, true, true);

            /* Start the decoder since flush with reset true moves decoder to READY state */
            AUDIODECODER_START_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Stop the decoder */
            AUDIODECODER_STOP_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, true);
        }
    }
}

/**
* @brief Test for negative scenarios of flushing
*
* This test checks the negative scenarios of flush function
*
* **Test Group ID:** Basic: 01@n
* **Test Case ID:** 022@n
* **Priority:** High@n
* @n
* **Pre-Conditions:** None@n
* **Dependencies:** None@n
* **User Interaction:** None@n
* @n
* **Test Procedure:**@n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|Flush the decoder before start with reset flag false|reset=false|status should not be ok|Should pass|
* |05|Flush the decoder before start with reset flag true|reset=true|status should not be ok|Should pass|
* |06|start the decoder|audioDecoderController|status should be ok|Should pass|
* |07|Stop the decoder|None|state should be READY, status should be ok|Should pass|
* |08|Check the state of the decoder|None|state should be STARTED, status should be ok|Should pass|
* |09|Flush the decoder before start with reset flag false|reset=false|status should not be ok|Should pass|
* |10|Flush the decoder before start with reset flag true|reset=true|status should not be ok|Should pass|
* |11|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
* |12|Flush the decoder before start with reset flag false|reset=false|status should not be ok|Should pass|
* |13|Flush the decoder before start with reset flag true|reset=true|status should not be ok|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, flush_neg)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    audiodecoder::Capabilities capabilities;
    ::android::binder::Status status;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Get Capabilities of Audio Decoder */
        AUDIODECODER_GETCAPABILITIES(audioDecoder, &capabilities);

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            sp<IAudioDecoderController> audioDecoderController;
            sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();
            sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

            /* Open the Decoder */
            AUDIODECODER_OPEN_SUCCESS(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, audioDecoderController, true);

            /* Flush the decoder before start with reset flag false */
            AUDIODECODER_FLUSH_FAILURE(audioDecoderController, false, Status::EX_ILLEGAL_STATE);

            /* Flush the decoder before start with reset flag true */
            AUDIODECODER_FLUSH_FAILURE(audioDecoderController, true, Status::EX_ILLEGAL_STATE);

            /* Start the decoder */
            AUDIODECODER_START_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Stop the decoder */
            AUDIODECODER_STOP_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Flush the decoder with reset flag false*/
            AUDIODECODER_FLUSH_FAILURE(audioDecoderController, false, Status::EX_ILLEGAL_STATE);

            /* Flush the decoder with reset flag false*/
            AUDIODECODER_FLUSH_FAILURE(audioDecoderController, true, Status::EX_ILLEGAL_STATE);

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Flush the decoder with reset flag false*/
            AUDIODECODER_FLUSH_FAILURE(audioDecoderController, false, Status::EX_ILLEGAL_STATE);

            /* Flush the decoder with reset flag false*/
            AUDIODECODER_FLUSH_FAILURE(audioDecoderController, true, Status::EX_ILLEGAL_STATE);
        }
    }
}

/**
* @brief Test to check the functionality of signalDiscontinuity in the AudioDecoder
*
* This test validates the signalDiscontinuity method on the AudioDecoderController to ensure it handles
* a signal discontinuity appropriately without any errors.
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 023 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None @n
* **Dependencies:** None @n
* **User Interaction:** None @n
* @n
* **Test Procedure:** @n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|start the decoder|audioDecoderController|status should be ok|Should pass|
* |05|Check the state of the decoder|None|state should be STARTED, status should be ok|Should pass|
* |06|Signal discontinuity to the decoder|None|status should be ok and state should be STARTED|Should pass|
* |07|Stop the decoder|None|state should be READY, status should be ok|Should pass|
* |08|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, signalDiscontinuity_pos)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    audiodecoder::Capabilities capabilities;
    ::android::binder::Status status;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Get Capabilities of Audio Decoder */
        AUDIODECODER_GETCAPABILITIES(audioDecoder, &capabilities);

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            sp<IAudioDecoderController> audioDecoderController;
            sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();
            sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

            /* Open the Decoder */
            AUDIODECODER_OPEN_SUCCESS(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, audioDecoderController, true);

            /* Start the decoder */
            AUDIODECODER_START_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Signal discontinuity to the decoder */
            AUDIODECODER_SIGNAL_DISCONTINUITY_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Stop the decoder */
            AUDIODECODER_STOP_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, true);
        }
    }
}

/**
* @brief Test to check the functionality of signalDiscontinuity in the AudioDecoder in negative scenarios
*
* This test validates the signalDiscontinuity method on the AudioDecoderController to ensure it handles
* negative scenarios of signalDiscontinuity appropriately
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 024 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None @n
* **Dependencies:** None @n
* **User Interaction:** None @n
* @n
* **Test Procedure:** @n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|Signal discontinuity to the decoder before start|None|status should not be ok|Should pass|
* |05|start the decoder|audioDecoderController|status should be ok|Should pass|
* |06|Stop the decoder|None|state should be READY, status should be ok|Should pass|
* |07|Signal discontinuity to the decoder after start->stop|None|status should not be ok|Should pass|
* |08|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
* |09|Signal discontinuity to the decoder after close|None|status should not be ok|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, signalDiscontinuity_neg)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    audiodecoder::Capabilities capabilities;
    ::android::binder::Status status;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Get Capabilities of Audio Decoder */
        AUDIODECODER_GETCAPABILITIES(audioDecoder, &capabilities);

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            sp<IAudioDecoderController> audioDecoderController;
            sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();
            sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

            /* Open the Decoder */
            AUDIODECODER_OPEN_SUCCESS(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, audioDecoderController, true);

            /* Signal discontinuity to the decoder before start */
            AUDIODECODER_SIGNAL_DISCONTINUITY_FAILURE(audioDecoderController, Status::EX_ILLEGAL_STATE);

            /* Start the decoder */
            AUDIODECODER_START_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Stop the decoder */
            AUDIODECODER_STOP_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Signal discontinuity to the decoder after start->stop */
            AUDIODECODER_SIGNAL_DISCONTINUITY_FAILURE(audioDecoderController, Status::EX_ILLEGAL_STATE);

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Signal discontinuity to the decoder after close */
            //AUDIODECODER_SIGNAL_DISCONTINUITY_FAILURE(audioDecoderController, Status::EX_ILLEGAL_STATE);
        }
    }
}

/**
* @brief This test case verifies that the audio decoder can successfully signal an End-of-Stream (EOS)
* condition in a positive scenario.
*
* This test ensures that the audio decoder can transition to an EOS state and handle it correctly.
* Signaling EOS is a key functionality that ensures proper stream termination
* and resource management, which is crucial for maintaining performance and stability
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 025 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None @n
* **Dependencies:** None @n
* **User Interaction:** None @n
* @n
* **Test Procedure:** @n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|start the decoder|audioDecoderController|status should be ok|Should pass|
* |05|Check the state of the decoder|None|state should be STARTED, status should be ok|Should pass|
* |06|Signal EOS to the decoder|None|status should be ok and state should be STARTED|Should pass|
* |07|Stop the decoder|None|state should be READY, status should be ok|Should pass|
* |08|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, signalEOS_pos)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    audiodecoder::Capabilities capabilities;
    ::android::binder::Status status;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Get Capabilities of Audio Decoder */
        AUDIODECODER_GETCAPABILITIES(audioDecoder, &capabilities);

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            sp<IAudioDecoderController> audioDecoderController;
            sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();
            sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

            /* Open the Decoder */
            AUDIODECODER_OPEN_SUCCESS(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, audioDecoderController, true);

            /* Start the decoder */
            AUDIODECODER_START_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Signal EOS to the decoder */
            AUDIODECODER_SIGNAL_EOS_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Stop the decoder */
            AUDIODECODER_STOP_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, true);
        }
    }
}

/**
* @brief Test to check the functionality of signalEOS in the AudioDecoder in negative scenarios
*
* This test validates the signalEOS method on the AudioDecoderController to ensure it handles
* negative scenarios of signal eos appropriately
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 026 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None @n
* **Dependencies:** None @n
* **User Interaction:** None @n
* @n
* **Test Procedure:** @n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|Signal EOS to the decoder before start|None|status should not be ok|Should pass|
* |05|start the decoder|audioDecoderController|status should be ok|Should pass|
* |06|Stop the decoder|None|state should be READY, status should be ok|Should pass|
* |07|Signal EOS to the decoder after start->stop|None|status should not be ok|Should pass|
* |08|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
* |09|Signal EOS to the decoder after close|None|status should not be ok|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, signalEOS_neg)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    audiodecoder::Capabilities capabilities;
    ::android::binder::Status status;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Get Capabilities of Audio Decoder */
        AUDIODECODER_GETCAPABILITIES(audioDecoder, &capabilities);

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            sp<IAudioDecoderController> audioDecoderController;
            sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();
            sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

            /* Open the Decoder */
            AUDIODECODER_OPEN_SUCCESS(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, audioDecoderController, true);

            /* Signal EOS to the decoder before start */
            AUDIODECODER_SIGNAL_EOS_FAILURE(audioDecoderController, Status::EX_ILLEGAL_STATE);

            /* Start the decoder */
            AUDIODECODER_START_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Stop the decoder */
            AUDIODECODER_STOP_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Signal EOS to the decoder after start->stop */
            AUDIODECODER_SIGNAL_EOS_FAILURE(audioDecoderController, Status::EX_ILLEGAL_STATE);

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Signal EOS to the decoder after close */
            //AUDIODECODER_SIGNAL_EOS_FAILURE(audioDecoderController, Status::EX_ILLEGAL_STATE);
        }
    }
}

/**
* @brief This test case verifies parseCodecSpecificData functionality.
*
* This test ensures that the audio decoder can transition to an EOS state and handle it correctly.
* Signaling EOS is a key functionality that ensures proper stream termination
* and resource management, which is crucial for maintaining performance and stability
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 027 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None @n
* **Dependencies:** None @n
* **User Interaction:** None @n
* @n
* **Test Procedure:** @n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|start the decoder|audioDecoderController|status should be ok|Should pass|
* |05|Check the state of the decoder|None|state should be STARTED, status should be ok|Should pass|
* |06|Set CSD data to the decoder|CSD data|status should be ok and state should be STARTED|Should pass|
* |07|Stop the decoder|None|state should be READY, status should be ok|Should pass|
* |08|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, parseCodecSpecificData_pos)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    audiodecoder::Capabilities capabilities;
    ::android::binder::Status status;

    /*
      Audio Object Type: 2 (AAC LC)
      Sampling Frequency Index: 4 (44.1 kHz)
      Channel Configuration: 2 (stereo)
    */
    std::vector<uint8_t> codecData = {0x12, 0x10};

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Get Capabilities of Audio Decoder */
        AUDIODECODER_GETCAPABILITIES(audioDecoder, &capabilities);

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            if (_codec == Codec::AAC_LC ||
                _codec == Codec::HE_AAC ||
                _codec == Codec::HE_AAC2)
            {
                CSDAudioFormat csdAudioFormat = CSDAudioFormat::MP4_AUDIO_SPECIFIC_CONFIG;

                sp<IAudioDecoderController> audioDecoderController;
                sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();
                sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

                /* Open the Decoder */
                AUDIODECODER_OPEN_SUCCESS(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, audioDecoderController, true);

                /* Start the decoder */
                AUDIODECODER_START_SUCCESS(audioDecoder, audioDecoderController, true);

                /* Set CSD data to the decoder */
                AUDIODECODER_PARSE_CSD_SUCCESS(audioDecoder, audioDecoderController, csdAudioFormat, codecData, true);

                /* Stop the decoder */
                AUDIODECODER_STOP_SUCCESS(audioDecoder, audioDecoderController, true);

                /* Close the decoder */
                AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, true);
            }
        }
    }
}

/**
* @brief This test case verifies negative scenarios for parseCodecSpecificData functionality.
*
* This test checks the negagive scenarios of parseCodecSpecificData functionality
* 1. invalid arguments
* 2, invalid state
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 028 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None @n
* **Dependencies:** None @n
* **User Interaction:** None @n
* @n
* **Test Procedure:** @n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|start the decoder|audioDecoderController|status should be ok|Should pass|
* |05|Check the state of the decoder|None|state should be STARTED, status should be ok|Should pass|
* |06|Set CSD data to the decoder|CSD data|status should be ok and state should be STARTED|Should pass|
* |07|Stop the decoder|None|state should be READY, status should be ok|Should pass|
* |08|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, parseCodecSpecificData_neg)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    audiodecoder::Capabilities capabilities;
    ::android::binder::Status status;

    /*
      Audio Object Type: 2 (AAC LC)
      Sampling Frequency Index: 4 (44.1 kHz)
      Channel Configuration: 2 (stereo)
    */
    std::vector<uint8_t> valid_codecData = {0x12, 0x10};
    std::vector<uint8_t> invalid_codecData_1 = {0xFF, 0xFF};
    std::vector<uint8_t> invalid_codecData_2 = {};

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Get Capabilities of Audio Decoder */
        AUDIODECODER_GETCAPABILITIES(audioDecoder, &capabilities);

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            if (_codec == Codec::AAC_LC ||
                _codec == Codec::HE_AAC ||
                _codec == Codec::HE_AAC2)
            {
                CSDAudioFormat csdAudioFormat = CSDAudioFormat::MP4_AUDIO_SPECIFIC_CONFIG;

                sp<IAudioDecoderController> audioDecoderController;
                sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();
                sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

                /* Open the Decoder */
                AUDIODECODER_OPEN_SUCCESS(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, audioDecoderController, true);

                /* Set CSD data to the decoder before STARTED*/
                AUDIODECODER_PARSE_CSD_FAILURE(audioDecoderController, csdAudioFormat, valid_codecData, Status::EX_ILLEGAL_STATE);

                /* Start the decoder */
                AUDIODECODER_START_SUCCESS(audioDecoder, audioDecoderController, true);

                /* Set invalid CSD data to the decoder */
                AUDIODECODER_PARSE_CSD_FAILURE(audioDecoderController, csdAudioFormat, invalid_codecData_1, Status::EX_ILLEGAL_ARGUMENT);

                /* Set invalid CSD data to the decoder */
                AUDIODECODER_PARSE_CSD_FAILURE(audioDecoderController, csdAudioFormat, invalid_codecData_2, Status::EX_ILLEGAL_ARGUMENT);

                /* Stop the decoder */
                AUDIODECODER_STOP_SUCCESS(audioDecoder, audioDecoderController, true);

                /* Set CSD data to the decoder after STOP */
                AUDIODECODER_PARSE_CSD_FAILURE(audioDecoderController, csdAudioFormat, valid_codecData, Status::EX_ILLEGAL_STATE);

                /* Close the decoder */
                AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, true);

                /* Set CSD data to the decoder after CLOSE */
                //AUDIODECODER_PARSE_CSD_FAILURE(audioDecoderController, csdAudioFormat, valid_codecData, Status::EX_ILLEGAL_STATE);
            }
        }
    }
}

std::multimap<Property, int32_t> validIntPropertyReady =
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

std::multimap<Property, int32_t> validAC4IntPropertyAll =
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
    {Property::AC4_MIXER_BALANCE, 32},
    {Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE, -1},
    {Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE, 0},
    {Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE, 1}
};

std::multimap<Property, android::String16> validAC4StringPropertyAll =
{
    {Property::AC4_PREFERRED_LANG1, android::String16("")},
    {Property::AC4_PREFERRED_LANG1, android::String16("eng")},
    {Property::AC4_PREFERRED_LANG1, android::String16("fra")},
    {Property::AC4_PREFERRED_LANG2, android::String16("")},
    {Property::AC4_PREFERRED_LANG2, android::String16("eng")},
    {Property::AC4_PREFERRED_LANG1, android::String16("fra")}
};

std::multimap<Property, int32_t> invalidIntPropertyReady =
{
    {Property::LOW_LATENCY_MODE, -1},
    {Property::LOW_LATENCY_MODE, 2},
    {Property::AV_SOURCE , int32_t(static_cast<int32_t>(AVSource::AUTO))-1},
    {Property::AV_SOURCE , int32_t(static_cast<int32_t>(AVSource::COMPOSITE_5 ))+1}
};

std::multimap<Property, int32_t> invalidAC4IntPropertyAll =
{
    {Property::AC4_PRESENTATION_GROUP_INDEX , -2},
    {Property::AC4_ASSOCIATED_TYPE, -2},
    {Property::AC4_ASSOCIATED_TYPE, 3},
    {Property::AC4_AUTO_SELECTION_PRIORITY, -2},
    {Property::AC4_AUTO_SELECTION_PRIORITY, 2},
    {Property::AC4_MIXER_BALANCE, 256},
    {Property::AC4_MIXER_BALANCE, -33},
    {Property::AC4_MIXER_BALANCE, 33},
    {Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE, -2},
    {Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE, 2}
};

std::multimap<Property, android::String16> invalidAC4StringPropertyAll =
{
    {Property::AC4_PREFERRED_LANG1, android::String16("en£")},
    {Property::AC4_PREFERRED_LANG1, android::String16("111")},
    {Property::AC4_PREFERRED_LANG1, android::String16("a11")},
    {Property::AC4_PREFERRED_LANG2, android::String16("en&")},
    {Property::AC4_PREFERRED_LANG2, android::String16("111")},
    {Property::AC4_PREFERRED_LANG2, android::String16("a11")}
};

/**
* @brief Tests the positive scenario for setting property in AudioDecoderController.
*
* This test ensures that the properties can be set without an error
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 029 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None @n
* **Dependencies:** None @n
* **User Interaction:** None @n
* @n
* **Test Procedure:** @n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|set the properties listed in table|property|status should be ok|Should pass|
* |05|start the decoder|audioDecoderController|status should be ok|Should pass|
* |06|Check the state of the decoder|None|state should be STARTED, status should be ok|Should pass|
* |07|set the properties listed in table|property|status should be ok|Should pass|
* |08|Stop the decoder|None|state should be READY, status should be ok|Should pass|
* |09|set the properties listed in table|property|status should be ok|Should pass|
* |10|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
* |11|set the properties listed in table|property|status should be ok|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, setProperty_pos)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    audiodecoder::Capabilities capabilities;
    ::android::binder::Status status;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Get Capabilities of Audio Decoder */
        AUDIODECODER_GETCAPABILITIES(audioDecoder, &capabilities);

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            sp<IAudioDecoderController> audioDecoderController;
            sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();
            sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

            /* Open the Decoder */
            AUDIODECODER_OPEN_SUCCESS(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, audioDecoderController, true);

            /*Set AC4 property only if the decopder support AC4 codec*/
            if (Codec::DOLBY_AC4 == _codec)
            {
                /* set the properties which can be set at any state */
                AUDIODECODER_SETPROPERTY_LIST_SUCCESS(audioDecoderController, validAC4IntPropertyAll, intValue);
                AUDIODECODER_SETPROPERTY_STRING_LIST_SUCCESS(audioDecoderController, validAC4StringPropertyAll);
            }
            /* set the properties which can be set after Ready */
            AUDIODECODER_SETPROPERTY_LIST_SUCCESS(audioDecoderController, validIntPropertyReady, intValue);

            /* Start the decoder */
            AUDIODECODER_START_SUCCESS(audioDecoder, audioDecoderController, true);

            /*Set AC4 property only if the decopder support AC4 codec*/
            if (Codec::DOLBY_AC4 == _codec)
            {
                /* set the properties which can be set at any state */
                AUDIODECODER_SETPROPERTY_LIST_SUCCESS(audioDecoderController, validAC4IntPropertyAll, intValue);
                AUDIODECODER_SETPROPERTY_STRING_LIST_SUCCESS(audioDecoderController, validAC4StringPropertyAll);
            }
            /* Stop the decoder */
            AUDIODECODER_STOP_SUCCESS(audioDecoder, audioDecoderController, true);

            /*Set AC4 property only if the decopder support AC4 codec*/
            if (Codec::DOLBY_AC4 == _codec)
            {
                /* set the properties which can be set at any state */
                AUDIODECODER_SETPROPERTY_LIST_SUCCESS(audioDecoderController, validAC4IntPropertyAll, intValue);
                AUDIODECODER_SETPROPERTY_STRING_LIST_SUCCESS(audioDecoderController, validAC4StringPropertyAll);
            }

            /* set the properties which can be set after Ready */
            AUDIODECODER_SETPROPERTY_LIST_SUCCESS(audioDecoderController, validIntPropertyReady, intValue);

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, true);

            /*Set AC4 property only if the decopder support AC4 codec*/
            if (Codec::DOLBY_AC4 == _codec)
            {
                /* set the properties which can be set at any state */
                AUDIODECODER_SETPROPERTY_LIST_SUCCESS(audioDecoderController, validAC4IntPropertyAll, intValue);
                AUDIODECODER_SETPROPERTY_STRING_LIST_SUCCESS(audioDecoderController, validAC4StringPropertyAll);
            }
        }
    }
}

/**
* @brief Tests the negative scenarios for setting property in AudioDecoderController.
*
* This test ensures that the negative scenarios of set properties works with proper error
*
* **Test Group ID:** Basic: 01 @n
* **Test Case ID:** 030 @n
* **Priority:** High @n
* @n
* **Pre-Conditions:** None @n
* **Dependencies:** None @n
* **User Interaction:** None @n
* @n
* **Test Procedure:** @n
* |Variation / Step|Description|Test Data|Expected Result|Notes|
* |----------------|-----------|---------|---------------|-----|
* |01|Get the supported IDs and loop through the available decoder|None|None|None|
* |02|Open the audio decoder with the codec and create audioDecoderController with listener|_codec, _decoder.supportsSecure, listener, audioDecoderController|status.isOk() should be true, audioDecoderController != nullptr|Should pass|
* |03|Verify the state of the decoder after open|state|state should be READY|Should pass|
* |04|set the invalid properties listed in table|invalid property|status should not be ok|Should pass|
* |05|start the decoder|audioDecoderController|status should be ok|Should pass|
* |06|Check the state of the decoder|None|state should be STARTED, status should be ok|Should pass|
* |07|set the valid properties which can be set in only READY state|property|invalid state error|Should pass|
* |08|Stop the decoder|None|state should be READY, status should be ok|Should pass|
* |09|Close the audio decoder and verify cleanup succeeded|audioDecoderController, _aidl_return|status.isOk() should be true, _aidl_return = true|Should pass|
* |10|set the valid properties which can be set in only READY state|property|invalid state error|Should pass|
*/
UT_ADD_TEST(VTS_L1_AudioDecoder, setProperty_neg)
{
    std::vector<IAudioDecoder::Id> decoderIds;
    sp<IAudioDecoder> audioDecoder;
    audiodecoder::Capabilities capabilities;
    ::android::binder::Status status;

    /* Get the Audio decoder IDs */
    AUDIODECODER_MANAGER_GETAUDIODECODERIDS(decoderIds);

    for(const auto& _decoderId: decoderIds)
    {
        /* Get Audio decoder handle */
        AUDIODECODER_MANAGER_GETAUDIODECODER(_decoderId, audioDecoder);

        /* Get Capabilities of Audio Decoder */
        AUDIODECODER_GETCAPABILITIES(audioDecoder, &capabilities);

        for(const auto& _codec: capabilities.supportedCodecs)
        {
            sp<IAudioDecoderController> audioDecoderController;
            sp<IAudioDecoderControllerListener> controllerListener = new AudioDecoderControllerListener();
            sp<IAudioDecoderEventListener> eventListener = new AudioDecoderEventListener();

            /* Open the Decoder */
            AUDIODECODER_OPEN_SUCCESS(audioDecoder, _codec, capabilities.supportsSecure, controllerListener, audioDecoderController, true);

            /*Set AC4 property only if the decopder support AC4 codec*/
            if (Codec::DOLBY_AC4 == _codec)
            {
                /* set the invalid properties */
                AUDIODECODER_SETPROPERTY_LIST_FAILURE(audioDecoderController, invalidAC4IntPropertyAll, intValue, Status::EX_ILLEGAL_ARGUMENT);
                AUDIODECODER_SETPROPERTY_STRING_LIST_FAILURE(audioDecoderController, invalidAC4StringPropertyAll, Status::EX_ILLEGAL_ARGUMENT);
            }
            AUDIODECODER_SETPROPERTY_LIST_FAILURE(audioDecoderController, invalidIntPropertyReady, intValue, Status::EX_ILLEGAL_ARGUMENT);

            /* Start the decoder */
            AUDIODECODER_START_SUCCESS(audioDecoder, audioDecoderController, true);

            /* set the properties in invalid state */
            AUDIODECODER_SETPROPERTY_LIST_FAILURE(audioDecoderController, validIntPropertyReady, intValue, Status::EX_ILLEGAL_STATE);

            /* Stop the decoder */
            AUDIODECODER_STOP_SUCCESS(audioDecoder, audioDecoderController, true);

            /* Close the decoder */
            AUDIODECODER_CLOSE_SUCCESS(audioDecoder, audioDecoderController, true);

            /* set the properties in invalid state */
            AUDIODECODER_SETPROPERTY_LIST_FAILURE(audioDecoderController, validIntPropertyReady, intValue, Status::EX_ILLEGAL_STATE);
        }
    }
}

/** @} */ // End of AUDIO_DECODER_HALTEST L1 File
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER Module
/** @} */ // End of HPK
