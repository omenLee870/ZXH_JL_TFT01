#include "effects/anpu_api.h"
#include "anpu_performance_test.h"
#include "system/includes.h"

int anpu_performance_test(void)
{
    anpu_api_init();


    void *conv2d_test_handle = anpu_conv2d_layer_init(&conv2d_layer_param_test_1);

    uint32_t conv2d_1_out_element_size = 0;
    if (conv2d_layer_param_test_1.output_dtype == ANPU_DTYPE_INT8_V1) {
        conv2d_1_out_element_size = 1;
    } else if (conv2d_layer_param_test_1.output_dtype == ANPU_DTYPE_INT16_V1) {
        conv2d_1_out_element_size = 2;
    } else if (conv2d_layer_param_test_1.output_dtype == ANPU_DTYPE_INT32_V1) {
        conv2d_1_out_element_size = 4;
    }

    int ekh = (conv2d_layer_param_test_1.kernel_h - 1) * conv2d_layer_param_test_1.dilation_h + 1;

    int ekw = (conv2d_layer_param_test_1.kernel_w - 1) * conv2d_layer_param_test_1.dilation_w + 1;

    int eih = conv2d_layer_param_test_1.input_h + conv2d_layer_param_test_1.in_pad_up + conv2d_layer_param_test_1.in_pad_down;

    int eiw = conv2d_layer_param_test_1.input_w + conv2d_layer_param_test_1.in_pad_left + conv2d_layer_param_test_1.in_pad_right;
    int output_h = (eih - ekh) / conv2d_layer_param_test_1.stride_h + 1;
    int output_w = (eiw - ekw) / conv2d_layer_param_test_1.stride_w + 1;  // must be greater than or equal to 4
    uint32_t conv2d_1_out_size = conv2d_1_out_element_size * (output_h + conv2d_layer_param_test_1.out_pad_up + conv2d_layer_param_test_1.out_pad_down) *
                                 (output_w + conv2d_layer_param_test_1.out_pad_left + conv2d_layer_param_test_1.out_pad_right) * conv2d_layer_param_test_1.output_channel;

    uint32_t conv2d_1_input_size = sizeof(input_data_test);
    uint32_t conv2d_1_weight_size = sizeof(weight_data_test);
    uint32_t conv2d_1_bias_size = sizeof(bias_data_test);

    void *conv2d_1_input_adr = anpu_malloc(conv2d_1_input_size);
    anpu_memcpy(conv2d_1_input_adr, input_data_test, conv2d_1_input_size);
    void *conv2d_1_weight_adr = anpu_malloc(conv2d_1_weight_size);
    anpu_memcpy(conv2d_1_weight_adr, weight_data_test, conv2d_1_weight_size);
    void *conv2d_1_bias_adr = anpu_malloc(conv2d_1_bias_size);
    anpu_memcpy(conv2d_1_bias_adr, bias_data_test, conv2d_1_bias_size);
    void *conv2d_1_output_adr = anpu_malloc(conv2d_1_out_size);


    // run
    // per-time consumption:871us
    // total MACs = 3.32M
    // OPS = 3.32*2*1000/0.871 * (320/240) = 10G
    unsigned long time0 = jiffies_usec();
    for (int i = 0; i < 1000; i++) {
        anpu_conv2d_layer_forward(conv2d_test_handle, conv2d_1_input_adr, conv2d_1_weight_adr, conv2d_1_bias_adr, conv2d_1_output_adr);
    }
    unsigned long time1 = jiffies_usec();
    printf("use time = all_time:%lu, per_time:%.6f\n", time1 - time0, ((float)(time1 - time0) / 100.f));

    int16_t *conv2d_1_output_data = (int16_t*)malloc(conv2d_1_out_size);
    int performance_test_pass = conv2d_1_output_data != NULL;
    if (conv2d_1_output_data != NULL) {
        anpu_memcpy(conv2d_1_output_data, conv2d_1_output_adr, conv2d_1_out_size);
        for (uint32_t i = 0; i < sizeof(conv2d_performance_expected_output_data) / sizeof(conv2d_performance_expected_output_data[0]); ++i) {
            if (conv2d_1_output_data[i] != conv2d_performance_expected_output_data[i]) {
                performance_test_pass = 0;
                break;
            }
        }
    }
    printf(performance_test_pass ? "performance conv2d test pass\n" : "performance conv2d test fail\n");
    free(conv2d_1_output_data);



    // destroy
    anpu_free(conv2d_1_input_adr);
    anpu_free(conv2d_1_weight_adr);
    anpu_free(conv2d_1_bias_adr);
    anpu_free(conv2d_1_output_adr);

    anpu_conv2d_layer_destroy(conv2d_test_handle);

    anpu_api_destroy();

    return 0;
}















