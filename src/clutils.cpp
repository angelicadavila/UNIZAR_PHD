#include "clutils.hpp"

bool CLUtils::checkErrorFunc(cl_int error, string msg, const char* file, const int line) {
  bool ret;
  if (error == CL_SUCCESS) {
    ret = false;
  } else {
    ostringstream os(std::ios_base::ate);
    os << file << ":" << line << " [OpenCL error] " << clErrorToString(error);
    if (!msg.empty()) {
      os << ": " << msg;
    }
    cout << msg;
    cerr << os.str() << "\n";
    // qDebug("%s!! %s:%d:%s OpenCL error '%s': %s.", debugColor(RED), file, line,
    //        debugColor(DEFAULT), qPrintable(clErrorString(error)), qPrintable(msg));
    throw runtime_error(os.str());
  }
  return ret;
}

// bool CLUtils::checkErrorFunc(cl_int error, const char* file, const int line, string msg = ""){
//   bool ret;
//   if (error == CL_SUCCESS){
//     ret = false;
//   }else{
//     ostringstream os;
//     os << file << ":" << line << " [OpenCL error] " << clErrorString(error);
//     cerr << os.str() << "\n";
//     // qDebug("%s!! %s:%d:%s OpenCL error '%s': %s.", debugColor(RED), file, line,
//     //        debugColor(DEFAULT), qPrintable(clErrorString(error)), qPrintable(msg));
//     throw runtime_error(os.str());
//   }
//   return ret;
// }

// string CLUtils::clErrorString(cl_int error){
//   // OpenCL 1.2 enums
//   switch (error) {
//   case CL_SUCCESS:                            return "Success";
//   case CL_DEVICE_NOT_FOUND:                   return "Device not found.";
//   case CL_DEVICE_NOT_AVAILABLE:               return "Device not available";
//   case CL_COMPILER_NOT_AVAILABLE:             return "Compiler not available";
//   case CL_MEM_OBJECT_ALLOCATION_FAILURE:      return "Memory object allocation failure";
//   case CL_OUT_OF_RESOURCES:                   return "Out of resources";
//   case CL_OUT_OF_HOST_MEMORY:                 return "Out of host memory";
//   case CL_PROFILING_INFO_NOT_AVAILABLE:       return "Profiling information not available";
//   case CL_MEM_COPY_OVERLAP:                   return "Memory copy overlap";
//   case CL_IMAGE_FORMAT_MISMATCH:              return "Image format mismatch";
//   case CL_IMAGE_FORMAT_NOT_SUPPORTED:         return "Image format not supported";
//   case CL_BUILD_PROGRAM_FAILURE:              return "Program build failure";
//   case CL_MAP_FAILURE:                        return "Map failure";
//   case CL_MISALIGNED_SUB_BUFFER_OFFSET:       return "Misaligned sub buffer offset";
//   case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST: return "Exec status error for events in wait list";
//   case CL_COMPILE_PROGRAM_FAILURE:             return "Compile program failure";
//   case CL_LINKER_NOT_AVAILABLE:                return "Linker not available";
//   case CL_LINK_PROGRAM_FAILURE:                return "Link program failure";
//   case CL_DEVICE_PARTITION_FAILED:             return "Device partition failed";
//   case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:       return "Kernel arg info not available";
//   case CL_INVALID_VALUE:                      return "Invalid value";
//   case CL_INVALID_DEVICE_TYPE:                return "Invalid device type";
//   case CL_INVALID_PLATFORM:                   return "Invalid platform";
//   case CL_INVALID_DEVICE:                     return "Invalid device";
//   case CL_INVALID_CONTEXT:                    return "Invalid context";
//   case CL_INVALID_QUEUE_PROPERTIES:           return "Invalid queue properties";
//   case CL_INVALID_COMMAND_QUEUE:              return "Invalid command queue";
//   case CL_INVALID_HOST_PTR:                   return "Invalid host pointer";
//   case CL_INVALID_MEM_OBJECT:                 return "Invalid memory object";
//   case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:    return "Invalid image format descriptor";
//   case CL_INVALID_IMAGE_SIZE:                 return "Invalid image size";
//   case CL_INVALID_SAMPLER:                    return "Invalid sampler";
//   case CL_INVALID_BINARY:                     return "Invalid binary";
//   case CL_INVALID_BUILD_OPTIONS:              return "Invalid build options";
//   case CL_INVALID_PROGRAM:                    return "Invalid program";
//   case CL_INVALID_PROGRAM_EXECUTABLE:         return "Invalid program executable";
//   case CL_INVALID_KERNEL_NAME:                return "Invalid kernel name";
//   case CL_INVALID_KERNEL_DEFINITION:          return "Invalid kernel definition";
//   case CL_INVALID_KERNEL:                     return "Invalid kernel";
//   case CL_INVALID_ARG_INDEX:                  return "Invalid argument index";
//   case CL_INVALID_ARG_VALUE:                  return "Invalid argument value";
//   case CL_INVALID_ARG_SIZE:                   return "Invalid argument size";
//   case CL_INVALID_KERNEL_ARGS:                return "Invalid kernel arguments";
//   case CL_INVALID_WORK_DIMENSION:             return "Invalid work dimension";
//   case CL_INVALID_WORK_GROUP_SIZE:            return "Invalid work group size";
//   case CL_INVALID_WORK_ITEM_SIZE:             return "Invalid work item size";
//   case CL_INVALID_GLOBAL_OFFSET:              return "Invalid global offset";
//   case CL_INVALID_EVENT_WAIT_LIST:            return "Invalid event wait list";
//   case CL_INVALID_EVENT:                      return "Invalid event";
//   case CL_INVALID_OPERATION:                  return "Invalid operation";
//   case CL_INVALID_GL_OBJECT:                  return "Invalid OpenGL object";
//   case CL_INVALID_BUFFER_SIZE:                return "Invalid buffer size";
//   case CL_INVALID_MIP_LEVEL:                  return "Invalid mip-map level";
//   case CL_INVALID_GLOBAL_WORK_SIZE:			return "Invalid global work size";
//   case CL_INVALID_PROPERTY:                   return "Invalid property";
//   case CL_INVALID_IMAGE_DESCRIPTOR:           return "Invalid image descriptor";
//   case CL_INVALID_COMPILER_OPTIONS:           return "Invalid compiler options";
//   case CL_INVALID_LINKER_OPTIONS:             return "Invalid linker options";
//   case CL_INVALID_DEVICE_PARTITION_COUNT:     return "Invalid device partition count";
//   default: return "Unknown";
//   }
// }

string CLUtils::clErrorToString(cl_int error) {
  // OpenCL 1.2 enums
  switch (error) {
    case CL_SUCCESS:
      return "CL_SUCCESS";
    case CL_DEVICE_NOT_FOUND:
      return "CL_DEVICE_NOT_FOUND";
    case CL_DEVICE_NOT_AVAILABLE:
      return "CL_DEVICE_NOT_AVAILABLE";
    case CL_COMPILER_NOT_AVAILABLE:
      return "CL_COMPILER_NOT_AVAILABLE";
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:
      return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case CL_OUT_OF_RESOURCES:
      return "CL_OUT_OF_RESOURCES";
    case CL_OUT_OF_HOST_MEMORY:
      return "CL_OUT_OF_HOST_MEMORY";
    case CL_PROFILING_INFO_NOT_AVAILABLE:
      return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case CL_MEM_COPY_OVERLAP:
      return "CL_MEM_COPY_OVERLAP";
    case CL_IMAGE_FORMAT_MISMATCH:
      return "CL_IMAGE_FORMAT_MISMATCH";
    case CL_IMAGE_FORMAT_NOT_SUPPORTED:
      return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case CL_BUILD_PROGRAM_FAILURE:
      return "CL_BUILD_PROGRAM_FAILURE";
    case CL_MAP_FAILURE:
      return "CL_MAP_FAILURE";
    case CL_MISALIGNED_SUB_BUFFER_OFFSET:
      return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
    case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
      return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
    case CL_COMPILE_PROGRAM_FAILURE:
      return "CL_COMPILE_PROGRAM_FAILURE";
    case CL_LINKER_NOT_AVAILABLE:
      return "CL_LINKER_NOT_AVAILABLE";
    case CL_LINK_PROGRAM_FAILURE:
      return "CL_LINK_PROGRAM_FAILURE";
    case CL_DEVICE_PARTITION_FAILED:
      return "CL_DEVICE_PARTITION_FAILED";
    case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:
      return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";
    case CL_INVALID_VALUE:
      return "CL_INVALID_VALUE";
    case CL_INVALID_DEVICE_TYPE:
      return "CL_INVALID_DEVICE_TYPE";
    case CL_INVALID_PLATFORM:
      return "CL_INVALID_PLATFORM";
    case CL_INVALID_DEVICE:
      return "CL_INVALID_DEVICE";
    case CL_INVALID_CONTEXT:
      return "CL_INVALID_CONTEXT";
    case CL_INVALID_QUEUE_PROPERTIES:
      return "CL_INVALID_QUEUE_PROPERTIES";
    case CL_INVALID_COMMAND_QUEUE:
      return "CL_INVALID_COMMAND_QUEUE";
    case CL_INVALID_HOST_PTR:
      return "CL_INVALID_HOST_PTR";
    case CL_INVALID_MEM_OBJECT:
      return "CL_INVALID_MEM_OBJECT";
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
      return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case CL_INVALID_IMAGE_SIZE:
      return "CL_INVALID_IMAGE_SIZE";
    case CL_INVALID_SAMPLER:
      return "CL_INVALID_SAMPLER";
    case CL_INVALID_BINARY:
      return "CL_INVALID_BINARY";
    case CL_INVALID_BUILD_OPTIONS:
      return "CL_INVALID_BUILD_OPTIONS";
    case CL_INVALID_PROGRAM:
      return "CL_INVALID_PROGRAM";
    case CL_INVALID_PROGRAM_EXECUTABLE:
      return "CL_INVALID_PROGRAM_EXECUTABLE";
    case CL_INVALID_KERNEL_NAME:
      return "CL_INVALID_KERNEL_NAME";
    case CL_INVALID_KERNEL_DEFINITION:
      return "CL_INVALID_KERNEL_DEFINITION";
    case CL_INVALID_KERNEL:
      return "CL_INVALID_KERNEL";
    case CL_INVALID_ARG_INDEX:
      return "CL_INVALID_ARG_INDEX";
    case CL_INVALID_ARG_VALUE:
      return "CL_INVALID_ARG_VALUE";
    case CL_INVALID_ARG_SIZE:
      return "CL_INVALID_ARG_SIZE";
    case CL_INVALID_KERNEL_ARGS:
      return "CL_INVALID_KERNEL_ARGS";
    case CL_INVALID_WORK_DIMENSION:
      return "CL_INVALID_WORK_DIMENSION";
    case CL_INVALID_WORK_GROUP_SIZE:
      return "CL_INVALID_WORK_GROUP_SIZE";
    case CL_INVALID_WORK_ITEM_SIZE:
      return "CL_INVALID_WORK_ITEM_SIZE";
    case CL_INVALID_GLOBAL_OFFSET:
      return "CL_INVALID_GLOBAL_OFFSET";
    case CL_INVALID_EVENT_WAIT_LIST:
      return "CL_INVALID_EVENT_WAIT_LIST";
    case CL_INVALID_EVENT:
      return "CL_INVALID_EVENT";
    case CL_INVALID_OPERATION:
      return "CL_INVALID_OPERATION";
    case CL_INVALID_GL_OBJECT:
      return "CL_INVALID_GL_OBJECT";
    case CL_INVALID_BUFFER_SIZE:
      return "CL_INVALID_BUFFER_SIZE";
    case CL_INVALID_MIP_LEVEL:
      return "CL_INVALID_MIP_LEVEL";
    case CL_INVALID_GLOBAL_WORK_SIZE:
      return "CL_INVALID_GLOBAL_WORK_SIZE";
    case CL_INVALID_PROPERTY:
      return "CL_INVALID_PROPERTY";
    case CL_INVALID_IMAGE_DESCRIPTOR:
      return "CL_INVALID_IMAGE_DESCRIPTOR";
    case CL_INVALID_COMPILER_OPTIONS:
      return "CL_INVALID_COMPILER_OPTIONS";
    case CL_INVALID_LINKER_OPTIONS:
      return "CL_INVALID_LINKER_OPTIONS";
    case CL_INVALID_DEVICE_PARTITION_COUNT:
      return "CL_INVALID_DEVICE_PARTITION_COUNT";
    default:
      return "Unknown";
  }
}

// TODO have a look CL/cl.hpp Event.getInfo<cl_command_execution_status>()
// template <typename T>
string CLUtils::clEnumToString(cl_int error) {
  // OpenCL 1.2 enums
  switch (error) {
      /* command execution status */
    case CL_COMPLETE:
      return "CL_COMPLETE";
    case CL_RUNNING:
      return "CL_RUNNING";
    case CL_SUBMITTED:
      return "CL_SUBMITTED";
    case CL_QUEUED:
      return "CL_QUEUED";
    default:
      return "Unknown";
  }
}
