/**
 *  If not stated otherwise in this file or this component's LICENSE
 *  file the following copyright and licenses apply:
 *
 *  Copyright 2024 RDK Management
 *
 *  Licensed under the Apache License, Version 2.0 (the License);
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an AS IS BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
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
 * @defgroup AUDIO_DECODER_HALTEST_HELPER Audio Decoder HAL Tests helper functions
 * @{
 * @parblock
 *
 * ### Helper functions for VTS tests :
 *
 * Helper functions required for the module across all vendors.
 *
 * **Pre-Conditions:**  None @n
 * **Dependencies:** None @n
 *
 * @endparblock
 *
 */

/**
 * @file vts_audiodecoder_helper_functions.cpp
 *
 */

#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>

#include "vts_audiodecoder_helper_functions.h"
#include "vts_audiodecoder_parse_hfp.h"


void vts_audiodecoder_helper_validateCapabilities(int decoder_id, const audiodecoder::Capabilities& capabilities)
{
    bool decoderID = false;

    /*
     "decoders" is a vector, which contains information of audio decoders available in hal feature profile.
     The following are the information for each audio decoder:
     - The decoder ID.
     - Whether the decoder supports secure audio path (SAP) mode.
     - A list of supported codecs.
   */
    for(const auto& _decoder: gAudiodecoders_hfp)
    {
        if(_decoder.id == decoder_id)
        {
            decoderID = true;

            /* number of supported codec from Hal Feature Profile should be equal to  number of supported codec from getCapabilities() api */
            UT_ASSERT_EQUAL(_decoder.supportedCodecs.size(),capabilities.supportedCodecs.size());

            /* iterating over each supported codec from getCapabilities() api */
            for(const auto& supportedcodec_from_api: capabilities.supportedCodecs)
            {
                auto result = std::find(_decoder.supportedCodecs.begin(),_decoder.supportedCodecs.end(),supportedcodec_from_api);
                UT_ASSERT_NOT_EQUAL(result, _decoder.supportedCodecs.end());
            }

            /* ensure that supportsSecure value from  hal feature profile  is equal to the value from getCapabilities() api */
            UT_ASSERT_EQUAL(_decoder.supportsSecure, capabilities.supportsSecure);
        }
    }
    UT_ASSERT_TRUE(decoderID);
}

bool vts_audiodecoder_helper_areElementsUnique(const std::vector<int>& values)
{
    std::unordered_set<int> uniqueElements;

    for (const auto& value : values)
    {
        if(value < 0)
        {
            UT_LOG_ERROR("find Invalid decoder ID : %d", value);
            return false;
        }
        if (uniqueElements.find(value) != uniqueElements.end())
        {
            UT_LOG_ERROR("decoder ID is not unique : %d", value);
            return false;
        }
        uniqueElements.insert(value);
    }
    return true;
}

bool vts_audiodecoder_helper_validatePropertyValue(int decoder_id, const std::optional<PropertyValue>& propertyValue, const audiodecoder::Property property)
{
    if (!propertyValue.has_value() || !propertyValue->value.has_value())
    {
        return false;
    }

    std::variant<int32_t, std::string> val;
    if (propertyValue->value.has_value())
    {
        const auto& propVal = propertyValue->value.value();
        switch (propVal.getTag())
        {
            case PropertyValue::Value::Tag::intValue:
                val = propVal.get<PropertyValue::Value::Tag::intValue>();
                break;
            case PropertyValue::Value::Tag::stringValue:
            {
                val = propVal.get<PropertyValue::Value::Tag::stringValue>();  // std::string (@utf8InCpp)
            }
                break;
            case PropertyValue::Value::Tag::booleanValue:
                val = propVal.get<PropertyValue::Value::Tag::booleanValue>() ? 1 : 0;  // Store as int32_t (1 for true, 0 for false)
                break;

            default:
                std::cerr << "Unsupported type in PropertyValue::Value" << std::endl;
                break;
        }
    }

    switch (property)
    {
        case audiodecoder::Property::RESOURCE_ID:
             if(std::holds_alternative<int>(val) && std::get<int>(val) == decoder_id)
             {
                return true;
             }
             else
             {
                return false;
             }

        case audiodecoder::Property::LOW_LATENCY_MODE:
            return std::holds_alternative<int32_t>(val) &&
                   (std::get<int32_t>(val) == 0 || std::get<int32_t>(val) == 1);

        case audiodecoder::Property::AV_SOURCE:
            return std::holds_alternative<int32_t>(val) &&
                   (std::get<int32_t>(val) == static_cast<int32_t>(AVSource::AUTO) ||
                    std::get<int32_t>(val) == static_cast<int32_t>(AVSource::UNKNOWN) ||
                    (std::get<int32_t>(val) >= static_cast<int32_t>(AVSource::IP) &&
                     std::get<int32_t>(val) <= static_cast<int32_t>(AVSource::SYSTEM)) ||
                    (std::get<int32_t>(val) >= static_cast<int32_t>(AVSource::HDMI_1) &&
                     std::get<int32_t>(val) <= static_cast<int32_t>(AVSource::HDMI_5)) ||
                    (std::get<int32_t>(val) >= static_cast<int32_t>(AVSource::COMPOSITE_1) &&
                     std::get<int32_t>(val) <= static_cast<int32_t>(AVSource::COMPOSITE_5)));

        case audiodecoder::Property::SECURE_AUDIO:
            return std::holds_alternative<int32_t>(val) &&
                   (std::get<int32_t>(val) == 0 || std::get<int32_t>(val) == 1);

        case audiodecoder::Property::AC4_PRESENTATION_GROUP_INDEX:
            return std::holds_alternative<int32_t>(val) && std::get<int32_t>(val) >= -1;

        case audiodecoder::Property::AC4_PREFERRED_LANG1:
        case audiodecoder::Property::AC4_PREFERRED_LANG2:
            return std::holds_alternative<std::string>(val) &&
                   (std::get<std::string>(val).empty() || std::get<std::string>(val).size() == 3);

        case audiodecoder::Property::AC4_ASSOCIATED_TYPE:
            return std::holds_alternative<int32_t>(val) &&
                   (std::get<int32_t>(val) >= -1 && std::get<int32_t>(val) <= 2);

        case audiodecoder::Property::AC4_AUTO_SELECTION_PRIORITY:
            return std::holds_alternative<int32_t>(val) &&
                   (std::get<int32_t>(val) >= -1 && std::get<int32_t>(val) <= 1);

        case audiodecoder::Property::AC4_MIXER_BALANCE:
            return std::holds_alternative<int32_t>(val) &&
                   ((std::get<int32_t>(val) >= -32 && std::get<int32_t>(val) <= 32) || std::get<int32_t>(val) == 255);

        case audiodecoder::Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE:
            return std::holds_alternative<int32_t>(val) &&
                   (std::get<int32_t>(val) >= -1 && std::get<int32_t>(val) <= 1);

        default:
            return true;
    }
}

static uint32_t readLE(const uint8_t* data, size_t bytes)
{
    uint32_t val = 0;
    for (size_t i = 0; i < bytes; ++i) val |= data[i] << (8 * i);
    return val;
}

static inline uint32_t readBE(const uint8_t* data, size_t bytes)
{
    uint32_t val = 0;
    for (size_t i = 0; i < bytes; ++i) val |= data[i] << (8 * (bytes - 1 - i));
    return val;
}

static inline double bytesToDouble(const uint8_t* data, PCMFormat format)
{
    switch (format)
    {
        case PCMFormat::F64LE: return *reinterpret_cast<const double*>(data);
        case PCMFormat::F64BE: {
            uint64_t raw = readBE(data, 8);
            double d;
            std::memcpy(&d, &raw, 8);
            return d;
        }
        case PCMFormat::F32LE: return *reinterpret_cast<const float*>(data);
        case PCMFormat::F32BE: {
            uint32_t raw = readBE(data, 4);
            float f;
            std::memcpy(&f, &raw, 4);
            return f;
        }
        case PCMFormat::S32LE: return (int32_t)readLE(data, 4) / 2147483648.0;
        case PCMFormat::S32BE: return (int32_t)readBE(data, 4) / 2147483648.0;
        case PCMFormat::U32LE: return (readLE(data, 4) - 2147483648U) / 2147483648.0;
        case PCMFormat::U32BE: return (readBE(data, 4) - 2147483648U) / 2147483648.0;
        case PCMFormat::S24_32LE:
        case PCMFormat::S24LE: return ((int32_t)(readLE(data, 3) << 8)) / 2147483648.0;
        case PCMFormat::S24_32BE:
        case PCMFormat::S24BE: return ((int32_t)(readBE(data, 3) << 8)) / 2147483648.0;
        case PCMFormat::U24_32LE:
        case PCMFormat::U24LE: return ((readLE(data, 3) << 8) - 0x800000) / 8388608.0;
        case PCMFormat::U24_32BE:
        case PCMFormat::U24BE: return ((readBE(data, 3) << 8) - 0x800000) / 8388608.0;
        case PCMFormat::S20LE: return ((int32_t)(readLE(data, 3) << 12)) / 2147483648.0;
        case PCMFormat::S20BE: return ((int32_t)(readBE(data, 3) << 12)) / 2147483648.0;
        case PCMFormat::U20LE: return ((readLE(data, 3) << 12) - 0x800000) / 8388608.0;
        case PCMFormat::U20BE: return ((readBE(data, 3) << 12) - 0x800000) / 8388608.0;
        case PCMFormat::S18LE: return ((int32_t)(readLE(data, 3) << 14)) / 2147483648.0;
        case PCMFormat::S18BE: return ((int32_t)(readBE(data, 3) << 14)) / 2147483648.0;
        case PCMFormat::U18LE: return ((readLE(data, 3) << 14) - 0x800000) / 8388608.0;
        case PCMFormat::U18BE: return ((readBE(data, 3) << 14) - 0x800000) / 8388608.0;
        case PCMFormat::S16LE: return (int16_t)readLE(data, 2) / 32768.0;
        case PCMFormat::S16BE: return (int16_t)readBE(data, 2) / 32768.0;
        case PCMFormat::U16LE: return (readLE(data, 2) - 32768) / 32768.0;
        case PCMFormat::U16BE: return (readBE(data, 2) - 32768) / 32768.0;
        case PCMFormat::S8:    return static_cast<int8_t>(*data) / 128.0;
        case PCMFormat::U8:    return (*data - 128) / 128.0;
    }

    return 0.0;
}

double vts_computeRMSDiff(const void* ref_buf, const void* act_buf, size_t size, PCMFormat format)
{
    const uint8_t* ref = static_cast<const uint8_t*>(ref_buf);
    const uint8_t* act = static_cast<const uint8_t*>(act_buf);
    const size_t bps = vts_bytesPerSample(format);
    size_t num_samples = size / bps;

    double sum_sq = 0.0;

    for (size_t i = 0; i < num_samples; ++i)
    {
        double r = bytesToDouble(ref + i * bps, format);
        double a = bytesToDouble(act + i * bps, format);
        double diff = r - a;
        sum_sq += diff * diff;
    }

    return std::sqrt(sum_sq / num_samples);
}

int32_t vts_audioDecoder_helper_getSamplesPerFrame(audiodecoder::Codec codec)
{
    switch (codec)
    {
        case Codec::AAC_LC: return 1024;
        case Codec::HE_AAC:
        case Codec::HE_AAC2: return 2048;
        case Codec::AAC_ELD: return 512;
        case Codec::USAC:
        case Codec::X_HE_AAC: return 1024;
        case Codec::DOLBY_AC3:
        case Codec::DOLBY_AC3_PLUS:
        case Codec::DOLBY_AC3_PLUS_JOC:
        case Codec::DOLBY_MAT:
        case Codec::DOLBY_MAT2: return 1536;
        case Codec::MPEG2:
        case Codec::MP3: return 1152;
        case Codec::DTS: return 512;
        case Codec::OPUS: return 960;
        case Codec::SBC: return 128;
        default: return 0; /* variable or unknown — skip validation */
    }
}

int64_t vts_audio_frameDurationNs(int64_t frameBytes, int32_t sampleRate, int32_t numChannels, PCMFormat format)
{
    int32_t bytesPerSample = static_cast<int32_t>(vts_bytesPerSample(format)) * numChannels;
    int64_t numSamples     = (bytesPerSample > 0) ? frameBytes / bytesPerSample : 0;
    return (sampleRate > 0 && numSamples > 0) ? (numSamples * 1000000000LL) / sampleRate : 0;
}

int64_t vts_audio_totalSamples(int64_t accumulatedBytes, int32_t numChannels, PCMFormat format)
{
    int64_t bytesPerSample = static_cast<int64_t>(vts_bytesPerSample(format)) * numChannels;
    return (bytesPerSample > 0) ? accumulatedBytes / bytesPerSample : 0;
}

int64_t vts_audio_expectedSamples(int64_t durationNs, int32_t sampleRate)
{
    return static_cast<int64_t>(std::ceil(static_cast<double>(durationNs) * sampleRate / 1e9));
}

/** @} */ // End of AUDIO_DECODER_HALTEST_HELPER
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER
/** @} */ // End of HPK
