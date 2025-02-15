#ifndef __CONNX_TENSOR_H__
#define __CONNX_TENSOR_H__

#include <connx/hal.h>
#include <connx/types.h>

// Iterator
/**
 * iterator - 1 + ndim * 4
 */
int32_t connx_Iterator_size(int32_t ndim);
void connx_Iterator_init(int32_t* iterator, int32_t ndim, int32_t* start, int32_t* stop, int32_t* step);
bool connx_Iterator_next(int32_t* iterator);
int32_t connx_Iterator_ndim(int32_t* iterator);
int32_t* connx_Iterator_start(int32_t* iterator);
int32_t* connx_Iterator_stop(int32_t* iterator);
int32_t* connx_Iterator_step(int32_t* iterator);
int32_t* connx_Iterator_index(int32_t* iterator);
int32_t connx_Iterator_offset(int32_t* iterator, int32_t* shape);

// tensor structure follow Numpy's ndarray
typedef struct _connx_Tensor {
    connx_DataType dtype;         // data type
    int32_t ndim;                 // Number of dimensions
    int32_t* shape;               // Shape array
    void* buffer;                 // Data buffer
    uint32_t size;                // size of buffer
    struct _connx_Tensor* parent; // Parent tensor that share the buffer
    int32_t ref_count;            // Reference count
    connx_Lock lock;              // Reference lock
} connx_Tensor;

int32_t connx_Iterator_size_tensor(connx_Tensor* tensor);

connx_Tensor* connx_Tensor_alloc(connx_DataType dtype, int32_t ndim, int32_t* shape);
connx_Tensor* connx_Tensor_alloc_like(connx_Tensor* tensor);
connx_Tensor* connx_Tensor_alloc_buffer(void* buf);
connx_Tensor* connx_Tensor_copy(connx_Tensor* tensor);
connx_Tensor* connx_Tensor_reshape(connx_Tensor* tensor, int32_t ndim, int32_t* shape);

void connx_Tensor_ref(connx_Tensor* tensor);
void connx_Tensor_unref(connx_Tensor* tensor);

int connx_Tensor_get(connx_Tensor* tensor, int32_t* idx, void* data);
int connx_Tensor_set(connx_Tensor* tensor, int32_t* idx, void* data);

#endif /* __CONNX_TENSOR_H__ */
