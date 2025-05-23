/*
 * Copyright (C) 2024-present, Guanyou.Chen. All rights reserved.
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

#ifndef ANDROID_ART_RUNTIME_NTERP_HELPERS_H_
#define ANDROID_ART_RUNTIME_NTERP_HELPERS_H_

#include "runtime/interpreter/quick_frame.h"
#include "runtime/quick/quick_method_frame_info.h"

namespace art {

QuickMethodFrameInfo NterpFrameInfo(ArtMethod& method);
inline QuickMethodFrameInfo NterpFrameInfo(QuickFrame& frame) {
    return NterpFrameInfo(frame.GetMethod());
}
uint64_t NterpGetFrameDexPcPtr(QuickFrame& frame);
void NterpGetFrameVRegs(QuickFrame& frame);

} // namespace art

#endif // ANDROID_ART_RUNTIME_NTERP_HELPERS_H_
