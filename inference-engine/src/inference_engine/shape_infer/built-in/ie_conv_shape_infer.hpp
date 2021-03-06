// Copyright (C) 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <debug.h>

#include <cmath>
#include <description_buffer.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ie_built_in_impl.hpp"

namespace InferenceEngine {
namespace ShapeInfer {

/**
 *@brief Implementation of Shape inference for Convolution layer
 */
class ConvShapeProp : public BuiltInShapeInferImpl {
public:
    explicit ConvShapeProp(const std::string& type): BuiltInShapeInferImpl(type) {}

    void inferShapesImpl(const std::vector<Blob::CPtr>& inBlobs, const std::map<std::string, std::string>& params,
                         const std::map<std::string, Blob::Ptr>& blobs, std::vector<SizeVector>& outShapes) override {
        LayerParams lp {};
        ConvolutionLayer convLayer(lp);
        convLayer.params = params;
        convLayer.type = _type;
        validate(&convLayer, inBlobs, params, blobs);

        auto dims = inShapes[0];
        auto dims_size = dims.size();
        auto spacial_d_size = dims.size() - 2;
        float* OD_temp = new float[spacial_d_size];
        size_t* KDims = new size_t[spacial_d_size];
        size_t inputN = dims[0];
        for (int i = 0; i < spacial_d_size; i++) {
            if (convLayer._dilation[i])
                KDims[i] = (convLayer._kernel[i] - 1) * convLayer._dilation[i] + 1;
            else
                KDims[i] = convLayer._kernel[i];
        }
        size_t OC = convLayer._out_depth;
        std::string padType = convLayer._auto_pad;
        if (padType == "valid") {
            for (int i = 0; i < spacial_d_size; i++)
                OD_temp[i] = std::ceil((dims[dims_size - 1 - i] - KDims[i] + 1.f) / convLayer._stride[i]);
        } else if (padType == "same_upper") {
            for (int i = 0; i < spacial_d_size; i++)
                OD_temp[i] = std::ceil(1.f * dims[dims_size - 1 - i] / convLayer._stride[i]);
        } else if (padType == "same_lower") {
            for (int i = 0; i < spacial_d_size; i++)
                OD_temp[i] = std::floor(1.f * dims[dims_size - 1 - i] / convLayer._stride[i]);
        } else {
            for (int i = 0; i < spacial_d_size; i++) {
                OD_temp[i] =
                    std::floor(1.f *
                               (dims[dims_size - 1 - i] + convLayer._padding[i] + convLayer._pads_end[i] - KDims[i]) /
                               convLayer._stride[i]) +
                    1.f;
            }
        }

        for (int i = 0; i < spacial_d_size; i++)
            if (OD_temp[i] < 0)
                THROW_IE_EXCEPTION << "New shapes " << details::dumpVec(dims) << " make output shape negative";

        SizeVector outShape = {inputN, OC};
        for (int i = spacial_d_size - 1; i >= 0; i--) outShape.push_back(static_cast<size_t>(OD_temp[i]));

        outShapes.push_back(outShape);

        delete[] OD_temp;
        delete[] KDims;
    }
};

}  // namespace ShapeInfer
}  // namespace InferenceEngine
