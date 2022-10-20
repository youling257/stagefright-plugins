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

#define LOG_TAG "codec_utils"
#include <utils/Log.h>

extern "C" {

#include "config.h"
#include "libavcodec/xiph.h"
#include "libavutil/intreadwrite.h"

}

#include <utils/Errors.h>
#include <media/stagefright/foundation/ABitReader.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include "include/avc_utils.h"

#include "codec_utils.h"

namespace android {

static void EncodeSize14(uint8_t **_ptr, size_t size) {
    CHECK_LE(size, 0x3fffu);

    uint8_t *ptr = *_ptr;

    *ptr++ = 0x80 | (size >> 7);
    *ptr++ = size & 0x7f;

    *_ptr = ptr;
}

static sp<ABuffer> MakeMPEGVideoESDS(const sp<ABuffer> &csd) {
    sp<ABuffer> esds = new ABuffer(csd->size() + 25);

    uint8_t *ptr = esds->data();
    *ptr++ = 0x03;
    EncodeSize14(&ptr, 22 + csd->size());

    *ptr++ = 0x00;  // ES_ID
    *ptr++ = 0x00;

    *ptr++ = 0x00;  // streamDependenceFlag, URL_Flag, OCRstreamFlag

    *ptr++ = 0x04;
    EncodeSize14(&ptr, 16 + csd->size());

    *ptr++ = 0x40;  // Audio ISO/IEC 14496-3

    for (size_t i = 0; i < 12; ++i) {
        *ptr++ = 0x00;
    }

    *ptr++ = 0x05;
    EncodeSize14(&ptr, csd->size());

    memcpy(ptr, csd->data(), csd->size());

    return esds;
}

//video

//H.264 Video Types
//http://msdn.microsoft.com/en-us/library/dd757808(v=vs.85).aspx

// H.264 bitstream without start codes.
sp<MetaData> setAVCFormat(AVCodecParameters *avpar)
{
    ALOGV("AVC");

    CHECK_GT(avpar->extradata_size, 0);
    CHECK_EQ((int)avpar->extradata[0], 1); //configurationVersion

    if (avpar->width == 0 || avpar->height == 0) {
         int32_t width, height;
         sp<ABuffer> seqParamSet = new ABuffer(avpar->extradata_size - 8);
         memcpy(seqParamSet->data(), avpar->extradata + 8, avpar->extradata_size - 8);
         FindAVCDimensions(seqParamSet, &width, &height);
         avpar->width  = width;
         avpar->height = height;
     }

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_AVC);
    meta->setData(kKeyAVCC, kTypeAVCC, avpar->extradata, avpar->extradata_size);

    return meta;
}

// H.264 bitstream with start codes.
sp<MetaData> setH264Format(AVCodecParameters *avpar)
{
    ALOGV("H264");

    CHECK_NE((int)avpar->extradata[0], 1); //configurationVersion

    sp<ABuffer> buffer = new ABuffer(avpar->extradata_size);
    memcpy(buffer->data(), avpar->extradata, avpar->extradata_size);
    return MakeAVCCodecSpecificData(buffer);
}

sp<MetaData> setMPEG4Format(AVCodecParameters *avpar)
{
    ALOGV("MPEG4");

    sp<ABuffer> csd = new ABuffer(avpar->extradata_size);
    memcpy(csd->data(), avpar->extradata, avpar->extradata_size);
    sp<ABuffer> esds = MakeMPEGVideoESDS(csd);

    sp<MetaData> meta = new MetaData;
    meta->setData(kKeyESDS, kTypeESDS, esds->data(), esds->size());

    int divxVersion = getDivXVersion(avpar);
    if (divxVersion >= 0) {
        meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_DIVX);
        meta->setInt32(kKeyDivXVersion, divxVersion);
    } else {
        meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_MPEG4);
    }
    return meta;
}

sp<MetaData> setH263Format(AVCodecParameters *avpar __unused)
{
    ALOGV("H263");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_H263);

    return meta;
}

sp<MetaData> setMPEG2VIDEOFormat(AVCodecParameters *avpar)
{
    ALOGV("MPEG%uVIDEO", avpar->codec_id == AV_CODEC_ID_MPEG2VIDEO ? 2 : 1);

    sp<ABuffer> csd = new ABuffer(avpar->extradata_size);
    memcpy(csd->data(), avpar->extradata, avpar->extradata_size);
    sp<ABuffer> esds = MakeMPEGVideoESDS(csd);

    sp<MetaData> meta = new MetaData;
    meta->setData(kKeyESDS, kTypeESDS, esds->data(), esds->size());
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_MPEG2);

    return meta;
}

sp<MetaData> setVC1Format(AVCodecParameters *avpar)
{
    ALOGV("VC1");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_VC1);
    meta->setData(kKeyRawCodecSpecificData, 0, avpar->extradata, avpar->extradata_size);

    return meta;
}

sp<MetaData> setWMV1Format(AVCodecParameters *avpar __unused)
{
    ALOGV("WMV1");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_WMV);
    meta->setInt32(kKeyWMVVersion, kTypeWMVVer_7);

    return meta;
}

sp<MetaData> setWMV2Format(AVCodecParameters *avpar)
{
    ALOGV("WMV2");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_WMV);
    meta->setData(kKeyRawCodecSpecificData, 0, avpar->extradata, avpar->extradata_size);
    meta->setInt32(kKeyWMVVersion, kTypeWMVVer_8);

    return meta;
}

sp<MetaData> setWMV3Format(AVCodecParameters *avpar)
{
    ALOGV("WMV3");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_WMV);
    meta->setData(kKeyRawCodecSpecificData, 0, avpar->extradata, avpar->extradata_size);
    meta->setInt32(kKeyWMVVersion, kTypeWMVVer_9);

    return meta;
}

sp<MetaData> setRV20Format(AVCodecParameters *avpar)
{
    ALOGV("RV20");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_RV);
    meta->setData(kKeyRawCodecSpecificData, 0, avpar->extradata, avpar->extradata_size);
    meta->setInt32(kKeyRVVersion, kTypeRVVer_G2); //http://en.wikipedia.org/wiki/RealVide

    return meta;
}

sp<MetaData> setRV30Format(AVCodecParameters *avpar)
{
    ALOGV("RV30");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_RV);
    meta->setData(kKeyRawCodecSpecificData, 0, avpar->extradata, avpar->extradata_size);
    meta->setInt32(kKeyRVVersion, kTypeRVVer_8); //http://en.wikipedia.org/wiki/RealVide

    return meta;
}

sp<MetaData> setRV40Format(AVCodecParameters *avpar)
{
    ALOGV("RV40");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_RV);
    meta->setData(kKeyRawCodecSpecificData, 0, avpar->extradata, avpar->extradata_size);
    meta->setInt32(kKeyRVVersion, kTypeRVVer_9); //http://en.wikipedia.org/wiki/RealVide

    return meta;
}

sp<MetaData> setFLV1Format(AVCodecParameters *avpar)
{
    ALOGV("FLV1(Sorenson H263)");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_FLV1);
    meta->setData(kKeyRawCodecSpecificData, 0, avpar->extradata, avpar->extradata_size);

    return meta;
}

sp<MetaData> setHEVCFormat(AVCodecParameters *avpar)
{
    ALOGV("HEVC");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_HEVC);
    meta->setData(kKeyHVCC, kTypeHVCC, avpar->extradata, avpar->extradata_size);

    return meta;
}

sp<MetaData> setVP8Format(AVCodecParameters *avpar __unused)
{
    ALOGV("VP8");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_VP8);

    return meta;
}

sp<MetaData> setVP9Format(AVCodecParameters *avpar __unused)
{
    ALOGV("VP9");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_VP9);

    return meta;
}

//audio

sp<MetaData> setMP2Format(AVCodecParameters *avpar __unused)
{
    ALOGV("MP2");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_MPEG_LAYER_II);

    return meta;
}

sp<MetaData> setMP3Format(AVCodecParameters *avpar __unused)
{
    ALOGV("MP3");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_MPEG);

    return meta;
}

sp<MetaData> setVORBISFormat(AVCodecParameters *avpar)
{
    ALOGV("VORBIS");

    const uint8_t *header_start[3];
    int header_len[3];
    if (avpriv_split_xiph_headers(avpar->extradata,
                avpar->extradata_size, 30,
                header_start, header_len) < 0) {
        ALOGE("vorbis extradata corrupt.");
        return NULL;
    }

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_VORBIS);
    //identification header
    meta->setData(kKeyVorbisInfo,  0, header_start[0], header_len[0]);
    //setup header
    meta->setData(kKeyVorbisBooks, 0, header_start[2], header_len[2]);

    return meta;
}

sp<MetaData> setAC3Format(AVCodecParameters *avpar __unused)
{
    ALOGV("AC3");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_AC3);

    return meta;
}

sp<MetaData> setAACFormat(AVCodecParameters *avpar)
{
    ALOGV("AAC");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_AAC);
    meta->setData(kKeyRawCodecSpecificData, 0, avpar->extradata, avpar->extradata_size);
    meta->setInt32(kKeyAACAOT, avpar->profile + 1);
    return meta;
}

sp<MetaData> setWMAV1Format(AVCodecParameters *avpar)
{
    ALOGV("WMAV1");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_WMA);
    meta->setData(kKeyRawCodecSpecificData, 0, avpar->extradata, avpar->extradata_size);
    meta->setInt32(kKeyWMAVersion, kTypeWMA); //FIXME version?

    return meta;
}

sp<MetaData> setWMAV2Format(AVCodecParameters *avpar)
{
    ALOGV("WMAV2");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_WMA);
    meta->setData(kKeyRawCodecSpecificData, 0, avpar->extradata, avpar->extradata_size);
    meta->setInt32(kKeyWMAVersion, kTypeWMA);

    return meta;
}

sp<MetaData> setWMAProFormat(AVCodecParameters *avpar)
{
    ALOGV("WMAPro");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_WMA);
    meta->setData(kKeyRawCodecSpecificData, 0, avpar->extradata, avpar->extradata_size);
    meta->setInt32(kKeyWMAVersion, kTypeWMAPro);

    return meta;
}

sp<MetaData> setWMALossLessFormat(AVCodecParameters *avpar)
{
    ALOGV("WMALOSSLESS");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_WMA);
    meta->setData(kKeyRawCodecSpecificData, 0, avpar->extradata, avpar->extradata_size);
    meta->setInt32(kKeyWMAVersion, kTypeWMALossLess);

    return meta;
}

sp<MetaData> setRAFormat(AVCodecParameters *avpar)
{
    ALOGV("COOK");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_RA);
    meta->setData(kKeyRawCodecSpecificData, 0, avpar->extradata, avpar->extradata_size);

    return meta;
}

sp<MetaData> setALACFormat(AVCodecParameters *avpar)
{
    ALOGV("ALAC");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_ALAC);
    meta->setData(kKeyRawCodecSpecificData, 0, avpar->extradata, avpar->extradata_size);

    return meta;
}

sp<MetaData> setAPEFormat(AVCodecParameters *avpar)
{
    ALOGV("APE");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_APE);
    meta->setData(kKeyRawCodecSpecificData, 0, avpar->extradata, avpar->extradata_size);

    return meta;
}

sp<MetaData> setDTSFormat(AVCodecParameters *avpar)
{
    ALOGV("DTS");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_DTS);
    meta->setData(kKeyRawCodecSpecificData, 0, avpar->extradata, avpar->extradata_size);

    return meta;
}

sp<MetaData> setFLACFormat(AVCodecParameters *avpar)
{
    ALOGV("FLAC");

    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_FLAC);
    meta->setData(kKeyRawCodecSpecificData, 0, avpar->extradata, avpar->extradata_size);

    if (avpar->extradata_size < 10) {
        ALOGE("Invalid extradata in FLAC file! (size=%d)", avpar->extradata_size);
        return meta;
    }

    ABitReader br(avpar->extradata, avpar->extradata_size);
    int32_t minBlockSize = br.getBits(16);
    int32_t maxBlockSize = br.getBits(16);
    int32_t minFrameSize = br.getBits(24);
    int32_t maxFrameSize = br.getBits(24);

    meta->setInt32('mibs', minBlockSize);
    meta->setInt32('mabs', maxBlockSize);
    meta->setInt32('mifs', minFrameSize);
    meta->setInt32('mafs', maxFrameSize);

    return meta;
}

//Convert H.264 NAL format to annex b
status_t convertNal2AnnexB(uint8_t *dst, size_t dst_size,
        uint8_t *src, size_t src_size, size_t nal_len_size)
{
    size_t i = 0;
    size_t nal_len = 0;
    status_t status = OK;

    CHECK_EQ(dst_size, src_size);
    CHECK(nal_len_size == 3 || nal_len_size == 4);

    while (src_size >= nal_len_size) {
        nal_len = 0;
        for( i = 0; i < nal_len_size; i++ ) {
            nal_len = (nal_len << 8) | src[i];
            dst[i] = 0;
        }
        dst[nal_len_size - 1] = 1;
        if (nal_len > INT_MAX || nal_len > src_size) {
            status = ERROR_MALFORMED;
            break;
        }
        dst += nal_len_size;
        src += nal_len_size;
        src_size -= nal_len_size;

        memcpy(dst, src, nal_len);

        dst += nal_len;
        src += nal_len;
        src_size -= nal_len;
    }

    return status;
}

int getDivXVersion(AVCodecParameters *avpar)
{
    if (avpar->codec_tag == AV_RL32("DIV3")
            || avpar->codec_tag == AV_RL32("div3")
            || avpar->codec_tag == AV_RL32("DIV4")
            || avpar->codec_tag == AV_RL32("div4")) {
        return kTypeDivXVer_3_11;
    }
    if (avpar->codec_tag == AV_RL32("DIVX")
            || avpar->codec_tag == AV_RL32("divx")) {
        return kTypeDivXVer_4;
    }
    if (avpar->codec_tag == AV_RL32("DX50")
           || avpar->codec_tag == AV_RL32("dx50")) {
        return kTypeDivXVer_5;
    }
    return -1;
}

status_t parseMetadataTags(AVFormatContext *ctx, const sp<MetaData> &meta) {
    if (meta == NULL || ctx == NULL) {
        return NO_INIT;
    }

    AVDictionary *dict = ctx->metadata;
    if (dict == NULL) {
        return NO_INIT;
    }

    struct MetadataMapping {
        const char *from;
        int to;
    };

    // avformat -> android mapping
    static const MetadataMapping kMap[] = {
        { "track", kKeyCDTrackNumber },
        { "disc", kKeyDiscNumber },
        { "album", kKeyAlbum },
        { "artist", kKeyArtist },
        { "album_artist", kKeyAlbumArtist },
        { "composer", kKeyComposer },
        { "date", kKeyDate },
        { "genre", kKeyGenre },
        { "title", kKeyTitle },
        { "year", kKeyYear },
        { "compilation", kKeyCompilation },
        { "location", kKeyLocation },
    };

    static const size_t kNumEntries = sizeof(kMap) / sizeof(kMap[0]);

    for (size_t i = 0; i < kNumEntries; ++i) {
        AVDictionaryEntry *entry = av_dict_get(dict, kMap[i].from, NULL, 0);
        if (entry != NULL) {
            ALOGV("found key %s with value %s", entry->key, entry->value);
            meta->setCString(kMap[i].to, entry->value);
        }
    }

    // now look for album art- this will be in a separate stream
    for (size_t i = 0; i < ctx->nb_streams; i++) {
        if (ctx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) {
            AVPacket& pkt = ctx->streams[i]->attached_pic;
            if (pkt.size > 0) {
                if (ctx->streams[i]->codecpar != NULL) {
                    const char *mime;
                    if (ctx->streams[i]->codecpar->codec_id == AV_CODEC_ID_MJPEG) {
                        mime = MEDIA_MIMETYPE_IMAGE_JPEG;
                    } else if (ctx->streams[i]->codecpar->codec_id == AV_CODEC_ID_PNG) {
                        mime = "image/png";
                    } else {
                        mime = NULL;
                    }
                    if (mime != NULL) {
                        ALOGV("found albumart in stream %zu with type %s len %d", i, mime, pkt.size);
                        meta->setData(kKeyAlbumArt, MetaData::TYPE_NONE, pkt.data, pkt.size);
                        meta->setCString(kKeyAlbumArtMIME, mime);
                    }
                }
            }
        }
    }
    return OK;
}

AudioEncoding sampleFormatToEncoding(AVSampleFormat fmt) {

    // we resample planar formats to interleaved
    switch (fmt) {
        case AV_SAMPLE_FMT_U8:
        case AV_SAMPLE_FMT_U8P:
            return kAudioEncodingPcm8bit;
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S16P:
            return kAudioEncodingPcm16bit;
        case AV_SAMPLE_FMT_S32:
        case AV_SAMPLE_FMT_S32P:
            return kAudioEncodingPcm32bit;
        case AV_SAMPLE_FMT_FLT:
        case AV_SAMPLE_FMT_FLTP:
            return kAudioEncodingPcmFloat;
        case AV_SAMPLE_FMT_DBL:
        case AV_SAMPLE_FMT_DBLP:
            return kAudioEncodingPcmFloat;
        default:
            return kAudioEncodingInvalid;
    }

}

AVSampleFormat encodingToSampleFormat(AudioEncoding encoding) {
    switch (encoding) {
        case kAudioEncodingPcm8bit:
            return AV_SAMPLE_FMT_U8;
        case kAudioEncodingPcm16bit:
            return AV_SAMPLE_FMT_S16;
        case kAudioEncodingPcm24bitPacked:
        case kAudioEncodingPcm32bit:
            return AV_SAMPLE_FMT_S32;
        case kAudioEncodingPcmFloat:
            return AV_SAMPLE_FMT_FLT;
        default:
            return AV_SAMPLE_FMT_NONE;
    }
}

}  // namespace android

