#ifndef __ANPU_API_H__
#define __ANPU_API_H__

#include <stdint.h>

typedef enum {
    ANPU_DTYPE_INT8_V1 = 0,
    ANPU_DTYPE_INT16_V1 = 1,
    ANPU_DTYPE_INT32_V1 = 2,
} anpu_data_type_e;

typedef enum {
    ANPU_ACT_TYPE_NONE_V1 = 0,    // no activation function
    ANPU_ACT_TYPE_RELU_V1 = 1,    // ReLU activation function
    ANPU_ACT_TYPE_RELUN_V1 = 2,   // ReLU activation function, but the maximum value is set as relu_N
    ANPU_ACT_TYPE_PRELU_V1 = 3,   // PReLU activation function
    ANPU_ACT_TYPE_PRELUN_V1 = 4,  // PReLU activation function, but the maximum value is set as prelu_N
    ANPU_ACT_TYPE_SIGMOID_V1 = 5, // Sigmoid activation function
    ANPU_ACT_TYPE_TANH_V1 = 6,    // Tanh activation function
} anpu_act_type_e;

typedef struct {
    uint8_t          kernel_h;       // kernel height
    uint8_t          kernel_w;       // kernel width
    uint8_t          dilation_h;     // dilation rate along the height dimension
    uint8_t          dilation_w;     // dilation rate along the width dimension
    uint8_t          stride_h;       // stride along the height dimension
    uint8_t          stride_w;       // stride along the width dimension
    anpu_data_type_e weight_dtype;   // weight data type; only INT8 or INT16 is supported
    uint32_t         groups;         // number of convolution groups

    uint16_t         input_h;        // input height
    uint16_t         input_w;        // input width
    uint16_t         input_channel;  // channels of input
    anpu_data_type_e input_dtype;    // input data type; only INT8 or INT16 is supported
    uint8_t          in_pad_up;      // number of padding rows added to the top of the input
    uint8_t          in_pad_down;    // number of padding rows added to the bottom of the input
    uint8_t          in_pad_left;    // number of padding columns added to the left of the input
    uint8_t          in_pad_right;   // number of padding columns added to the right of the input
    int16_t          in_pad_val;     // value used for input padding

    uint16_t         output_channel; // channels of output
    anpu_data_type_e output_dtype;   // output data type; supports INT8, INT16, or INT32
    uint8_t          out_pad_up;     // number of padding rows added to the top of the output
    uint8_t          out_pad_down;   // number of padding rows added to the bottom of the output
    uint8_t          out_pad_left;   // number of padding columns added to the left of the output
    uint8_t          out_pad_right;  // number of padding columns added to the right of the output

    uint8_t          bias_en;        // whether bias is enabled: 0 means disabled
    anpu_data_type_e bias_dtype;     // bias data type; supports INT8, INT16, or INT32

    anpu_act_type_e  act_type;       // activation type: none/relu/relun/prelu/prelun/sigmoid/tanh
    int32_t          relu_N;         // upper bound used by RELUN activation
    int16_t          prelu_N;        // upper bound used by PRELUN activation

    float            prelu_weight;   // coefficient used by PRELU or PRELUN activation

    float            input_scale;    // quantization scale of input
    float            weight_scale;   // quantization scale of weight
    float            output_scale;   // quantization scale of output
} anpu_conv2d_layer_param_t;

typedef struct {
    uint16_t         input_channel;  // channels of input
    uint16_t         output_channel; // channels of output

    anpu_data_type_e input_dtype;    // input data type; only INT8 or INT16 is supported
    anpu_data_type_e weight_dtype;   // weight data type; only INT8 or INT16 is supported
    anpu_data_type_e output_dtype;   // output data type; supports INT8, INT16, or INT32

    uint8_t          bias_en;        // whether bias is enabled: 0 means disabled
    anpu_data_type_e bias_dtype;     // bias data type; supports INT8, INT16, or INT32

    anpu_act_type_e  act_type;       // activation type: none/relu/relun/prelu/prelun/sigmoid/tanh
    int32_t          relu_N;         // upper bound used by RELUN activation
    int16_t          prelu_N;        // upper bound used by PRELUN activation

    float            prelu_weight;   // coefficient used by PRELU or PRELUN activation

    float            input_scale;    // quantization scale of input
    float            weight_scale;   // quantization scale of weight
    float            output_scale;   // quantization scale of output
} anpu_fc_layer_param_t;

typedef struct {
    uint16_t         input_count;    // total number of input elements; valid range: [1, 16383]
    anpu_data_type_e input_dtype;    // input data type; supports INT8, INT16, or INT32
    anpu_data_type_e output_dtype;   // output data type; supports INT8, INT16, or INT32

    anpu_act_type_e  act_type;       // activation type: none/prelu/prelun/sigmoid/tanh
    int32_t          prelu_N;        // upper bound used by PRELUN activation

    float            prelu_weight;   // coefficient used by PRELU or PRELUN activation

    float            input0_scale;   // quantization scale of input0
    float            input1_scale;   // quantization scale of input1
    float            output_scale;   // quantization scale of output
} anpu_elemwise_add_layer_param_t;

typedef struct {
    uint16_t         input_count;    // total number of input elements; valid range: [1, 16383]
    anpu_data_type_e input_dtype;    // input data type; only INT8 or INT16 is supported
    anpu_data_type_e output_dtype;   // output data type; supports INT8, INT16, or INT32

    anpu_act_type_e  act_type;       // activation type: none/prelu/prelun/sigmoid/tanh
    int32_t          prelu_N;        // upper bound used by PRELUN activation

    float            prelu_weight;   // coefficient used by PRELU or PRELUN activation

    float            input0_scale;   // quantization scale of input0
    float            input1_scale;   // quantization scale of input1
    float            output_scale;   // quantization scale of output
} anpu_elemwise_mul_layer_param_t;

typedef struct {
    uint16_t         input_count;    // total number of input elements; valid range: [1, 16383]
    anpu_data_type_e mul_data_dtype; // mul data type; only INT8 or INT16 is supported
    anpu_data_type_e add_data_dtype; // add data type; supports INT8, INT16, or INT32
    anpu_data_type_e output_dtype;   // output data type; supports INT8, INT16, or INT32

    anpu_act_type_e  act_type;       // activation type: none/prelu/prelun/sigmoid/tanh
    int32_t          prelu_N;        // upper bound used by PRELUN activation

    float            prelu_weight;   // coefficient used by PRELU or PRELUN activation

    float            mul_data0_scale; // quantization scale of mul_data0
    float            mul_data1_scale; // quantization scale of mul_data1
    float            output_scale;    // quantization scale of output
} anpu_elemwise_mac_layer_param_t;

typedef struct {
    uint16_t         input_count;     // total number of input elements; valid range: [1, 16383]
    anpu_data_type_e input_dtype;     // input data type; supports INT8, INT16, or INT32

    float            input_scale;     // quantization scale of input
} anpu_elemwise_one_minus_layer_param_t;

typedef struct {
    uint16_t         input_h;         // height of the original input
    uint16_t         input_w;         // width of the original input
    uint16_t         input_channel;   // number of channels in the original input
    anpu_data_type_e input_dtype;     // input data type; only INT8 or INT16 is supported

    uint8_t          kernel_h;        // height of the transposed-convolution kernel
    uint8_t          kernel_w;        // width of the transposed-convolution kernel
    uint8_t          dilation_h;      // kernel dilation rate along the height dimension
    uint8_t          dilation_w;      // kernel dilation rate along the width dimension
    uint8_t          stride_h;        // upsampling factor along the height dimension
    uint8_t          stride_w;        // upsampling factor along the width dimension

    uint8_t          padding_h;       // symmetric padding used by ConvTranspose2D along the height dimension
    uint8_t          padding_w;       // symmetric padding used by ConvTranspose2D along the width dimension
    uint8_t          output_padding_h; // additional output size along height; must be smaller than max(stride_h, dilation_h)
    uint8_t          output_padding_w; // additional output size along width; must be smaller than max(stride_w, dilation_w)
} anpu_upsample_param_t;

typedef struct {
    uint16_t         input_size;     // size of the input
    uint16_t         hidden_size;    // size of the hidden

    anpu_data_type_e input_dtype;    // input data type; only INT8 or INT16 is supported
    anpu_data_type_e weight_dtype;   // weight data type; only INT8 or INT16 is supported

    uint8_t          bias_en;        // whether bias is enabled: 0 means disabled
    anpu_data_type_e bias_dtype;     // bias data type; supports INT8, INT16, or INT32

    float            input_scale;       // quantization scale of input
    float            weight_ih_scale;   // quantization scale of weight_ih
    float            weight_hh_scale;   // quantization scale of weight_hh
} anpu_lstm_layer_param_t;

typedef struct {
    uint16_t         batch_size;     // size of the batch; must be at least 4, seq_len = 1
    uint16_t         input_size;     // size of the input
    uint16_t         hidden_size;    // size of the hidden

    anpu_data_type_e input_dtype;    // input data type; only INT8 or INT16 is supported
    anpu_data_type_e weight_dtype;   // weight data type; only INT8 or INT16 is supported

    uint8_t          bias_en;        // whether bias is enabled: 0 means disabled
    anpu_data_type_e bias_dtype;     // bias data type; supports INT8, INT16, or INT32

    float            input_scale;    // quantization scale of input
    float            weight_ih_scale;   // quantization scale of weight_ih
    float            weight_hh_scale;   // quantization scale of weight_hh
} anpu_batch_lstm_layer_param_t;

typedef struct {
    uint16_t         batch_size;     // number of sequence steps; must be at least 4, batch = 1
    uint16_t         input_size;     // size of the input
    uint16_t         hidden_size;    // size of the hidden

    anpu_data_type_e input_dtype;    // input data type; only INT8 or INT16 is supported
    anpu_data_type_e weight_dtype;   // weight data type; only INT8 or INT16 is supported

    uint8_t          bias_en;        // whether bias is enabled: 0 means disabled
    anpu_data_type_e bias_dtype;     // bias data type; supports INT8, INT16, or INT32

    float            input_scale;    // quantization scale of input
    float            forward_weight_ih_scale;  // quantization scale of forward weight_ih
    float            forward_weight_hh_scale;  // quantization scale of forward weight_hh
    float            reverse_weight_ih_scale;  // quantization scale of reverse weight_ih
    float            reverse_weight_hh_scale;  // quantization scale of reverse weight_hh
} anpu_intra_bidirectional_batch_lstm_layer_param_t;

typedef struct {
    uint16_t         input_size;     // size of the input
    uint16_t         hidden_size;    // size of the hidden

    anpu_data_type_e input_dtype;    // input data type; only INT8 or INT16 is supported
    anpu_data_type_e weight_dtype;   // weight data type; only INT8 or INT16 is supported

    uint8_t          bias_en;        // whether bias is enabled: 0 means disabled
    anpu_data_type_e bias_dtype;     // bias data type; supports INT8, INT16, or INT32

    float            input_scale;    // quantization scale of input
    float            weight_ih_scale;   // quantization scale of weight_ih
    float            weight_hh_scale;   // quantization scale of weight_hh
} anpu_gru_layer_param_t;

typedef struct {
    uint16_t         batch_size;    // size of the batch; must be at least 4, seq_len = 1
    uint16_t         input_size;    // size of the input
    uint16_t         hidden_size;   // size of the hidden

    anpu_data_type_e input_dtype;   // input data type; only INT8 or INT16 is supported
    anpu_data_type_e weight_dtype;  // weight data type; only INT8 or INT16 is supported

    uint8_t          bias_en;       // whether bias is enabled: 0 means disabled
    anpu_data_type_e bias_dtype;    // bias data type; supports INT8, INT16, or INT32

    float            input_scale;   // quantization scale of input
    float            weight_ih_scale;   // quantization scale of weight_ih
    float            weight_hh_scale;   // quantization scale of weight_hh
} anpu_batch_gru_layer_param_t;

typedef struct {
    uint16_t         batch_size;     // number of sequence steps; must be at least 4, batch = 1
    uint16_t         input_size;     // size of the input
    uint16_t         hidden_size;    // size of the hidden

    anpu_data_type_e input_dtype;    // input data type; only INT8 or INT16 is supported
    anpu_data_type_e weight_dtype;   // weight data type; only INT8 or INT16 is supported

    uint8_t          bias_en;        // whether bias is enabled: 0 means disabled
    anpu_data_type_e bias_dtype;     // bias data type; supports INT8, INT16, or INT32

    float            input_scale;    // quantization scale of input
    float            forward_weight_ih_scale;  // quantization scale of forward weight_ih
    float            forward_weight_hh_scale;  // quantization scale of forward weight_hh
    float            reverse_weight_ih_scale;  // quantization scale of reverse weight_ih
    float            reverse_weight_hh_scale;  // quantization scale of reverse weight_hh
} anpu_intra_bidirectional_batch_gru_layer_param_t;

typedef struct {
    uint16_t        input_count;    // total number of input elements; must be a nonzero multiple of 32
    float           input_scale;    // quantization scale of input
    float           epsilon;        // numerical-stability epsilon in real-value domain
    float           output_scale;   // quantization scale of output
} anpu_layernorm_layer_param_t;

typedef struct {
    uint16_t         input_count;   // total number of input elements; must be a nonzero multiple of 32
    uint8_t          gamma_en;      // whether gamma is enabled: 0 means disabled, 1 means enabled
    anpu_data_type_e gamma_dtype;   // gamma data type when enabled; only INT8 or INT16 is supported
    uint8_t          beta_en;       // whether beta is enabled: 0 means disabled, 1 means enabled; requires gamma_en == 1
    anpu_data_type_e beta_dtype;    // beta data type when enabled; must be identical to gamma_dtype
    float            input_scale;   // quantization scale of input
    float            epsilon;       // numerical-stability epsilon in real-value domain
    float            gamma_scale;   // shared scale of enabled gamma/beta
    float            output_scale;  // quantization scale of output
} anpu_layernorm_i_layer_param_t;


/**
 * @brief Initialize the ANPU runtime.
 * This function must be called before using any ANPU operator functions.
 */
void anpu_api_init(void);

/**
 * @brief Destroy the ANPU runtime and release global resources.
 * All initialized ANPU layers should be destroyed before calling this function.
 */
void anpu_api_destroy(void);


/**
 * @brief Allocate a memory buffer for ANPU data.
 * @param size Size of the buffer in bytes.
 * @return Pointer to the allocated buffer, or NULL if allocation fails.
 */
void *anpu_malloc(uint32_t size);

/**
 * @brief Copy data between memory buffers.
 * @param dst  Destination buffer address.
 * @param src  Source buffer address.
 * @param size Number of bytes to copy.
 */
void anpu_memcpy(void* dst, const void* src, uint32_t size);

/**
 * @brief Free a memory buffer allocated by anpu_malloc().
 * @param buf Pointer to the buffer to be freed.
 */
void anpu_free(void* buf);


/**
 * @brief Initialize a Conv2D layer.
 * @param conv2d_layer_param Pointer to the Conv2D layer parameters.
 * @return Conv2D layer handle, or NULL if initialization fails.
 * @note When output_dtype is INT32:
 *       - Only NONE, RELU, and RELUN activation functions are supported.
 *       - Output quantization is not performed, input_scale, weight_scale, and output_scale are ignored.
 * @note When output_dtype is INT16 or INT8:
 *       - NONE, RELU, RELUN, PRELU, PRELUN, SIGMOID, and TANH activation functions are supported.
 *       - The convolution result is requantized using input_scale, weight_scale, and output_scale, the effective requantization scale is: input_scale * weight_scale / output_scale.
 * @warning The calculated output width must be greater than or equal to 4.
 */
void *anpu_conv2d_layer_init(const anpu_conv2d_layer_param_t* conv2d_layer_param);

/**
 * @brief Execute forward inference for a Conv2D layer.
 *
 * @param m          Conv2D layer handle returned by anpu_conv2d_layer_init().
 * @param input_adr  Address of the input.
 * @param weight_adr Address of the convolution weights.
 * @param bias_adr   Address of the bias data. May be NULL when bias is disabled.
 * @param output_adr Address of the output.
 */
void anpu_conv2d_layer_forward(void* m, void* input_adr, void* weight_adr, void* bias_adr, void* output_adr);

/**
 * @brief Execute Conv2D forward inference using a circular input buffer.
 *
 * This function performs the same Conv2D computation as anpu_conv2d_layer_forward(), but supports an input feature map stored in a circular buffer along the height dimension.
 *
 * stream_h specifies the physical row index in the input buffer that should be treated as the first logical row of the input feature map. This allows the convolution to start
 * from the correct row without moving or rearranging the data in the buffer.
 *
 * For example, when input_h is 4:
 *
 *     stream_h = 0: logical row order is h0, h1, h2, h3,
 *                   physically stored as h0, h1, h2, h3
 *     stream_h = 1: logical row order is h0, h1, h2, h3,
 *                   physically stored as h3, h0, h1, h2
 *     stream_h = 2: logical row order is h0, h1, h2, h3,
 *                   physically stored as h2, h3, h0, h1
 *
 * @param m          Conv2D layer handle returned by anpu_conv2d_layer_init().
 * @param input_adr  Address of the circular input.
 * @param weight_adr Address of the convolution weights.
 * @param bias_adr   Address of the bias data; may be NULL when bias is disabled.
 * @param output_adr Address of the output.
 * @param stream_h   Physical row index of the first logical input row. The valid range is [0, input_h - 1].
 */
void anpu_conv2d_streamh_layer_forward(void* m, void* input_adr, void* weight_adr, void* bias_adr, void* output_adr, int stream_h);

/**
 * @brief Destroy a Conv2D layer and release its resources.
 *
 * @param m Conv2D layer handle returned by anpu_conv2d_layer_init().
 */
void anpu_conv2d_layer_destroy(void* m);


/**
 * @brief Initialize a fully connected layer.
 * @param fc_layer_param Pointer to the fully connected layer parameters.
 * @return Fully connected layer handle, or NULL if initialization fails.
 * @note When output_dtype is INT32:
 *       - Only NONE, RELU, and RELUN activation functions are supported.
 *       - Output quantization is not performed, input_scale, weight_scale, and output_scale are ignored.
 * @note When output_dtype is INT16 or INT8:
 *       - NONE, RELU, RELUN, PRELU, PRELUN, SIGMOID, and TANH activation functions are supported.
 *       - The fully connected result is requantized using input_scale, weight_scale, and output_scale, the effective requantization scale is: input_scale * weight_scale / output_scale.
 */
void *anpu_fc_layer_init(const anpu_fc_layer_param_t* fc_layer_param);

/**
 * @brief Execute forward inference for a fully connected layer.
 *
 * @param m          Fully connected layer handle returned by anpu_fc_layer_init().
 * @param input_adr  Address of the input.
 * @param weight_adr Address of the convolution weights.
 * @param bias_adr   Address of the bias data. May be NULL when bias is disabled.
 * @param output_adr Address of the output.
 */
void anpu_fc_layer_forward(void* m, void* input_adr, void* weight_adr, void* bias_adr, void* output_adr);

/**
 * @brief Destroy a fully connected layer and release its resources.
 * @param m Fully connected layer handle returned by anpu_fc_layer_init().
 */
void anpu_fc_layer_destroy(void* m);


/**
 * @brief Initialize an element-wise addition layer.
 * @param elemwise_add_layer_param Pointer to the element-wise addition parameters.
 * @return Addition layer handle, or NULL if initialization fails.
 * @note When output_dtype is INT32:
 *       - Only the NONE activation function is supported.
 *       - output_scale must be equal to either input0_scale or input1_scale.
 *       - The input whose scale differs from output_scale is internally requantized to output_scale before addition.
 *       - The input whose scale is equal to output_scale is used directly.
 * @note When output_dtype is INT16 or INT8:
 *       - NONE, PRELU, PRELUN, SIGMOID, and TANH activation functions are supported.
 *       - The addition result is requantized to output_scale.
 * @warning For INT32 output, initialization fails if output_scale is different from both input0_scale and input1_scale.
 */
void *anpu_elemwise_add_layer_init(const anpu_elemwise_add_layer_param_t* elemwise_add_layer_param);

/**
 * @brief Execute element-wise addition: output = input0 + input1
 * @param m           Addition layer handle returned by anpu_elemwise_add_layer_init().
 * @param input0_adr  Address of the first input.
 * @param input1_adr  Address of the second input.
 * @param output_adr  Address of the output.
 */
void anpu_elemwise_add_layer_forward(void* m, void* input0_adr, void* input1_adr, void* output_adr);

/**
 * @brief Destroy an element-wise addition layer and release its resources.
 * @param m Addition layer handle returned by anpu_elemwise_add_layer_init().
 */
void anpu_elemwise_add_layer_destroy(void* m);


/**
 * @brief Initialize an element-wise multiplication layer.
 * @param elemwise_mul_layer_param Pointer to the element-wise multiplication
 *                        parameters.
 * @return Multiplication layer handle, or NULL if initialization fails.
 * @note When output_dtype is INT32:
 *       - Only the NONE activation function is supported.
 *       - Output quantization is not performed, input0_scale, input1_scale, and output_scale are ignored.
 * @note When output_dtype is INT16 or INT8:
 *       - NONE, PRELU, PRELUN, SIGMOID, and TANH activation functions are supported.
 *       - The multiplication result is requantized using input0_scale, input1_scale, and output_scale, the effective requantization scale is: input0_scale * input1_scale / output_scale.
 */
void *anpu_elemwise_mul_layer_init(const anpu_elemwise_mul_layer_param_t* elemwise_mul_layer_param);

/**
 * @brief Execute element-wise multiplication: output = input0 * input1
 * @param m           Multiplication layer handle returned by anpu_elemwise_mul_layer_init().
 * @param input0_adr  Address of the first input.
 * @param input1_adr  Address of the second input.
 * @param output_adr  Address of the output .
 */
void anpu_elemwise_mul_layer_forward(void* m, void* input0_adr, void* input1_adr, void* output_adr);

/**
 * @brief Destroy an element-wise multiplication layer and release its resources.
 * @param m Multiplication layer handle returned by anpu_elemwise_mul_layer_init().
 */
void anpu_elemwise_mul_layer_destroy(void* m);


/**
 * @brief Initialize an element-wise multiply-accumulate layer.
 * @param elemwise_mac_layer_param Pointer to the multiply-accumulate layer parameters.
 * @return Multiply-accumulate layer handle, or NULL if initialization fails.
 * @note add_data is interpreted using output_scale.
 * @note When output_dtype is INT32:
 *       - Only the NONE activation function is supported.
 *       - Output quantization is not performed, mul_data0_scale, mul_data1_scale, and output_scale are ignored.
 * @note When output_dtype is INT16 or INT8:
 *       - NONE, PRELU, PRELUN, SIGMOID, and TANH activation functions are supported.
 *       - The multiply-accumulate result is requantized using mul_data0_scale, mul_data1_scale, and output_scale, the effective requantization scale is: mul_data0_scale * mul_data1_scale / output_scale.
 */
void *anpu_elemwise_mac_layer_init(const anpu_elemwise_mac_layer_param_t* elemwise_mac_layer_param);

/**
 * @brief Execute an element-wise multiply-accumulate operation: output = mul_data0 * mul_data1 + add_data
 * @param m              Multiply-accumulate layer handle returned by anpu_elemwise_mac_layer_init().
 * @param mul_data0_adr  Address of the first multiplication input.
 * @param mul_data1_adr  Address of the second multiplication input.
 * @param add_data_adr   Address of the addition input.
 * @param output_adr     Address of the output.
 */
void anpu_elemwise_mac_layer_forward(void* m, void* mul_data0_adr, void* mul_data1_adr, void* add_data_adr, void* output_adr);

/**
 * @brief Destroy a multiply-accumulate layer and release its resources.
 * @param m Multiply-accumulate layer handle returned by anpu_elemwise_mac_layer_init().
 */
void anpu_elemwise_mac_layer_destroy(void* m);


/**
 * @brief Initialize a one-minus layer.
 * @param elemwise_one_minus_layer_param Pointer to the one-minus layer parameters.
 * @return One-minus layer handle, or NULL if initialization fails.
 * @note The numerical representation of 1 is determined by input_dtype and input_scale.
 */
void *anpu_elemwise_one_minus_layer_init(const anpu_elemwise_one_minus_layer_param_t* elemwise_one_minus_layer_param);

/**
 * @brief Execute the element-wise one-minus operation: output = 1 - input
 * @param m          One-minus layer handle returned by anpu_elemwise_one_minus_layer_init().
 * @param input_adr  Address of the input.
 * @param output_adr Address of the output.
 */
void anpu_elemwise_one_minus_layer_forward(void* m, void* input_adr, void* output_adr);

/**
 * @brief Destroy a one-minus layer and release its resources.
 * @param m One-minus layer handle returned by anpu_elemwise_one_minus_layer_init().
 */
void anpu_elemwise_one_minus_layer_destroy(void* m);


/**
 * @brief Convert ConvTranspose2D input data into an upsampled tensor.
 * The function inserts zero-valued elements between input elements according to stride_h and stride_w, and adjusts the tensor boundary according to kernel size, dilation, padding, and output padding.
 * The generated tensor can be used as the input of an equivalent Conv2D operation.
 * @param upsample_param Pointer to the ConvTranspose2D upsampling parameters.
 * @param in             Address of the original input.
 * @param out            Address of the preallocated upsampled output.
 * @note The input and output tensors use HWC layout.
 * @note It is necessary to allocate sufficient memory for "out".
 */
void anpu_upsample_tensor(const anpu_upsample_param_t* upsample_param, const void* in, void* out);


/**
 * @brief Initialize a single-step LSTM layer.
 * @param lstm_layer_param Pointer to the LSTM layer parameters.
 * @return LSTM layer handle, or NULL if initialization fails.
 */
void *anpu_lstm_layer_init(const anpu_lstm_layer_param_t* lstm_layer_param);

/**
 * @brief Execute forward inference for a single-step LSTM layer.
 * @param m             LSTM layer handle returned by anpu_lstm_layer_init().
 * @param input_adr     Address of the input vector.
 * @param weight_ih_adr Address of the input-to-hidden weights.
 * @param bias_ih_adr   Address of the input-to-hidden bias; may be NULL when bias is disabled.
 * @param h_in_adr      Address of the input hidden state.
 * @param weight_hh_adr Address of the hidden-to-hidden weights.
 * @param bias_hh_adr   Address of the hidden-to-hidden bias; may be NULL when bias is disabled.
 * @param c_in_adr      Address of the input cell state.
 * @param c_out_adr     Address of the output cell state.
 * @param h_out_adr     Address of the output hidden state.
 * @param temp_adr      Address of temporary storage.
 * @note temp_adr requires 14 * hidden_size * sizeof(int16_t) bytes.
 */
void anpu_lstm_layer_forward(void* m, void* input_adr, void* weight_ih_adr, void* bias_ih_adr, void* h_in_adr,
                             void *weight_hh_adr, void *bias_hh_adr, void *c_in_adr, void *c_out_adr, void *h_out_adr, void *temp_adr);

/**
 * @brief Destroy an LSTM layer and release its resources.
 * @param m LSTM layer handle returned by anpu_lstm_layer_init().
 */
void anpu_lstm_layer_destroy(void* m);


/**
 * @brief Initialize a batched single-step LSTM layer.
 * @param batch_lstm_layer_param Pointer to the batched LSTM parameters.
 * @return Batched LSTM layer handle, or NULL if initialization fails.
 * @note batch_size must be greater than or equal to 4.
 */
void *anpu_batch_lstm_layer_init(const anpu_batch_lstm_layer_param_t* batch_lstm_layer_param);

/**
 * @brief Execute forward inference for a batched single-step LSTM layer.
 * @param m             Batched LSTM handle returned by anpu_batch_lstm_layer_init().
 * @param input_adr     Address of the batched input.
 * @param weight_ih_adr Address of the input-to-hidden weights.
 * @param bias_ih_adr   Address of the input-to-hidden bias; may be NULL when bias is disabled.
 * @param h_in_adr      Address of the input hidden states.
 * @param weight_hh_adr Address of the hidden-to-hidden weights.
 * @param bias_hh_adr   Address of the hidden-to-hidden bias; may be NULL when bias is disabled.
 * @param c_in_adr      Address of the input cell states.
 * @param c_out_adr     Address of the output cell states.
 * @param h_out_adr     Address of the output hidden states.
 * @param temp_adr      Address of temporary storage.
 * @note temp_adr requires (12 * batch_size + 2) * hidden_size * sizeof(int16_t) bytes.
 */
void anpu_batch_lstm_layer_forward(void* m, void* input_adr, void* weight_ih_adr, void* bias_ih_adr, void* h_in_adr,
                                   void *weight_hh_adr, void *bias_hh_adr, void *c_in_adr, void *c_out_adr, void *h_out_adr, void *temp_adr);

/**
 * @brief Destroy a batched LSTM layer and release its resources.
 * @param m Batched LSTM handle returned by anpu_batch_lstm_layer_init().
 */
void anpu_batch_lstm_layer_destroy(void* m);


/**
 * @brief Initialize an intra-sequence bidirectional LSTM layer.
 * @param intra_bidirectional_batch_lstm_layer_param Pointer to the bidirectional LSTM parameters.
 * @return Bidirectional LSTM layer handle, or NULL if initialization fails.
 * @note batch_size represents the number of sequence steps and must be greater than or equal to 4; the actual batch size is 1.
 */
void *anpu_intra_bidirectional_batch_lstm_layer_init(const anpu_intra_bidirectional_batch_lstm_layer_param_t* intra_bidirectional_batch_lstm_layer_param);

/**
 * @brief Execute forward inference for an intra-sequence bidirectional LSTM layer.
 * @param m                     Handle returned by anpu_intra_bidirectional_batch_lstm_layer_init().
 * @param input_adr             Address of the input sequence.
 * @param forward_weight_ih_adr Address of the forward input-to-hidden weights.
 * @param forward_bias_ih_adr   Address of the forward input-to-hidden bias; may be NULL when bias is disabled.
 * @param forward_weight_hh_adr Address of the forward hidden-to-hidden weights.
 * @param forward_bias_hh_adr   Address of the forward hidden-to-hidden bias; may be NULL when bias is disabled.
 * @param reverse_weight_ih_adr Address of the reverse input-to-hidden weights.
 * @param reverse_bias_ih_adr   Address of the reverse input-to-hidden bias; may be NULL when bias is disabled.
 * @param reverse_weight_hh_adr Address of the reverse hidden-to-hidden weights.
 * @param reverse_bias_hh_adr   Address of the reverse hidden-to-hidden bias; may be NULL when bias is disabled.
 * @param h_adr                 Address of the output hidden-state buffer.
 * @param c_adr                 Address of the forward and reverse cell-state buffer.
 * @param temp_adr              Address of temporary storage.
 * @note h_adr requires batch_size * 2 * hidden_size * sizeof(int16_t) bytes and stores [forward, reverse] for every step.
 * @note c_adr requires 2 * hidden_size * sizeof(int16_t) bytes.
 * @note The initial forward/reverse hidden and cell states must be zero.
 * @note temp_adr requires (4 * batch_size + 10) * hidden_size * sizeof(int16_t) bytes.
 */
void anpu_intra_bidirectional_batch_lstm_layer_forward(void* m, void* input_adr, void* forward_weight_ih_adr, void* forward_bias_ih_adr,
        void *forward_weight_hh_adr, void *forward_bias_hh_adr, void *reverse_weight_ih_adr, void *reverse_bias_ih_adr, void *reverse_weight_hh_adr,
        void *reverse_bias_hh_adr, void *h_adr, void *c_adr, void *temp_adr);

/**
 * @brief Destroy an intra-sequence bidirectional LSTM layer and release its resources.
 * @param m Handle returned by anpu_intra_bidirectional_batch_lstm_layer_init().
 */
void anpu_intra_bidirectional_batch_lstm_layer_destroy(void* m);


/**
 * @brief Initialize a single-step GRU layer.
 * @param gru_layer_param Pointer to the GRU layer parameters.
 * @return GRU layer handle, or NULL if initialization fails.
 */
void *anpu_gru_layer_init(const anpu_gru_layer_param_t* gru_layer_param);

/**
 * @brief Execute forward inference for a single-step GRU layer.
 * @param m             GRU layer handle returned by anpu_gru_layer_init().
 * @param input_adr     Address of the input vector.
 * @param weight_ih_adr Address of the input-to-hidden weights.
 * @param bias_ih_adr   Address of the input-to-hidden bias; may be NULL when bias is disabled.
 * @param h_in_adr      Address of the input hidden state.
 * @param weight_hh_adr Address of the hidden-to-hidden weights.
 * @param bias_hh_adr   Address of the hidden-to-hidden bias; may be NULL when bias is disabled.
 * @param h_out_adr     Address of the output hidden state.
 * @param temp_adr      Address of temporary storage.
 * @note temp_adr requires 11 * hidden_size * sizeof(int16_t) bytes.
 */
void anpu_gru_layer_forward(void* m, void* input_adr, void* weight_ih_adr, void* bias_ih_adr, void* h_in_adr,
                            void *weight_hh_adr, void *bias_hh_adr, void *h_out_adr, void *temp_adr);

/**
 * @brief Destroy a GRU layer and release its resources.
 * @param m GRU layer handle returned by anpu_gru_layer_init().
 */
void anpu_gru_layer_destroy(void* m);

/**
 * @brief Initialize a batched single-step GRU layer.
 * @param batch_gru_layer_param Pointer to the batched GRU parameters.
 * @return Batched GRU layer handle, or NULL if initialization fails.
 * @note batch_size must be greater than or equal to 4.
 */
void *anpu_batch_gru_layer_init(const anpu_batch_gru_layer_param_t* batch_gru_layer_param);

/**
 * @brief Execute forward inference for a batched single-step GRU layer.
 * @param m             Batched GRU handle returned by anpu_batch_gru_layer_init().
 * @param input_adr     Address of the batched input.
 * @param weight_ih_adr Address of the input-to-hidden weights.
 * @param bias_ih_adr   Address of the input-to-hidden bias; may be NULL when bias is disabled.
 * @param h_in_adr      Address of the input hidden states.
 * @param weight_hh_adr Address of the hidden-to-hidden weights.
 * @param bias_hh_adr   Address of the hidden-to-hidden bias; may be NULL when bias is disabled.
 * @param h_out_adr     Address of the output hidden states.
 * @param temp_adr      Address of temporary storage.
 * @note temp_adr requires (6 * batch_size + 5) * hidden_size * sizeof(int16_t) bytes.
 */
void anpu_batch_gru_layer_forward(void* m, void* input_adr, void* weight_ih_adr, void* bias_ih_adr, void* h_in_adr,
                                  void *weight_hh_adr, void *bias_hh_adr, void *h_out_adr, void *temp_adr);

/**
 * @brief Destroy a batched GRU layer and release its resources.
 * @param m Batched GRU handle returned by anpu_batch_gru_layer_init().
 */
void anpu_batch_gru_layer_destroy(void* m);


/**
 * @brief Initialize an intra-sequence bidirectional GRU layer.
 * @param intra_bidirectional_batch_gru_layer_param Pointer to the bidirectional GRU parameters.
 * @return Bidirectional GRU layer handle, or NULL if initialization fails.
 * @note batch_size represents the number of sequence steps and must be greater than or equal to 4; the actual batch size is 1.
 */
void *anpu_intra_bidirectional_batch_gru_layer_init(const anpu_intra_bidirectional_batch_gru_layer_param_t* intra_bidirectional_batch_gru_layer_param);

/**
 * @brief Execute forward inference for an intra-sequence bidirectional GRU layer.
 * @param m                     Handle returned by anpu_intra_bidirectional_batch_gru_layer_init().
 * @param input_adr             Address of the input sequence.
 * @param forward_weight_ih_adr Address of the forward input-to-hidden weights.
 * @param forward_bias_ih_adr   Address of the forward input-to-hidden bias; may be NULL when bias is disabled.
 * @param forward_weight_hh_adr Address of the forward hidden-to-hidden weights.
 * @param forward_bias_hh_adr   Address of the forward hidden-to-hidden bias; may be NULL when bias is disabled.
 * @param reverse_weight_ih_adr Address of the reverse input-to-hidden weights.
 * @param reverse_bias_ih_adr   Address of the reverse input-to-hidden bias; may be NULL when bias is disabled.
 * @param reverse_weight_hh_adr Address of the reverse hidden-to-hidden weights.
 * @param reverse_bias_hh_adr   Address of the reverse hidden-to-hidden bias; may be NULL when bias is disabled.
 * @param h_adr                 Address of the output hidden-state buffer.
 * @param temp_adr              Address of temporary storage.
 * @note h_adr requires batch_size * 2 * hidden_size * sizeof(int16_t) bytes and stores [forward, reverse] for every step.
 * @note The initial forward and reverse hidden states must be zero.
 * @note temp_adr requires (3 * batch_size + 8) * hidden_size * sizeof(int16_t) bytes.
 */
void anpu_intra_bidirectional_batch_gru_layer_forward(void* m, void* input_adr, void* forward_weight_ih_adr, void* forward_bias_ih_adr,
        void *forward_weight_hh_adr, void *forward_bias_hh_adr, void *reverse_weight_ih_adr, void *reverse_bias_ih_adr, void *reverse_weight_hh_adr,
        void *reverse_bias_hh_adr, void *h_adr, void *temp_adr);

/**
 * @brief Destroy an intra-sequence bidirectional GRU layer and release its resources.
 * @param m Handle returned by anpu_intra_bidirectional_batch_gru_layer_init().
 */
void anpu_intra_bidirectional_batch_gru_layer_destroy(void* m);


/**
 * @brief Initialize a LayerNorm layer with floating-point gamma and beta.
 * @param layernorm_layer_param Pointer to the LayerNorm parameters.
 * @return LayerNorm layer handle, or NULL if initialization fails.
 * @note input and output data are INT16; gamma and beta are float arrays.
 */
void *anpu_layernorm_layer_init(const anpu_layernorm_layer_param_t* layernorm_layer_param);

/**
 * @brief Execute LayerNorm with floating-point gamma and beta.
 * @param m             LayerNorm handle returned by anpu_layernorm_layer_init().
 * @param cpu_input_adr CPU-accessible address of the INT16 input.
 * @param npu_input_adr NPU-accessible address containing the same INT16 input data.
 * @param gamma_adr     Address of the float gamma array.
 * @param beta_adr      Address of the float beta array.
 * @param output_adr    Address of the INT16 output array.
 * @param npu_temp_adr  Address of NPU-accessible temporary storage.
 * @note gamma, beta, input, and output each contain input_count elements.
 * @note cpu_input_adr, gamma_adr, beta_adr, and output_adr do not need to reside in NPU RAM.
 * @note npu_input_adr and npu_temp_adr must reside in NPU-accessible RAM.
 * @note npu_temp_adr requires (input_count / 8) * sizeof(int8_t) + 8 * sizeof(int32_t) bytes.
 */
void anpu_layernorm_layer_forward(void* m, void* cpu_input_adr, void* npu_input_adr, void* gamma_adr,
                                  void *beta_adr, void *output_adr, void *npu_temp_adr);

/**
 * @brief Destroy a LayerNorm layer and release its resources.
 * @param m LayerNorm handle returned by anpu_layernorm_layer_init().
 */
void anpu_layernorm_layer_destroy(void* m);


/**
 * @brief Initialize a LayerNorm layer with optional integer gamma and beta.
 * @param layernorm_layer_param Pointer to the integer LayerNorm parameters.
 * @return Integer LayerNorm layer handle, or NULL if initialization fails.
 * @note Supported affine modes are gamma plus beta, gamma only, and neither gamma nor beta.
 * @note When beta is enabled, gamma must also be enabled and both data types must be identical.
 */
void *anpu_layernorm_i_layer_init(const anpu_layernorm_i_layer_param_t* layernorm_layer_param);

/**
 * @brief Execute LayerNorm with optional INT8 or INT16 gamma and beta.
 * @param m             LayerNorm handle returned by anpu_layernorm_i_layer_init().
 * @param cpu_input_adr CPU-accessible address of the INT16 input.
 * @param npu_input_adr NPU-accessible address containing the same INT16 input data.
 * @param gamma_adr     Address of gamma in gamma_dtype; may be NULL when gamma is disabled.
 * @param beta_adr      Address of beta in beta_dtype; may be NULL when beta is disabled.
 * @param output_adr    Address of the INT16 output array.
 * @param npu_temp_adr  Address of NPU-accessible temporary storage.
 * @note Each enabled affine array, input, and output contains input_count elements.
 * @note cpu_input_adr, gamma_adr, beta_adr, and output_adr do not need to reside in NPU RAM.
 * @note npu_input_adr and npu_temp_adr must reside in NPU-accessible RAM.
 * @note npu_temp_adr requires (input_count / 8) * sizeof(int8_t) + 8 * sizeof(int32_t) bytes.
 */
void anpu_layernorm_i_layer_forward(void* m, void* cpu_input_adr, void* npu_input_adr, void* gamma_adr,
                                    void *beta_adr, void *output_adr, void *npu_temp_adr);

/**
 * @brief Destroy an integer LayerNorm layer and release its resources.
 * @param m LayerNorm handle returned by anpu_layernorm_i_layer_init().
 */
void anpu_layernorm_i_layer_destroy(void* m);

#endif




