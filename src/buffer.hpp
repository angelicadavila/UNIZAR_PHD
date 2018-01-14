#ifndef CLBALANCER_BUFFER_HPP
#define CLBALANCER_BUFFER_HPP 1

// #include "clutils.hpp"
// #include "semaphore.hpp"
// #include "sched.hpp"
// #include "device.hpp"
#include <memory>
#include <vector>

#include <CL/cl.h>

using namespace std;

namespace clb {

enum class Type {
  Null = 0,
  Int = 1,
  Float = 2,
  ClUchar4 = 3,
  ClFloat = 4,
  // String = 2,
};

enum class Direction { In = 0, Out = 1, InOut = 2 };

class Buffer {
 public:
  // Buffer() {}
  // Buffer(Type type) : m_type(type) {}

  Buffer(Direction direction);

  Buffer(Buffer const&) = delete;
  Buffer& operator=(Buffer const&) = delete;

  // make it movable
  Buffer(Buffer&&) = default;
  Buffer& operator=(Buffer&&) = default;

  Type type();
  Direction direction();
  size_t size();
  size_t itemSize();
  size_t bytes();
  // uint size(){
  //   switch(m_type){
  //   case Type::Null: return 0; break;
  //   case Type::Int: return ; break;
  //   }
  // }
  void set(shared_ptr<vector<int>>& in);
  void set(shared_ptr<vector<cl_uchar4>>& in);
  void set(shared_ptr<vector<float>>& in);
  // void set(vector<int>& in);
  // void set(Type type, vector<int>& in);

  // void set(Type type, vector<void*>&& in) {
  //   m_type = type;
  //   m_values = move(in);
  // }

  // void set(vector<int>&& in){
  //   m_type = Type::Int;
  //   m_values = move(in);
  // }
  // void set(vector<float>&& in){
  //   m_type = Type::Float;
  //   m_values = move(in);
  // }

  void* get();
  void* data();
  void* dataWithOffset(size_t offset);

  size_t byBytes(size_t size);
  // vector<int> getInt(){
  //   return m_int;
  // }
  // vector<float> getFloat(){
  //   return m_float;
  // }

  // vector<int> get(){
  //   return m_int;
  // }
  // vector<float> get(){
  //   return m_float;
  // }

  // template<T>
  // T get(Type type){

  // }
  // template<Type>
  // vector<int> get<Type::Int>(){
  //   return m_int;
  // }

 private:
  Direction m_direction;
  Type m_type;
  size_t m_item_size;
  size_t m_size;
  size_t m_bytes;
  void* m_data;
  void* m_address;
  // vector<void*> m_values;
  // vector<int> m_int;
  // vector<float> m_float;
};

}  // namespace clb

#endif /* CLBALANCER_BUFFER_HPP */
