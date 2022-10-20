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

#ifndef CODEC_UTILS_H_

#define CODEC_UTILS_H_

#include <unistd.h>
#include <stdlib.h>

#include <utils/Errors.h>
#include <media/stagefright/foundation/ABuffer.h>

#include "ffmpeg_utils.h"

namespace android {

//video
sp<MetaData> setAVCFormat(AVCodecParameters *avpar);
sp<MetaData> setH264Format(AVCodecParameters *avpar);
sp<MetaData> setMPEG4Format(AVCodecParameters *avpar);
sp<MetaData> setH263Format(AVCodecParameters *avpar);
sp<MetaData> setMPEG2VIDEOFormat(AVCodecParameters *avpar);
sp<MetaData> setVC1Format(AVCodecParameters *avpar);
sp<MetaData> setWMV1Format(AVCodecParameters *avpar);
sp<MetaData> setWMV2Format(AVCodecParameters *avpar);
sp<MetaData> setWMV3Format(AVCodecParameters *avpar);
sp<MetaData> setRV20Format(AVCodecParameters *avpar);
sp<MetaData> setRV30Format(AVCodecParameters *avpar);
sp<MetaData> setRV40Format(AVCodecParameters *avpar);
sp<MetaData> setFLV1Format(AVCodecParameters *avpar);
sp<MetaData> setHEVCFormat(AVCodecParameters *avpar);
sp<MetaData> setVP8Format(AVCodecParameters *avpar);
sp<MetaData> setVP9Format(AVCodecParameters *avpar);
//audio
sp<MetaData> setMP2Format(AVCodecParameters *avpar);
sp<MetaData> setMP3Format(AVCodecParameters *avpar);
sp<MetaData> setVORBISFormat(AVCodecParameters *avpar);
sp<MetaData> setAC3Format(AVCodecParameters *avpar);
sp<MetaData> setAACFormat(AVCodecParameters *avpar);
sp<MetaData> setWMAV1Format(AVCodecParameters *avpar);
sp<MetaData> setWMAV2Format(AVCodecParameters *avpar);
sp<MetaData> setWMAProFormat(AVCodecParameters *avpar);
sp<MetaData> setWMALossLessFormat(AVCodecParameters *avpar);
sp<MetaData> setRAFormat(AVCodecParameters *avpar);
sp<MetaData> setAPEFormat(AVCodecParameters *avpar);
sp<MetaData> setDTSFormat(AVCodecParameters *avpar);
sp<MetaData> setFLACFormat(AVCodecParameters *avpar);
sp<MetaData> setALACFormat(AVCodecParameters *avpar);

//Convert H.264 NAL format to annex b
status_t convertNal2AnnexB(uint8_t *dst, size_t dst_size,
        uint8_t *src, size_t src_size, size_t nal_len_size);

int getDivXVersion(AVCodecParameters *avpar);

status_t parseMetadataTags(AVFormatContext *ctx, const sp<MetaData> &meta);

AudioEncoding sampleFormatToEncoding(AVSampleFormat fmt);
AVSampleFormat encodingToSampleFormat(AudioEncoding encoding);

}  // namespace android

#endif  // CODEC_UTILS_H_
