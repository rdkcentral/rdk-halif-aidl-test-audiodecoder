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

/** @brief
* AudioDecoder.cpp
*
* Implementation of AudioDecoder service
*/

#include "AudioDecoder.h"
#include <com/rdk/hal/audiodecoder/FrameMetadata.h>
#include <com/rdk/hal/audiodecoder/PCMMetadata.h>
#include <com/rdk/hal/audiodecoder/FrameType.h>
#include <com/rdk/hal/audiodecoder/PCMFormat.h>
#include <binder/IServiceManager.h>
#include <iostream>
#include <thread>
#include <chrono>
namespace com
{
    namespace rdk
    {
        namespace hal
        {
            namespace audiodecoder
            {

                bool isAlphabetOnly(const android::String16& str16)
                {
                    std::u16string str = str16.string();  // Convert String16 to std::u16string
                    return !str.empty() && std::all_of(str.begin(), str.end(), [](char16_t c) {
                        return (c >= u'A' && c <= u'Z') || (c >= u'a' && c <= u'z');
                    });
                }

                /* Read only Properties */
                // Codecs advertised as supported per decoder instance
                std::vector<::com::rdk::hal::audiodecoder::Codec> g_supported_codecs[NUM_AUDIO_DECODER] = {
                    {com::rdk::hal::audiodecoder::Codec::AAC_LC},
                    {com::rdk::hal::audiodecoder::Codec::AAC_LC}
                };
                IAudioDecoder::Id g_supported_id[NUM_AUDIO_DECODER];

                AudioDecoderManager::AudioDecoderManager()
                {
                    for (int32_t i = 0;i<NUM_AUDIO_DECODER;i++)
                    {
                        g_supported_id[i].value = i;
                    }
                }

                AudioDecoderManager::~AudioDecoderManager()
                {
                }

                android::binder::Status AudioDecoderManager::getAudioDecoderIds(  std::vector<IAudioDecoder::Id>* _aidl_return  )
                {
                    for (int32_t i = 0;i<NUM_AUDIO_DECODER;i++)
                    {
                        _aidl_return->push_back(g_supported_id[i]);
                    }
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderManager::getAudioDecoder(  const IAudioDecoder::Id& decoderResourceId, android::sp<IAudioDecoder>* _aidl_return  )
                {
                    android::binder::Status status;
                    int32_t found = 0;
                    for(int32_t i = 0; i < NUM_AUDIO_DECODER; i++)
                    {
                        if(g_supported_id[i].value == decoderResourceId.value)
                        {
                            found = 1;
                            break;
                        }
                    }
                    if(!found)
                    {
                        /* Invalid resource index: return null, no exception */
                        *_aidl_return = nullptr;
                        return android::binder::Status::ok();
                    }

                    android::sp<IAudioDecoder> audioDecoder = new AudioDecoder(decoderResourceId);
                    *_aidl_return = audioDecoder;
                    return android::binder::Status::ok();
                }

                AudioDecoder::AudioDecoder(IAudioDecoder::Id id)
                {
                    this->mId = id;
                    this->mEventListener = false;
                    this->mAudioDecoderEventListener = nullptr;
                    this->mLowLatency = 0;
                    this->mAVSource = AVSource::UNKNOWN;
                    this->mSupportsSecure = 1;
                    this->mMetricsFrameDecoded = 0;
                    this->mMetricsDecodeErrors = 0;
                    this->mMetricsFramesDropped = 0;
                    this->mAC4PresentationGroupIndex = -1;
                    this->mAC4PreferredLang1 = android::String16("");
                    this->mAC4PreferredLang2 = android::String16("");
                    this->mAC4AssociatedType = -1;
                    this->mAutoSelectionPriority = -1;
                    this->mAC4MixerBalance = 255;
                    this->mAC4AssociatedAudioMixingEnable = -1;

                    this->setState(State::CLOSED);
                    this->mSupportedCodecs = g_supported_codecs[this->mId.value];
                }

                AudioDecoder::~AudioDecoder()
                {
                }

                android::binder::Status AudioDecoder::getCapabilities(  audiodecoder::Capabilities* _aidl_return  )
                {
                    com::rdk::hal::audiodecoder::Capabilities capabilities;
                    capabilities.supportedCodecs = this->mSupportedCodecs;
                    capabilities.supportsSecure = this->mSupportsSecure;
                    *_aidl_return = capabilities;
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoder::getProperty(audiodecoder::Property property, std::optional<com::rdk::hal::PropertyValue>* _aidl_return)
                {
                    if (!_aidl_return)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
                    }
                    com::rdk::hal::PropertyValue value;
                    switch (property)
                    {
                        case audiodecoder::Property::AC4_PRESENTATION_GROUP_INDEX:
                        case audiodecoder::Property::AC4_PREFERRED_LANG1:
                        case audiodecoder::Property::AC4_PREFERRED_LANG2:
                        case audiodecoder::Property::AC4_ASSOCIATED_TYPE:
                        case audiodecoder::Property::AC4_AUTO_SELECTION_PRIORITY:
                        case audiodecoder::Property::AC4_MIXER_BALANCE:
                        case audiodecoder::Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE:
                            if (std::find(this->mSupportedCodecs.begin(), this->mSupportedCodecs.end(), audiodecoder::Codec::DOLBY_AC4) == this->mSupportedCodecs.end())
                            {
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            break;
                        default:
                            break;
                    }
                    switch (property)
                    {
                        case audiodecoder::Property::RESOURCE_ID:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->mId.value);
                            break;
                        case audiodecoder::Property::LOW_LATENCY_MODE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->mLowLatency);
                            break;
                        case audiodecoder::Property::AV_SOURCE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(static_cast<int32_t>(this->mAVSource));
                            break;
                        case audiodecoder::Property::SECURE_AUDIO:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->mSupportsSecure);
                            break;
                        case audiodecoder::Property::AC4_PRESENTATION_GROUP_INDEX:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->mAC4PresentationGroupIndex);
                            break;
                        case audiodecoder::Property::AC4_PREFERRED_LANG1:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::stringValue>(this->mAC4PreferredLang1);
                            break;
                        case audiodecoder::Property::AC4_PREFERRED_LANG2:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::stringValue>(this->mAC4PreferredLang2);
                            break;
                        case audiodecoder::Property::AC4_ASSOCIATED_TYPE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->mAC4AssociatedType);
                            break;
                        case audiodecoder::Property::AC4_AUTO_SELECTION_PRIORITY:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->mAutoSelectionPriority);
                            break;
                        case audiodecoder::Property::AC4_MIXER_BALANCE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->mAC4MixerBalance);
                            break;
                        case audiodecoder::Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->mAC4AssociatedAudioMixingEnable);
                            break;
                        case audiodecoder::Property::METRIC_FRAMES_DECODED:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->mMetricsFrameDecoded);
                            break;
                        case audiodecoder::Property::METRIC_DECODE_ERRORS:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->mMetricsDecodeErrors);
                            break;
                        case audiodecoder::Property::METRIC_FRAMES_DROPPED:
                            value.value = com::rdk::hal::PropertyValue::Value::make<com::rdk::hal::PropertyValue::Value::intValue>(this->mMetricsFramesDropped);
                            break;
                        default:
                            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    *_aidl_return = value;
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoder::getState(  State* _aidl_return  )
                {
                    *_aidl_return = this->getState();
                    return android::binder::Status::ok();
                }

                State AudioDecoder::getState()
                {
                    std::lock_guard<std::mutex> lock(this->mStateMutex);
                    State state = this->mAudioDecoderState;
                    return state;
                }
                void AudioDecoder::setState(  State newState  )
                {
                    std::lock_guard<std::mutex> lock(this->mStateMutex);
                    this->mAudioDecoderState = newState;
                }

                android::binder::Status AudioDecoder::open(  audiodecoder::Codec codec, bool secure, const android::sp<audiodecoder::IAudioDecoderControllerListener>& audioDecoderControllerListener, android::sp<audiodecoder::IAudioDecoderController>* _aidl_return  )
                {
                    android::binder::Status status;

                    if(std::find(this->mSupportedCodecs.begin(), this->mSupportedCodecs.end(), codec) == this->mSupportedCodecs.end())
                    {
                        *_aidl_return = nullptr;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    if(audioDecoderControllerListener == nullptr)
                    {
                        *_aidl_return = nullptr;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
                    }

                    if(this->getState() != State::CLOSED)
                    {
                        *_aidl_return = nullptr;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }

                    /* To simulate State Transitions */
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));

                    if (this->mEventListener)
                        auto status = this->mAudioDecoderEventListener->onStateChanged(this->getState(), State::OPENING);

                    this->setState(State::OPENING);
                    this->mCodec = codec;

                    std::this_thread::sleep_for(std::chrono::milliseconds(5));

                    android::sp<AudioDecoderController> controller = new AudioDecoderController(
                        this->mId, this, audioDecoderControllerListener, codec, secure);

                    if (!controller->isAVBufferAvailable())
                    {
                        /* Fail fast: without AVBuffer, decode/output buffer management can never
                         * work correctly, so reject the session instead of degrading silently. */
                        *_aidl_return = nullptr;
                        this->setState(State::CLOSED);
                        return android::binder::Status::fromExceptionCode(
                            android::binder::Status::EX_SERVICE_SPECIFIC, "AVBuffer service unavailable");
                    }

                    *_aidl_return = controller;

                    this->mMetricsFrameDecoded = 0;
                    this->mMetricsDecodeErrors = 0;
                    this->mMetricsFramesDropped = 0;

                    if (this->mEventListener)
                        this->mAudioDecoderEventListener->onStateChanged(State::OPENING, State::READY);

                    this->setState(State::READY);

                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoder::close(  const android::sp<audiodecoder::IAudioDecoderController>& audioDecoderController, bool* _aidl_return  )
                {
                    android::binder::Status status;
                    if(this->getState() != State::READY)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }

                    /* To simulate State Transitions */
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));

                    if (this->mEventListener)
                        auto status = this->mAudioDecoderEventListener->onStateChanged(this->getState(), State::CLOSING);

                    this->setState(State::CLOSING);

                    std::this_thread::sleep_for(std::chrono::milliseconds(5));

                    * _aidl_return = true;

                    if (this->mEventListener)
                        this->mAudioDecoderEventListener->onStateChanged(State::CLOSING, State::CLOSED);

                    this->setState(State::CLOSED);

                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoder::registerEventListener(  const android::sp<audiodecoder::IAudioDecoderEventListener>& audioDecoderEventListener, bool* _aidl_return  )
                {
                    android::binder::Status status;

                    if(audioDecoderEventListener == nullptr)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
                    }
                    if(this->mEventListener == true)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::ok();
                    }
                    *_aidl_return = true;
                    this->mEventListener = true;
                    this->mAudioDecoderEventListener = audioDecoderEventListener;
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoder::unregisterEventListener(  const android::sp<audiodecoder::IAudioDecoderEventListener>& audioDecoderEventListener, bool* _aidl_return  )
                {
                    android::binder::Status status;

                    if(audioDecoderEventListener == nullptr)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
                    }
                    if(this->mEventListener ==false)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::ok();
                    }
                    *_aidl_return = true;
                    this->mEventListener = false;
                    this->mAudioDecoderEventListener = nullptr;
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::getCurrentOperationalMode(OperationalMode* _aidl_return)
                {
                    if (_aidl_return == nullptr)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
                    }
                    if(this->mAudioDecoderState != State::STARTED)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    *_aidl_return = OperationalMode::NON_TUNNEL;
                    return android::binder::Status::ok();
                }

                AudioDecoderController::AudioDecoderController(IAudioDecoder::Id id, AudioDecoder *parentDecoder,
                                                               android::sp<audiodecoder::IAudioDecoderControllerListener> listener,
                                                               Codec codec, bool secure)
                {
                    this->mId = id;
                    this->mParentDecoder = parentDecoder;
                    this->mEosSignalled = false;
                    this->mDiscontinuityPending = false;
                    this->mControllerListener = listener;
                    this->mQueueCount = 0;
                    this->mQueueGeneration = 0;
                    this->mTotalBuffersSubmitted = 0;
                    this->mCodec = codec;
                    this->mChannels = 2;
                    this->mSampleRate = 48000;
                    this->mFirstFrameAfterStart = false;
                    this->mSecure = secure;
                    this->mOutputPoolValid = false;
                    this->mOutputPoolSpaceListener = new OutputPoolSpaceListener();
                    this->mAVbufferHelper = getAVBufferHelperInstance();

                    android::sp<android::IBinder> binder = android::defaultServiceManager()->getService(
                        android::String16(::com::rdk::hal::avbuffer::IAVBuffer::serviceName().c_str()));
                    this->mAVBuffer = android::interface_cast<::com::rdk::hal::avbuffer::IAVBuffer>(binder);
                }

                bool AudioDecoderController::isAVBufferAvailable() const
                {
                    return this->mAVBuffer != nullptr;
                }

                int32_t AudioDecoderController::getSamplesPerFrameForCodec(Codec codec)
                {
                    /* Codec-standard samples-per-frame constants */
                    switch (codec)
                    {
                        case Codec::AAC_LC:
                            return 1024;
                        default:
                            return 0;
                    }
                }

                AudioDecoderController::~AudioDecoderController()
                {
                    // Tear down the session's output pool
                    if (this->mAVBuffer != nullptr && this->mOutputPoolValid)
                    {
                        bool destroyResult = false;
                        android::binder::Status destroyStatus = this->mAVBuffer->destroyPool(this->mOutputPool, &destroyResult);
                        if (!destroyStatus.isOk() || !destroyResult)
                        {
                            std::cout << "[AudioDecoder] destroyPool for internal output pool failed at session end: "
                                      << "exceptionCode=" << destroyStatus.exceptionCode()
                                      << " message=" << destroyStatus.exceptionMessage()
                                      << " destroyResult=" << destroyResult << std::endl;
                        }
                        this->mOutputPoolValid = false;
                    }
                }

                // ADTS (AAC) fixed-header validity check: syncword, sampling_frequency_index,
                // frame_length vs. actual buffer size.
                bool AudioDecoderController::isFrameContentValid(Codec codec, const uint8_t *data, uint32_t size, std::string *reason)
                {
                    if (codec != Codec::AAC_LC)
                    {
                        // No parser for other codecs; assume valid
                        return true;
                    }
                    if (data == nullptr || size < 7)
                    {
                        if (reason) *reason = "buffer too small for an ADTS header";
                        return false;
                    }
                    if (data[0] != 0xFF || (data[1] & 0xF0) != 0xF0)
                    {
                        if (reason) *reason = "ADTS syncword invalid";
                        return false;
                    }
                    uint8_t samplingFreqIndex = (data[2] >> 2) & 0x0F;
                    if (samplingFreqIndex == 0x0F)
                    {
                        if (reason) *reason = "ADTS sampling_frequency_index reserved (15)";
                        return false;
                    }
                    uint32_t frameLength = (static_cast<uint32_t>(data[3] & 0x03) << 11)
                                          | (static_cast<uint32_t>(data[4]) << 3)
                                          | (static_cast<uint32_t>(data[5]) >> 5);
                    if (frameLength != size)
                    {
                        if (reason) *reason = "ADTS frame_length does not match actual buffer size";
                        return false;
                    }
                    return true;
                }

                android::binder::Status AudioDecoderController::start()
                {
                    android::binder::Status status;
                    if(this->mParentDecoder->getState() != State::READY)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }

                    /* To simulate State Transitions */
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));

                    if (this->mParentDecoder->mEventListener)
                        auto status = this->mParentDecoder->mAudioDecoderEventListener->onStateChanged(this->mParentDecoder->getState(), State::STARTING);

                    this->mParentDecoder->setState(State::STARTING);

                    std::this_thread::sleep_for(std::chrono::milliseconds(5));

                    // PCM output needs a working output pool; SOC passthrough codecs (Dolby/DTS)
                    // never allocate output buffers, so a pool failure doesn't block them. Done
                    // before committing to STARTED so a failure can cleanly revert to READY
                    // instead of silently running with outputBufferHandle stuck at -1 forever.
                    if (this->mAVBuffer != nullptr && !this->mOutputPoolValid &&
                        getFrameTypeForCodec(this->mCodec) == FrameType::PCM)
                    {
                        ::com::rdk::hal::avbuffer::Pool pool;
                        android::binder::Status poolStatus = this->mAVBuffer->createAudioPool(
                            this->mSecure, this->mId, this->mOutputPoolSpaceListener, &pool);
                        if (poolStatus.isOk())
                        {
                            this->mOutputPool = pool;
                            this->mOutputPoolValid = true;
                        }
                        else
                        {
                            std::cout << "[AudioDecoder] createAudioPool for internal output pool failed: "
                                      << "exceptionCode=" << poolStatus.exceptionCode()
                                      << " message=" << poolStatus.exceptionMessage()
                                      << std::endl;
                            this->mParentDecoder->setState(State::READY);
                            return android::binder::Status::fromExceptionCode(
                                android::binder::Status::EX_SERVICE_SPECIFIC, "Failed to create output pool");
                        }
                    }

                    if (this->mParentDecoder->mEventListener)
                        this->mParentDecoder->mAudioDecoderEventListener->onStateChanged(State::STARTING, State::STARTED);

                    this->mParentDecoder->setState(State::STARTED);
                    this->mFirstFrameAfterStart = true;

                    // Metrics are cumulative across stop()+start() (Property.aidl) - reset only
                    // happens on open() (fresh AudioDecoder instance) or flush(reset=true).

                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::stop()
                {
                    android::binder::Status status;
                    if(this->mParentDecoder->getState() != State::STARTED)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }

                    this->mEosSignalled = false;
                    this->mDiscontinuityPending = false;
                    this->mQueueGeneration++;
                    this->mQueueCount = 0;
                    this->mTotalBuffersSubmitted = 0;
                    this->mFirstFrameAfterStart = false;

                    // Output pool spans the whole open() session; not torn down on stop()

                    /* To simulate State Transitions */
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));

                    if (this->mParentDecoder->mEventListener)
                        auto status = this->mParentDecoder->mAudioDecoderEventListener->onStateChanged(this->mParentDecoder->getState(), State::STOPPING);

                    this->mParentDecoder->setState(State::STOPPING);

                    std::this_thread::sleep_for(std::chrono::milliseconds(5));

                    if (this->mParentDecoder->mEventListener)
                        this->mParentDecoder->mAudioDecoderEventListener->onStateChanged(State::STOPPING, State::READY);

                    this->mParentDecoder->setState(State::READY);

                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::decodeBufferWithMetadata( int64_t bufferHandle, const audiodecoder::InputBufferMetadata& metadata, bool* _aidl_return )
                {
                    if(bufferHandle < 0)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    if(this->mParentDecoder->getState() != State::STARTED)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    if(this->mEosSignalled)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    if(metadata.nsPresentationTime < 0)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    if(metadata.trimStartNs < 0)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    if(metadata.trimEndNs < 0)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    if(metadata.discontinuity)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    // CAS loop so check-and-increment is one atomic step, avoiding a race that could overbook the queue.
                    int32_t expectedQueueCount = this->mQueueCount.load(std::memory_order_relaxed);
                    for (;;)
                    {
                        if (expectedQueueCount >= kQueueDepth)
                        {
                            *_aidl_return = false;
                            android::sp<audiodecoder::IAudioDecoderControllerListener> listener = this->mControllerListener;
                            std::thread([listener]() {
                                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                                if (listener)
                                    listener->onDecodeBufferAvailable();
                            }).detach();
                            return android::binder::Status::ok();
                        }
                        if (this->mQueueCount.compare_exchange_weak(expectedQueueCount, expectedQueueCount + 1,
                                                                      std::memory_order_acq_rel, std::memory_order_relaxed))
                        {
                            break;
                        }
                        // Someone else changed it first; expectedQueueCount now holds the latest value, so retry.
                    }

                    // Buffer accepted - queue slot reserved above. Tag the slot with the current
                    // generation so its eventual release is skipped if stop()/flush() resets the
                    // queue for a new session before this frame finishes processing.
                    uint32_t queueGeneration = this->mQueueGeneration.load(std::memory_order_acquire);
                    this->mTotalBuffersSubmitted++;
                    *_aidl_return = true;

                    // Validate frame content; failures reported via onDecodeError()
                    bool frameValid = true;
                    if (this->mAVbufferHelper != nullptr)
                    {
                        uint32_t mappedSize = 0;
                        void *mapped = this->mAVbufferHelper->mapHandle(static_cast<uint64_t>(bufferHandle), &mappedSize);
                        if (mapped != nullptr)
                        {
                            std::string reason;
                            frameValid = isFrameContentValid(this->mCodec, static_cast<const uint8_t *>(mapped), mappedSize, &reason);
                            if (!frameValid)
                            {
                                std::cout << "[AudioDecoder] frame at PTS " << metadata.nsPresentationTime
                                          << " failed content validation: " << reason << std::endl;
                            }
                            this->mAVbufferHelper->unmapHandle(static_cast<uint64_t>(bufferHandle));
                        }
                    }

                    // Input buffer ownership transfers to the decoder; free it here
                    if (this->mAVBuffer != nullptr)
                    {
                        bool freeResult = false;
                        this->mAVBuffer->free(bufferHandle, &freeResult);
                    }

                    if (!frameValid)
                    {
                        // No output for this frame - release its queue slot
                        this->releaseQueueSlot(queueGeneration);
                        this->mParentDecoder->mMetricsDecodeErrors++;
                        if (this->mParentDecoder->mEventListener)
                        {
                            this->mParentDecoder->mAudioDecoderEventListener->onDecodeError(
                                ErrorCode::INVALID_ARGUMENT, 0);
                        }
                        return android::binder::Status::ok();
                    }

                    this->mParentDecoder->mMetricsFrameDecoded++;

                    // Consume the pending discontinuity flag (exchange: one atomic step so a
                    // racing decodeBufferWithMetadata() call can't also observe it as pending)
                    bool isDiscontinuous = this->mDiscontinuityPending.exchange(false, std::memory_order_acq_rel);

                    // Allocate a real output buffer for PCM frames; SOC passthrough
                    // codecs (Dolby/DTS) keep frameAVBufferHandle = -1
                    int64_t outputBufferHandle = -1;
                    if (getFrameTypeForCodec(this->mCodec) == FrameType::PCM && this->mOutputPoolValid)
                    {
                        int32_t samplesPerFrame = getSamplesPerFrameForCodec(this->mCodec);
                        if (samplesPerFrame > 0)
                        {
                            int32_t bytes = samplesPerFrame * this->mChannels * static_cast<int32_t>(sizeof(float));
                            int64_t handle = -1;
                            android::binder::Status allocStatus = this->mAVBuffer->alloc(this->mOutputPool, bytes, &handle);
                            if (allocStatus.isOk() && handle >= 0)
                            {
                                outputBufferHandle = handle;
                            }
                        }
                    }

                    fireOnFrameOutputCallback(metadata.nsPresentationTime, this->mCodec,
                                              metadata.trimStartNs, metadata.trimEndNs, outputBufferHandle, isDiscontinuous,
                                              queueGeneration);
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::flush(  bool reset  )
                {
                    android::binder::Status status;
                    if(this->mParentDecoder->getState() != State::STARTED)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }

                    this->mEosSignalled = false;
                    this->mDiscontinuityPending = false;
                    this->mQueueGeneration++;
                    this->mQueueCount = 0;

                    /* To simulate State Transitions */
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));

                    if (this->mParentDecoder->mEventListener)
                        auto status = this->mParentDecoder->mAudioDecoderEventListener->onStateChanged(this->mParentDecoder->getState(), State::FLUSHING);

                    this->mParentDecoder->setState(State::FLUSHING);

                    std::this_thread::sleep_for(std::chrono::milliseconds(5));

                    if (reset)
                    {
                        if (this->mParentDecoder->mEventListener)
                            this->mParentDecoder->mAudioDecoderEventListener->onStateChanged(State::FLUSHING, State::READY);
                        this->mParentDecoder->setState(State::READY);

                        // Reset per-session metrics (Property.aidl: metrics reset only on open()/flush()).
                        this->mParentDecoder->mMetricsFrameDecoded = 0;
                        this->mParentDecoder->mMetricsDecodeErrors = 0;
                        this->mParentDecoder->mMetricsFramesDropped = 0;

                        // Full reset back to READY (per spec) starts a new session for
                        // signalEndOfStream()'s "no buffers submitted on this session" check.
                        this->mTotalBuffersSubmitted = 0;
                    }
                    else
                    {
                        if (this->mParentDecoder->mEventListener)
                            this->mParentDecoder->mAudioDecoderEventListener->onStateChanged(State::FLUSHING, State::STARTED);
                        this->mParentDecoder->setState(State::STARTED);
                        this->mFirstFrameAfterStart = true;
                    }
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::signalDiscontinuity()
                {
                    android::binder::Status status;
                    if(this->mParentDecoder->getState() != State::STARTED)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    this->mDiscontinuityPending = true;
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::signalEndOfStream()
                {
                    if(this->mParentDecoder->getState() != State::STARTED)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    if(this->mTotalBuffersSubmitted == 0)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    // CAS so two concurrent signalEndOfStream() calls can't both pass and both fire the callback
                    bool expectedEos = false;
                    if (!this->mEosSignalled.compare_exchange_strong(expectedEos, true, std::memory_order_acq_rel))
                    {
                        // Second call is a no-op
                        return android::binder::Status::ok();
                    }
                    fireOnEndOfStreamCallback();
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::setAudioFormat( int32_t channels, int32_t sampleRate )
                {
                    if(this->mParentDecoder->getState() != State::READY)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    if(channels <= 0 || sampleRate <= 0)
                    {
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    this->mChannels = channels;
                    this->mSampleRate = sampleRate;
                    return android::binder::Status::ok();
                }

                FrameType AudioDecoderController::getFrameTypeForCodec(Codec codec)
                {
                    // Dolby/DTS passthrough codecs output as SOC_PROPRIETARY
                    switch(codec)
                    {
                        case Codec::DOLBY_AC3:
                        case Codec::DOLBY_AC3_PLUS:
                        case Codec::DOLBY_AC3_PLUS_JOC:
                        case Codec::DOLBY_AC4:
                        case Codec::DOLBY_MAT:
                        case Codec::DOLBY_MAT2:
                        case Codec::DOLBY_TRUEHD:
                        case Codec::DTS:
                            return FrameType::SOC_PROPRIETARY;
                        default:
                            return FrameType::PCM;
                    }
                }

                void AudioDecoderController::releaseQueueSlot(uint32_t queueGeneration)
                {
                    // stop()/flush() already started a new session; this slot belongs to a stale one, so skip it.
                    if (this->mQueueGeneration.load(std::memory_order_acquire) != queueGeneration)
                    {
                        return;
                    }

                    // CAS loop that never decrements past 0, so a race with a concurrent reset can't go negative.
                    int32_t current = this->mQueueCount.load(std::memory_order_relaxed);
                    while (current > 0 &&
                           !this->mQueueCount.compare_exchange_weak(current, current - 1,
                                                                     std::memory_order_acq_rel, std::memory_order_relaxed))
                    {
                    }
                }

                void AudioDecoderController::fireOnFrameOutputCallback(int64_t nsPresentationTime, Codec sourceCodec,
                                                                       int32_t trimStartNs, int32_t trimEndNs,
                                                                       int64_t outputBufferHandle, bool discontinuity,
                                                                       uint32_t queueGeneration)
                {
                    android::sp<audiodecoder::IAudioDecoderControllerListener> listener = this->mControllerListener;
                    // Capture strong references (not raw `this`/`parent`) so the detached thread
                    // below keeps both RefBase objects alive for its own duration - otherwise a
                    // close() that drops the last sp<> while this thread is still sleeping would
                    // leave it dereferencing freed memory.
                    android::sp<AudioDecoderController> self = this;
                    android::sp<AudioDecoder> parent = this->mParentDecoder;
                    // Exchange: one atomic step so a racing decodeBufferWithMetadata() call can't
                    // also observe this frame as the first one.
                    bool firstFrame = this->mFirstFrameAfterStart.exchange(false, std::memory_order_acq_rel);
                    int32_t channels = this->mChannels;
                    int32_t sampleRate = this->mSampleRate;

                    std::thread([self, listener, parent, sourceCodec, nsPresentationTime, trimStartNs, trimEndNs,
                                 firstFrame, channels, sampleRate, outputBufferHandle, discontinuity, queueGeneration]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));

                        // Frame processing complete - release its queue slot
                        self->releaseQueueSlot(queueGeneration);

                        if (!listener) return;

                        // Build FrameMetadata
                        FrameMetadata frameMetadata;

                        // Determine frame type and PCM metadata
                        if (sourceCodec == Codec::DOLBY_AC3 || sourceCodec == Codec::DOLBY_AC3_PLUS ||
                            sourceCodec == Codec::DOLBY_AC3_PLUS_JOC || sourceCodec == Codec::DOLBY_AC4 ||
                            sourceCodec == Codec::DOLBY_MAT || sourceCodec == Codec::DOLBY_MAT2 ||
                            sourceCodec == Codec::DOLBY_TRUEHD || sourceCodec == Codec::DTS)
                        {
                            frameMetadata.type = FrameType::SOC_PROPRIETARY;
                            frameMetadata.metadata = std::nullopt;  // Per spec: "Else metadata is null"
                            frameMetadata.SoCPrivate = {};     // Empty in mock (no real SoC opaque data)
                        }
                        else
                        {
                            frameMetadata.type = FrameType::PCM;
                            if (firstFrame)
                            {
                                // Build PCM metadata in-place (PCMMetadata has deleted move assignment)
                                frameMetadata.metadata.emplace();
                                frameMetadata.metadata->numChannels = channels;
                                frameMetadata.metadata->sampleRate = sampleRate;
                                frameMetadata.metadata->format = PCMFormat::F32LE;
                                frameMetadata.metadata->planarFormat = false;
                            }
                            else
                            {
                                frameMetadata.metadata = std::nullopt;  // Null=unchanged optimization after first frame
                            }
                            frameMetadata.SoCPrivate = {};
                        }

                        frameMetadata.sourceCodec = sourceCodec;
                        frameMetadata.isDolbyAtmos = false;
                        frameMetadata.trimStartNs = trimStartNs;
                        frameMetadata.trimEndNs = trimEndNs;
                        frameMetadata.lowLatency = (parent->mLowLatency == 1);
                        frameMetadata.bitstreamEOS = false;
                        frameMetadata.discontinuity = discontinuity;
                        frameMetadata.source = parent->mAVSource;

                        // Real handle for PCM frames, -1 for SOC passthrough/tunnelled
                        listener->onFrameOutput(nsPresentationTime, outputBufferHandle, frameMetadata);
                    }).detach();
                }

                void AudioDecoderController::fireOnEndOfStreamCallback()
                {
                    android::sp<audiodecoder::IAudioDecoderControllerListener> listener = this->mControllerListener;

                    std::thread([listener]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        if (listener)
                            listener->onEndOfStream();
                    }).detach();
                }

                // Per-format sanity check for codec-specific-data payloads
                bool isValidCodecSpecificData(rdk::hal::audiodecoder::CSDAudioFormat format, const std::vector<uint8_t>& codecData)
                {
                    // Minimum length per format
                    size_t minLength = (format == rdk::hal::audiodecoder::CSDAudioFormat::AC4_SPECIFIC_CONFIG) ? 4 : 2;
                    if (codecData.size() < minLength)
                    {
                        return false;
                    }

                    // Reject degenerate all-identical-byte payloads
                    bool allSameByte = true;
                    for (uint8_t b : codecData)
                    {
                        if (b != codecData[0])
                        {
                            allSameByte = false;
                            break;
                        }
                    }
                    if (allSameByte)
                    {
                        return false;
                    }

                    if (format == rdk::hal::audiodecoder::CSDAudioFormat::MP4_AUDIO_SPECIFIC_CONFIG)
                    {
                        // AAC AudioSpecificConfig: audioObjectType is top 5 bits of byte 0
                        uint8_t audioObjectType = (codecData[0] >> 3) & 0x1F;
                        if (audioObjectType == 0 || (audioObjectType == 31 && codecData.size() < 3))
                        {
                            return false;
                        }
                    }

                    return true;
                }

                android::binder::Status AudioDecoderController::parseCodecSpecificData(  rdk::hal::audiodecoder::CSDAudioFormat csdAudioFormat, const std::vector<uint8_t>& codecData, bool* _aidl_return  )
                {
                    if(this->mParentDecoder->getState() != State::STARTED)
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                    }
                    if(!isValidCodecSpecificData(csdAudioFormat, codecData))
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    *_aidl_return = true;
                    return android::binder::Status::ok();
                }

                android::binder::Status AudioDecoderController::setProperty( audiodecoder::Property property, const PropertyValue& propertyValue, bool* _aidl_return )
                {
                    int32_t intvalue;
                    android::String16 strvalue;
                    *_aidl_return = false;
                    if (!propertyValue.value.has_value())
                    {
                        *_aidl_return = false;
                        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    switch (property)
                    {
                        case audiodecoder::Property::RESOURCE_ID:
                        case audiodecoder::Property::SECURE_AUDIO:
                        case audiodecoder::Property::METRIC_FRAMES_DECODED:
                        case audiodecoder::Property::METRIC_DECODE_ERRORS:
                        case audiodecoder::Property::METRIC_FRAMES_DROPPED:
                            *_aidl_return = false;
                            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_UNSUPPORTED_OPERATION);
                        case audiodecoder::Property::LOW_LATENCY_MODE:
                            if(this->mParentDecoder->getState() != State::READY)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                            }
                            intvalue  = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();

                            if(intvalue < 0 || intvalue > 1)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            this->mParentDecoder->mLowLatency = intvalue;
                            break;
                        case audiodecoder::Property::AV_SOURCE:
                            if(this->mParentDecoder->getState() != State::READY)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
                            }
                            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();
                            if (intvalue < static_cast<int32_t>(AVSource::AUTO) || intvalue > static_cast<int32_t>(AVSource::COMPOSITE_5))
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            this->mParentDecoder->mAVSource = static_cast<AVSource>(intvalue);
                            break;
                        case audiodecoder::Property::AC4_PRESENTATION_GROUP_INDEX:
                            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();

                            if(intvalue < -1)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            this->mParentDecoder->mAC4PresentationGroupIndex = intvalue;
                            break;
                        case audiodecoder::Property::AC4_PREFERRED_LANG1:
                            strvalue = propertyValue.value->get<PropertyValue::Value::Tag::stringValue>();

                            if((strvalue.size() != 0) && !isAlphabetOnly(strvalue))
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            this->mParentDecoder->mAC4PreferredLang1 = strvalue;
                            break;
                        case audiodecoder::Property::AC4_PREFERRED_LANG2:
                            strvalue = propertyValue.value->get<PropertyValue::Value::Tag::stringValue>();

                            if((strvalue.size() != 0) && !isAlphabetOnly(strvalue))
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            this->mParentDecoder->mAC4PreferredLang2 = strvalue;
                            break;
                        case audiodecoder::Property::AC4_ASSOCIATED_TYPE:
                            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();

                            if(intvalue < -1 || intvalue > 2)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            this->mParentDecoder->mAC4AssociatedType = intvalue;
                            break;
                        case audiodecoder::Property::AC4_AUTO_SELECTION_PRIORITY:
                            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();

                            if(intvalue < -1 || intvalue > 1)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            this->mParentDecoder->mAutoSelectionPriority = intvalue;
                            break;
                        case audiodecoder::Property::AC4_MIXER_BALANCE:
                            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();

                            if(intvalue < -32 || (intvalue > 32 && intvalue != 255))
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            this->mParentDecoder->mAC4MixerBalance = intvalue;
                            break;
                        case audiodecoder::Property::AC4_ASSOCIATED_AUDIO_MIXING_ENABLE:
                            intvalue = propertyValue.value->get<PropertyValue::Value::Tag::intValue>();

                            if(intvalue < -1 || intvalue > 1)
                            {
                                *_aidl_return = false;
                                return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                            }
                            this->mParentDecoder->mAC4AssociatedAudioMixingEnable = intvalue;
                            break;
                        default:
                            *_aidl_return = false;
                            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
                    }
                    *_aidl_return = true;
                    return android::binder::Status::ok();
                }
            }
        }
    }
}
