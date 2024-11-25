// BSD 3 Clause
// Copyright 2023 Advanced Micro Devices, Inc.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
// 3. Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software without
// specific prior written permission. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
// HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
// OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#undef __HIP_NO_HALF_CONVERSIONS__

#include <cstdlib>
#include <initializer_list>
#include <numeric>

#include "params.hpp"

#if defined(__WMMA__)
// wmma forward gemm
#include "ck/tensor_operation/gpu/device/impl/device_grouped_query_attention_forward_wmma.hpp"
#include "ck/tensor_operation/gpu/device/impl/device_multi_query_attention_forward_wmma.hpp"
#endif

namespace device_gemm_trait {
template <ck::index_t... Is>
using S = ck::Sequence<Is...>;

using Int32 = int;
using Int16 = unsigned short;
using Int8 = uint8_t;
using Float32 = float;
using BFloat16 = ck::bhalf_t;
using Float16 = ck::half_t;

using PassThrough = ck::tensor_operation::element_wise::PassThrough;
using Scale = ck::tensor_operation::element_wise::Scale;
using MaskingSpec = ck::tensor_operation::device::MaskingSpecialization;
using TensorSpec = ck::tensor_operation::device::TensorSpecialization;
using GemmSpec = ck::tensor_operation::device::GemmSpecialization;
using Index = ck::index_t;

static constexpr bool kDeterministic = true;
static constexpr bool kNonDeterministic = false;
static constexpr auto kGemmSpecDefault = GemmSpec::Default;
static constexpr auto kGemmSpecPadding = GemmSpec::MNKOPadding;
static constexpr auto kMaskingSpecDefault = MaskingSpec::MaskDisabled;
static constexpr auto kMaskingSpecCausal =
    MaskingSpec::MaskOutUpperTriangle;

template <typename InputDataType_, GemmSpec kGemmSpec_,
          MaskingSpec kMaskingSpec_, int kQueryGroupNumber_, bool kIsDeterministic_ = kNonDeterministic>
struct Forward {
  using QDataType = InputDataType_;
  using KDataType = InputDataType_;
  using VDataType = InputDataType_;
  using AccDataType = Float32;
  using OutShuffleDataType = Float32;
  using OutDataType = InputDataType_;
  using GemmDataType = InputDataType_;
  using ZDataType = Int8;
  using LSEDataType = Float32;
  using Acc0BiasDataType = ck::Tuple<>;
  using Acc1BiasDataType = ck::Tuple<>;

  using QElementOp = PassThrough;
  using KElementOp = PassThrough;
  using VElementOp = PassThrough;
  using OutElementOp = PassThrough;
  using Acc0ElementOp = Scale;

  static constexpr Index kNumDimG = 2;
  static constexpr Index kNumDimM = 1;
  static constexpr Index kNumDimN = 1;
  static constexpr Index kNumDimK = 1;
  static constexpr Index kNumDimO = 1;
  static constexpr Index kQueryGroupNumber = kQueryGroupNumber_;

  static constexpr auto kGemmSpec = kGemmSpec_;

  static constexpr auto kTensorSpecQ = TensorSpec::Default;
  static constexpr auto kTensorSpecK = TensorSpec::Default;
  static constexpr auto kTensorSpecV = TensorSpec::Default;
  static constexpr auto kTensorSpecOut = TensorSpec::Default;

  static constexpr auto kMaskingSpec = kMaskingSpec_;
  static constexpr bool kIsDeterministic = kIsDeterministic_;
}; // device gemm traits forward
} // namespace device_gemm_trait
