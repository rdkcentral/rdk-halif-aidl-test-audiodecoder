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
 * @defgroup AUDIO_DECODER_HALTEST_L3 Audio Decoder HAL Tests L3 File
 * @{
 * @parblock
 *
 * ### L3 Test Cases for Audio Decoder HAL :
 *
 * This module includes Level 3 functional tests (success and failure scenarios).
 * This is to ensure that the API meets the operational requirements of the module across all vendors.
 *
 * **Pre-Conditions:** @n
 * **Dependencies:** None@n
 *
 * @endparblock
 *
 */

/**
 * @file vts_audiodecoder_l3_tests.c
 *
 */

#include "vts_audiodecoder.h"
#include "vts_avbuffer.h"

/* Test class registration with UT_Core */
UT_ADD_TEST_TO_GROUP(VTS_L3_AudioDecoder, UT_TESTS_L3)

static const char *gModuleName = "audiodecoder";
static const char *gTestGroup = "L3";

static sp<IAVBuffer> vts_avbuffer_getService()
{
    return
        android::interface_cast<IAVBuffer>(
            android::defaultServiceManager()->getService(
                android::String16(IAVBuffer::serviceName().c_str())
            )
        );
}

class VTS_L3_AudioDecoder : public UTCore
{
public:
    VTS_L3_AudioDecoder() : UTCore() {}

    ~VTS_L3_AudioDecoder() override = default;

    vector<tuple<IAudioDecoder::Id, sp<IAudioDecoder>, sp<AudioDecoderControllerListener>, sp<AudioDecoderEventListener>, audiodecoder::Capabilities *>> mAudioDecoderList;
    sp<IAVBuffer> mAVBufferManager;
    IAVBufferHelper* mAVbufferHelper;

    void SetUp() override
    {
        try {
            mAVBufferManager = vts_avbuffer_getService();

            if (mAVBufferManager == nullptr) {
                UT_FAIL_FATAL("AVBuffer Service not available");
            }

            if (gAudioDecoderManager == nullptr) {
                UT_FAIL_FATAL("AudioDecoder Service not available");
            }

            vector<IAudioDecoder::Id> decoderIds;

            mAVbufferHelper = getAVBufferHelperInstance();

            if (mAVbufferHelper == nullptr) {
                UT_FAIL_FATAL("AVBuffer Helper not available");
            }

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

                mAudioDecoderList.push_back({_decoderId, audioDecoder, controlListener, eventListener, capabilities});
            }
        } catch (const std::exception& ex) {
            UT_LOG_ERROR("Exception in SetUp(): %s", ex.what());
            UT_ASSERT_TRUE(false);
        } catch (...) {
            UT_LOG_ERROR("Unknown exception in SetUp()");
            UT_ASSERT_TRUE(false);
        }
    }

    bool isDecoderUsable(const IAudioDecoder::Id& decoderId, audiodecoder::Capabilities* capabilities,
                        const testConfiguration_t& testConfig, bool secureDecode)
    {
        auto it = std::find_if(gAudiodecoders_hfp.begin(), gAudiodecoders_hfp.end(), [decoderId](const audioDecoder& item) {
            return item.id == decoderId.value;
        });
        if(it == gAudiodecoders_hfp.end())
            return false;
        if (secureDecode && !capabilities->supportsSecure)
            return false;
        auto result = std::find(capabilities->supportedCodecs.begin(), capabilities->supportedCodecs.end(), testConfig.codec);
        if(result == capabilities->supportedCodecs.end())
            return false;
        return true;
    }

    bool frameOutputMatchesTunnelMode(bool tunnelMode) const
    {
        bool outputObserved = false;
        for (const auto& decoderEntry : mAudioDecoderList)
        {
            const auto outputMode = std::get<2>(decoderEntry)->getFrameOutputMode();
            if (outputMode != FrameOutputMode::UNKNOWN)
            {
                outputObserved = true;
                if ((outputMode == FrameOutputMode::TUNNEL) != tunnelMode)
                {
                    return false;
                }
            }
        }
        return outputObserved;
    }

    void runSequentialDecode(testConfiguration_t& testConfig, bool secureDecode)
    {
        for (const auto& [_decoderId, decoderHandle, controlListener, eventListener, capabilities] : mAudioDecoderList)
        {
            HeapMetrics beforeHeapMetrics, afterHeapMetrics;
            bool isSinkAttached = false;

            if (!isDecoderUsable(_decoderId, capabilities, testConfig, secureDecode))
            {
                continue;
            }

            /* Reset per-stream state so each stream's errors and frames are validated independently */
            resetDecodeSession(eventListener, controlListener);

            /* Get heap metrics */
            AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, beforeHeapMetrics);

            {
                AudioDecoderTest decodeTest(_decoderId, decoderHandle, controlListener,
                                        eventListener, mAVBufferManager, mAVbufferHelper, secureDecode, testConfig, isSinkAttached);

                decodeTest.run(true);
            } /* decodeTest destructor called here — destroyPool fires before metrics check */

            /* Get heap metrics */
            AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, afterHeapMetrics);

            /* check if the decoder freed all the buffer pools */
            UT_ASSERT_TRUE(beforeHeapMetrics.secure == afterHeapMetrics.secure);
            UT_ASSERT_TRUE(beforeHeapMetrics.bytesUsed == afterHeapMetrics.bytesUsed);
            UT_ASSERT_TRUE(beforeHeapMetrics.bytesTotal == afterHeapMetrics.bytesTotal);

            /* TODO: check if this required or not */
            //controlListener->shutdown();
        }
    }

    void runConcurrentDecode(testConfiguration_t& testConfig, bool secureDecode)
    {
        try {
            vector<thread> threads;
            vector<AudioDecoderTest> decoderTestList;
            HeapMetrics beforeHeapMetrics, afterHeapMetrics;
            bool isSinkAttached = false;

            //TODO: add check of decoder metrics
            /* Get heap metrics */
            AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, beforeHeapMetrics);

            {
                std::vector<std::unique_ptr<AudioDecoderTest>> decoderTests;
                for (const auto& [_decoderId, decoderHandle, controlListener, eventListener, capabilities] : mAudioDecoderList)
                {
                    if (!isDecoderUsable(_decoderId, capabilities, testConfig, secureDecode))
                    {
                        continue;
                    }

                    /* Reset per-stream state so each decoder's errors and frames are validated independently */
                    resetDecodeSession(eventListener, controlListener);

                    auto decoderTest = std::make_unique<AudioDecoderTest>(_decoderId, decoderHandle,
                                                                    controlListener, eventListener,
                                                                    mAVBufferManager, mAVbufferHelper,
                                                                    secureDecode, testConfig, isSinkAttached);
                    threads.emplace_back(&AudioDecoderTest::run, decoderTest.get(), true);

                    decoderTests.push_back(std::move(decoderTest));
                }
                for (auto& t : threads)
                {
                    if (t.joinable())
                        t.join();
                }
            } /* all AudioDecoderTest destructors called here — destroyPool fires before metrics check */

            /* Get heap metrics */
            AVBUFFER_GETMETRICS(mAVBufferManager, secureDecode, afterHeapMetrics);

            /* check if the decoder freed all the buffer pools */
            UT_ASSERT_TRUE(beforeHeapMetrics.secure == afterHeapMetrics.secure);
            UT_ASSERT_TRUE(beforeHeapMetrics.bytesUsed == afterHeapMetrics.bytesUsed);
            UT_ASSERT_TRUE(beforeHeapMetrics.bytesTotal == afterHeapMetrics.bytesTotal);

        } catch (const std::exception& ex) {
            UT_LOG_ERROR("Exception in runConcurrentDecode: %s", ex.what());
            UT_ASSERT_TRUE(false);
        } catch (...) {
            UT_LOG_ERROR("Unknown exception in runConcurrentDecode");
            UT_ASSERT_TRUE(false);
        }
    }

    void runSingleProcessTest(const char *testName, bool secureMode, bool tunnelMode, bool concurrentMode)
    {
        if (secureMode && !isSecureModeSupported())
        {
            UT_LOG_INFO("Skipping test '%s': Secure mode is not supported", testName);
            GTEST_SKIP() << "Secure mode is not supported";
            return;
        }

        vector<testConfiguration_t> testConfiguration;

        vts_audiodecoder_parse_testconfig(gAudioDecoderTestConfiguration, gModuleName, gTestGroup, testName, testConfiguration);

        if(concurrentMode)
        {
            for(auto& test: testConfiguration)
            {
                runConcurrentDecode(test, secureMode);
            }
        }
        else
        {
            for(auto& test: testConfiguration)
            {
                runSequentialDecode(test, secureMode);
            }
        }

        if (!frameOutputMatchesTunnelMode(tunnelMode))
        {
            const char* requestedMode = tunnelMode ? "tunnelled" : "non-tunnelled";
            UT_LOG_INFO("Skipping test '%s': observed frame output does not match requested %s mode",
                        testName, requestedMode);
            GTEST_SKIP() << "Requested audio decoder output mode is not supported";
        }
    }

    void TearDown() override
    {
        try {
            for (const auto& [_decoderId, decoder, controlListener, eventListener, capabilities] : mAudioDecoderList)
            {
                /* Un-register the events */
                AUDIODECODER_UNREGISTER_EVENT_SUCCESS(decoder, eventListener);
                delete capabilities;
            }
            mAudioDecoderList.clear();
            //TODO: check right place to call joinThreadPool
            //android::IPCThreadState::self()->joinThreadPool();
        } catch (const std::exception& ex) {
            UT_LOG_ERROR("Exception in TearDown(): %s", ex.what());
        } catch (...) {
            UT_LOG_ERROR("Unknown exception in TearDown()");
        }
    }
};

/**
 * @brief Test the ability to decode audio frames
 *        Decoding type: Sequential
 *        Modes: Non-Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 001@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_NonTunnel_NonSecure)
{
    const char *testName = "Decode";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder behavior for error conditions in sequential decode
 *        Modes: Non-Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 002@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_NonTunnel_NonSecure_Error)
{
    const char *testName = "Decode_Error";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test trimming functionality during sequential decoding
 *        Modes: Non-Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 003@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_NonTunnel_NonSecure_Trim)
{
    const char *testName = "Decode_Trim";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity signal in sequential decode
 *        Modes: Non-Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 004@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_NonTunnel_NonSecure_Discontinuity)
{
    const char *testName = "Decode_Discontinuity";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of End-of-Stream (EOS) condition in sequential decode
 *        Modes: Non-Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 005@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_NonTunnel_NonSecure_EOS)
{
    const char *testName = "Decode_EOS";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder flushing capability mid-stream
 *        Modes: Non-Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 006@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_NonTunnel_NonSecure_Flush)
{
    const char *testName = "Decode_Flush";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder flush followed by reset operation
 *        Modes: Non-Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 007@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_NonTunnel_NonSecure_FlushReset)
{
    const char *testName = "Decode_FlushReset";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder stops and restart
 *        Modes: Non-Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 008@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_NonTunnel_NonSecure_StopStart)
{
    const char *testName = "Decode_Stop_Start";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Positive stress test verifying the HAL decodes without input queue back-pressure.
 *        Modes: Non-Tunnel, Non-Secure
 *
 * Submits a larger stream at delay=0 to stress the HAL's input pipeline.
 * The HAL must accept all frames without filling its internal input queue.
 * Both decodeBufferWithMetadata returning false (queueFullCount) and
 * onDecodeBufferAvailable firing (decodeBufferAvailCount) must remain at zero.
 * A non-zero count indicates the decoder is too slow relative to the submission
 * rate — the HAL's input queue filled during decode.
 *
 * This check runs on all decode tests. The dedicated test uses more frames and
 * delay=0 to maximise input queue pressure.
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 009@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_NonTunnel_NonSecure_OnDecodeBufAvail)
{
    const char *testName = "Decode_OnDecodeBufAvail";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test the ability to decode audio frames
 *        Decoding type: Sequential
 *        Modes: Non-Tunnel, Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 009@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_NonTunnel_Secure)
{
    const char *testName = "Decode";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder behavior for error conditions in sequential decode
 *        Modes: Non-Tunnel, Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 010@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_NonTunnel_Secure_Error)
{
    const char *testName = "Decode_Error";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test trimming functionality during sequential decoding
 *        Modes: Non-Tunnel, Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 011@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_NonTunnel_Secure_Trim)
{
    const char *testName = "Decode_Trim";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity signal in sequential decode
 *        Modes: Non-Tunnel, Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 012@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_NonTunnel_Secure_Discontinuity)
{
    const char *testName = "Decode_Discontinuity";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of End-of-Stream (EOS) condition in sequential decode
 *        Modes: Non-Tunnel, Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 013@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_NonTunnel_Secure_EOS)
{
    const char *testName = "Decode_EOS";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder flushing capability mid-stream
 *        Modes: Non-Tunnel, Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 014@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_NonTunnel_Secure_Flush)
{
    const char *testName = "Decode_Flush";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder flush followed by reset operation
 *        Modes: Non-Tunnel, Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 015@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_NonTunnel_Secure_FlushReset)
{
    const char *testName = "Decode_FlushReset";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder stops and restart
 *        Modes: Non-Tunnel, Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 016@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_NonTunnel_Secure_StopStart)
{
    const char *testName = "Decode_Stop_Start";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test sequential decode in tunnel mode (Non-Secure)
 *        Modes: Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 017@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_Tunnel_NonSecure)
{
    const char *testName = "Decode";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder behavior for error conditions in tunnel mode (Non-Secure)
 *        Modes: Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 018@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_Tunnel_NonSecure_Error)
{
    const char *testName = "Decode_Error";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test trimming functionality in tunnel mode (Non-Secure)
 *        Modes: Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 019@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_Tunnel_NonSecure_Trim)
{
    const char *testName = "Decode_Trim";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity in tunnel mode (Non-Secure)
 *        Modes: Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 020@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_Tunnel_NonSecure_Discontinuity)
{
    const char *testName = "Decode_Discontinuity";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of End-of-Stream (EOS) in tunnel mode (Non-Secure)
 *        Modes: Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 021@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_Tunnel_NonSecure_EOS)
{
    const char *testName = "Decode_EOS";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder flushing in tunnel mode (Non-Secure)
 *        Modes: Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 022@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_Tunnel_NonSecure_Flush)
{
    const char *testName = "Decode_Flush";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush followed by reset in tunnel mode (Non-Secure)
 *        Modes: Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 023@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_Tunnel_NonSecure_FlushReset)
{
    const char *testName = "Decode_FlushReset";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder stops and restart
 *        Modes: Tunnel, Non-Secure
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 024@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_Tunnel_NonSecure_StopStart)
{
    const char *testName = "Decode_Stop_Start";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test sequential decode in secure mode (Tunnel)
 *        Modes: Tunnel, Secure, Sequential
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 025@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_Tunnel_Secure)
{
    const char *testName = "Decode";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder error handling in sequential decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Sequential
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 026@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_Tunnel_Secure_Error)
{
    const char *testName = "Decode_Error";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test trimming functionality in sequential decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Sequential
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 027@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_Tunnel_Secure_Trim)
{
    const char *testName = "Decode_Trim";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity in sequential decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Sequential
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 028@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_Tunnel_Secure_Discontinuity)
{
    const char *testName = "Decode_Discontinuity";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of EOS in sequential decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Sequential
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 029@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_Tunnel_Secure_EOS)
{
    const char *testName = "Decode_EOS";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder flush in sequential decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Sequential
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 030@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_Tunnel_Secure_Flush)
{
    const char *testName = "Decode_Flush";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush and reset in sequential decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Sequential
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 031@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_Tunnel_Secure_FlushReset)
{
    const char *testName = "Decode_FlushReset";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder stops and restart
 *        Modes: Tunnel, Secure, Sequential
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 032@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Sequential_Tunnel_Secure_StopStart)
{
    const char *testName = "Decode_Stop_Start";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = false;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decode in non-tunnel mode (Non-Secure)
 *        Modes: Non-Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 033@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_NonTunnel_NonSecure)
{
    const char *testName = "Decode";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder error handling in concurrent decode (Non-Tunnel, Non-Secure)
 *        Modes: Non-Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 034@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_NonTunnel_NonSecure_Error)
{
    const char *testName = "Decode_Error";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test trimming functionality in concurrent decode (Non-Tunnel, Non-Secure)
 *        Modes: Non-Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 035@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_NonTunnel_NonSecure_Trim)
{
    const char *testName = "Decode_Trim";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity in concurrent decode (Non-Tunnel, Non-Secure)
 *        Modes: Non-Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 036@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_NonTunnel_NonSecure_Discontinuity)
{
    const char *testName = "Decode_Discontinuity";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of EOS in concurrent decode (Non-Tunnel, Non-Secure)
 *        Modes: Non-Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 037@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_NonTunnel_NonSecure_EOS)
{
    const char *testName = "Decode_EOS";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder flush in concurrent decode (Non-Tunnel, Non-Secure)
 *        Modes: Non-Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 038@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_NonTunnel_NonSecure_Flush)
{
    const char *testName = "Decode_Flush";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush and reset in concurrent decode (Non-Tunnel, Non-Secure)
 *        Modes: Non-Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 039@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_NonTunnel_NonSecure_FlushReset)
{
    const char *testName = "Decode_FlushReset";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder stops and restart
 *        Modes: Non-Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 040@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_NonTunnel_NonSecure_StopStart)
{
    const char *testName = "Decode_Stop_Start";
    bool secureMode = false;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}
/**
 * @brief Test concurrent decode in secure mode (Non-Tunnel)
 *        Modes: Non-Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 041@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_NonTunnel_Secure)
{
    const char *testName = "Decode_Secure";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder error handling in concurrent decode (Non-Tunnel, Secure)
 *        Modes: Non-Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 042@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_NonTunnel_Secure_Error)
{
    const char *testName = "Decode_Error";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test trimming functionality in concurrent decode (Non-Tunnel, Secure)
 *        Modes: Non-Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 043@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_NonTunnel_Secure_Trim)
{
    const char *testName = "Decode_Trim";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity in concurrent decode (Non-Tunnel, Secure)
 *        Modes: Non-Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 044@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_NonTunnel_Secure_Discontinuity)
{
    const char *testName = "Decode_Discontinuity";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of EOS in concurrent decode (Non-Tunnel, Secure)
 *        Modes: Non-Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 045@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_NonTunnel_Secure_EOS)
{
    const char *testName = "Decode_EOS";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder flush in concurrent decode (Non-Tunnel, Secure)
 *        Modes: Non-Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 046@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_NonTunnel_Secure_Flush)
{
    const char *testName = "Decode_Flush";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush and reset in concurrent decode (Non-Tunnel, Secure)
 *        Modes: Non-Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 047@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_NonTunnel_Secure_FlushReset)
{
    const char *testName = "Decode_FlushReset";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder stops and restart
 *        Modes: Non-Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 048@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_NonTunnel_Secure_StopStart)
{
    const char *testName = "Decode_Stop_Start";
    bool secureMode = true;
    bool tunnelMode = false;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decode in secure mode (Tunnel)
 *        Modes: Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 049@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_Tunnel_NonSecure)
{
    const char *testName = "Decode";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder error handling in concurrent decode (Tunnel, Non-Secure)
 *        Modes: Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 050@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_Tunnel_NonSecure_Error)
{
    const char *testName = "Decode_Error";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test trimming functionality in concurrent decode (Tunnel, Non-Secure)
 *        Modes: Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 051@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_Tunnel_NonSecure_Trim)
{
    const char *testName = "Decode_Trim";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity in concurrent decode (Tunnel, Non-Secure)
 *        Modes: Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 052@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_Tunnel_NonSecure_Discontinuity)
{
    const char *testName = "Decode_Discontinuity";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of EOS in concurrent decode (Tunnel, Non-Secure)
 *        Modes: Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 053@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_Tunnel_NonSecure_EOS)
{
    const char *testName = "Decode_EOS";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder flush in concurrent decode (Tunnel, Non-Secure)
 *        Modes: Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 054@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_Tunnel_NonSecure_Flush)
{
    const char *testName = "Decode_Flush";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush and reset in concurrent decode (Tunnel, Non-Secure)
 *        Modes: Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 055@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_Tunnel_NonSecure_FlushReset)
{
    const char *testName = "Decode_FlushReset";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder stops and restart
 *        Modes: Tunnel, Non-Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 056@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_Tunnel_NonSecure_StopStart)
{
    const char *testName = "Decode_Stop_Start";
    bool secureMode = false;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test concurrent decode in secure mode (Tunnel)
 *        Modes: Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 057@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_Tunnel_Secure)
{
    const char *testName = "Decode";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder error handling in concurrent decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 058@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_Tunnel_Secure_Error)
{
    const char *testName = "Decode_Error";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test trimming functionality in concurrent decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 059@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_Tunnel_Secure_Trim)
{
    const char *testName = "Decode_Trim";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of discontinuity in concurrent decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 060@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_Tunnel_Secure_Discontinuity)
{
    const char *testName = "Decode_Discontinuity";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test handling of EOS in concurrent decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 061@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_Tunnel_Secure_EOS)
{
    const char *testName = "Decode_EOS";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder flush in concurrent decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 062@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_Tunnel_Secure_Flush)
{
    const char *testName = "Decode_Flush";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test flush and reset in concurrent decode (Tunnel, Secure)
 *        Modes: Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 063@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_Tunnel_Secure_FlushReset)
{
    const char *testName = "Decode_FlushReset";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/**
 * @brief Test decoder stops and restart
 *        Modes: Tunnel, Secure, Concurrent
 *
 * **Test Group ID:** 003@n
 * **Test Case ID:** 064@n
 *
 * **Test Procedure:**
 * Refer to UT specification documentation [audioDecoder_High-Level_TestSpecification.md](../../docs/pages/audioDecoder_High-Level_TestSpecification.md)
 */
UT_ADD_TEST(VTS_L3_AudioDecoder, ADecode_Concurrent_Tunnel_Secure_StopStart)
{
    const char *testName = "Decode_Stop_Start";
    bool secureMode = true;
    bool tunnelMode = true;
    bool concurrentMode = true;
    runSingleProcessTest(testName, secureMode, tunnelMode, concurrentMode);
}

/** @} */ // End of AUDIO_DECODER_HALTEST L3 File
/** @} */ // End of AUDIO_DECODER_HALTEST
/** @} */ // End of AUDIO_DECODER Module
/** @} */ // End of HPK
