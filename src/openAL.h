#include <AL/al.h>
#include <AL/alc.h>

namespace OpenAL
{

    static int64_t audio_current_pts = 0;
    static bool first_run = true;
    bool SetupAudio(ALCdevice *device, ALCcontext *context)
    {

        device = alcOpenDevice(nullptr);
        if (!device)
        {
            LOGD("Failed to open audio device");
            return false;
        }
        context = alcCreateContext(device, nullptr);
        if (!context)
        {
            alcCloseDevice(device);
            LOGD("Failed to create audio context");
            return false;
        }
        alcMakeContextCurrent(context);
        LOGD("Initialized OpenAL context");

        return true;
    }
    void PauseAudioSamples(ALuint *buffers, const int BUFFER_COUNT, ALuint source)
    {

        ALint queued, processed;
        alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
        alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

        while (processed > 0)
        {
            ALuint buffer;
            alSourceUnqueueBuffers(source, 1, &buffer);
            processed --;
        }
        alSourceStop(source);

        ALint state;
        alGetSourcei(source,AL_SOURCE_STATE,&state);
        assert(state!=AL_PLAYING);
        alSourcei(source, AL_BUFFER, 0);
        first_run=true;
    }
    void PlayAudioSamples(AVStreamInfo &video_stream_info, int num_samples, int16_t *audio_data, const int BUFFER_COUNT, ALuint *buffers, ALuint source, bool video_behind)
    {

        // TODO: Get actual audio num and den
        // TODO: Make all of this actually obey the trim

        auto channels = video_stream_info.AudioCodecContext->ch_layout.nb_channels;
        auto sample_rate = video_stream_info.AudioCodecContext->sample_rate;

        ALenum format = (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

        if (first_run)
        {
            alSourceStop(source);
            alSourcei(source, AL_BUFFER, 0);
            audio_current_pts = video_stream_info.last_audio_pts;

            // Fill first buffer with provided audio_data
            alBufferData(buffers[0], format, audio_data, num_samples * sizeof(int16_t), sample_rate);

            // Fill the other 3 buffers

            for (int i = 1; i < BUFFER_COUNT; i++)
            {

                int16_t *data = ReadNextAudioFrame(video_stream_info);
                alBufferData(buffers[i], format, data,
                             num_samples * sizeof(int16_t), sample_rate);
                free(data);
            }

            alSourceQueueBuffers(source, BUFFER_COUNT, buffers);
            alSourcePlay(source);
            first_run = false;
            ALint queued;
            alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
        }
        else
        {
            if (!video_behind)
            {
                ALint queued, processed;
                alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
                alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

                while (processed > 0)
                {
                    ALuint buffer;
                    // Unqueue a processed buffer
                    alSourceUnqueueBuffers(source, 1, &buffer);
                    audio_current_pts = video_stream_info.last_audio_pts;
                    // Fill it with new data

                    int16_t *new_data = ReadNextAudioFrame(video_stream_info);

                    alBufferData(buffer, format, new_data,
                                 num_samples * sizeof(int16_t), sample_rate);
                    delete new_data;

                    // Queue it back
                    alSourceQueueBuffers(source, 1, &buffer);
                    processed--;
                }

                // Check if playback has stopped due to underrun
                ALint state;
                alGetSourcei(source, AL_SOURCE_STATE, &state);
                if (state != AL_PLAYING)
                {
                    alSourcePlay(source);
                }
            }
        }

        // Check for OpenAL errors
        ALenum error = alGetError();
        if (error != AL_NO_ERROR)
        {
            printf("OpenAL error: %s\n", alGetString(error));
            return;
        }
    }

    void CloseAudioStream(ALCdevice *device, ALCcontext *context)
    {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context);
        alcCloseDevice(device);
    }

}