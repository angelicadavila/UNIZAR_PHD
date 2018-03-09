#include "ray.hpp"

using namespace std::chrono;

Primitive*
load_scene(data_t* data)
{
  const char* scene = data->scene;

  int num_primitives = 0;
  int* n_primitives = &num_primitives;

  FILE* scene_file;
  scene_file = fopen(scene, "r");

  if (scene_file == NULL) {
    fprintf(stderr, "Scene file not found\n");
    return NULL;
  }

  Primitive* prim_list;

  char buf[256];
  // scenefile format
  // line 1: n_primitives
  // line 2+: primitive
  // type,r,g,b,refl,refr,refr_index,diff,spec,is_light,center/normal_x,center/normal_y,center/normal_z,radius/depth
  // type 0 = PLANE, type 1 = SPHERE; is_light 0 = FALSE, 1 = TRUE
  if (fgets(buf, 256, scene_file) == NULL || sscanf(buf, "%d", n_primitives) != 1) {
    fprintf(stderr, "Scene file format invalid\n");
    return NULL;
  }

  IF_LOGGING(cout << "num primitives: " << *n_primitives << "\n");
  prim_list = (Primitive*)malloc(sizeof(Primitive) * *n_primitives);

  if (prim_list == NULL) {
    fprintf(stderr, "Error: Failed to allocate primitive list memory on host.\n");
    return NULL;
  }

  memset(prim_list, 0, sizeof(Primitive) * (*n_primitives));

  int i;
  for (i = 0; i < *n_primitives; i++) {
    int type = 0, light = 0;
    cl_float4 center_normal;
    center_normal.s[0] = 0;
    center_normal.s[1] = 0;
    center_normal.s[2] = 0;
    center_normal.s[3] = 0;
    cl_float radius_depth = 1;
    if (fgets(buf, 256, scene_file) == NULL || sscanf(buf,
                                                      "%d,%f,%f,%f,%f,%f,%f,%f,%f,%d,%f,%f,%f,%f",
                                                      &type,
                                                      &prim_list[i].m_color.s[0],
                                                      &prim_list[i].m_color.s[1],
                                                      &prim_list[i].m_color.s[2],
                                                      &prim_list[i].m_refl,
                                                      &prim_list[i].m_refr,
                                                      &prim_list[i].m_refr_index,
                                                      &prim_list[i].m_diff,
                                                      &prim_list[i].m_spec,
                                                      &light,
                                                      &center_normal.s[0],
                                                      &center_normal.s[1],
                                                      &center_normal.s[2],
                                                      &radius_depth) != 14) {
      fprintf(stderr, "Scene file format invalid. Primitive %d\n", i + 1);
      return NULL;
    }

    prim_list[i].is_light = (light == 0) ? 0 : 1;

    switch (type) {
      case 0:
        prim_list[i].type = prim_type::PLANE;
        prim_list[i].normal.s[0] = center_normal.s[0];
        prim_list[i].normal.s[1] = center_normal.s[1];
        prim_list[i].normal.s[2] = center_normal.s[2];
        prim_list[i].depth = radius_depth;
        break;
      case 1:
        prim_list[i].type = prim_type::SPHERE;
        prim_list[i].center.s[0] = center_normal.s[0];
        prim_list[i].center.s[1] = center_normal.s[1];
        prim_list[i].center.s[2] = center_normal.s[2];
        prim_list[i].radius = radius_depth;
        prim_list[i].sq_radius = radius_depth * radius_depth;
        prim_list[i].r_radius = 1.0f / radius_depth;
        break;
      default:
        fprintf(stderr, "Scene file format invalid. Primitive %d\n", i + 1);
        return NULL;
    }
    IF_LOGGING(printf("--->%f\n", prim_list[i].m_refl));
  }

  // rnoz
  data->n_primitives = *n_primitives;

  return prim_list;
}

int
ray_begin(data_t* data)
{
  const char* scene = data->scene;
  int total_size = data->total_size;

  // Load scene
  IF_LOGGING(printf("Loading scene..\n"));
  Primitive* primitive_list;
  primitive_list = load_scene(data);

  cout << data->n_primitives << "\n";

  IF_LOGGING(printf("- primitive: %f\n", primitive_list[2].m_refl));
  if (primitive_list == NULL) {
    fprintf(stderr, "Failed to load scene from file: %s\n", scene);
    return 1;
  }

  // Allocate pixels for result
  Pixel* out_pixels;

  cl_uint pixel_size_bytes = sizeof(Pixel) * total_size;
  out_pixels = (Pixel*)malloc(pixel_size_bytes);

  if (out_pixels == NULL) {
    fprintf(stderr, "Error: Failed to allocate output pixel memory on host.\n");
    return 1;
  }

  // null all colors to 0
  memset(out_pixels, 0, pixel_size_bytes);

  data->A = primitive_list;
  data->C = out_pixels;
}

int
ray_end(data_t* data)
{
  data->retval = write_bmp_file(data->C, data->width, data->height, data->out_file);
  /* int r = 0; */
  /* for (r = 0; r < width * height; r++) { */
  /*   //	fprintf(stdout, "%d\n", out_pixels[r].x); */
  /* } */
  /* if (retval == 1) { */
  /*   fprintf(stderr, "Image write failed.\n"); */
  /*   return 1; */
  /* } */
}

void
data_t_init(data_t* data)
{
  data->depth = 5;
  data->fast_norm = 0;
  data->buil_norm = 0;
  data->nati_sqrt = 0;
  data->buil_dot = 0;
  data->buil_len = 0;
  data->viewp_w = 6.0;
  data->viewp_h = 4.5;
  data->camera_x = 0.0;
  data->camera_y = 0.25;
  data->camera_z = -7.0;
  data->scene = "def.scn";
  data->out_file = "/tmp/ray_out.bmp";
  data->progname = "raytracer";
  data->n_primitives = 0;
  data->retval = 0;
}

auto
check_ray(vector<int> in1, vector<int> in2, vector<int> out, int size)
{
  auto pos = -1;
  for (auto i = 0; i < size; ++i) {
    if ((in1[i] + in2[i]) != out[i]) {
      pos = i;
      break;
    }
  }
  return pos;
}

void
do_ray_base(int tscheduler,
            int tdevices,
            uint check,
            int wsize,
            int chunksize,
            vector<float>& props,
            string scene_path)
{

  bool use_binaries = (check >= 10) ? true : false;
  check = (check >= 10) ? check - 10 : check;

  string kernel_str;
  try {
    kernel_str = file_read("support/kernels/ray.cl");
  } catch (std::ios::failure& e) {
    cout << "io failure: " << e.what() << "\n";
  }

  srand(0);

  data_t data;
  data_t_init(&data);

  data.width = wsize;
  data.height = wsize;
  auto image_size = wsize * wsize;
  data.total_size = image_size;
  data.scene = scene_path.c_str();

  int depth = data.depth;
  int fast_norm = data.fast_norm;
  int buil_norm = data.buil_norm;
  int nati_sqrt = data.nati_sqrt;
  int buil_dot = data.buil_dot;
  int buil_len = data.buil_len;
  int width = data.width;
  int height = data.height;
  float viewp_w = data.viewp_w;
  float viewp_h = data.viewp_h;
  float camera_x = data.camera_x;
  float camera_y = data.camera_y;
  float camera_z = data.camera_z;

  ray_begin(&data);

  int n_primitives = data.n_primitives;

  auto in_prim_list = make_shared<vector<Primitive>>(n_primitives);
  in_prim_list.get()->assign(data.A, data.A + n_primitives);
  auto in_ptr = reinterpret_cast<Primitive*>(in_prim_list.get()->data());

  auto out_pixels = make_shared<vector<Pixel>>(image_size);
  out_pixels.get()->assign(data.C, data.C + image_size);
  auto out_ptr = reinterpret_cast<Pixel*>(out_pixels.get()->data());

  auto lws = 128;
  auto gws = image_size;

  auto sel_platform = PLATFORM;
  auto sel_device = tdevices == 0 ? 1 : 0; // invert, tdevices: 0 = CPU, 1 = GPU

  vector<char> kernel_bin;
  if (use_binaries) {
    switch (tdevices) {
      case 200:
        kernel_bin = file_read_binary("support/kernels/ray_sapu:0:1.cl.bin");
        break;
      case 201:
        kernel_bin = file_read_binary("support/kernels/ray_sapu:0:0.cl.bin");
        break;
      case 300:
        kernel_bin = file_read_binary("support/kernels/ray_batel:1:0.cl.bin");
        break;
      case 301:
        kernel_bin = file_read_binary("support/kernels/ray_batel:1:1.cl.bin");
        break;
      case 310:
        kernel_bin = file_read_binary("support/kernels/ray_batel:0:0.cl.bin");
        break;
    }
  }

  auto time_init = std::chrono::system_clock::now().time_since_epoch();

  switch (tdevices) {
    case 200:
      sel_platform = 0;
      sel_device = 1;
      break;
    case 201:
      sel_platform = 0;
      sel_device = 0;
      break;
    case 300:
      sel_platform = 1;
      sel_device = 0;
      break;
    case 301:
      sel_platform = 1;
      sel_device = 1;
      break;
    case 310:
      sel_platform = 0;
      sel_device = 0;
      break;
  }

  auto in_bytes = n_primitives * sizeof(Primitive);
  auto out_bytes = image_size * sizeof(Pixel);

  vector<cl::Platform> platforms;
  vector<vector<cl::Device>> platfordevices;
  cl::Device device;

  IF_LOGGING(cout << "discoverDevices\n");
  cl::Platform::get(&platforms);
  IF_LOGGING(cout << "platforms: " << platforms.size() << "\n");
  auto i = 0;
  for (auto& platform : platforms) {
    vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
    IF_LOGGING(cout << "platform: " << i++ << " devices: " << devices.size() << "\n");
    platfordevices.push_back(move(devices));
  }

  auto last_platform = platforms.size() - 1;
  if (sel_platform > last_platform) {
    throw runtime_error("invalid platform selected");
  }

  auto last_device = platfordevices[sel_platform].size() - 1;
  if (sel_device > last_device) {
    throw runtime_error("invalid device selected");
  }

  device = move(platfordevices[sel_platform][sel_device]);

  cl_int cl_err = CL_SUCCESS;
  cl::Context context(device);

  cl::CommandQueue queue(context, device, 0, &cl_err);
  CL_CHECK_ERROR(cl_err, "CommandQueue queue");

  IF_LOGGING(cout << "initBuffers\n");

  cl_int buffer_in_flags = CL_MEM_READ_WRITE;
  cl_int buffer_out_flags = CL_MEM_READ_WRITE;

  cl::Buffer in_buffer(context, buffer_in_flags, in_bytes, NULL);
  CL_CHECK_ERROR(cl_err, "in buffer ");
  cl::Buffer out_buffer(context, buffer_out_flags, out_bytes, NULL);
  CL_CHECK_ERROR(cl_err, "out buffer ");

  CL_CHECK_ERROR(queue.enqueueWriteBuffer(in_buffer, CL_FALSE, 0, in_bytes, in_ptr, NULL));

  IF_LOGGING(cout << "initKernel\n");

  cl::Program::Sources sources;
  cl::Program::Binaries binaries;

  cl::Program program;
  if (use_binaries) {
    cout << "using binary\n";
    binaries.push_back({ kernel_bin.data(), kernel_bin.size() });
    vector<cl_int> status = { -1 };
    program = std::move(cl::Program(context, { device }, binaries, &status, &cl_err));
    CL_CHECK_ERROR(cl_err, "building program from binary failed for device ");
  } else {
    sources.push_back({ kernel_str.c_str(), kernel_str.length() });
    program = std::move(cl::Program(context, sources));
  }

  string options;
  options.reserve(32);
  options += "-DECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED=" +
             to_string(ECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED);

  cl_err = program.build({ device }, options.c_str());
  if (cl_err != CL_SUCCESS) {
    IF_LOGGING(cout << " Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device)
                    << "\n");
    CL_CHECK_ERROR(cl_err);
  }

  cl::Kernel kernel(program, "raytracer_kernel", &cl_err);

  cl_err = kernel.setArg(0, out_buffer);
  CL_CHECK_ERROR(cl_err, "kernel arg 0 in1 buffer");

  cl_err = kernel.setArg(1, width);
  CL_CHECK_ERROR(cl_err, "kernel arg 0 in1 buffer");

  cl_err = kernel.setArg(2, height);
  CL_CHECK_ERROR(cl_err, "kernel arg 1 in2 buffer");

  cl_err = kernel.setArg(3, camera_x);
  CL_CHECK_ERROR(cl_err, "kernel arg 2 out buffer");

  cl_err = kernel.setArg(4, camera_y);
  CL_CHECK_ERROR(cl_err, "kernel arg 3 size");

  cl_err = kernel.setArg(5, camera_z);
  CL_CHECK_ERROR(cl_err, "kernel arg 3 size");

  cl_err = kernel.setArg(6, viewp_w);
  CL_CHECK_ERROR(cl_err, "kernel arg 3 size");

  cl_err = kernel.setArg(7, viewp_h);
  CL_CHECK_ERROR(cl_err, "kernel arg 3 size");

  cl_err = kernel.setArg(8, in_buffer);
  CL_CHECK_ERROR(cl_err, "kernel arg 3 size");

  cl_err = kernel.setArg(9, n_primitives);
  CL_CHECK_ERROR(cl_err, "kernel arg 3 size");

  cl_err = kernel.setArg(10, n_primitives * sizeof(Primitive), NULL);
  CL_CHECK_ERROR(cl_err, "kernel arg 3 size");

  auto offset = 0;
  queue.enqueueNDRangeKernel(
    kernel, cl::NDRange(offset), cl::NDRange(gws), cl::NDRange(lws), NULL, NULL);

  queue.enqueueReadBuffer(out_buffer, CL_TRUE, 0, out_bytes, out_ptr);

  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - time_init).count();

  cout << "time: " << diff_ms << "\n";

  // auto img = write_bmp_file(in1.data(), image_width, image_height, "g_base.bmp");
  // IF_LOGGING(cout << img << "\n");
  // img = write_bmp_file(out.data(), image_width, image_height, "gf_base.bmp");
  // IF_LOGGING(cout << img << "\n");

  if (check) {
    data.C = out_pixels.get()->data();
    data.out_file = "ray_base.bmp";

    ray_end(&data);
    cout << "Writing to ray_base.bmp\n";

    // auto threshold = 0.01f;
    // auto pos = check_binomial(in_ptr, out_ptr, samplesPerVectorWidth, samples, steps, threshold);
    // auto ok = pos == -1;

    // auto time = 0;
    // if (ok) {
    //   cout << "Success (" << time << ")\n";
    // } else {
    //   cout << "Failure (" << time << " in pos " << pos << ")\n";
    // }
  } else {
    cout << "Done\n";
  }

  free(data.C);
  free(data.A);
  exit(0);
}

void
do_ray(int tscheduler,
       int tdevices,
       uint check,
       int wsize,
       int chunksize,
       vector<float>& props,
       string scene_path)
{
  bool use_binaries = (check >= 10) ? true : false;
  check = (check >= 10) ? check - 10 : check;

  string kernel_str;
  try {
    kernel_str = file_read("support/kernels/ray.cl");
  } catch (std::ios::failure& e) {
    cout << "io failure: " << e.what() << "\n";
  }

  srand(0);

  data_t data;
  data_t_init(&data);

  data.width = wsize;
  data.height = wsize;
  auto image_size = wsize * wsize;
  data.total_size = image_size;
  data.scene = scene_path.c_str();

  int depth = data.depth;
  int fast_norm = data.fast_norm;
  int buil_norm = data.buil_norm;
  int nati_sqrt = data.nati_sqrt;
  int buil_dot = data.buil_dot;
  int buil_len = data.buil_len;
  int width = data.width;
  int height = data.height;
  float viewp_w = data.viewp_w;
  float viewp_h = data.viewp_h;
  float camera_x = data.camera_x;
  float camera_y = data.camera_y;
  float camera_z = data.camera_z;

  ray_begin(&data);

  int n_primitives = data.n_primitives;

  auto in_prim_list = make_shared<vector<Primitive>>(n_primitives);
  in_prim_list.get()->assign(data.A, data.A + n_primitives);
  auto in_ptr = reinterpret_cast<Primitive*>(in_prim_list.get()->data());

  auto out_pixels = make_shared<vector<Pixel>>(image_size);
  out_pixels.get()->assign(data.C, data.C + image_size);
  auto out_ptr = reinterpret_cast<Pixel*>(out_pixels.get()->data());

  auto lws = 128;
  auto gws = image_size;

  auto platform = PLATFORM;

  vector<char> kernel_bin;
  if (use_binaries) {
    switch (tdevices) {
      case 200:
        kernel_bin = file_read_binary("support/kernels/ray_sapu:0:1.cl.bin");
        break;
      case 201:
        kernel_bin = file_read_binary("support/kernels/ray_sapu:0:0.cl.bin");
        break;
      case 300:
        kernel_bin = file_read_binary("support/kernels/ray_batel:1:0.cl.bin");
        break;
      case 301:
        kernel_bin = file_read_binary("support/kernels/ray_batel:1:1.cl.bin");
        break;
      case 310:
        kernel_bin = file_read_binary("support/kernels/ray_batel:0:0.cl.bin");
        break;
    }
  }

  auto time_init = std::chrono::system_clock::now().time_since_epoch();

  vector<ecl::Device> devices;
  switch (tdevices) {
    case 200: {
      ecl::Device cpu(0, 1);
      if (use_binaries) {
        cpu.setKernel(kernel_bin);
      }
      devices.push_back(move(cpu));
    } break;
    case 201: {
      ecl::Device igpu(0, 0);
      if (use_binaries) {
        igpu.setKernel(kernel_bin);
      }
      devices.push_back(move(igpu));
    } break;
    case 300: {
      ecl::Device cpu(1, 0);
      if (use_binaries) {
        cpu.setKernel(kernel_bin);
      }
      devices.push_back(move(cpu));
    } break;
    case 301: {
      ecl::Device phi(1, 1);
      if (use_binaries) {
        phi.setKernel(kernel_bin);
      }
      devices.push_back(move(phi));
    } break;
    case 310: {
      ecl::Device gpu(0, 0);
      if (use_binaries) {
        gpu.setKernel(kernel_bin);
      }
      devices.push_back(move(gpu));
    } break;
  }

  ecl::StaticScheduler stSched;
  ecl::DynamicScheduler dynSched;
  // ecl::HGuidedScheduler hgSched;

  ecl::Runtime runtime(move(devices), gws, lws);
  if (tscheduler == 0) {
    runtime.setScheduler(&stSched);
    stSched.setRawProportions(props);
  } else if (tscheduler == 1) {
    runtime.setScheduler(&dynSched);
    dynSched.setWorkSize(chunksize);
    // } else { // tscheduler == 2
    //   runtime.setScheduler(&hgSched);
    //   hgSched.setWorkSize(worksize);
    //   hgSched.setRawProportions({ prop });
  }
  runtime.setInBuffer(in_prim_list);
  runtime.setOutBuffer(out_pixels);
  runtime.setKernel(kernel_str, "raytracer_kernel");

  runtime.setKernelArg(0, out_pixels);
  runtime.setKernelArg(1, width);
  runtime.setKernelArg(2, height);
  runtime.setKernelArg(3, camera_x);
  runtime.setKernelArg(4, camera_y);
  runtime.setKernelArg(5, camera_z);
  runtime.setKernelArg(6, viewp_w);
  runtime.setKernelArg(7, viewp_h);
  runtime.setKernelArg(8, in_prim_list);
  runtime.setKernelArg(9, n_primitives);
  // runtime.setKernelArg(10, sizeof(Primitive) * n_primitives);
  // auto sizePrim = sizeof(Primitive) * n_primitives;
  // runtime.setKernelArg(10, n_primitives * sizeof(Primitive), ArgType::LocalAlloc);
  runtime.setKernelArgLocalAlloc(10, n_primitives * sizeof(Primitive));
  // runtime.setKernelArg(10, sizeof(Primitive) * n_primitives);

  runtime.run();

  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - time_init).count();

  cout << "time: " << diff_ms << "\n";

  runtime.printStats();

  data.C = out_pixels.get()->data();

  if (check) {
    data.out_file = "ray.bmp";
    ray_end(&data);
    //   auto in1 = *in1_array.get();
    //   auto in2 = *in2_array.get();
    //   auto out = *out_array.get();

    //   auto pos = check_ray(in1, in2, out, size);
    //   auto ok = pos == -1;

    //   auto time = 0;
    //   if (ok) {
    //     cout << "Success (" << time << ")\n";
    //   } else {
    //     cout << "Failure (" << time << " in pos " << pos << ")\n";
    //   }
    cout << "Writing to ray.bmp\n";
  } else {
    cout << "Done\n";
  }

  exit(0);
  // free(data.C);
  // free(data.A);
}
