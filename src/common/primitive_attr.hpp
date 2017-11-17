/*******************************************************************************
* Copyright 2017 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#ifndef PRIMITIVE_ATTR_HPP
#define PRIMITIVE_ATTR_HPP

#include "mkldnn.h"

#include "c_types_map.hpp"
#include "nstl.hpp"
#include "utils.hpp"

namespace mkldnn {
namespace impl {

struct scales_t: public c_compatible {
    scales_t(): count_(1), mask_(0), scales_(scales_buf_)
    { set(1.); }

    scales_t(const scales_t &rhs): scales_t()
    { set(rhs.count_, rhs.mask_, rhs.scales_); }

    ~scales_t() { cleanup(); }

    scales_t &operator=(const scales_t &rhs) {
        if (&rhs == this)
            return *this;
        status_t status = set(rhs.count_, rhs.mask_, rhs.scales_);
        assert(status == status::success);
        (void)status;
        return *this;
    }

    status_t set(int count, int mask, const float *scales);
    status_t set(float single_scale) { return this->set(1, 0, &single_scale); }

    int count_;
    int mask_;
    float *scales_;

private:
    enum { scales_buf_size = 16 };
    alignas(64) float scales_buf_[scales_buf_size];

    void cleanup() {
        if (scales_ != scales_buf_ && scales_ != nullptr)
            impl::free(scales_);

        count_ = 1;
        mask_ = 0;
        scales_ = scales_buf_;
    }
};

}
}

struct mkldnn_post_ops: public mkldnn::impl::c_compatible {
    struct entry_t {
        mkldnn::impl::primitive_kind_t kind;
        union {
            struct { float scale; } sum;
            struct {
                mkldnn::impl::alg_kind_t alg;
                float scale, alpha, beta;
            } eltwise;
        };
    };

    mkldnn_post_ops(): len_(0) {}

    mkldnn::impl::status_t append_sum(float scale);
    mkldnn::impl::status_t append_eltwise(float scale,
            mkldnn::impl::alg_kind_t alg, float alpha, float beta);

    int find(mkldnn::impl::primitive_kind_t kind, int start = 0,
            int stop = -1) const {
        if (stop == -1) stop = len_;
        stop = mkldnn::impl::nstl::min(stop, len_);
        for (int idx = start; idx < stop; ++idx)
            if (entry_[idx].kind == kind) return idx;
        return -1;
    }

    bool contain(mkldnn::impl::primitive_kind_t kind, int index) const
    { return find(kind, index, index + 1) == index; }

    enum { capacity = 4 };

    int len_;
    entry_t entry_[capacity];
};

struct mkldnn_primitive_attr: public mkldnn::impl::c_compatible {
    mkldnn_primitive_attr()
        : round_mode_(mkldnn::impl::round_mode::nearest) {}

    mkldnn_primitive_attr *clone() const
    { return new mkldnn_primitive_attr(*this); }

    mkldnn::impl::status_t set_round_mode(
            mkldnn::impl::round_mode_t round_mode);
    mkldnn::impl::status_t set_post_ops(
            const mkldnn::impl::post_ops_t &post_ops);

    mkldnn::impl::round_mode_t round_mode_;
    mkldnn::impl::scales_t output_scales_;
    mkldnn::impl::post_ops_t post_ops_;
};

#endif