#include <stdio.h>
#include <string.h>
#include <connx/connx.h>

int connx_set_model(const char* path);
int connx_set_tensorin(const char* path);
int connx_set_tensorout(const char* path);

int main(int argc, char** argv) {
    if(argc < 2) {
        connx_info("Usage: connx [connx model path] [[tensor in pipe] tensor out pipe]]\n");
        return 0;
    }

    int32_t ret;

    ret = connx_set_model(argv[1]);
    if(ret != 0) {
        return ret;
    }

    if(argc > 3) {
        ret = connx_set_tensorin(argv[2]);
        if(ret != 0) {
            return ret;
        }

        ret = connx_set_tensorout(argv[3]);
        if(ret != 0) {
            return ret;
        }
    }

    // Parse connx model
    connx_Model model;
    ret = connx_Model_init(&model);
    if(ret != 0) {
        return ret;
    }

    // loop: input -> inference -> output
    // If input_count is -1 then exit the loop
    while(true) {
        // Read input count from HAL
        uint32_t input_count;
        if(connx_read(&input_count, sizeof(uint32_t)) != sizeof(uint32_t)) {
            connx_error("Cannot read input count from Tensor I/O module.\n");

            ret = -CONNX_IO_ERROR;
            connx_write(&ret, sizeof(int32_t));

            return CONNX_IO_ERROR;
        }

        if(input_count == (uint32_t)-1) {
            break;
        }

        // Read input data from HAL
        connx_Tensor* inputs[input_count];
        for(uint32_t i = 0; i < input_count; i++) {
            int32_t dtype;
            if(connx_read(&dtype, sizeof(int32_t)) != sizeof(int32_t)) {
                connx_error("Cannot read input data type from Tensor I/O module.\n");

                ret = -CONNX_IO_ERROR;
                connx_write(&ret, sizeof(int32_t));

                return CONNX_IO_ERROR;
            }

            int32_t ndim;
            if(connx_read(&ndim, sizeof(int32_t)) != sizeof(int32_t)) {
                connx_error("Cannot read input ndim from Tensor I/O module.\n");

                ret = -CONNX_IO_ERROR;
                connx_write(&ret, sizeof(int32_t));

                return CONNX_IO_ERROR;
            }

            int32_t shape[ndim];
            if(connx_read(&shape, sizeof(int32_t) * ndim) != sizeof(int32_t) * ndim) {
                connx_error("Cannot read input shape from Tensor I/O module.\n");

                ret = -CONNX_IO_ERROR;
                connx_write(&ret, sizeof(int32_t));

                return CONNX_IO_ERROR;
            }

            connx_Tensor* input = inputs[i] = connx_Tensor_alloc(dtype, ndim, shape);
            if(input == NULL) {
                connx_error("Out of memory\n");

                ret = -CONNX_NOT_ENOUGH_MEMORY;
                connx_write(&ret, sizeof(int32_t));

                return CONNX_NOT_ENOUGH_MEMORY;
            }

            int32_t len = connx_read(input->buffer, input->size);
            if(len != (int32_t)input->size) {
                connx_error("Cannot read input data from Tensor I/O moudle.\n");

                ret = -CONNX_IO_ERROR;
                connx_write(&ret, sizeof(int32_t));

                return CONNX_IO_ERROR;
            }
        }

        // Run model
        uint32_t output_count = 16;
        connx_Tensor* outputs[output_count];

        ret = connx_Model_run(&model, input_count, inputs, &output_count, outputs);
        if(ret != CONNX_OK) {
            int32_t ret2 = -ret;
            connx_write(&ret2, sizeof(int32_t));

            return ret;
        }

        // Write outputs
        // Write output count
        if(connx_write(&output_count, sizeof(uint32_t)) != sizeof(uint32_t)) {
            connx_error("Cannot write output data to Tensor I/O moudle.\n");

            ret = -CONNX_IO_ERROR;
            connx_write(&ret, sizeof(int32_t));

            return CONNX_IO_ERROR;
        }

        for(uint32_t i = 0; i < output_count; i++) {
            connx_Tensor* output = outputs[i];

            int32_t dtype = output->dtype;
            if(connx_write(&dtype, sizeof(int32_t)) != sizeof(int32_t)) {
                connx_error("Cannot write output data type to Tensor I/O module.\n");

                ret = -CONNX_IO_ERROR;
                connx_write(&ret, sizeof(int32_t));

                return CONNX_IO_ERROR;
            }

            if(connx_write(&output->ndim, sizeof(int32_t)) != sizeof(int32_t)) {
                connx_error("Cannot write output ndim to Tensor I/O module.\n");

                ret = -CONNX_IO_ERROR;
                connx_write(&ret, sizeof(int32_t));

                return CONNX_IO_ERROR;
            }

            if(connx_write(output->shape, sizeof(int32_t) * output->ndim) != sizeof(int32_t) * output->ndim) {
                connx_error("Cannot write output shape to Tensor I/O module.\n");

                ret = -CONNX_IO_ERROR;
                connx_write(&ret, sizeof(int32_t));

                return CONNX_IO_ERROR;
            }

            if(connx_write(output->buffer, output->size) != (int32_t)output->size) {
                connx_error("Cannot write output data to Tensor I/O module.\n");

                ret = -CONNX_IO_ERROR;
                connx_write(&ret, sizeof(int32_t));

                return CONNX_IO_ERROR;
            }
        }

        /*
        for(uint32_t i = 0; i < output_count; i++) {
            printf("***** output[%u]\n", i);
            connx_Tensor_dump(outputs[i]);
        }
        */

        for(uint32_t i = 0; i < output_count; i++) {
            connx_Tensor_unref(outputs[i]);
        }
    }

    connx_Model_destroy(&model);

    return 0;
}
