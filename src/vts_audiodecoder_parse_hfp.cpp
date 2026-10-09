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
 * @defgroup AUDIO_DECODER_HALTEST_PARSE_HFP Audio Decoder HAL Tests parse Hardware feature profile
 * @{
 * @parblock
 *
 * ### Parse HFP functions for Audio Decoder :
 *
 * Parse HFP functions required for the module across all vendors.
 *
 * **Pre-Conditions:**  None @n
 * **Dependencies:** None @n
 *
 * @endparblock
 *
 */

/**
 * @file vts_audiodecoder_parse_hfp.cpp
 *
 */

#include "ut_kvp_profile.h"
#include "vts_audiodecoder_helper_functions.h"
#include "vts_audiodecoder_parse_hfp.h"

using namespace std;

vector<audioDecoder> gAudiodecoders_hfp;

void vts_audiodecoder_get_available_id(std::vector<int32_t> &availableIds)
{
    std::string audiodecoderpath = AUDIODECODERMGR_IAUDIODECODER;

    availableIds.clear();

    int32_t number_of_audiodecoders = UT_KVP_PROFILE_GET_LIST_COUNT(audiodecoderpath.c_str());
    for (int32_t i = 0; i < number_of_audiodecoders; ++i)
    {
        availableIds.push_back(i);
    }
}

UT_status_t vts_audiodecoder_parseAudioDecoderHFP()
{
    std::string audiodecoderpath = AUDIODECODERMGR_IAUDIODECODER;

    int32_t number_of_audiodecoders = 0;
    int32_t supportedcodec_count = 0;

    audioDecoder temporary;

    number_of_audiodecoders = UT_KVP_PROFILE_GET_LIST_COUNT(audiodecoderpath.c_str());
    if (number_of_audiodecoders <= 0)
    {
        UT_LOG_ERROR("No audiodecoders found in the HFP");
        return UT_STATUS_FAILURE;
    }

    gAudiodecoders_hfp.clear();

    for (int32_t i = 0; i < number_of_audiodecoders; ++i)
    {
        std::string decoderEntryPath = audiodecoderpath + "." + std::to_string(i);
        std::string decoderIdKey = std::to_string(i);
        std::string decoderbasepath = decoderEntryPath + "." + decoderIdKey;

        std::string supportedcodecpath = decoderbasepath + ".supportedCodecs";
        std::string supportsSecurepath = decoderbasepath + ".supportsSecure";

        temporary.id = i;

        temporary.supportsSecure = UT_KVP_PROFILE_GET_BOOL(supportsSecurepath.c_str());
        supportedcodec_count = UT_KVP_PROFILE_GET_LIST_COUNT(supportedcodecpath.c_str());

        temporary.supportedCodecs.clear();
        for (int32_t j = 0; j < supportedcodec_count; ++j)
        {
            char result_kvp[UT_KVP_MAX_ELEMENT_SIZE] = {0};
            std::string codecpath = supportedcodecpath + "." + std::to_string(j);
            UT_KVP_PROFILE_GET_STRING(codecpath.c_str(), result_kvp);
            std::string _codec = std::string(result_kvp);
            temporary.supportedCodecs.push_back(vts_audioDecoder_helper_StringToCodec(_codec));
        }

        gAudiodecoders_hfp.push_back(temporary);
    }

    return UT_STATUS_OK;
}

bool isSecureModeSupported()
{
    for (const auto& decoder : gAudiodecoders_hfp)
    {
        if (decoder.supportsSecure)
            return true;
    }
    return false;
}
/** @} */ // End of AUDIO_DECODER_HALTEST_PARSE_HFP
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER
/** @} */ // End of HPK
