// Tencent is pleased to support the open source community by making TNN available.
//
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include "landmark_smoothing_filter.h"

namespace TNN_NS {

static float GetObjectScale(const NormalizedLandmarkList& landmarks, int image_width,
                            int image_height) {
    const auto& lm_min_x = std::min_element(
                                            landmarks.begin(), landmarks.end(),
                                            [](const auto& a, const auto& b) { return a.X() < b.X(); });
    const auto& lm_max_x = std::max_element(
                                            landmarks.begin(), landmarks.end(),
                                            [](const auto& a, const auto& b) { return a.X() > b.X(); });
    
    if (landmarks.size() <= 0)
        return 0;
    
    const float x_min = lm_min_x->X();
    const float x_max = lm_max_x->X();
    
    const auto& lm_min_y = std::min_element(
                                            landmarks.begin(), landmarks.end(),
                                            [](const auto& a, const auto& b) { return a.Y() < b.Y(); });
    const auto& lm_max_y = std::max_element(
                                            landmarks.begin(), landmarks.end(),
                                            [](const auto& a, const auto& b) { return a.Y() > b.Y(); });
    
    const float y_min = lm_min_y->Y();
    const float y_max = lm_max_y->Y();
    
    const float object_width = (x_max - x_min) * image_width;
    const float object_height = (y_max - y_min) * image_height;
    
    return (object_width + object_height) / 2.0f;
}

static float GetObjectScale(const Normalized2DLandmarkList& landmarks, int image_width,
                            int image_height) {
    const auto& lm_min_x = std::min_element(
                                            landmarks.begin(), landmarks.end(),
                                            [](const auto& a, const auto& b) { return a.X() < b.X(); });
    const auto& lm_max_x = std::max_element(
                                            landmarks.begin(), landmarks.end(),
                                            [](const auto& a, const auto& b) { return a.X() > b.X(); });
    
    if (landmarks.size() <= 0)
        return 0;
    
    const float x_min = lm_min_x->X();
    const float x_max = lm_max_x->Y();
    
    const auto& lm_min_y = std::min_element(
                                            landmarks.begin(), landmarks.end(),
                                            [](const auto& a, const auto& b) { return a.Y() < b.Y(); });
    const auto& lm_max_y = std::max_element(
                                            landmarks.begin(), landmarks.end(),
                                            [](const auto& a, const auto& b) { return a.Y() > b.Y(); });
    
    const float y_min = lm_min_y->Y();
    const float y_max = lm_max_y->Y();
    
    const float object_width = (x_max - x_min) * image_width;
    const float object_height = (y_max - y_min) * image_height;
    
    return (object_width + object_height) / 2.0f;
}

TNN_NS::Status VelocityFilter::Reset() {
    x_filters_.clear();
    y_filters_.clear();
    z_filters_.clear();
    return TNN_NS::TNN_OK;
}

bool VelocityFilter::isValidLandMark(const NormalizedLandmark& m) {
    bool valid = (m.X() >= 0 &&
                  m.Y() >= 0 &&
                  m.Z() >= 0);
    return valid;
}

bool VelocityFilter::isValidLandMark(const Normalized2DLandmark& m) {
    bool valid = (m.X() >=0) && (m.Y() >= 0);
    return valid;
}

TNN_NS::Status VelocityFilter::Apply(const NormalizedLandmarkList& in_landmarks,
                                     const std::pair<int, int>& image_size,
                                     const TimeStamp& timestamp,
                                     NormalizedLandmarkList* out_landmarks) {
    // Get image size.
    int image_width;
    int image_height;
    std::tie(image_height, image_width) = image_size;
    
    // Get value scale as inverse value of the object scale.
    // If value is too small smoothing will be disabled and landmarks will be
    // returned as is.
    NormalizedLandmarkList filterd;
    auto filter = [](const NormalizedLandmark&m){
        return isValidLandMark(m);
    };
    std::copy_if(in_landmarks.begin(), in_landmarks.end(), std::back_inserter(filterd), filter);
    const float object_scale = GetObjectScale(filterd, image_width, image_height);
    if (object_scale < min_allowed_object_scale_) {
        *out_landmarks = in_landmarks;
        return TNN_NS::TNN_OK;
    }
    const float value_scale = 1.0f / object_scale;
    
    // Initialize filters once.
    auto status = InitializeFiltersIfEmpty(in_landmarks.size());
    RETURN_ON_NEQ(status, TNN_OK);
    
    // Filter landmarks. Every axis of every landmark is filtered separately.
    for (int i = 0; i < in_landmarks.size(); ++i) {
        const NormalizedLandmark& in_landmark = in_landmarks[i];
        
        if (!isValidLandMark(in_landmark)) {
            out_landmarks->push_back(in_landmark);
            continue;
        }
        
        float out_x = x_filters_[i].Apply(timestamp, value_scale,
                                          in_landmark.X() * image_width) / image_width;
        float out_y = y_filters_[i].Apply(timestamp, value_scale,
                                          in_landmark.Y() * image_height) / image_height;
        float out_z = z_filters_[i].Apply(timestamp, value_scale,
                                          in_landmark.Z() * image_width) / image_width;
        
        NormalizedLandmark out_landmark = Landmark3D(out_x, out_y, out_z);
        out_landmarks->push_back(std::move(out_landmark));
    }
    
    return TNN_OK;
}

TNN_NS::Status VelocityFilter::Apply2D(const Normalized2DLandmarkList& in_landmarks,
                                       const std::pair<int, int>& image_size,
                                       const TimeStamp& timestamp,
                                       Normalized2DLandmarkList* out_landmarks) {
    // Get image size.
    int image_width;
    int image_height;
    std::tie(image_height, image_width) = image_size;
    
    // Get value scale as inverse value of the object scale.
    // If value is too small smoothing will be disabled and landmarks will be
    // returned as is.
    Normalized2DLandmarkList filterd;
    auto filter = [](const Normalized2DLandmark&m){
        return isValidLandMark(m);
    };
    std::copy_if(in_landmarks.begin(), in_landmarks.end(), std::back_inserter(filterd), filter);
    const float object_scale = GetObjectScale(filterd, image_width, image_height);
    if (object_scale < min_allowed_object_scale_) {
        *out_landmarks = in_landmarks;
        return TNN_NS::TNN_OK;
    }
    const float value_scale = 1.0f / object_scale;
    
    // Initialize filters once.
    auto status = InitializeFiltersIfEmpty(in_landmarks.size());
    RETURN_ON_NEQ(status, TNN_OK);
    
    // Filter landmarks. Every axis of every landmark is filtered separately.
    for (int i = 0; i < in_landmarks.size(); ++i) {
        const Normalized2DLandmark& in_landmark = in_landmarks[i];
        
        if (!isValidLandMark(in_landmark)) {
            out_landmarks->push_back(in_landmark);
            continue;
        }
        
        float out_x = x_filters_[i].Apply(timestamp, value_scale,
                                          in_landmark.X() * image_width) / image_width;
        float out_y = y_filters_[i].Apply(timestamp, value_scale,
                                          in_landmark.Y() * image_height) / image_height;
        
        Normalized2DLandmark out_landmark = Landmark2D(out_x, out_y);
        out_landmarks->push_back(std::move(out_landmark));
    }
    
    return TNN_OK;
}

TNN_NS::Status VelocityFilter::InitializeFiltersIfEmpty(const size_t n_landmarks) {
    if (!x_filters_.empty()) {
        RETURN_VALUE_ON_NEQ(x_filters_.size(), n_landmarks, Status(TNNERR_PARAM_ERR, "invalid landmark size!"));
        RETURN_VALUE_ON_NEQ(y_filters_.size(), n_landmarks, Status(TNNERR_PARAM_ERR, "invalid landmark size!"));
        RETURN_VALUE_ON_NEQ(z_filters_.size(), n_landmarks, Status(TNNERR_PARAM_ERR, "invalid landmark size!"));
        return TNN_OK;
    }
    
    x_filters_.resize(n_landmarks,
                      RelativeVelocityFilter(window_size_, velocity_scale_, target_fps_));
    y_filters_.resize(n_landmarks,
                      RelativeVelocityFilter(window_size_, velocity_scale_, target_fps_));
    z_filters_.resize(n_landmarks,
                      RelativeVelocityFilter(window_size_, velocity_scale_, target_fps_));
    
    return TNN_OK;
}

}
