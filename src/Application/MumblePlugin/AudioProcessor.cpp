// For conditions of distribution and use, see copyright notice in LICENSE

#include "AudioProcessor.h"
#include "MumblePlugin.h"
#include "MumbleData.h"
#include "CeltCodec.h"
#include "celt/celt.h"

#include "Framework.h"
#include "Profiler.h"
#include "AudioAPI.h"
#include "CoreDefines.h"
#include "LoggingFunctions.h"

#include <QMutexLocker>

namespace MumbleAudio
{
    AudioProcessor::AudioProcessor(Framework *framework_, MumbleAudio::AudioSettings settings) :
        LC("[MumbleAudioProcessor]: "),
        framework(framework_),
        codec(new CeltCodec()),
        speexPreProcessor(0),
        outputPreProcessed(false),
        preProcessorReset(true),
        isSpeech(false),
        wasPreviousSpeech(false),
        holdFrames(0),
        qualityFramesPerPacket(MUMBLE_AUDIO_FRAMES_PER_PACKET_ULTRA)
    {
        ApplySettings(settings);
    }

    AudioProcessor::~AudioProcessor()
    {
        // Cleanup is done in run() when thread exits.
    }

    void AudioProcessor::ResetSpeexProcessor()
    {
        if (!preProcessorReset)
            return;
        preProcessorReset = false;
        
        AudioSettings currentSettings;
        
        {
            QMutexLocker lock(&mutexAudioSettings);
            currentSettings = audioSettings;
        }
        
        outputPreProcessed = false;
        if (currentSettings.suppression < 0 || currentSettings.amplification > 0)
            outputPreProcessed = true;

        // Only usage of speexPreProcessor ptr in another thread is behind this lock.
        QMutexLocker outputLock(&mutexOutputPCM);
        
        if (speexPreProcessor)
            speex_preprocess_state_destroy(speexPreProcessor);

        speexPreProcessor = speex_preprocess_state_init(MUMBLE_AUDIO_SAMPLES_IN_FRAME, MUMBLE_AUDIO_SAMPLE_RATE);

        int enabled = 1;
        int arg = 0;
        float fArg = 0.0f;

        speex_preprocess_ctl(speexPreProcessor, SPEEX_PREPROCESS_SET_VAD, &enabled);
        speex_preprocess_ctl(speexPreProcessor, SPEEX_PREPROCESS_SET_AGC, &enabled);
        speex_preprocess_ctl(speexPreProcessor, SPEEX_PREPROCESS_SET_DENOISE, &enabled);
        speex_preprocess_ctl(speexPreProcessor, SPEEX_PREPROCESS_SET_DEREVERB, &enabled);
      
        arg = 30000;
        speex_preprocess_ctl(speexPreProcessor, SPEEX_PREPROCESS_SET_AGC_TARGET, &arg);

        float v = 30000.0f / static_cast<float>(currentSettings.amplification);
        arg = static_cast<int>(floorf(20.0f * log10f(v)));
        speex_preprocess_ctl(speexPreProcessor, SPEEX_PREPROCESS_SET_AGC_MAX_GAIN, &arg);

        arg = 12;
        speex_preprocess_ctl(speexPreProcessor, SPEEX_PREPROCESS_SET_AGC_INCREMENT, &arg);

        arg = -60;
        speex_preprocess_ctl(speexPreProcessor, SPEEX_PREPROCESS_SET_AGC_DECREMENT, &arg);
        
        arg = currentSettings.suppression;
        speex_preprocess_ctl(speexPreProcessor, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &arg);

        fArg = 0.0f;
        speex_preprocess_ctl(speexPreProcessor, SPEEX_PREPROCESS_SET_DEREVERB_DECAY, &fArg);
        speex_preprocess_ctl(speexPreProcessor, SPEEX_PREPROCESS_SET_DEREVERB_LEVEL, &fArg);
    }

    void AudioProcessor::run()
    {
        qobjTimerId_ = startTimer(15); // Audio processing with ~60 fps.

        exec(); // Blocks untill quit()

        killTimer(qobjTimerId_);

        SAFE_DELETE(codec);

        {
            QMutexLocker lockInput(&mutexInput);
            inputAudioStates.clear();
            framework = 0;
        }

        if (speexPreProcessor)
            speex_preprocess_state_destroy(speexPreProcessor);
    }

    void AudioProcessor::timerEvent(QTimerEvent *event)
    {
        // This function processed queued PCM frames with speexdsp and celt at ~60fps and adds them to
        // a pending encoded frames list to be sent out to the network from the main thread.
        // Mutex mutexOutputPCM and mutexOutputEncoded are the main locks for queuing the frames back and forth.
        if (!codec)
            return;

        QMutexLocker outputLock(&mutexOutputPCM);
        if (pendingPCMFrames.size() == 0)
            return;

        int localQualityBitrate = 0;
        int localFramesPerPacket = 0;
        int localSuppress = 0;
        int localGain = 0;
        bool detectVAD = false;
        bool localPreProcess = false;
        float VADmin = 0.0;
        float VADmax = 0.0;

        {
            QMutexLocker lock(&mutexAudioSettings);
            localQualityBitrate = qualityBitrate;
            localFramesPerPacket = qualityFramesPerPacket;
            localPreProcess = outputPreProcessed;
            localSuppress = audioSettings.suppression;
            detectVAD = audioSettings.transmitMode == TransmitVoiceActivity;
            VADmin = audioSettings.VADmin;
            VADmax = audioSettings.VADmax;
        }
        
        QMutexLocker lockEncoded(&mutexOutputEncoded);

        for (std::vector<SoundBuffer>::const_iterator pcmIter = pendingPCMFrames.begin(); pcmIter != pendingPCMFrames.end(); ++pcmIter)
        {
            const SoundBuffer &pcmFrame = (*pcmIter);
            if (pcmFrame.data.size() == 0)
                continue;

            isSpeech = true;
            if (localPreProcess)
            {
                speex_preprocess_ctl(speexPreProcessor, SPEEX_PREPROCESS_GET_AGC_GAIN, &localGain);
                int suppression = localSuppress - localGain;
                if (suppression > 0)
                    suppression = 0;
                speex_preprocess_ctl(speexPreProcessor, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &suppression);
                speex_preprocess_run(speexPreProcessor, (spx_int16_t*)&pcmFrame.data[0]);

                if (detectVAD)
                {
                    float sum = 1.0f;
                    short *data = (short*)&pcmFrame.data[0];
                    for (int index=0; index<MUMBLE_AUDIO_SAMPLES_IN_FRAME; index++)
                    {
                        int value = data[index];
                        sum += static_cast<float>(value * value);
                    }

                    levelPeakMic = qMax(20.0f * log10f(sqrtf(sum / static_cast<float>(MUMBLE_AUDIO_SAMPLES_IN_FRAME)) / 32768.0f), -96.0f);
                    levelPeakMic = qMax(levelPeakMic - localGain, -96.0f);
                    levelMic = (1.0f + levelPeakMic / 96.0f);

                    // Detect mic level if speaking
                    if (levelMic > VADmax)
                        isSpeech = true;
                    else if (levelMic > VADmin && wasPreviousSpeech)
                        isSpeech = true;
                    else
                        isSpeech = false;

                    if (isSpeech)
                        holdFrames = 0;
                    else
                    {
                        // Hold certain amount of frames even if not speaking.
                        // This allows end of sentences to get to the outgoing buffer safely.
                        holdFrames++;
                        if (holdFrames < 20)
                            isSpeech = true;
                    }
                }
            }

            // Encode
            unsigned char compressedBuffer[512];
            int bytesWritten = codec->Encode(pcmFrame, compressedBuffer, localQualityBitrate);
            if (bytesWritten > 0)
            {
                QByteArray encodedFrame(reinterpret_cast<const char*>(compressedBuffer), bytesWritten);

                // If speech, add to encoded frames. But first
                // append any 'prediction' buffered frames so start of sentences
                // can get to the outgoing buffer safely.
                if (isSpeech || wasPreviousSpeech)
                {    
                    if (detectVAD && pendingVADPreBuffer.size() > 0)
                    {
                        pendingEncodedFrames.append(pendingVADPreBuffer);
                        pendingVADPreBuffer.clear();
                    }
                    pendingEncodedFrames.push_back(encodedFrame);
                }
                // If voice activity detection is enabled but this is 
                // not speech, add the frame to the VAD 'prediction' buffer.
                else if (detectVAD && !isSpeech && !wasPreviousSpeech)
                {
                    while(pendingVADPreBuffer.size() >= 5)
                    {
                        if (!pendingVADPreBuffer.isEmpty())
                            pendingVADPreBuffer.removeFirst();
                        else
                            break;
                    }
                    pendingVADPreBuffer.push_back(encodedFrame);
                }
            }
            wasPreviousSpeech = isSpeech;
        }

        pendingPCMFrames.clear();
    }

    void AudioProcessor::GetLevels(float &peakMic, bool &speaking)
    {
        // The peak mic level and is speaking are written in a mutexOutputPCM lock.
        // So use the same lock to read the data out for main thread usage.
        {
            QMutexLocker pcmLock(&mutexOutputPCM);
            peakMic = levelPeakMic;
            speaking = isSpeech;
        }
    }

    void AudioProcessor::SetOutputAudioMuted(bool outputAudioMuted_)
    {
        // This function is called in the main thread
        if (!framework)
            return;

        {
            QMutexLocker lock(&mutexAudioMute);
            outputAudioMuted = outputAudioMuted_;
        }

        if (!outputAudioMuted)
        {
            QMutexLocker lock(&mutexAudioSettings);
            {
                // Reset back to ultra state, gets increased automatically if necessary.
                qualityFramesPerPacket = MUMBLE_AUDIO_FRAMES_PER_PACKET_ULTRA;
                
                // Recording buffer of 40 celt frames, 38400 bytes
                int bufferSize = (MUMBLE_AUDIO_SAMPLES_IN_FRAME * MUMBLE_AUDIO_SAMPLE_WIDTH / 8) * 40;
                if (!framework->Audio()->StartRecording(audioSettings.recordingDevice, MUMBLE_AUDIO_SAMPLE_RATE, true, false, bufferSize))
                {
                    LogWarning("Could not open recording device '" + audioSettings.recordingDevice + "'. Trying to open the default device instead.");
                    framework->Audio()->StartRecording("", MUMBLE_AUDIO_SAMPLE_RATE, true, false, bufferSize);
                }
            }
        }
        else
            framework->Audio()->StopRecording();

        ClearOutputAudio();
    }

    void AudioProcessor::SetInputAudioMuted(bool inputAudioMuted_)
    {
        // This function is called in the main thread
        if (!framework)
            return;

        {
            QMutexLocker lock(&mutexAudioMute);
            inputAudioMuted = inputAudioMuted_;
        }
        
        ClearInputAudio();
    }
    
    void AudioProcessor::ApplyFramesPerPacket(int framesPerPacket)
    {
        if (framesPerPacket < 2)
            framesPerPacket = 2;
        if (framesPerPacket > 10)
            framesPerPacket = 10;
            
        LogInfo(LC + "Changing frames per packet to " + QString::number(framesPerPacket));
        {   
            QMutexLocker lock(&mutexAudioSettings);
            qualityFramesPerPacket = framesPerPacket;
        }
        ClearOutputAudio();
    }

    void AudioProcessor::ApplySettings(AudioSettings settings)
    {
        bool positionalRangesChanged = false;
        bool recondingDeviceChanged = false;
        int changedInnerRange = 0;
        int changedOuterRange = 0;

        // This function is called in the main thread
        {
            QMutexLocker lock(&mutexAudioSettings);

            // Detect recording device changed
            if (audioSettings.recordingDevice != settings.recordingDevice)
                recondingDeviceChanged = true;
                
            // Detect if positional playback ranges changed.
            if (audioSettings.innerRange != settings.innerRange || audioSettings.outerRange != settings.outerRange)
            {
                positionalRangesChanged = true;
                changedInnerRange = settings.innerRange;
                changedOuterRange = settings.outerRange;
            }

            audioSettings = settings;
            if (audioSettings.suppression > 0)
                audioSettings.suppression = 0;

            switch(audioSettings.quality)
            {
                case QualityLow:
                {
                    qualityBitrate = MUMBLE_AUDIO_QUALITY_LOW;
                    break;
                }
                case QualityBalanced:
                {
                    qualityBitrate = MUMBLE_AUDIO_QUALITY_BALANCED;
                    break;
                }
                case QualityHigh:
                {
                    qualityBitrate = MUMBLE_AUDIO_QUALITY_ULTRA;
                    break;
                }
            }
            
            // Reset back to ultra state, gets increased automatically if necessary.
            qualityFramesPerPacket = MUMBLE_AUDIO_FRAMES_PER_PACKET_ULTRA;
        }

        // Apply new positional ranges to existing positional sound channels.
        if (positionalRangesChanged)
        {
            QMutexLocker lockStates(&mutexInput);
            for (AudioStateMap::iterator iter = inputAudioStates.begin(); iter != inputAudioStates.end(); ++iter)
            {
                UserAudioState &userAudioState = iter->second;
                if (userAudioState.soundChannel.get() && userAudioState.soundChannel->IsPositional())
                    userAudioState.soundChannel->SetRange(static_cast<float>(changedInnerRange), static_cast<float>(changedOuterRange), 1.0f);
            }
        }

        preProcessorReset = true;
        ResetSpeexProcessor();
        
        // Start recording with the new device if changed.
        // If recording is not enabled, change to false,
        // it will be applied on the when output mute state is changed.
        {
            QMutexLocker lock(&mutexAudioMute);
            if (recondingDeviceChanged && outputAudioMuted)
                recondingDeviceChanged = false;
        }
        if (recondingDeviceChanged)
        {
            LogInfo(LC + "Recording device change detected to '" + (settings.recordingDevice.isEmpty() ? "Default Recording Device" : settings.recordingDevice) + "'.");
            framework->Audio()->StopRecording();
            
            // Recording buffer of 40 celt frames, 38400 bytes
            int bufferSize = (MUMBLE_AUDIO_SAMPLES_IN_FRAME * MUMBLE_AUDIO_SAMPLE_WIDTH / 8) * 40;
            framework->Audio()->StartRecording(settings.recordingDevice, MUMBLE_AUDIO_SAMPLE_RATE, true, false, bufferSize);
            
            ClearOutputAudio();
        }
    }

    MumbleAudio::AudioSettings AudioProcessor::GetSettings()
    {
        QMutexLocker lock(&mutexAudioSettings);
        return audioSettings;
    }

    ByteArrayVector AudioProcessor::ProcessOutputAudio()
    {
        // This function is called in the main thread
        if (!framework)
            return ByteArrayVector();

        // Get recorded PCM frames from AudioAPI.
        PROFILE(Mumble_ProcessOutputAudio_OpenAL)
        std::vector<SoundBuffer> pcmFrames;
        uint celtFrameSize = MUMBLE_AUDIO_SAMPLES_IN_FRAME * MUMBLE_AUDIO_SAMPLE_WIDTH / 8;
        
        while (framework->Audio()->GetRecordedSoundSize() >= celtFrameSize)
        {
            SoundBuffer outputPCM;
            outputPCM.data.resize(celtFrameSize);
            uint bytesOut = framework->Audio()->GetRecordedSoundData(&outputPCM.data[0], celtFrameSize);
            if (bytesOut == celtFrameSize)
                pcmFrames.push_back(outputPCM);
        }
        ELIFORP(Mumble_ProcessOutputAudio_OpenAL)

        int pendingPCMFrameCount = 0;
        {
            QMutexLocker outputLock(&mutexOutputPCM);
            // Queue for speexdsp preprocessing and celt encoding for audio thread
            if (pcmFrames.size() > 0)
            {
                PROFILE(Mumble_ProcessOutputAudio_Queue_Processing)
                pendingPCMFrames.insert(pendingPCMFrames.end(), pcmFrames.begin(), pcmFrames.end());
                ELIFORP(Mumble_ProcessOutputAudio_Queue_Processing)
            }
            pendingPCMFrameCount = pendingPCMFrames.size();
        }

        PROFILE(Mumble_ProcessOutputAudio_Get_Encoded)

        QMutexLocker lockEncoded(&mutexOutputEncoded);

        // No queued encoded frames for playback.
        if (pendingEncodedFrames.size() == 0)
            return ByteArrayVector();

        // Get packet count per frame
        int framesPerPacket = 0;
        {
            QMutexLocker lock(&mutexAudioSettings);
            framesPerPacket = qualityFramesPerPacket;
        }
        
        // Ensure we are not buffing faster than what is sent to network. Automatically adjust how many frames are sent to network at at time to certain extent (<=10).
        if (pendingEncodedFrames.size() > framesPerPacket * 10)
        {
            // Do some helpful info logs if we are auto incresing frames per packet count.
            if (framesPerPacket <= 8)
                LogInfo(LC + QString("Output buffer full with %1/%2 frames, auto increasing frames/packet to %3").arg(pendingEncodedFrames.size()).arg(framesPerPacket*10).arg(framesPerPacket+2));
            else
                LogInfo(LC + QString("Output buffer full with %1/%2 frames, frames/packet is %3").arg(pendingEncodedFrames.size()).arg(framesPerPacket*10).arg(framesPerPacket));
                
            // Remove oldest frames to get the buffer to a acceptable size.
            while(pendingEncodedFrames.size() > framesPerPacket * 10)
            {
                if (!pendingEncodedFrames.isEmpty())
                    pendingEncodedFrames.removeFirst();
                else
                    break;
            }

            {
                QMutexLocker lock(&mutexAudioSettings);
                if (qualityFramesPerPacket <= 8)
                {
                    qualityFramesPerPacket += 2;
                    framesPerPacket = qualityFramesPerPacket;
                }
            }
        }
        
        // If we are speaking send out full 'framesPerPacket' frames. If we are not speaking send whatever is left in the buffer but max is still 'framesPerPacket'.
        int framesToPacket = (isSpeech || wasPreviousSpeech) ? framesPerPacket : qMin(framesPerPacket, pendingEncodedFrames.size());

        // Enough encoded frames in the ready queue
        if (pendingEncodedFrames.size() >= framesToPacket)
        {
            ByteArrayVector sendOutNow;
            for (int i=0; i<framesToPacket; ++i)
            {
                if (!pendingEncodedFrames.isEmpty())
                    sendOutNow.push_back(pendingEncodedFrames.takeFirst());
                else
                    break;
            }
            return sendOutNow;
        }
        else
            return ByteArrayVector();
    }

    void AudioProcessor::PlayInputAudio(MumblePlugin *mumble)
    {
        // This function is called in the main thread
        if (!framework)
            return;

        // Read positional playback settings
        int allowReceivingPositional = false;
        int positionalInnerRange = 0;
        int positionalOuterRange = 0;

        {
            QMutexLocker lockSettings(&mutexAudioSettings);
            allowReceivingPositional = audioSettings.allowReceivingPositional;
            if (allowReceivingPositional)
            {
                positionalInnerRange = audioSettings.innerRange;
                positionalOuterRange = audioSettings.outerRange;
            }
        }

        // Lock pending audio channels, these cannot be release in the audio thread
        QMutexLocker lockChannels(&mutexAudioChannels);
        // Lock user audio states
        QMutexLocker lock(&mutexInput);
        if (inputAudioStates.empty())
            return;

        AudioStateMap::iterator end = inputAudioStates.end();
        for (AudioStateMap::iterator iter = inputAudioStates.begin(); iter != end; ++iter)
        {
            uint userId = iter->first;
            UserAudioState &userAudioState = iter->second;

            // We must have the user if we are receiving audio from him.
            MumbleUser *user = mumble->User(userId);
            if (!user)            
                continue;

            // If user is muted or it's sound channel is pending for removal.
            // - MumbleUser::isMuted is the local mute that is not informed to the server.
            // - Pending channel removes will happen on certain error states.
            if (user->isMuted || pendingSoundChannelRemoves.contains(userId))
            {
                if (userAudioState.soundChannel.get())
                {
                    userAudioState.soundChannel->Stop();
                    userAudioState.soundChannel.reset();
                }
                if (pendingSoundChannelRemoves.contains(userId))
                    pendingSoundChannelRemoves.removeAll(userId);
                if (user->isMuted)
                    userAudioState.frames.clear();
            }

            // Check speaking state, not speaking if pending frames is empty and SoundChannel is not playing.
            if (user->isMuted || userAudioState.frames.empty())
            {
                bool playing = false;
                if (userAudioState.soundChannel.get() && userAudioState.soundChannel->State() == SoundChannel::Playing)
                    playing = true;
                if (!playing)
                    user->SetAndEmitSpeaking(false);
                continue;
            }

            // Iterate audio frames for user
            AudioFrameDeque::iterator frameEnd = userAudioState.frames.end();
            for (AudioFrameDeque::iterator frameIter = userAudioState.frames.begin(); frameIter != frameEnd; ++frameIter)
            {
                const SoundBuffer &frame = (*frameIter);
                if (userAudioState.soundChannel.get())
                {
                    // Create new AudioAsset to be added to the sound channels playback buffer.
                    AudioAssetPtr audioAsset = framework->Audio()->CreateAudioAssetFromSoundBuffer(frame);
                    if (audioAsset.get())
                    {
                        if (allowReceivingPositional)
                        {
                            // Update positional and position info.
                            if (userAudioState.soundChannel->IsPositional() != userAudioState.isPositional)
                                userAudioState.soundChannel->SetPositional(userAudioState.isPositional);
                            if (userAudioState.isPositional)
                            {
                                userAudioState.soundChannel->SetRange(static_cast<float>(positionalInnerRange), static_cast<float>(positionalOuterRange), 1.0f);
                                userAudioState.soundChannel->SetPosition(userAudioState.pos);
                            }

                            // Check and update user positional state.
                            if (!user->isMe && user->isPositional != userAudioState.isPositional)
                            {
                                user->pos = userAudioState.pos;
                                user->SetAndEmitPositional(userAudioState.isPositional);
                            }
                        }
                        else
                        {
                            // Reset positional info from the channel
                            if (userAudioState.soundChannel->IsPositional())
                                userAudioState.soundChannel->SetPositional(false);

                            // Reset users positional state.
                            if (!user->isMe && user->isPositional)
                            {
                                user->pos = float3::zero;
                                user->SetAndEmitPositional(false);
                            }
                        }

                        // Update user speaking state.
                        user->SetAndEmitSpeaking(true);

                        // Add buffer to the sound channel.
                        userAudioState.soundChannel->AddBuffer(audioAsset);
                    }
                    else
                    {
                        // Something went wrong, release "broken" SoundChannel and its data.
                        userAudioState.soundChannel->Stop();
                        userAudioState.soundChannel.reset();
                    }
                }
                else
                {
                    // Create sound channel with initial audio frame
                    userAudioState.soundChannel = framework->Audio()->PlaySoundBuffer(frame, SoundChannel::Voice);
                    if (userAudioState.soundChannel.get())
                    {
                        // Set positional if available and our local settings allows it
                        if (allowReceivingPositional && userAudioState.isPositional)
                        {
                            userAudioState.soundChannel->SetPositional(true);
                            userAudioState.soundChannel->SetRange(static_cast<float>(positionalInnerRange), static_cast<float>(positionalOuterRange), 1.0f);
                            userAudioState.soundChannel->SetPosition(userAudioState.pos);
                        }
                        else
                            userAudioState.soundChannel->SetPositional(false);

                        // Update user speaking state
                        user->SetAndEmitSpeaking(true);
                    }
                }
            }

            // Clear users input frames
            userAudioState.frames.clear();
        }
    }
    
    void AudioProcessor::ClearInputAudio()
    {
        // This function should be called in the main thread
        QMutexLocker lock(&mutexInput);
        if (inputAudioStates.size() > 0)
            inputAudioStates.clear();
    }

    void AudioProcessor::ClearInputAudio(uint userId)
    {
        // This function should be called in the main thread
        QMutexLocker lock(&mutexInput);
        
        AudioStateMap::iterator userStateIter = inputAudioStates.find(userId);
        if (userStateIter != inputAudioStates.end())
        {
            UserAudioState &userState = userStateIter->second;
            userState.frames.clear();
            if (userState.soundChannel.get())
                userState.soundChannel->Stop();
            userState.soundChannel.reset();
            inputAudioStates.erase(userStateIter);
        }
    }

    void AudioProcessor::ClearOutputAudio()
    {
        // This function should be called in the main thread
        {
            QMutexLocker lockEncoded(&mutexOutputEncoded);
            pendingEncodedFrames.clear();
            pendingVADPreBuffer.clear();
        }
        {
            QMutexLocker lockOutput(&mutexOutputPCM);
            pendingPCMFrames.clear();
        }
    }

    int AudioProcessor::CodecBitStreamVersion()
    {
        if (this->isRunning())
        {
            LogError(LC + "CodecBitStreamVersion() called when audio thread is running, -1 will be returned!");
            return -1;
        }

        if (!codec)
            return -1;
        else
            return codec->BitStreamVersion();
    }

    void AudioProcessor::PrintCeltError(int celtError, bool decoding)
    {
        // This function is called in the audio thread
        if (celtError == CELT_OK)
            return;

        QString base(LC + (decoding == true ? "celt decoding error: " : "celt encoding error: "));

        switch (celtError)
        {
            case CELT_BAD_ARG:
                LogError(base + "CELT_BAD_ARG");
                break;
            case CELT_INVALID_MODE:
                LogError(base + "CELT_INVALID_MODE");
                break;
            case CELT_INTERNAL_ERROR:
                LogError(base + "CELT_INTERNAL_ERROR");
                break;
            case CELT_CORRUPTED_DATA:
                LogError(base + "CELT_CORRUPTED_DATA");
                break;
            case CELT_UNIMPLEMENTED:
                LogError(base + "CELT_UNIMPLEMENTED");
                break;
            case CELT_INVALID_STATE:
                LogError(base + "CELT_INVALID_STATE");
                break;
            case CELT_ALLOC_FAIL:
                LogError(base + "CELT_ALLOC_FAIL");
                break;
            default:
                LogError(base + "Unknown error enum: " + QString::number(celtError));
                break;
        }
    }

    void AudioProcessor::OnAudioReceived(uint userId, uint seq, ByteArrayVector frames, bool isPositional, float3 pos)
    {
        if (!codec)
            return;

        // This function is called in the audio thread
        if (frames.size() == 0)
            return;
        
        {
            // This will never* hit if the server was properly informed that we don't want to receive audio,
            // from all or from certain users. See MumblePlugin::SetInputAudioMuted.
            // *The return here will only hit for a short period when input was muted
            // to where the server receives this information and shuts down sending audio to us.
            QMutexLocker lockAudio(&mutexAudioMute);
            if (inputAudioMuted)
                return;
        }

        QMutexLocker lockBuffers(&mutexInput);
        UserAudioState &userAudioState = inputAudioStates[userId]; // Creates a new one if does not exist already.
        
        // If you change audio output settings in Mumble or various other things, sequence will reset to 0.
        // If this is received we need to reset our tracking sequence number as well.
        if (seq == 0)
            userAudioState.lastSeq = 0;

        // If this sequence is older than what has been previously received, ignore the frames
        if (userAudioState.lastSeq > seq)
            return;

        // Update the users audio state struct
        userAudioState.lastSeq = seq;
        userAudioState.isPositional = isPositional;
        if (userAudioState.isPositional)
            userAudioState.pos = pos;

        // Check frame counts, clear input frames and pending frames from SoundChannel.
        // This happens when main thread is blocked from reading the queued frames but the network thread
        // is still filling the frame queue. If we release too much frames to AudioAPI/OpenAL it will eventually crash.
        if ((userAudioState.frames.size() + frames.size()) > 10)
        {
            userAudioState.frames.clear();
            QMutexLocker lockChannels(&mutexAudioChannels);
            if (!pendingSoundChannelRemoves.contains(userId))
                pendingSoundChannelRemoves.push_back(userId);
            return;
        }

        for (ByteArrayVector::const_iterator frameIter = frames.begin(); frameIter != frames.end(); ++frameIter)
        {
            const QByteArray &inputFrame = (*frameIter);
            SoundBuffer soundFrame;

            int celtResult = codec->Decode(inputFrame.data(), inputFrame.size(), soundFrame);
            if (celtResult == CELT_OK)
                userAudioState.frames.push_back(soundFrame);
            else
            {
                PrintCeltError(celtResult, true);
                userAudioState.frames.clear();
                return;
            }            
        }
    }
}
