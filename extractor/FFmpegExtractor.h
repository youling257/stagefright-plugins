/*
 * Copyright 2012 Michael Chen <omxcodec@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SUPER_EXTRACTOR_H_

#define SUPER_EXTRACTOR_H_

#include <media/stagefright/foundation/ABase.h>
#include <media/stagefright/MediaExtractor.h>
#include <utils/threads.h>
#include <utils/KeyedVector.h>
#include <media/stagefright/MediaSource.h>

#include "utils/ffmpeg_utils.h"

namespace android {

struct ABuffer;
struct AMessage;
class String8;
struct FFmpegSource;

struct FFmpegExtractor : public MediaExtractor {
    FFmpegExtractor(const sp<DataSource> &source, const sp<AMessage> &meta);

    virtual size_t countTracks();
    virtual sp<IMediaSource> getTrack(size_t index);
    virtual sp<MetaData> getTrackMetaData(size_t index, uint32_t flags);

    virtual sp<MetaData> getMetaData();

    virtual uint32_t flags() const;

protected:
    virtual ~FFmpegExtractor();

private:
    friend struct FFmpegSource;

    struct TrackInfo {
        int mIndex; //stream index
        sp<MetaData> mMeta;
        AVStream *mStream;
        PacketQueue *mQueue;
        bool mSeek;
    };

    Vector<TrackInfo> mTracks;

    mutable Mutex mLock;

    sp<DataSource> mDataSource;
    sp<MetaData> mMeta;

    char mFilename[PATH_MAX];
    int mGenPTS;
    int mVideoDisable;
    int mAudioDisable;
    int mShowStatus;
    int mSeekByBytes;
    int64_t mDuration;
    bool mEOF;
    size_t mPktCounter;
    int mAbortRequest;

    PacketQueue mAudioQ;
    PacketQueue mVideoQ;

    AVFormatContext *mFormatCtx;
    int mVideoStreamIdx;
    int mAudioStreamIdx;
    AVStream *mVideoStream;
    AVStream *mAudioStream;
    bool mDefersToCreateVideoTrack;
    bool mDefersToCreateAudioTrack;
    AVBSFContext *mVideoBsfc;
    AVBSFContext *mAudioBsfc;
    bool mParsedMetadata;

    static int decodeInterruptCb(void *ctx);

    int initStreams();
    void deInitStreams();
    void fetchStuffsFromSniffedMeta(const sp<AMessage> &meta);
    void setFFmpegDefaultOpts();
    int feedNextPacket();
    int getPacket(int trackIndex, AVPacket *pkt);
    bool isCodecSupported(enum AVCodecID codec_id);
    sp<MetaData> setVideoFormat(AVStream *stream);
    sp<MetaData> setAudioFormat(AVStream *stream);
    void setDurationMetaData(AVStream *stream, sp<MetaData> &meta);
    int streamComponentOpen(int streamIndex);
    void streamComponentClose(int streamIndex);
    int streamSeek(int trackIndex, int64_t pos,
                    MediaSource::ReadOptions::SeekMode mode);
    int checkExtradata(AVCodecParameters *avpar);

    DISALLOW_EVIL_CONSTRUCTORS(FFmpegExtractor);
};

extern "C" {

static const char *findMatchingContainer(const char *name);

bool SniffFFMPEG(
        const sp<DataSource> &source, String8 *mimeType, float *confidence,
        sp<AMessage> *);

MediaExtractor* CreateFFMPEGExtractor(const sp<DataSource> &source,
        const char *mime, const sp<AMessage> &meta);

}

}  // namespace android

#endif  // SUPER_EXTRACTOR_H_

