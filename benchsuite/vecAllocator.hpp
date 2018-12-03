#ifndef CLBALANCER_BENCHS_VECALLOCATOR_HPP  
#define CLBALANCER_BENCHS_VECALLOCATOR_HPP

#include <cstdlib>

template <typename T>
class vecAllocator {
public:
    typedef T value_type;
    size_t N = 64;
    typedef T* pointer;
    typedef const T* const_pointer;

    typedef T& reference;
    typedef const T& const_reference;

    /* Because this makes sense... */
    template <class U>
    struct rebind {
        typedef vecAllocator<U> other;
    };

    pointer allocate(size_t num, const void* = 0)
    {
        return reinterpret_cast<pointer>(aligned_alloc(N, num*sizeof(value_type)));
    }

    //void deallocate(pointer p,[[maybe_unused]] size_t num)
    void deallocate(pointer p,size_t num)
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
