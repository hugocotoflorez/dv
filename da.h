/* Generic Dynamic Array Implementation
 *
 * Author: Hugo Coto Florez
 * Github: github.com/hugocotoflorez
 *
 * I know it is not readable at all but it works fine. ~ Hugo C.F. (me)
 *
 * Licenceless, fully opensource
 */

#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stdlib.h> // alloc

#define DA(type)              \
        typedef struct        \
        {                     \
                int capacity; \
                int size;     \
                type *data;   \
        }

#include <assert.h>
/* Initialize DA_PTR (that is a pointer to a DA). Initial size (int) can be
 * passed as second argument, default is 0. */
#define da_init(da_ptr, ...)                                                            \
        ({                                                                              \
                (da_ptr)->capacity = 0##__VA_ARGS__ ? __VA_ARGS__ : 4;                  \
                (da_ptr)->size = 0;                                                     \
                (da_ptr)->data = malloc(sizeof *((da_ptr)->data) * (da_ptr)->capacity); \
                assert(da_ptr);                                                         \
                sizeof *((da_ptr)->data);                                               \
        })

#include <assert.h>
// add E to DA_PTR that is a pointer to a DA of the same type as E
#define da_append(da_ptr, e)                                                     \
        ({                                                                       \
                if ((da_ptr)->size >= (da_ptr)->capacity)                        \
                {                                                                \
                        (da_ptr)->capacity += 3;                                 \
                        (da_ptr)->data =                                         \
                        realloc((da_ptr)->data,                                  \
                                sizeof(*((da_ptr)->data)) * (da_ptr)->capacity); \
                        assert(da_ptr);                                          \
                }                                                                \
                assert((da_ptr)->size < (da_ptr)->capacity);                     \
                (da_ptr)->data[(da_ptr)->size++] = (e);                          \
                (da_ptr)->size - 1;                                              \
        })

/* Destroy DA pointed by DA_PTR. DA can be initialized again but previous values
 * are not accessible anymore. */
#define da_destroy(da_ptr)        \
        ({                        \
                (da_ptr)->capacity = 0; \
                (da_ptr)->size = 0;     \
                free((da_ptr)->data);   \
                (da_ptr)->data = NULL;  \
        })

/* Insert element E into DA pointed by DA_PTR at index I. */
#include <string.h> // memmove
#define da_insert(da_ptr, e, i)                                                 \
        ({                                                                      \
                assert((i) >= 0 && (i) <= (da_ptr)->size);                      \
                da_append(da_ptr, e);                                           \
                memmove((da_ptr)->data + (i) + 1, (da_ptr)->data + (i),         \
                        ((da_ptr)->size - (i) - 1) * sizeof *((da_ptr)->data)); \
                (da_ptr)->data[i] = (e);                                        \
        })

/* Get size */
#define da_getsize(da) (da->size)

/* can be used as:
 * for_da_each(i, DA), where
 * - i: varuable where a pointer to an element from DA is going to be stored
 * - DA: is a valid DA */
#define for_da_each(_i_, da) \
        for (__auto_type _i_ = (da).data; (int) (_i_ - (da).data) < (da).size; ++_i_)


#endif
