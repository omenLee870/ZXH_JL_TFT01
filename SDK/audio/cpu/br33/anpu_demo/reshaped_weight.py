import numpy as np

ANPU_DTYPE_INT8_V1 = 0
ANPU_DTYPE_INT16_V1 = 1

# reshaped weight
def reshaped_weight(groups, inchannel, outchannel, weight_dtype, weight):
    is_depthwise = groups != 1 and groups == inchannel and groups == outchannel
    if not is_depthwise:
        if weight_dtype == ANPU_DTYPE_INT16_V1:
            alignment = 8
        elif weight_dtype == ANPU_DTYPE_INT8_V1:
            alignment = 16

        else:
            assert 0, "reshaped_weight: is_depthwise = 0: weight_dtype not find"
    else:
        alignment = 4

    # Pad and reshape the weight
    if is_depthwise:
        co, h, w, ci = weight.shape
        weight = weight.reshape(1, co, h, w, ci)
    else:
        co, h, w, ci = weight.shape
        weight = weight.reshape(groups, co // groups, h, w, ci)

    co = weight.shape[1]
    padded_co = (co + alignment - 1) // alignment * alignment
    padded_weight = np.pad(weight, ((0, 0), (0, padded_co - co), (0, 0), (0, 0), (0, 0)), mode='constant', constant_values=0)
    reshaped_weight = padded_weight.reshape(weight.shape[0], padded_co // alignment, alignment, weight.shape[2], weight.shape[3], weight.shape[4])
    reshaped_weight = reshaped_weight.transpose(0, 1, 3, 4, 5, 2)

    return reshaped_weight
