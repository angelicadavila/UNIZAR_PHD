#include "clutils.hpp"

#include <cerrno>
#include <cstdio>
#include <string>

#include <array>
#include <iostream>
#include <vector>

using std::FILE;
using std::fclose;
using std::fopen;
using std::fwrite;
using std::move;
using std::stoi;
using std::to_string;
using std::vector;

std::string
get_file_contents(string filename)
{
  std::FILE* fp = std::fopen(filename.c_str(), "rb");
  if (fp) {
    std::string contents;
    std::fseek(fp, 0, SEEK_END);
    contents.resize(std::ftell(fp));
    std::rewind(fp);
    std::fread(&contents[0], 1, contents.size(), fp);
    std::fclose(fp);
    return (contents);
  }
  throw(errno);
}

class KernelManager
{
public:
  KernelManager() { m_info_buffer.reserve(128); }

  void discoverDevices()
  {
    cout << "Discovering platform and devices\n";
    cl::Platform::get(&m_platforms);
    cout << "platforms: " << m_platforms.size() << "\n";
    auto i = 0;
    for (auto& platform : m_platforms) {
      CL_CHECK_ERROR(platform.getInfo(CL_PLATFORM_NAME, &m_info_buffer));
      if (m_info_buffer.size() > 2)
        m_info_buffer.erase(m_info_buffer.size() - 1, 1);
      cout << "#" << i << " platform: " << m_info_buffer << "\n";

      vector<cl::Device> devices;
      platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
      cout << "  platform: " << i++ << " devices: " << devices.size() << "\n";

      auto j = 0;
      for (auto& device : devices) {
        CL_CHECK_ERROR(device.getInfo(CL_DEVICE_NAME, &m_info_buffer));
        if (m_info_buffer.size() > 2)
          m_info_buffer.erase(m_info_buffer.size() - 1, 1);
        cout << "  #" << j << " device: " << m_info_buffer << "\n";
        j++;
      }

      m_platform_devices.push_back(move(devices));
      i++;
    }
  }

  void setDevice(uint sel_platform, uint sel_device)
  {
    cout << "Selecting platform and device:\n";
    m_platform = move(usePlatformDiscovery(sel_platform));
    m_device = move(useDeviceDiscovery(sel_platform, sel_device));

    CL_CHECK_ERROR(m_platform.getInfo(CL_PLATFORM_NAME, &m_info_buffer));
    if (m_info_buffer.size() > 2)
      m_info_buffer.erase(m_info_buffer.size() - 1, 1);
    cout << "#" << sel_platform << " platform: " << m_info_buffer << "\n";

    CL_CHECK_ERROR(m_device.getInfo(CL_DEVICE_NAME, &m_info_buffer));
    if (m_info_buffer.size() > 2)
      m_info_buffer.erase(m_info_buffer.size() - 1, 1);
    cout << "  #" << sel_device << " device: " << m_info_buffer << "\n";

    string selected;
    char sep = ':';
    selected.reserve(32);
    const char* env_p = getenv("NODE");
    if (env_p == nullptr) {
      env_p = getenv("HOSTNAME");
    }
    if (env_p) {
      selected = string(env_p);
    } else {
      selected = "unknown";
    }
    selected += sep;
    selected += to_string(sel_platform);
    selected += sep;
    selected += to_string(sel_device);
    m_selected_str = move(selected);
  }

  void setKernel(string kernel_str) { m_kernel_str = move(kernel_str); }

  void build()
  {
    cl::Context context(m_device);

    cl_int cl_err;
    cl::Program::Sources sources;
    cout << "Building kernel:\n";
    cout << m_kernel_str;
    sources.push_back({ m_kernel_str.c_str(), m_kernel_str.length() });

    cl::Program program(context, sources);
    string options;
    options.reserve(32);
    options += "-DCL_SUPPORT_KERNEL_OFFSET=" + to_string(CL_SUPPORT_KERNEL_OFFSET);

    cout << "Options:\n";
    cout << options << "\n";
    cl_err = program.build({ m_device }, options.c_str());
    if (cl_err != CL_SUCCESS) {
      cout << " Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(m_device) << "\n";
      CL_CHECK_ERROR(cl_err);
    }

    cl_build_status status;
    cl_err = program.getBuildInfo(m_device, CL_PROGRAM_BUILD_STATUS, &status);
    cout << "Build status: " << status << "\n";

    string log;
    cl_err = program.getBuildInfo(m_device, CL_PROGRAM_BUILD_LOG, &log);
    cout << "Build log:\n" << log << "\n";

    vector<size_t> binary_sizes;
    cl_err = program.getInfo(CL_PROGRAM_BINARY_SIZES, &binary_sizes);
    cout << "Binary sizes: " << binary_sizes.size() << "\n";
    auto i = 0;
    for (auto& bin : binary_sizes) {
      cout << "#" << i << " Binary size: " << bin << "\n";
      i++;
    }

    cl_uint num_devices;
    cl_err = program.getInfo(CL_PROGRAM_NUM_DEVICES, &num_devices);
    cout << "Num devices: " << num_devices << "\n";

    vector<vector<char>> vbinaries;
    vector<char*> binaries;
    binaries.reserve(num_devices);
    binaries.resize(num_devices);
    vbinaries.reserve(num_devices);
    vbinaries.resize(num_devices);
    for (auto i = 0; i < num_devices; ++i) {
      vbinaries[i].reserve(binary_sizes[i]);
      vbinaries[i].resize(binary_sizes[i]);
      binaries[i] = vbinaries[i].data();
      // binaries[i].reserve(binary_sizes[i]);
    }
    cl_err = program.getInfo(CL_PROGRAM_BINARIES, &binaries);
    cout << "Binaries: " << binaries.size() << "\n";

    m_binaries = move(vbinaries);
  }
  void write(string filename)
  {
    auto n = filename.find(".cl");
    if (n != string::npos) {
      filename.replace(n, 3, "");
    }
    if (filename == "") {
      cout << "filename not valid\n";
      return;
    }

    if (m_binaries.size() > 1) {
      cout << "writing first binary\n";
    }

    auto i = 0;
    // for (auto& bin : vbinaries){
    // cout << "#" << i << " Binary\n";
    auto bin = m_binaries.at(i);

    string file = filename + "_" + m_selected_str + ".cl.bin";
    cout << "Writting to " << file << "\n";
    if (FILE* f1 = fopen(file.c_str(), "wb")) {
      fwrite(bin.data(), sizeof(char), bin.size(), f1);
      fclose(f1);
    }

    i++;
    // }
  }

private:
  cl::Platform usePlatformDiscovery(uint sel_platform)
  {
    auto last_platform = m_platforms.size() - 1;
    if (sel_platform > last_platform) {
      throw runtime_error("invalid platform selected");
    }
    return m_platforms[sel_platform];
  }

  cl::Device useDeviceDiscovery(uint sel_platform, uint sel_device)
  {
    auto last_platform = m_platforms.size() - 1;
    if (sel_platform > last_platform) {
      throw runtime_error("invalid platform selected");
    }
    auto last_device = m_platform_devices[sel_platform].size() - 1;
    if (sel_device > last_device) {
      throw runtime_error("invalid device selected");
    }
    return m_platform_devices[sel_platform][sel_device];
  }

  vector<cl::Platform> m_platforms;
  vector<vector<cl::Device>> m_platform_devices;
  string m_info_buffer;
  string m_kernel_str;
  string m_selected_str;
  cl::Platform m_platform;
  cl::Device m_device;
  vector<vector<char>> m_binaries;
};

int
main(int argc, char* argv[])
{
  auto ret = 0;
  KernelManager kmanager;

  if (argc == 1) {
    kmanager.discoverDevices();
  } else if (argc == 4) {
    auto sel_platform = stoi(argv[1]);
    auto sel_device = stoi(argv[2]);
    string filename = argv[3];
    auto src = get_file_contents(filename);
    // auto src = R"(
    //       __kernel void add(__global float* buffer, float scalar) {
    //           // buffer[get_global_id(0)] += scalar;
    //       }
    //   )";

    kmanager.discoverDevices();
    kmanager.setDevice(sel_platform, sel_device);

    kmanager.setKernel(src);
    kmanager.build();
    kmanager.write(filename);
  } else {
    cout << "wrong arguments\n";
    cout << "\nusage: clkernel <platform> <device> <kernel file>\n"
            "  platform: uint\n"
            "  device: uint\n"
            "  kernel file: string path\n"
            "  NOTE: run without arguments to discover devices\n";
    ret = 1;
  }

  return ret;
}
