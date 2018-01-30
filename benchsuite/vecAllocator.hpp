#ifndef CLBALANCER_BENCHS_VECALLOCATOR_HPP  
#define CLBALANCER_BENCHS_VECALLOCATOR_HPP

#include <stdlib.h>
//#include <cstdlib.h>


template <typename T>
class vecAllocator {
public:
    typedef T value_type;
    std::size_t N = 64;
    typedef T* pointer;
    typedef const T* const_pointer;

    typedef T& reference;
    typedef const T& const_reference;

    /* Because this makes sense... */
    template <class U>
    struct rebind {
        typedef vecAllocator<U> other;
    };

    pointer allocate(std::size_t num, const void* = 0)
    {
        return (pointer) (aligned_alloc(N, num*sizeof(value_type)));
    }

    void deallocate(pointer p,[[maybe_unused]] std::size_t num)
    {	
        std::free (p);
    }

    void construct(pointer p, const_reference value) {
        new ((void*)p) T(value);
    }

    void destroy(pointer p) {
        p->~T();
    }
};

#endif
