////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2013
//
// Ray tracing renderer
//

#if OCTET_OPENCL
class raytracer {
public:
  struct context {
    int width;
    int height;
    GLuint texture_handle;

    mat4 cameraToWorld;
    float near_plane;
    float far_plane;
    float near_plane_xmax;
    float near_plane_ymax;

    lighting *lights;

    int num_objects;
    mat4 *modelToWorld;
    mesh **meshes;
    bump_material **materials;
  };

  struct tri_context {
    int dom_axis;
    float xplane, yplane, zplane;
    float pxa,  pya,  dxdya,  pxb,  pyb,  dxdyb;
    int x0p, x1p;
    int x0, x1, x2;
    int xmin, xmax;
    unsigned triangle;
    mesh *mesh;
  };

private:
  context ctxt;
  dynarray<unsigned char> image;
  tri_context tc;
  cl_context cl;
  cl_command_queue queue;
  cl_program prog;
  cl_kernel gen_voxels;
  dynarray<unsigned char> src;

public:
  raytracer() {
    // in case of disaster, clear variables
    cl = 0;
    queue = 0;
    prog = 0;
    gen_voxels = 0;
  }

  void init() {
    // set up OpenCL
    cl_platform_id platform;
    if (clGetPlatformIDs(1, &platform, NULL)) { app::error("clGetPlatformIDs"); }

    cl_device_id devices[32];
    cl_uint devices_size;
    if (clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 32, devices, &devices_size)) { app::error("clGetDeviceIDs"); }
    cl_device_id device = devices[0];

    /*int image_support = 0;
    clGetDeviceInfo(devices[0], CL_DEVICE_IMAGE_SUPPORT, sizeof(int), &image_support, 0);
    printf("image_support=%d\n", image_support);*/

    cl_int error = 0;

    #if 0
    // Create CL context properties, add WGL context & handle to DC 
    cl_context_properties properties[] = { 
      CL_GL_CONTEXT_KHR,   (cl_context_properties)wglGetCurrentContext(), // WGL Context 
      //CL_WGL_HDC_KHR,      (cl_context_properties)wglGetCurrentDC(),      // WGL HDC
      //CL_CONTEXT_PLATFORM, (cl_context_properties)platform,               // OpenCL platform
      0
    };

    /*clGetGLContextInfoKHR_fn clGetGLContextInfoKHR =
      (clGetGLContextInfoKHR_fn)clGetExtensionFunctionAddress("clGetGLContextInfoKHR")
    ;

    // Find CL capable devices in the current GL context 
    cl_device_id devices[32];
    size_t size = 0;
    clGetGLContextInfoKHR(
      properties, CL_DEVICES_FOR_GL_CONTEXT_KHR, sizeof(devices), devices, &size
    );*/
    #endif

    // Create a context using the supported devices
    //int count = size / sizeof(cl_device_id);
    //cl_context context = clCreateContext(properties, count, devices, NULL, 0, 0);
    cl = clCreateContext(0, 1, &device, NULL, NULL, &error);
    if (error) { app::error("clCreateContext"); }

    queue = clCreateCommandQueue(cl, device, 0, &error);
    if (error) { app::error("clCreateCommandQueue"); }

    // build the program
    app_utils::get_url(src, "assets/opencl/raytracer.cl");
    size_t src_size = src.size();
    const char *src_addr = (const char*)&src[0];
    if (!src_size) { app::error("assets/opencl/raytracer.cl not found - check launch directory"); }
    prog = clCreateProgramWithSource(cl, 1, &src_addr, &src_size, &error);
    if (error) { app::error("clCreateProgramWithSource"); }

    error = clBuildProgram(prog, 1, &device, 0, 0, 0);

    size_t log_size = 0;
    clGetProgramBuildInfo(prog, device, CL_PROGRAM_BUILD_LOG, 0, 0, &log_size);
    dynarray<char> log(log_size+1);

    clGetProgramBuildInfo(prog, device, CL_PROGRAM_BUILD_LOG, log_size, &log[0], &log_size);
    if (log_size) {
      log[log_size] = 0;
      printf("log: %s\n", &log[0]);
      fflush(stdout);
    }
    if (error) { app::error("clBuildProgram"); }

    #if 0
    // you can use this code to get the LLVM asm generated by the compiler
    size_t bin_size;
    clGetProgramInfo(prog, CL_PROGRAM_BINARY_SIZES, sizeof(bin_size), &bin_size, 0);
    dynarray<unsigned char> binary(bin_size);
    unsigned char *bin = &binary[0];
    clGetProgramInfo(prog, CL_PROGRAM_BINARIES, sizeof(bin), &bin, 0);

    FILE *file = fopen("c:/tmp/disasm.txt", "wb");
    fwrite(bin + 12, 1, (unsigned&)bin[4], file);
    /*fprintf(file, "\n");
    fprintf(file, "\n");
    for (int i = 0; i < bin_size; ++i) {
      fprintf(file, "%02x%c", bin[i], i % 16 == 15 ? '\n' : ' ');
    }
    fprintf(file, "\n");*/
    fclose(file);
    #endif

    gen_voxels = clCreateKernel(prog, "gen_voxels", &error);
    if (error) { app::error("clCreateKernel - gen_voxels"); }

  }

  ~raytracer() {
    clReleaseKernel(gen_voxels);
    clReleaseProgram(prog);
    clReleaseCommandQueue(queue);
    clReleaseContext(cl);
  }

  // public interface
  void ray_trace(const context &ctxt) {
    this->ctxt = ctxt;
    image.resize(ctxt.width * ctxt.height * 3);
    memset(&image[0], 0x80, ctxt.width * ctxt.height * 3);


    /*for (int i = 0; i != ctxt.num_objects; ++i) {
      mat4 modelToWorld = ctxt.modelToWorld[i];
      mesh *mesh = ctxt.meshes[i];
      vec4 min, max;
      modelToWorld.translate(3, 3, 3);
      modelToWorld.scale(8, 8, 8);
      const float *vertices = (const float *)mesh->get_vertices();
      const unsigned short *indices = (const unsigned short *)mesh->get_indices();
      unsigned stride = mesh->get_stride();
      vec4 pos0a = mesh->get_value(0, 141*3);
      vec4 pos1a = mesh->get_value(0, 141*3+1);
      vec4 pos2a = mesh->get_value(0, 141*3+2);
      vec4 pos0 = (vec4&)vertices[stride/4*(141*3+0)];
      vec4 pos1 = (vec4&)vertices[stride/4*(141*3+1)];
      vec4 pos2 = (vec4&)vertices[stride/4*(141*3+2)];
      //printf("%d %d %d\n", indices[141*3+0], indices[141*3+1], indices[141*3+2]);
      printf("%s\n", pos0a.toString());
      printf("%s\n", pos1a.toString());
      printf("%s\n", pos2a.toString());
      printf("%s\n", pos0.toString());
      printf("%s\n", pos1.toString());
      printf("%s\n", pos2.toString());
    }*/


    cl_int error;
    //cl_mem image_mem = clCreateFromGLTexture2D(cl, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, ctxt.texture_handle, &error);
    cl_mem image_mem = clCreateBuffer(cl, CL_MEM_READ_WRITE|CL_MEM_USE_HOST_PTR, ctxt.width * ctxt.height * 3, &image[0], &error);
    if (error) app::error("clCreateFromGLTexture2D");

    dynarray<unsigned> octree(0x100000);
    memset(&octree[0], 0, octree.size()*sizeof(octree[0]));
    octree[0] = 1 + 8 + 4; // allocator + addr*8 + normal
    cl_mem octree_mem = clCreateBuffer(cl, CL_MEM_READ_WRITE|CL_MEM_USE_HOST_PTR, octree.size()*sizeof(octree[0]), (void*)&octree[0], &error);
    if (error) app::error("clCreateBuffer - octree");

    for (int i = 0; i != ctxt.num_objects; ++i) {
      mat4 modelToWorld = ctxt.modelToWorld[i];
      mesh *mesh = ctxt.meshes[i];
      vec4 min, max;
      // adjust size of voxels
      modelToWorld.scale(32, 32, 32);
      // add offset to make all numbers positive. Note: this is not the same as translate()
      modelToWorld.w() += vec4(128, 128, 128, 0);

      cl_mem vertices_mem = clCreateBuffer(cl, CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, mesh->get_vertices_size(), (void*)mesh->get_vertices(), &error);
      if (error) app::error("clCreateBuffer");

      cl_mem indices_mem = clCreateBuffer(cl, CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, mesh->get_indices_size(), (void*)mesh->get_indices(), &error);
      if (error) app::error("clCreateBuffer - indices");


      int num_indices = mesh->get_num_indices();
      int stride = mesh->get_stride() / 4;

      if (
        clSetKernelArg(gen_voxels, 0, sizeof(image_mem), &image_mem)
        | clSetKernelArg(gen_voxels, 1, sizeof(ctxt.width), &ctxt.width)
        | clSetKernelArg(gen_voxels, 2, sizeof(ctxt.height), &ctxt.height)
        | clSetKernelArg(gen_voxels, 3, sizeof(vertices_mem), &vertices_mem)
        | clSetKernelArg(gen_voxels, 4, sizeof(indices_mem), &indices_mem)
        | clSetKernelArg(gen_voxels, 5, sizeof(num_indices), &num_indices)
        | clSetKernelArg(gen_voxels, 6, sizeof(stride), &stride)
        | clSetKernelArg(gen_voxels, 7, sizeof(modelToWorld), &modelToWorld)
        | clSetKernelArg(gen_voxels, 8, sizeof(octree_mem), &octree_mem)
      ) app::error("clSetKernelArg");

      const int work_size = 32;
      size_t local = 1;
      size_t global = (num_indices / 3) / work_size + 1;
      static int times;
      if (times++ == 0) error = clEnqueueNDRangeKernel(queue, gen_voxels, 1, NULL, &global, &local, 0, 0, 0);
      if (error) app::error("running gen_voxels");

      clReleaseMemObject(vertices_mem);
      clReleaseMemObject(indices_mem);
    }
    clFinish(queue);


    glBindTexture(GL_TEXTURE_2D, ctxt.texture_handle);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ctxt.width, ctxt.height, GL_RGB, GL_UNSIGNED_BYTE, &image[0]);

    clReleaseMemObject(image_mem);

    dump_octree(&octree[0], 1, 0, 0, 0, 0);
    exit(1);

    clReleaseMemObject(octree_mem);
  }

  void dump_octree(unsigned *ptr, unsigned index, int depth, int x, int y, int z) {
    printf("%*s@%04x %02x %02x %02x\n", depth*2, "", index, x, y, z);
    int half = 128 >> depth;
    for (int i = 0; i != 8; ++i) {
      if (ptr[index+i]) {
        dump_octree(ptr, ptr[index+i], depth+1, i&1?x+half:x, i&2?y+half:y, i&4?z+half:z);
      }
    }
  }
};

#else
class raytracer {
public:
  void init() {
  }
};

#endif
