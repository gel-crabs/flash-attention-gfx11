// !!! This is a file automatically generated by hipify!!!
// Adapted from https://github.com/NVIDIA/apex/blob/master/csrc/fused_dense_hip.cu
#include <ATen/ATen.h>
#include <ATen/hip/HIPContext.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <torch/torch.h>

/* Includes, cuda */
#include <hipblas/hipblas.h>
#include <hip/hip_runtime.h>

#if defined(HIPBLASLT_VERSION)
#include <hipblaslt/hipblaslt.h>
#endif

// FP16 Tensor core wrapper around hipblas GEMMEx
hipblasStatus_t gemm_bias(
    hipblasHandle_t handle,
    hipblasOperation_t transa,
    hipblasOperation_t transb,
    int64_t m,
    int64_t n,
    int64_t k,
    const float* alpha,
    const at::Half* A,
    int64_t lda,
    const at::Half* B,
    int64_t ldb,
    const float* beta,
    at::Half* C,
    int64_t ldc) {
  return hipblasGemmEx_v2(
      handle,
      transa,
      transb,
      m,
      n,
      k,
      alpha,
      A,
      HIP_R_16F,
      lda,
      B,
      HIP_R_16F,
      ldb,
      beta,
      C,
      HIP_R_16F,
      ldc,
      HIP_R_32F,
      HIPBLAS_GEMM_DEFAULT);
}

// BF16 Tensor core wrapper around hipblas GEMMEx
hipblasStatus_t gemm_bias(
    hipblasHandle_t handle,
    hipblasOperation_t transa,
    hipblasOperation_t transb,
    int64_t m,
    int64_t n,
    int64_t k,
    const float* alpha,
    const at::BFloat16* A,
    int64_t lda,
    const at::BFloat16* B,
    int64_t ldb,
    const float* beta,
    at::BFloat16* C,
    int64_t ldc) {
  return hipblasGemmEx_v2(
      handle,
      transa,
      transb,
      m,
      n,
      k,
      alpha,
      A,
      HIP_R_16BF,
      lda,
      B,
      HIP_R_16BF,
      ldb,
      beta,
      C,
      HIP_R_16BF,
      ldc,
      HIP_R_32F,
      HIPBLAS_GEMM_DEFAULT);
}

#if defined(HIPBLASLT_VERSION)

template <typename Dtype>
int gemm_bias_act_lt(
    hipblasOperation_t transa,
    hipblasOperation_t transb,
    int64_t m,
    int64_t n,
    int64_t k,
    float alpha,
    const Dtype* A,
    int64_t lda,
    const Dtype* B,
    int64_t ldb,
    const Dtype* bias,
    Dtype* C,
    int64_t ldc,
    void* pre_act,
    bool is_gelu,
    int heuristic,
    void *lt_workspace,
    size_t workspaceSize
    ) {
  static_assert(std::is_same<Dtype, at::Half>::value || std::is_same<Dtype, at::BFloat16>::value,
                "gemm_bias_act_lt only supports fp16 and bf16");
  bool save_pre_act = pre_act != nullptr;
  float beta = 0.0;
  hipblasComputeType_t abcType = std::is_same<Dtype, at::Half>::value ? HIP_R_16F : HIP_R_16BF;

  hipblasLtHandle_t ltHandle =
    reinterpret_cast<hipblasLtHandle_t>(at::cuda::getCurrentCUDABlasHandle());

  hipblasStatus_t status = HIPBLAS_STATUS_SUCCESS;

  hipblasLtMatmulDescOpaque_t operationDesc = {};
  hipblasLtMatrixLayoutOpaque_t Adesc = {}, Bdesc = {}, Cdesc = {};
  hipblasLtMatmulPreferenceOpaque_t preference = {};

  int returnedResults                             = 0;
  constexpr int requestedAlgoCount = 5;
  hipblasLtMatmulHeuristicResult_t heuristicResult[requestedAlgoCount] = {0};
  // constexpr int requestedAlgoCount = 1;
  // hipblasLtMatmulHeuristicResult_t heuristicResult = {};
  hipblasLtEpilogue_t epilogue = is_gelu
      ? (save_pre_act ? HIPBLASLT_EPILOGUE_GELU_AUX : HIPBLASLT_EPILOGUE_GELU)
      : (save_pre_act ? HIPBLASLT_EPILOGUE_RELU_AUX : HIPBLASLT_EPILOGUE_RELU);

  // Create operation descriptor; see hipblasLtMatmulDescAttributes_t
  // for details about defaults; here we just set the transforms for
  // A and B.
  status = hipblasLtMatmulDescCreate(&operationDesc, HIPBLAS_COMPUTE_32F, HIP_R_32F);
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;
  status = hipblasLtMatmulDescSetAttribute(&operationDesc, HIPBLASLT_MATMUL_DESC_TRANSA, &transa, sizeof(transa));
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;
  status = hipblasLtMatmulDescSetAttribute(&operationDesc, HIPBLASLT_MATMUL_DESC_TRANSB, &transb, sizeof(transa));
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;

  if (save_pre_act) {
    status = hipblasLtMatmulDescSetAttribute(&operationDesc, HIPBLASLT_MATMUL_DESC_EPILOGUE_AUX_POINTER, &pre_act, sizeof(pre_act));
    status = hipblasLtMatmulDescSetAttribute(&operationDesc, HIPBLASLT_MATMUL_DESC_EPILOGUE_AUX_LD, &ldc, sizeof(ldc));
  }

  if (bias != nullptr) {
    status = hipblasLtMatmulDescSetAttribute(&operationDesc, HIPBLASLT_MATMUL_DESC_BIAS_POINTER, &bias, sizeof(bias));
    if (status != HIPBLAS_STATUS_SUCCESS) {
      goto CLEANUP;
    }
    epilogue = is_gelu
        ? (save_pre_act ? HIPBLASLT_EPILOGUE_GELU_AUX_BIAS : HIPBLASLT_EPILOGUE_GELU_BIAS)
        : (save_pre_act ? HIPBLASLT_EPILOGUE_RELU_AUX_BIAS : HIPBLASLT_EPILOGUE_RELU_BIAS);
  } else {
    epilogue = is_gelu
        ? (save_pre_act ? HIPBLASLT_EPILOGUE_GELU_AUX : HIPBLASLT_EPILOGUE_GELU)
        : (save_pre_act ? HIPBLASLT_EPILOGUE_RELU_AUX : HIPBLASLT_EPILOGUE_RELU);
  }

  status = hipblasLtMatmulDescSetAttribute(&operationDesc, HIPBLASLT_MATMUL_DESC_EPILOGUE, &epilogue, sizeof(epilogue));
  if (status != HIPBLAS_STATUS_SUCCESS) {
    goto CLEANUP;
  }

  // Create matrix descriptors. Not setting any extra attributes.
  status = hipblasLtMatrixLayoutCreate(
    &Adesc, abcType, transa == HIPBLAS_OP_N ? m : k, transa == HIPBLAS_OP_N ? k : m, lda);
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;
  status = hipblasLtMatrixLayoutCreate(
    &Bdesc, abcType, transb == HIPBLAS_OP_N ? k : n, transb == HIPBLAS_OP_N ? n : k, ldb);
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;
  status = hipblasLtMatrixLayoutCreate(&Cdesc, abcType, m, n, ldc);
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;

  // Create preference handle; In general, extra attributes can be
  // used here to disable tensor ops or to make sure algo selected
  // will work with badly aligned A, B, C. However, for simplicity
  // here we assume A,B,C are always well aligned (e.g., directly
  // come from hipMalloc)
  status = hipblasLtMatmulPreferenceCreate(&preference);
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;
  status = hipblasLtMatmulPreferenceSetAttribute(
    &preference, HIPBLASLT_MATMUL_PREF_MAX_WORKSPACE_BYTES, &workspaceSize, sizeof(workspaceSize));
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;

  // We just need the best available heuristic to try and run matmul.
  // There is no guarantee that this will work. For example, if A is
  // badly aligned, you can request more (e.g. 32) algos and try to
  // run them one by one until something works.
  status = hipblasLtMatmulAlgoGetHeuristic(
    ltHandle, &operationDesc, &Adesc, &Bdesc, &Cdesc, &Cdesc, &preference, requestedAlgoCount, heuristicResult, &returnedResults);
    // ltHandle, &operationDesc, &Adesc, &Bdesc, &Cdesc, &Cdesc, &preference, 1, &heuristicResult, &returnedResults);
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;

  if (returnedResults == 0) {
    status = HIPBLAS_STATUS_NOT_SUPPORTED;
    goto CLEANUP;
  }
  status = hipblasLtMatmul(ltHandle,
                          &operationDesc,
                          &alpha,
                          A,
                          &Adesc,
                          B,
                          &Bdesc,
                          &beta,
                          C,
                          &Cdesc,
                          C,
                          &Cdesc,
                          // &heuristicResult.algo,
                          // TD [2022-04-29] Somehow algo 0 and 2 are a lot slower than other algos
                          &heuristicResult[heuristic].algo,
                          // NULL,
                          lt_workspace,
                          workspaceSize,
                          at::hip::getCurrentHIPStream());

CLEANUP:
  // Descriptors are no longer needed as all GPU work was already
  // enqueued.
  return status == HIPBLAS_STATUS_SUCCESS ? 0 : 1;
}

template int gemm_bias_act_lt(
    hipblasOperation_t transa,
    hipblasOperation_t transb,
    int64_t m,
    int64_t n,
    int64_t k,
    float alpha,
    const at::Half* A,
    int64_t lda,
    const at::Half* B,
    int64_t ldb,
    const at::Half* bias,
    at::Half* C,
    int64_t ldc,
    void* pre_act,
    bool is_gelu,
    int heuristic,
    void *lt_workspace,
    size_t workspaceSize);

template int gemm_bias_act_lt(
    hipblasOperation_t transa,
    hipblasOperation_t transb,
    int64_t m,
    int64_t n,
    int64_t k,
    float alpha,
    const at::BFloat16* A,
    int64_t lda,
    const at::BFloat16* B,
    int64_t ldb,
    const at::BFloat16* bias,
    at::BFloat16* C,
    int64_t ldc,
    void* pre_act,
    bool is_gelu,
    int heuristic,
    void *lt_workspace,
    size_t workspaceSize);

template <typename Dtype>
int gemm_bgradb_lt(
    hipblasOperation_t transa,
    hipblasOperation_t transb,
    int64_t m,
    int64_t n,
    int64_t k,
    float alpha,
    const Dtype* A,
    int64_t lda,
    const Dtype* B,
    int64_t ldb,
    Dtype* C,
    int64_t ldc,
    Dtype* bgrad,
    void *lt_workspace,
    size_t workspaceSize) {
  static_assert(std::is_same<Dtype, at::Half>::value || std::is_same<Dtype, at::BFloat16>::value,
                "gemm_bgradb_lt only supports fp16 and bf16");
  float beta = 0.0;
  hipblasComputeType_t abcType = std::is_same<Dtype, at::Half>::value ? HIP_R_16F : HIP_R_16BF;

  hipblasLtHandle_t ltHandle =
    reinterpret_cast<hipblasLtHandle_t>(at::cuda::getCurrentCUDABlasHandle());

  hipblasStatus_t status = HIPBLAS_STATUS_SUCCESS;

  hipblasLtMatmulDescOpaque_t operationDesc = {};
  hipblasLtMatrixLayoutOpaque_t Adesc = {}, Bdesc = {}, Cdesc = {};
  hipblasLtMatmulPreferenceOpaque_t preference = {};

  int returnedResults                             = 0;
  hipblasLtMatmulHeuristicResult_t heuristicResult = {};
  hipblasLtEpilogue_t epilogue = HIPBLASLT_EPILOGUE_DEFAULT;

  // Create operation descriptor; see hipblasLtMatmulDescAttributes_t
  // for details about defaults; here we just set the transforms for
  // A and B.
  status = hipblasLtMatmulDescCreate(&operationDesc, HIPBLAS_COMPUTE_32F, HIP_R_32F);
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;
  status = hipblasLtMatmulDescSetAttribute(&operationDesc, HIPBLASLT_MATMUL_DESC_TRANSA, &transa, sizeof(transa));
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;
  status = hipblasLtMatmulDescSetAttribute(&operationDesc, HIPBLASLT_MATMUL_DESC_TRANSB, &transb, sizeof(transa));
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;

  if (bgrad != nullptr) {
    status = hipblasLtMatmulDescSetAttribute(&operationDesc, HIPBLASLT_MATMUL_DESC_BIAS_POINTER, &bgrad, sizeof(bgrad));
    if (status != HIPBLAS_STATUS_SUCCESS) {
      goto CLEANUP;
    }
      epilogue = HIPBLASLT_EPILOGUE_BGRADB;
  }

  status = hipblasLtMatmulDescSetAttribute(&operationDesc, HIPBLASLT_MATMUL_DESC_EPILOGUE, &epilogue, sizeof(epilogue));
  if (status != HIPBLAS_STATUS_SUCCESS) {
    goto CLEANUP;
  }

  // Create matrix descriptors. Not setting any extra attributes.
  status = hipblasLtMatrixLayoutCreate(
    &Adesc, abcType, transa == HIPBLAS_OP_N ? m : k, transa == HIPBLAS_OP_N ? k : m, lda);
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;
  status = hipblasLtMatrixLayoutCreate(
    &Bdesc, abcType, transb == HIPBLAS_OP_N ? k : n, transb == HIPBLAS_OP_N ? n : k, ldb);
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;
  status = hipblasLtMatrixLayoutCreate(&Cdesc, abcType, m, n, ldc);
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;

  // Create preference handle; In general, extra attributes can be
  // used here to disable tensor ops or to make sure algo selected
  // will work with badly aligned A, B, C. However, for simplicity
  // here we assume A,B,C are always well aligned (e.g., directly
  // come from hipMalloc)
  status = hipblasLtMatmulPreferenceCreate(&preference);
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;
  status = hipblasLtMatmulPreferenceSetAttribute(
    &preference, HIPBLASLT_MATMUL_PREF_MAX_WORKSPACE_BYTES, &workspaceSize, sizeof(workspaceSize));
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;

  // We just need the best available heuristic to try and run matmul.
  // There is no guarantee that this will work. For example, if A is
  // badly aligned, you can request more (e.g. 32) algos and try to
  // run them one by one until something works.
  status = hipblasLtMatmulAlgoGetHeuristic(
    ltHandle, &operationDesc, &Adesc, &Bdesc, &Cdesc, &Cdesc, &preference, 1, &heuristicResult, &returnedResults);
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;

  if (returnedResults == 0) {
    status = HIPBLAS_STATUS_NOT_SUPPORTED;
    goto CLEANUP;
  }
  status = hipblasLtMatmul(ltHandle,
                          &operationDesc,
                          &alpha,
                          A,
                          &Adesc,
                          B,
                          &Bdesc,
                          &beta,
                          C,
                          &Cdesc,
                          C,
                          &Cdesc,
                          //&heuristicResult.algo,
                          NULL,
                          lt_workspace,
                          workspaceSize,
                          at::hip::getCurrentHIPStream());

CLEANUP:
  // Descriptors are no longer needed as all GPU work was already
  // enqueued.
  return status == HIPBLAS_STATUS_SUCCESS ? 0 : 1;
}


template int gemm_bgradb_lt(
    hipblasOperation_t transa,
    hipblasOperation_t transb,
    int64_t m,
    int64_t n,
    int64_t k,
    float alpha,
    const at::Half* A,
    int64_t lda,
    const at::Half* B,
    int64_t ldb,
    at::Half* C,
    int64_t ldc,
    at::Half* bgrad,
    void *lt_workspace,
    size_t workspaceSize);

template int gemm_bgradb_lt(
    hipblasOperation_t transa,
    hipblasOperation_t transb,
    int64_t m,
    int64_t n,
    int64_t k,
    float alpha,
    const at::BFloat16* A,
    int64_t lda,
    const at::BFloat16* B,
    int64_t ldb,
    at::BFloat16* C,
    int64_t ldc,
    at::BFloat16* bgrad,
    void *lt_workspace,
    size_t workspaceSize);

template <typename Dtype>
int gemm_dact_bgradb_lt(
    hipblasOperation_t transa,
    hipblasOperation_t transb,
    int64_t m,
    int64_t n,
    int64_t k,
    float alpha,
    const Dtype* A,
    int64_t lda,
    const Dtype* B,
    int64_t ldb,
    const void* pre_act,
    Dtype* C,
    int64_t ldc,
    Dtype* bgrad,
    bool is_gelu,
    int heuristic,
    void *lt_workspace,
    size_t workspaceSize) {
  static_assert(std::is_same<Dtype, at::Half>::value || std::is_same<Dtype, at::BFloat16>::value,
                "gemm_dact_bgradb_lt only supports fp16 and bf16");
  float beta = 0.0;
  hipblasComputeType_t abcType = std::is_same<Dtype, at::Half>::value ? HIP_R_16F : HIP_R_16BF;

  hipblasLtHandle_t ltHandle =
    reinterpret_cast<hipblasLtHandle_t>(at::cuda::getCurrentCUDABlasHandle());

  hipblasStatus_t status = HIPBLAS_STATUS_SUCCESS;

  hipblasLtMatmulDescOpaque_t operationDesc = {};
  hipblasLtMatrixLayoutOpaque_t Adesc = {}, Bdesc = {}, Cdesc = {};
  hipblasLtMatmulPreferenceOpaque_t preference = {};

  int returnedResults                             = 0;
  constexpr int requestedAlgoCount = 5;
  hipblasLtMatmulHeuristicResult_t heuristicResult[requestedAlgoCount] = {0};
  hipblasLtEpilogue_t epilogue = is_gelu ? HIPBLASLT_EPILOGUE_DGELU_BGRAD : HIPBLASLT_EPILOGUE_DGELU_BGRAD;

  // Create operation descriptor; see hipblasLtMatmulDescAttributes_t
  // for details about defaults; here we just set the transforms for
  // A and B.
  status = hipblasLtMatmulDescCreate(&operationDesc, HIPBLAS_COMPUTE_32F, HIP_R_32F);
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;
  status = hipblasLtMatmulDescSetAttribute(&operationDesc, HIPBLASLT_MATMUL_DESC_TRANSA, &transa, sizeof(transa));
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;
  status = hipblasLtMatmulDescSetAttribute(&operationDesc, HIPBLASLT_MATMUL_DESC_TRANSB, &transb, sizeof(transa));
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;

  status = hipblasLtMatmulDescSetAttribute(&operationDesc, HIPBLASLT_MATMUL_DESC_BIAS_POINTER, &bgrad, sizeof(bgrad));
  if (status != HIPBLAS_STATUS_SUCCESS) {
    goto CLEANUP;
  }
  status = hipblasLtMatmulDescSetAttribute(&operationDesc, HIPBLASLT_MATMUL_DESC_EPILOGUE_AUX_POINTER, &pre_act, sizeof(pre_act));
  if (status != HIPBLAS_STATUS_SUCCESS) {
    goto CLEANUP;
  }
  status = hipblasLtMatmulDescSetAttribute(&operationDesc, HIPBLASLT_MATMUL_DESC_EPILOGUE_AUX_LD, &ldc, sizeof(ldc));

  status = hipblasLtMatmulDescSetAttribute(&operationDesc, HIPBLASLT_MATMUL_DESC_EPILOGUE, &epilogue, sizeof(epilogue));
  if (status != HIPBLAS_STATUS_SUCCESS) {
    goto CLEANUP;
  }

  // Create matrix descriptors. Not setting any extra attributes.
  status = hipblasLtMatrixLayoutCreate(
    &Adesc, abcType, transa == HIPBLAS_OP_N ? m : k, transa == HIPBLAS_OP_N ? k : m, lda);
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;
  status = hipblasLtMatrixLayoutCreate(
    &Bdesc, abcType, transb == HIPBLAS_OP_N ? k : n, transb == HIPBLAS_OP_N ? n : k, ldb);
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;
  status = hipblasLtMatrixLayoutCreate(&Cdesc, abcType, m, n, ldc);
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;

  // Create preference handle; In general, extra attributes can be
  // used here to disable tensor ops or to make sure algo selected
  // will work with badly aligned A, B, C. However, for simplicity
  // here we assume A,B,C are always well aligned (e.g., directly
  // come from hipMalloc)
  status = hipblasLtMatmulPreferenceCreate(&preference);
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;
  status = hipblasLtMatmulPreferenceSetAttribute(
    &preference, HIPBLASLT_MATMUL_PREF_MAX_WORKSPACE_BYTES, &workspaceSize, sizeof(workspaceSize));
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;

  // We just need the best available heuristic to try and run matmul.
  // There is no guarantee that this will work. For example, if A is
  // badly aligned, you can request more (e.g. 32) algos and try to
  // run them one by one until something works.
  status = hipblasLtMatmulAlgoGetHeuristic(
    ltHandle, &operationDesc, &Adesc, &Bdesc, &Cdesc, &Cdesc, &preference, requestedAlgoCount, heuristicResult, &returnedResults);
  if (status != HIPBLAS_STATUS_SUCCESS) goto CLEANUP;

  if (returnedResults == 0) {
    status = HIPBLAS_STATUS_NOT_SUPPORTED;
    goto CLEANUP;
  }
  status = hipblasLtMatmul(ltHandle,
                          &operationDesc,
                          &alpha,
                          A,
                          &Adesc,
                          B,
                          &Bdesc,
                          &beta,
                          C,
                          &Cdesc,
                          C,
                          &Cdesc,
                          //&heuristicResult.algo,
                          &heuristicResult[heuristic].algo,
                          // NULL,
                          lt_workspace,
                          workspaceSize,
                          at::hip::getCurrentHIPStream());

CLEANUP:
  // Descriptors are no longer needed as all GPU work was already
  // enqueued.
  return status == HIPBLAS_STATUS_SUCCESS ? 0 : 1;
}

template int gemm_dact_bgradb_lt(
    hipblasOperation_t transa,
    hipblasOperation_t transb,
    int64_t m,
    int64_t n,
    int64_t k,
    float alpha,
    const at::Half* A,
    int64_t lda,
    const at::Half* B,
    int64_t ldb,
    const void* pre_act,
    at::Half* C,
    int64_t ldc,
    at::Half* bgrad,
    bool is_gelu,
    int heuristic,
    void *lt_workspace,
    size_t workspaceSize);

template int gemm_dact_bgradb_lt(
    hipblasOperation_t transa,
    hipblasOperation_t transb,
    int64_t m,
    int64_t n,
    int64_t k,
    float alpha,
    const at::BFloat16* A,
    int64_t lda,
    const at::BFloat16* B,
    int64_t ldb,
    const void* pre_act,
    at::BFloat16* C,
    int64_t ldc,
    at::BFloat16* bgrad,
    bool is_gelu,
    int heuristic,
    void *lt_workspace,
    size_t workspaceSize);

#endif

template <typename T>
int linear_bias_wgrad_hip(const T *input, const T *d_output, int64_t in_features, int64_t batch_size, int64_t out_features, T *d_weight, T *d_bias, void *lt_workspace, size_t workspaceSize) {
    const float alpha          = 1.0;
    const float beta_zero      = 0.0;
    int status = 1;
#if defined(HIPBLASLT_VERSION)
    status = gemm_bgradb_lt(
    // (hipblasLtHandle_t)handle,
    HIPBLAS_OP_N,
    HIPBLAS_OP_T,
    in_features,
    out_features,
    batch_size,
    alpha,
    input,
    in_features,
    d_output,
    out_features,
    d_weight,
    in_features,
    d_bias,
    lt_workspace,
    workspaceSize);
#endif

    if (status != 0){
        hipblasHandle_t handle = at::cuda::getCurrentCUDABlasHandle();
        status = gemm_bias(
          handle,
          HIPBLAS_OP_N,
          HIPBLAS_OP_T,
          in_features,
          out_features,
          batch_size,
          &alpha,
          input,
          in_features,
          d_output,
          out_features,
          &beta_zero,
          d_weight,
          in_features);
        // TD [2023-01-17]: I can't call Pytorch's gemm for now, due to linking error
        // https://discuss.pytorch.org/t/how-can-i-use-the-function-at-gemm-float/95341
        // at::cuda::blas::gemm<T>(
        //   'N',
        //   'T',
        //   in_features,
        //   out_features,
        //   batch_size,
        //   alpha,
        //   input,
        //   in_features,
        //   d_output,
        //   out_features,
        //   beta_zero,
        //   d_weight,
        //   in_features);
    }

    return status;
}

template <typename T>
int linear_act_forward_hip(const T *input, const T *weight, const T *bias, int64_t in_features, int64_t batch_size, int64_t out_features, bool is_gelu, int heuristic, T *output, void *pre_act, void *lt_workspace, size_t workspaceSize) {
    int status = 1;
#if defined(HIPBLASLT_VERSION)
    status = gemm_bias_act_lt(
    HIPBLAS_OP_T,
    HIPBLAS_OP_N,
    out_features,
    batch_size,
    in_features,
    /*alpha=*/1.0,
    weight,
    in_features,
    input,
    in_features,
    bias,
    output,
    out_features,
    pre_act,
    is_gelu,
    heuristic,
    lt_workspace,
    workspaceSize);
    return status;
#else
    return 1;
#endif
}

template <typename T>
int bias_act_linear_dgrad_bgrad_hip(const T *weight, const T *d_output, const void *pre_act, int64_t in_features, int64_t batch_size, int64_t out_features, bool is_gelu, int heuristic, T *d_input, T *d_bias, void *lt_workspace, size_t workspaceSize) {
    const float alpha          = 1.0;
    int status = 1;
#if defined(HIPBLASLT_VERSION)
    status = gemm_dact_bgradb_lt(
    HIPBLAS_OP_N,
    HIPBLAS_OP_N,
    in_features,
    batch_size,
    out_features,
    alpha,
    weight,
    in_features,
    d_output,
    out_features,
    pre_act,
    d_input,
    in_features,
    d_bias,
    is_gelu,
    heuristic,
    lt_workspace,
    workspaceSize);
#endif
    return status;

}

template int linear_bias_wgrad_hip<at::Half>(const at::Half *input, const at::Half *d_output, int64_t in_features, int64_t batch_size, int64_t out_features, at::Half *d_weight, at::Half *d_bias, void *lt_workspace, size_t workspaceSize);
template int linear_bias_wgrad_hip<at::BFloat16>(const at::BFloat16 *input, const at::BFloat16 *d_output, int64_t in_features, int64_t batch_size, int64_t out_features, at::BFloat16 *d_weight, at::BFloat16 *d_bias, void *lt_workspace, size_t workspaceSize);

template int linear_act_forward_hip<at::Half>(const at::Half *input, const at::Half *weight, const at::Half *bias, int64_t in_features, int64_t batch_size, int64_t out_features, bool is_gelu, int heuristic, at::Half *output, void *pre_act, void *lt_workspace, size_t workspaceSize);
template int linear_act_forward_hip<at::BFloat16>(const at::BFloat16 *input, const at::BFloat16 *weight, const at::BFloat16 *bias, int64_t in_features, int64_t batch_size, int64_t out_features, bool is_gelu, int heuristic, at::BFloat16 *output, void *pre_act, void *lt_workspace, size_t workspaceSize);

template int bias_act_linear_dgrad_bgrad_hip<at::Half>(const at::Half *weight, const at::Half *d_output, const void *pre_act, int64_t in_features, int64_t batch_size, int64_t out_features, bool is_gelu, int heuristic, at::Half *d_input, at::Half *d_bias, void *lt_workspace, size_t workspaceSize);
template int bias_act_linear_dgrad_bgrad_hip<at::BFloat16>(const at::BFloat16 *weight, const at::BFloat16 *d_output, const void *pre_act, int64_t in_features, int64_t batch_size, int64_t out_features, bool is_gelu, int heuristic, at::BFloat16 *d_input, at::BFloat16 *d_bias, void *lt_workspace, size_t workspaceSize);