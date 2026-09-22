#include "effects/anpu_api.h"
#include "anpu_example.h"
#include "system/includes.h"

int anpu_example(void)
{
    anpu_api_init();

    // init
    // conv2d_1
    void *conv2d_1_handle = anpu_conv2d_layer_init(&conv2d_layer_param_1);
    // calculate the output_size
    uint32_t conv2d_1_out_element_size = 0;
    if (conv2d_layer_param_1.output_dtype == ANPU_DTYPE_INT8_V1) {
        conv2d_1_out_element_size = 1;
    } else if (conv2d_layer_param_1.output_dtype == ANPU_DTYPE_INT16_V1) {
        conv2d_1_out_element_size = 2;
    } else if (conv2d_layer_param_1.output_dtype == ANPU_DTYPE_INT32_V1) {
        conv2d_1_out_element_size = 4;
    }
    // effective kernel height
    int ekh = (conv2d_layer_param_1.kernel_h - 1) * conv2d_layer_param_1.dilation_h + 1;
    // effective kernel width
    int ekw = (conv2d_layer_param_1.kernel_w - 1) * conv2d_layer_param_1.dilation_w + 1;
    // effective input height
    int eih = conv2d_layer_param_1.input_h + conv2d_layer_param_1.in_pad_up + conv2d_layer_param_1.in_pad_down;
    // effective input width
    int eiw = conv2d_layer_param_1.input_w + conv2d_layer_param_1.in_pad_left + conv2d_layer_param_1.in_pad_right;
    int output_h = (eih - ekh) / conv2d_layer_param_1.stride_h + 1;
    int output_w = (eiw - ekw) / conv2d_layer_param_1.stride_w + 1;  // must be greater than or equal to 4
    uint32_t conv2d_1_out_size = conv2d_1_out_element_size * (output_h + conv2d_layer_param_1.out_pad_up + conv2d_layer_param_1.out_pad_down) *
                                 (output_w + conv2d_layer_param_1.out_pad_left + conv2d_layer_param_1.out_pad_right) * conv2d_layer_param_1.output_channel;

    uint32_t conv2d_1_input_size = 1200;
    uint32_t conv2d_1_weight_size = 2880;
    uint32_t conv2d_1_bias_size = 120;
    // allocate buffer and transfer data in the anpu_buffer
    void *conv2d_1_input_adr = anpu_malloc(conv2d_1_input_size);
    void *conv2d_1_weight_adr = anpu_malloc(conv2d_1_weight_size);
    anpu_memcpy(conv2d_1_weight_adr, conv2d_1_weight_data, conv2d_1_weight_size);
    void *conv2d_1_bias_adr = anpu_malloc(conv2d_1_bias_size);
    anpu_memcpy(conv2d_1_bias_adr, conv2d_1_bias_data, conv2d_1_bias_size);
    void *conv2d_1_output_adr = anpu_malloc(conv2d_1_out_size);

    // conv2d_2
    // the output_pad of the upper layer serves as the input_pad_for this layer
    void *conv2d_2_handle = anpu_conv2d_layer_init(&conv2d_layer_param_2);
    uint32_t conv2d_2_out_size = 480;
    uint32_t conv2d_2_weight_size = 12000;
    uint32_t conv2d_2_bias_size = 16;
    // allocate buffer and transfer data in the anpu_buffer
    void *conv2d_2_weight_adr = anpu_malloc(conv2d_2_weight_size);
    anpu_memcpy(conv2d_2_weight_adr, conv2d_2_weight_data, conv2d_2_weight_size);
    void *conv2d_2_bias_adr = anpu_malloc(conv2d_2_bias_size);
    anpu_memcpy(conv2d_2_bias_adr, conv2d_2_bias_data, conv2d_2_bias_size);
    void *conv2d_2_output_adr = anpu_malloc(conv2d_2_out_size);

    // fc_1
    void *fc_1_handle = anpu_fc_layer_init(&fc_layer_param_1);
    uint32_t fc_1_out_size = 20;
    uint32_t fc_1_weight_size = 7680;
    uint32_t fc_1_bias_size = 40;
    // allocate buffer and transfer data in the anpu_buffer
    void *fc_1_weight_adr = anpu_malloc(fc_1_weight_size);
    anpu_memcpy(fc_1_weight_adr, fc_1_weight_data, fc_1_weight_size);
    void *fc_1_bias_adr = anpu_malloc(fc_1_bias_size);
    anpu_memcpy(fc_1_bias_adr, fc_1_bias_data, fc_1_bias_size);
    void *fc_1_output_adr = anpu_malloc(fc_1_out_size);


    // convtranspose2d_1
    void *convtranspose2d_1_handle = anpu_conv2d_layer_init(&convtransposed2d_param_1);
    // calculate upsample input size and allocate buffer
    uint32_t convtranspose2d_1_input_element_size = 0;
    if (convtransposed2d_param_1.input_dtype == ANPU_DTYPE_INT8_V1) {
        convtranspose2d_1_input_element_size = 1;
    } else if (convtransposed2d_param_1.input_dtype == ANPU_DTYPE_INT16_V1) {
        convtranspose2d_1_input_element_size = 2;
    }
    int upsample_input_h = (convtransposed2d_upsample_param_1.input_h - 1) * convtransposed2d_upsample_param_1.stride_h + 1 + 2 * (convtransposed2d_upsample_param_1.dilation_h *
                           (convtransposed2d_upsample_param_1.kernel_h - 1) - convtransposed2d_upsample_param_1.padding_h) + convtransposed2d_upsample_param_1.output_padding_h;
    int upsample_input_w = (convtransposed2d_upsample_param_1.input_w - 1) * convtransposed2d_upsample_param_1.stride_w + 1 + 2 * (convtransposed2d_upsample_param_1.dilation_w *
                           (convtransposed2d_upsample_param_1.kernel_w - 1) - convtransposed2d_upsample_param_1.padding_w) + convtransposed2d_upsample_param_1.output_padding_w;
    uint32_t upsample_input_size = upsample_input_h * upsample_input_w * convtransposed2d_upsample_param_1.input_channel * convtranspose2d_1_input_element_size;
    uint32_t convtranspose2d_1_out_size = 3200;
    uint32_t convtranspose2d_1_weight_size = 864;
    // allocate buffer and transfer data in the anpu_buffer
    void *convtranspose2d_1_input_adr = anpu_malloc(upsample_input_size);
    void *convtranspose2d_1_weight_adr = anpu_malloc(convtranspose2d_1_weight_size);
    anpu_memcpy(convtranspose2d_1_weight_adr, convtransposed2d_1_weight_data, convtranspose2d_1_weight_size);
    void *convtranspose2d_1_output_adr = anpu_malloc(convtranspose2d_1_out_size);


    // run
    anpu_memcpy(conv2d_1_input_adr, input_data, conv2d_1_input_size);
    anpu_conv2d_layer_forward(conv2d_1_handle, conv2d_1_input_adr, conv2d_1_weight_adr, conv2d_1_bias_adr, conv2d_1_output_adr);
    anpu_conv2d_layer_forward(conv2d_2_handle, conv2d_1_output_adr, conv2d_2_weight_adr, conv2d_2_bias_adr, conv2d_2_output_adr);
    anpu_fc_layer_forward(fc_1_handle, conv2d_2_output_adr, fc_1_weight_adr, fc_1_bias_adr, fc_1_output_adr);

    int16_t fc_1_output_data[10];
    int three_layers_test_pass = 1;
    anpu_memcpy(fc_1_output_data, fc_1_output_adr, fc_1_out_size);
    for (int i = 0; i < 10; ++i) {
        if (fc_1_output_data[i] != fc_1_expected_output_data[i]) {
            three_layers_test_pass = 0;
            break;
        }
    }
    printf(three_layers_test_pass ? "three layers test pass\n" : "three layers test fail\n");



    // convtranspose2d
    // upsample input(HWC) tensor
    anpu_upsample_tensor(&convtransposed2d_upsample_param_1, convtransposed2d_1_input_data, convtranspose2d_1_input_adr);
    anpu_conv2d_layer_forward(convtranspose2d_1_handle, convtranspose2d_1_input_adr, convtranspose2d_1_weight_adr, NULL, convtranspose2d_1_output_adr);

    int32_t convtranspose2d_1_output_data[800];
    int convtranspose2d_test_pass = 1;
    anpu_memcpy(convtranspose2d_1_output_data, convtranspose2d_1_output_adr, convtranspose2d_1_out_size);
    for (int i = 0; i < 800; ++i) {
        if (convtranspose2d_1_output_data[i] != convtranspose2d_1_expected_output_data[i]) {
            convtranspose2d_test_pass = 0;
            break;
        }
    }
    printf(convtranspose2d_test_pass ? "convtranspose2d test pass\n" : "convtranspose2d test fail\n");


    // destroy
    anpu_free(conv2d_1_input_adr);
    anpu_free(conv2d_1_weight_adr);
    anpu_free(conv2d_1_bias_adr);
    anpu_free(conv2d_1_output_adr);
    anpu_free(conv2d_2_weight_adr);
    anpu_free(conv2d_2_bias_adr);
    anpu_free(conv2d_2_output_adr);
    anpu_free(fc_1_weight_adr);
    anpu_free(fc_1_bias_adr);
    anpu_free(fc_1_output_adr);
    anpu_free(convtranspose2d_1_input_adr);
    anpu_free(convtranspose2d_1_weight_adr);
    anpu_free(convtranspose2d_1_output_adr);

    anpu_conv2d_layer_destroy(conv2d_1_handle);
    anpu_conv2d_layer_destroy(conv2d_2_handle);
    anpu_fc_layer_destroy(fc_1_handle);
    anpu_conv2d_layer_destroy(convtranspose2d_1_handle);

    anpu_api_destroy();

    return 0;
}















