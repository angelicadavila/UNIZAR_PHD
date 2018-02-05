#include "ray.hpp"

int
write_bmp_file(Pixel* pixels, int width, int height, const char* filename)
{
  // some calculations for bitmap size
  int row_size = (PIXEL_BIT_DEPTH / 8) * width;
  int row_size_padding = (row_size % 4) == 0 ? 0 : 4 - (row_size % 4);
  int pixel_array_size = (row_size + row_size_padding) * height;
  int file_size = BITMAP_HEADER_SIZE + BITMAP_INFO_HEADER_SIZE + pixel_array_size;

  // magic bytes for standard bitmap
  bmp_magic_t magic_bytes;
  magic_bytes.magic[0] = 'B';
  magic_bytes.magic[1] = 'M';
  // bitmap header
  BMP_HEADER bitmap_header;
  bitmap_header.filesz = file_size;
  bitmap_header.creator1 = 0; // not used
  bitmap_header.creator2 = 0; // not used
  bitmap_header.bmp_offset = BITMAP_HEADER_SIZE + BITMAP_INFO_HEADER_SIZE;
  // bitmap info header
  BMP_INFO_HEADER bitmap_info_header;
  bitmap_info_header.header_sz = BITMAP_INFO_HEADER_SIZE;
  bitmap_info_header.width = width;
  bitmap_info_header.height = height;
  bitmap_info_header.nplanes = 1;       // number of color planes
  bitmap_info_header.bitspp = 24;       // number of bits per pixel. 0-7 = b, 8-15 = g, 16-23 = r
  bitmap_info_header.compress_type = 0; // no compression
  bitmap_info_header.bmp_bytesz = pixel_array_size;
  bitmap_info_header.hres = 2835; // 2835 pixels/meter = 72 dpi of most monitors
  bitmap_info_header.vres = 2835;
  bitmap_info_header.ncolors = 0;    // we have no palette
  bitmap_info_header.nimpcolors = 0; // 0 = all colors are important

  // write file
  FILE* bitmap_file;
  bitmap_file = fopen(filename, "wb");
  if (bitmap_file == NULL) {
    fprintf(stderr, "Error opening file %s for writing\r\n", filename);
    return 1;
  }
  // write headers
  fwrite(&magic_bytes, sizeof magic_bytes, 1, bitmap_file);
  fwrite(&bitmap_header, sizeof bitmap_header, 1, bitmap_file);
  fwrite(&bitmap_info_header, sizeof bitmap_info_header, 1, bitmap_file);

  // write pixels
  // note that in standard bitmap format, the pixels at the bottom of the image
  // are written first, but our internal structure has those pictures at max
  // height
  // therefore, we loop backwards
  int i;
  for (i = height - 1; i > -1; i--) {
    int j;
    for (j = 0; j < width; j++) {
      int cur_pixel = i * width + j;
      // pixel = cl_char4
      // r = s[0] g = s[1] b = s[2]
      fputc(pixels[cur_pixel].s[2], bitmap_file);
      fputc(pixels[cur_pixel].s[1], bitmap_file);
      fputc(pixels[cur_pixel].s[0], bitmap_file);
    }
    int r;
    for (r = 0; r < row_size_padding; r++)
      fputc(0, bitmap_file);
  }

  fclose(bitmap_file);
  return 0;
}

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

  cout << *n_primitives << "\n";
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
    printf("--->%f\n", prim_list[i].m_refl);
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
  printf("Loading scene..\n");
  Primitive* primitive_list;
  primitive_list = load_scene(data);

  cout << data->n_primitives << "\n";

  printf("--->%f\n", primitive_list[2].m_refl);
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
do_ray(int tscheduler,
       int tdevices,
       bool check,
       int wsize,
       int chunksize,
       float prop,
       string scene_path)
{

  srand(0);

  data_t data;
  data_t_init(&data);

  data.width = wsize;
  data.height = wsize;
  auto total_size = wsize * wsize;
  data.total_size = total_size;
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

  // auto prim_list = make_shared<vector<Primitive>>(n_primitives);
  auto in_prim_list = make_shared<vector<Primitive>>(n_primitives);
  in_prim_list.get()->assign(data.A, data.A + n_primitives);
  // prim_list.data
  // auto prim_list = make_shared<vector<Primitive>>(5);
  // auto out_pixels = make_shared<vector<cl_uchar4>>(total_size);
  auto out_pixels = make_shared<vector<Pixel>>(total_size);
  out_pixels.get()->assign(data.C, data.C + total_size);

  free(data.C);
  free(data.A);

  cout << n_primitives << "\n";
  cout << in_prim_list.get()->size() << "\n";
  cout << in_prim_list.get()->capacity() << "\n";
  cout << out_pixels.get()->size() << "\n";
  cout << out_pixels.get()->capacity() << "\n";

  auto in_empty = make_shared<vector<Primitive>>(n_primitives);
  // return;

  // string kernel = get_kernel_str();
  string kernel = file_read("support/kernels/raytracer.cl");

  cout << kernel;

  int size = total_size;
  // int size = 128 * wsize;
  // int worksize = 128 * chunksize;
  int worksize = chunksize;

  cout << size << "\n";
  cout << worksize << "\n";
  // auto in1_array = make_shared<vector<int>>(size, 1);
  // auto in2_array = make_shared<vector<int>>(size, 2);
  // auto out_array = make_shared<vector<int>>(size, 0);

  vector<clb::Device> devices;
  auto platform = 0;
  if (tdevices == 0) { // CPU
    clb::Device device(platform, 1);
    devices.push_back(move(device));
  } else if (tdevices == 1) { // GPU
    clb::Device device2(platform, 0);
    devices.push_back(move(device2));
  } else { // CPU + GPU
    clb::Device device(platform, 1);
    clb::Device device2(platform, 0);
    devices.push_back(move(device));
    devices.push_back(move(device2));
  }

  clb::StaticScheduler stSched;
  clb::DynamicScheduler dynSched;
  clb::HGuidedScheduler hgSched;

  clb::Runtime runtime(move(devices), size);
  if (tscheduler == 0) {
    runtime.setScheduler(&stSched);
    stSched.setRawProportions({ prop });
  } else if (tscheduler == 1) {
    runtime.setScheduler(&dynSched);
    dynSched.setWorkSize(worksize);
  } else { // tscheduler == 2
    runtime.setScheduler(&hgSched);
    hgSched.setWorkSize(worksize);
    hgSched.setRawProportions({ prop });
  }
  // runtime.setInBuffer(in1_array);
  // runtime.setInBuffer(in2_array);
  // runtime.setOutBuffer(out_array);
  runtime.setInBuffer(in_prim_list);
  runtime.setOutBuffer(out_pixels);
  runtime.setKernel(kernel, "raytracer_kernel");

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
  runtime.setKernelArg(10, in_empty);
  // runtime.setKernelArg(10, sizeof(Primitive) * n_primitives);

  runtime.run();

  runtime.printStats();

  data.C = out_pixels.get()->data();

  ray_end(&data);
  // if (check) {
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
  // } else {
  //   cout << "Done\n";
  // }
}
