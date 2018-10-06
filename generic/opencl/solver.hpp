/**
 * \file
 * \brief Contains skeleton of all methods that need to be defined on a per solver basis.
 *
 * \author Nicholas Curtis
 * \date 03/10/2015
 *
 *
 */

 #ifndef SOLVER_H
 #define SOLVER_H

#include <iostream>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <sstream>
#include <chrono>
#include "error_codes.hpp"

// solvers
#include <rkf45.hpp>
// #include <ros.h>

#define CL_EXEC(__cmd__) {\
            cl_uint _ret = (__cmd__); \
            if (_ret != CL_SUCCESS) \
            { \
                 std::cerr << "Error executing CL cmd =" << std::endl; \
                 std::cerr << "\t" << #__cmd__ << std::endl; \
                 std::cerr << "\t" << "ret  = " << _ret << std::endl; \
                 std::cerr << "\t" << "line = " << __LINE__ << std::endl; \
                 std::cerr << "\t" << "file = " << file << std::endl; \
                 exit(-1); \
            } \
     }

#define __clerror(__errcode, CMD) \
     { \
            std::cerr << "Error executing CL cmd = " << cmd << std::endl; \
            std::cerr << "\terrcode = " << (__errcode); \
            if ((__errcode) == CL_INVALID_CONTEXT) \
                 std::cerr << "CL_INVALID_CONTEXT"; \
            else if ((__errcode) == CL_INVALID_VALUE) \
                 std::cerr << "CL_INVALID_VALUE"; \
            else if ((__errcode) == CL_INVALID_BUFFER_SIZE) \
                 std::cerr << "CL_INVALID_BUFFER_SIZE"; \
            else if ((__errcode) == CL_INVALID_HOST_PTR) \
                 std::cerr << "CL_INVALID_HOST_PTR"; \
            else if ((__errcode) == CL_MEM_OBJECT_ALLOCATION_FAILURE) \
                 std::cerr << "CL_MEM_OBJECT_ALLOCATION_FAILURE"; \
            else if ((__errcode) == CL_OUT_OF_RESOURCES) \
                 std::cerr << "CL_OUT_OF_RESOURCES"; \
            else if ((__errcode) == CL_OUT_OF_HOST_MEMORY) \
                 std::cerr << "CL_OUT_OF_HOST_MEMORY"; \
            std::cerr << std::endl; \
     }

namespace opencl_solvers {

    //! \brief Implementation of a initial value problem
    class IVP
    {
    public:
        IVP(const std::vector<std::string>& kernelSource,
            std::size_t requiredMemorySize):
                _kernelSource(kernelSource),
                _requiredMemorySize(requiredMemorySize);

        //! \brief Return the list of filenames of OpenCL kernels that implement
        //!        the source term and Jacobian
        const std::vector<std::string>& kernelSource()
        {
            return _kernelSource;
        }

        //! \brief Return the (unvectorized) memory-size required by the IVP kernels
        std::size_t requiredMemorySize()
        {
            return _requiredMemorySize;
        }

    protected:
        std::vector<std::string> _kernelSource;
        std::size_t _requiredMemorySize;

    };

    class SolverOptions
    {
    public:
        SolverOptions(std::size_t vectorSize=1, std::size_t blockSize=1,
                      int numBlocks=-1, double atol=1e-10, double rtol=1e-6,
                      bool logging=false, double h_init=1e-6,
                      bool use_queue=true, char order='C'):
            _vectorSize(vectorSize),
            _blockSize(blockSize),
            _numBlocks(numBlocks),
            _atol(atol),
            _rtol(rtol),
            _logging(logging),
            _h_init(h_init),
            _order(order)
        {
            std::ostringstream err;
            err << "Order " << order << " not recognized";
            std::static_assert(order == 'C' || order == 'F',  err.str());
        }

        inline double atol() const
        {
            return _atol;
        }

        inline double rtol() const
        {
            return _rtol;
        }

        inline bool logging() const
        {
            return _logging;
        }

        inline double h_init() const
        {
            return _h_init;
        }

        inline double vectorSize() const
        {
            return _vectorSize;
        }

        inline double blockSize() const
        {
            return _blockSize;
        }

        inline double numBlocks() const
        {
            return _numBlocks;
        }

        inline char order() const
        {
            return _order;
        }

    protected:
        //! vector size
        std::size_t _vectorSize;
        //! block size
        std::size_t _blockSize;
        //! number of blocks to launch
        std::size_t _numBlocks;
        //! the absolute tolerance for this integrator
        const double _atol;
        //! the relative tolerance for this integrator
        const double _rtol;
        //! whether logging is enabled or not
        bool _logging;
        //! The initial step-size
        const double _h_init;
        //! The data-ordering
        const char _order;
    };


    //! \brief A wrapper that contains information for the OpenCL kernel
    typedef struct
    {
        char function_name[1024];
        char attributes[1024];
        cl_uint num_args;
        cl_uint reference_count;
        size_t compile_work_group_size[3];
        size_t work_group_size;
        size_t preferred_work_group_size_multiple;
        cl_ulong local_mem_size;
        size_t global_work_size[3];
        cl_ulong private_mem_size;
    } kernelInfo_t;

    //! \brief the maximum number of platforms to check
    static constexpr std::size_t cl_max_platforms = 16;

    //! \brief A wrapper that contains information for the OpenCL platform
    typedef struct
    {
        cl_uint num_platforms;
        cl_platform_id platform_ids[cl_max_platforms]; // All the platforms ...
        cl_platform_id platform_id; // The one we'll use ...

        char name[1024];
        char version[1024];
        char vendor[1024];
        char extensions[2048];

        int is_nvidia;
    } platformInfo_t;

    //! \brief the maximum number of devices to query
    static const std::size_t cl_max_devices = 16;
    //! \brief A wrapper the contains information about the OpenCL device
    typedef struct
    {
        cl_device_id device_ids[cl_max_devices];
        cl_device_id device_id;
        cl_uint num_devices;

        cl_device_type type;

        char name[1024];
        char profile[1024];
        char version[1024];
        char vendor[1024];
        char driver_version[1024];
        char opencl_c_version[1024];
        char extensions[1024];

        cl_uint native_vector_width_char;
        cl_uint native_vector_width_short;
        cl_uint native_vector_width_int;
        cl_uint native_vector_width_long;
        cl_uint native_vector_width_float;
        cl_uint native_vector_width_double;
        cl_uint native_vector_width_half;

        cl_uint preferred_vector_width_char;
        cl_uint preferred_vector_width_short;
        cl_uint preferred_vector_width_int;
        cl_uint preferred_vector_width_long;
        cl_uint preferred_vector_width_float;
        cl_uint preferred_vector_width_double;
        cl_uint preferred_vector_width_half;

        cl_uint max_compute_units;
        cl_uint max_clock_frequency;

        cl_ulong max_constant_buffer_size;
        cl_uint  max_constant_args;

        cl_ulong max_work_group_size;

        cl_ulong max_mem_alloc_size;
        cl_ulong global_mem_size;
        cl_uint  global_mem_cacheline_size;
        cl_ulong global_mem_cache_size;

        cl_device_mem_cache_type global_mem_cache_type;

        cl_ulong local_mem_size;
        cl_device_local_mem_type local_mem_type;

        cl_device_fp_config fp_config;
    } deviceInfo_t;


    //! Wrapper for CL_DEVICE_TYPE
    enum DeviceType : cl_uint
    {
        //! CPU device
        CPU = CL_DEVICE_TYPE_CPU,
        //! GPU device
        GPU = CL_DEVICE_TYPE_GPU,
        //! Accelerator
        ACCELERATOR = CL_DEVICE_TYPE_ACCELERATOR,
        //! Default
        DEFAULT = CL_DEVICE_TYPE_DEFAULT
    };

    //! Wrapper for OpenCL kernel creation
    typedef struct
    {
        platformInfo_t platform_info;
        deviceInfo_t device_info;

        cl_context context;
        cl_command_queue command_queue;
        cl_program program;

        size_t blockSize;
        size_t numBlocks;
        int vectorSize;

        int use_queue;
    } cl_data_t;


    class IntegatorBase
    {
    private:
        std::size_t load_source_from_file (const char *flname, std::ostringstream& source_str)
        {
            std::ifstream ifile(flname);
            std::string content ((std::istreambuf_iterator<char>(ifile)),
                                 (std::istreambuf_iterator<char>()));
            size_t sz = content.length()
            source_str << content;
            return sz;
        }

        bool isPower2 (int x)
        {
             return ( (x > 0) && ((x & (x-1)) == 0) );
        }

        int imax(int a, int b) { return (a > b) ? a : b; }
        int imin(int a, int b) { return (a < b) ? a : b; }


        #define __Alignment (128)

        cl_mem CreateBuffer (cl_context *context, cl_mem_flags flags, std::size_t size, void *host_ptr)
        {
            cl_int ret;
            cl_mem buf = clCreateBuffer (*context, flags, size + __Alignment, host_ptr, &ret);
            //assert( ret == CL_SUCCESS );
            if (ret != CL_SUCCESS)
            {
                __clerror(ret, "CreateBuffer");
                exit(-1);
            }

            return buf;
        }

        cl_device_type getDeviceType (cl_device_id device_id)
        {
            cl_device_type val;
            CL_EXEC( clGetDeviceInfo (device_id, CL_DEVICE_TYPE, sizeof(cl_device_type), &val, NULL) );

            return val;
        }

        void printKernelInfo (const kernelInfo_t* info)
        {
            std::cout << "Kernel Info:" << std::endl;
            std::cout << "\t" << "function_name = " << info->function_name << std::endl;
            #if (__OPENCL_VERSION__ >= 120)
            std::cout << "\t" << "attributes = " << info->attributes << std::endl;
            #endif
            std::cout << "\t" << "num_args = " << info->num_args << std::endl;
            std::cout << "\t" << "reference_count = " << info->reference_count << std::endl;
            std::cout << "\t" << "compile_work_group_size = (" << info->compile_work_group_size[0] <<
                                    "," << info->compile_work_group_size[1] << "," << info->compile_work_group_size[2]
                                    ")" << std::endl;
            std::cout << "\t" << "work_group_size = " << info->work_group_size << std::endl;
            std::cout << "\n" << "preferred_work_group_size_multiple = " <<
                                    info->preferred_work_group_size_multiple << std::endl;
            std::cout << "local_mem_size" << info->local_mem_size << std::endl;
            std::cout << "private_mem_size" << info->private_mem_size << std::endl;
        }

        void getKernelInfo (kernelInfo_t *info, cl_kernel kernel, cl_device_id device_id)
        {
             CL_EXEC( clGetKernelInfo (kernel, CL_KERNEL_FUNCTION_NAME, sizeof(info->function_name), info->function_name, NULL) );
             #if (__OPENCL_VERSION__ >= 120)
             CL_EXEC( clGetKernelInfo (kernel, CL_KERNEL_ATTRIBUTES, sizeof(info->attributes), info->attributes, NULL) );
             #endif
             CL_EXEC( clGetKernelInfo (kernel, CL_KERNEL_NUM_ARGS, sizeof(info->num_args), &info->num_args, NULL) );
             CL_EXEC( clGetKernelInfo (kernel, CL_KERNEL_REFERENCE_COUNT, sizeof(info->reference_count), &info->reference_count, NULL) );
             CL_EXEC( clGetKernelWorkGroupInfo (kernel, device_id, CL_KERNEL_COMPILE_WORK_GROUP_SIZE, sizeof(info->compile_work_group_size), info->compile_work_group_size, NULL) );
             CL_EXEC( clGetKernelWorkGroupInfo (kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(info->work_group_size), &info->work_group_size, NULL) );
             CL_EXEC( clGetKernelWorkGroupInfo (kernel, device_id, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(info->preferred_work_group_size_multiple), &info->preferred_work_group_size_multiple, NULL) );
             //CL_EXEC( clGetKernelWorkGroupInfo (kernel, device_id, CL_KERNEL_GLOBAL_WORK_SIZE, sizeof(info->global_work_size), info->global_work_size, NULL) );
             CL_EXEC( clGetKernelWorkGroupInfo (kernel, device_id, CL_KERNEL_LOCAL_MEM_SIZE, sizeof(info->local_mem_size), &info->local_mem_size, NULL) );
             CL_EXEC( clGetKernelWorkGroupInfo (kernel, device_id, CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(info->private_mem_size), &info->private_mem_size, NULL) );
             printKernelInfo (info);
        }

        void getPlatformInfo (platformInfo_t *info)
        {
             cl_int ret;

             CL_EXEC ( clGetPlatformIDs ((cl_uint)cl_max_platforms, info->platform_ids, &info->num_platforms) );
             if (info->num_platforms == 0)
             {
                std::cerr << "clError: num_platforms = 0" << std::endl;
                exit(-1);
             }

             info->platform_id = info->platform_ids[0];

             #define __getInfo( __STR, __VAR) { \
                    CL_EXEC( clGetPlatformInfo (info->platform_id, (__STR), sizeof(__VAR), __VAR, NULL) ); \
                    std::cout << "\t" << #__STR << " = " << __VAR << std::endl; }

             printf("Platform Info:\n");
             __getInfo( CL_PLATFORM_NAME,       info->name);
             __getInfo( CL_PLATFORM_VERSION,    info->version);
             __getInfo( CL_PLATFORM_VENDOR,     info->vendor);
             __getInfo( CL_PLATFORM_EXTENSIONS, info->extensions);

             #undef __getInfo

             info->is_nvidia = 0; // Is an NVIDIA device?
             if (strstr(info->vendor, "NVIDIA") != NULL)
                    info->is_nvidia = 1;
             std::cout << "\tIs-NVIDIA = " << info->is_nvidia << std::endl;
        }

        void getDeviceInfo (const DeviceType device_type, deviceInfo_t *device_info,
                                                const platformInfo_t *platform_info)
        {
            const int verbose = 1;
            cl_uint ret;

            CL_EXEC( clGetDeviceIDs (platform_info->platform_id, CL_DEVICE_TYPE_ALL, (cl_uint)cl_max_devices, device_info->device_ids, &device_info->num_devices) );
            if (device_info->num_devices == 0)
            {
                std::cerr << "clError: num_devices = 0" << std::endl;
                exit(-1);
            }

            device_info->device_id = device_info->device_ids[0];

            #define get_char_info(__str__, __val__, __verbose__) { \
                CL_EXEC( clGetDeviceInfo (device_info->device_id, (__str__), sizeof(__val__), __val__, NULL) ); \
                if (__verbose__) printf("\t%-40s = %s\n", #__str__, __val__); \
            }
            #define get_info(__str__, __val__, __verbose__) { \
                CL_EXEC( clGetDeviceInfo (device_info->device_id, __str__, sizeof(__val__), &__val__, NULL) ); \
                if (__verbose__) printf("\t%-40s = %ld\n", #__str__, __val__); \
            }

            std::cout << "Device Info:" << std::endl;

            get_info( CL_DEVICE_TYPE, device_info->type, verbose);

            for (int i = 0; i < device_info->num_devices; ++i)
            {
                cl_device_type val;
                //get_info( CL_DEVICE_TYPE, val, 1);
                CL_EXEC( clGetDeviceInfo (device_info->device_ids[i], CL_DEVICE_TYPE, sizeof(cl_device_type), &val, NULL) );
                if (verbose)
                {
                        std::cout << "\t" << "CL_DEVICE_TYPE" << " = " << val << std::endl;
                }

                if (val == device_type)
                {
                        device_info->device_id = device_info->device_ids[i];
                        device_info->type = val;
                        break;
                }
            }

            std::string device_type_name;
            {
                if (device_info->type == CL_DEVICE_TYPE_GPU)

                     device_type_name = "GPU";
                else if (device_info->type == CL_DEVICE_TYPE_CPU)
                     device_type_name = "CPU";
                else if (device_info->type == CL_DEVICE_TYPE_ACCELERATOR)
                     device_type_name = "ACCELERATOR";
                else if (device_info->type == CL_DEVICE_TYPE_DEFAULT)
                     device_type_name = "DEFAULT";
            }
            if (verbose) std::cout<< "\t" << "Type Name = " << device_type_name << std::endl;

            get_char_info( CL_DEVICE_NAME, device_info->name, verbose );
            get_char_info( CL_DEVICE_PROFILE, device_info->profile, verbose );
            get_char_info( CL_DEVICE_VERSION, device_info->version, verbose );
            get_char_info( CL_DEVICE_VENDOR, device_info->vendor, verbose );
            get_char_info( CL_DRIVER_VERSION, device_info->driver_version, verbose );
            get_char_info( CL_DEVICE_OPENCL_C_VERSION, device_info->opencl_c_version, verbose );
            //get_char_info( CL_DEVICE_BUILT_IN_KERNELS );
            get_char_info( CL_DEVICE_EXTENSIONS, device_info->extensions, verbose );

            get_info( CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR, device_info->native_vector_width_char, verbose );
            get_info( CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT, device_info->native_vector_width_short, verbose );
            get_info( CL_DEVICE_NATIVE_VECTOR_WIDTH_INT, device_info->native_vector_width_int, verbose );
            get_info( CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG, device_info->native_vector_width_long, verbose );
            get_info( CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT, device_info->native_vector_width_float, verbose );
            get_info( CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE, device_info->native_vector_width_double, verbose );
            get_info( CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF, device_info->native_vector_width_half, verbose );

            get_info( CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, device_info->preferred_vector_width_char, verbose );
            get_info( CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, device_info->preferred_vector_width_short, verbose );
            get_info( CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, device_info->preferred_vector_width_int, verbose );
            get_info( CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, device_info->preferred_vector_width_long, verbose );
            get_info( CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, device_info->preferred_vector_width_float, verbose );
            get_info( CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, device_info->preferred_vector_width_double, verbose );
            get_info( CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF, device_info->preferred_vector_width_half, verbose );

            get_info( CL_DEVICE_MAX_COMPUTE_UNITS, device_info->max_compute_units, verbose );
            get_info( CL_DEVICE_MAX_CLOCK_FREQUENCY, device_info->max_clock_frequency, verbose );

            get_info( CL_DEVICE_MAX_WORK_GROUP_SIZE, device_info->max_work_group_size, verbose );

            get_info( CL_DEVICE_GLOBAL_MEM_SIZE, device_info->global_mem_size, verbose );
            get_info( CL_DEVICE_MAX_MEM_ALLOC_SIZE, device_info->max_mem_alloc_size, verbose );
            get_info( CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, device_info->global_mem_cacheline_size, verbose );
            get_info( CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, device_info->global_mem_cache_size, verbose );

            get_info( CL_DEVICE_GLOBAL_MEM_CACHE_TYPE, device_info->global_mem_cache_type, verbose);
            if (verbose)
            {
                std::cout << "\t" << " = " << "CL_DEVICE_GLOBAL_MEM_CACHE_TYPE";
                if (device_info->global_mem_cache_type == CL_NONE)
                     std::cout << "CL_NONE" << std::endl;
                else if (device_info->global_mem_cache_type == CL_READ_ONLY_CACHE)
                     std::cout << "CL_READ_ONLY_CACHE" << std::endl;
                else if (device_info->global_mem_cache_type == CL_READ_WRITE_CACHE)
                     std::cout << "CL_READ_WRITE_CACHE" << std::endl;
            }

            get_info( CL_DEVICE_LOCAL_MEM_SIZE, device_info->local_mem_size, verbose );
            get_info( CL_DEVICE_LOCAL_MEM_TYPE, device_info->local_mem_type, verbose );
            if (verbose)
            {
                std::cout << "\t" << " = " << "CL_DEVICE_LOCAL_MEM_TYPE" <<
                          ((device_info->local_mem_type == CL_LOCAL) ? "LOCAL" : "GLOBAL") <<
                          std::endl;
            }

            get_info( CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, device_info->max_constant_buffer_size, verbose );
            get_info( CL_DEVICE_MAX_CONSTANT_ARGS, device_info->max_constant_args, verbose );

            get_info( CL_DEVICE_DOUBLE_FP_CONFIG, device_info->fp_config, verbose );

            #undef get_char_info
            #undef get_info
        }

        int cl_init (cl_data_t* data)
        {
            cl_int ret;

            if (data != NULL)
                return CL_SUCCESS;

            getPlatformInfo(&cl_data->platform_info);

            getDeviceInfo(&cl_data->device_info, &cl_data->platform_info);

            cl_data->context = clCreateContext(NULL, 1, &cl_data->device_info.device_id, NULL, NULL, &ret);
            if (ret != CL_SUCCESS )
            {
                __clerror(ret, "clCreateContext");
                exit(-1);
            }

            cl_data->command_queue = clCreateCommandQueue(cl_data->context, cl_data->device_info.device_id, 0, &ret);
            if (ret != CL_SUCCESS )
            {
                __clerror(ret, "clCreateCommandQueue");
                exit(-1);
            }

            cl_data->use_queue = options.use_queue();
            cl_data->blockSize = options.blockSize();
            cl_data->numBlocks = cl_data->device_info.max_compute_units;
            if (options.numBlocks() > 0)
            {
                cl_data->numBlocks = options.numBlocks();
            }
            cl_data->vectorSize = options.vectorSize();
            std::ostringstream err;
            err << "Vector size: " << cl_data->vectorSize << " is not a power of 2!";
            std::static_assert(isPower2(cl_data->vectorSize),  err.str());
            std::ostringstream err2;
            err << "Block size: " << cl_data->blockSize << " is not a power of 2!";
            std::static_assert(isPower2(cl_data->blockSize), err2.str());

            if (cl_data->blockSize < cl_data->vectorSize)
                cl_data->blockSize = cl_data->vectorSize;

            cl_data->blockSize /= cl_data->vectorSize;

            std::ostringstream program_source_str;
            size_t program_source_size = 0;

            const char *dp_header = "#if defined(cl_khr_fp64) \n"
                                    "#pragma OPENCL EXTENSION cl_khr_fp64 : enable  \n"
                                    "#elif defined(cl_amd_fp64) \n"
                                    "#pragma OPENCL EXTENSION cl_amd_fp64 : enable  \n"
                                    "#endif \n";

            program_source_str << dp_header;
            program_source_size += std::strlen(dp_header);

            if (__Alignment)
            {
                std::ostringstream align;
                align << "#define __Alignment (" << __Alignment << ")" << std::endl;
                program_source_str << align.str();
                program_source_str += align.str().length();
            }

            std::ostringstream vsize;
            vsize << "#define __ValueSize (" << cl_data->vectorSize  << ")" << std::endl;
            program_source_str << vsize.str();
            program_source_size += vsize.str().length();

            std::ostringstream bsize;
            bsize << "#define __blockSize (" << cl_data->blockSize << ")" << std::endl;
            program_source_size += bsize.str().length();

            // neq
            std::ostringstream sneq;
            sneq << "#define neq (" << _neq << ")" << std::endl;
            program_source_str << sneq.str();
            program_source_size += sneq.str().length();

            // source rate evaluation work-size
            std::ostringstream rwk;
            rwk << "#define rk_lensrc (" << _ivp.requiredMemorySize() << ")" << std::endl;
            program_source_str << rwk.str();
            program_source_size += rwk.str().length();

            // order
            std::ostringstream sord;
            sord << "#define order ('" << options.order() << "')" << std::endl;
            program_source_str << sord.str();
            program_source_size += sord.str().length();

            if (cl_data->use_queue)
            {
                std::ostringstream oqueue;
                oqueue << "#define __EnableQueue" << std::endl;
                program_source_str << oqueue.str();
                program_source_size << oqueue.str().size();
            }

            // Load the common macros ...
            program_source_size += load_source_from_file ("cl_macros.h", program_source_str);
            program_source_size += load_source_from_file ("solver.h", program_source_str);

            // load the user specified kernels
            for (const std::string& kernel : _ivp.kernelSource())
            {
                program_source_size += load_source_from_file(kernel, program_source_str);
            }

            // Load the header and source files text ...
            for (const auto file& : solverFiles())
            {
                program_source_size += load_source_from_file (file, program_source_str);
            }


            /* Build Program */
            std::string psource_str = program_source_str.str();
            const char* psource = psource_str.c_str();
            cl_data->program = clCreateProgramWithSource(cl_data->context, 1,
                                                         (const char **)&psource,
                                                         (const size_t *)&program_source_size, &ret);
            if (ret != CL_SUCCESS)
            {
                __clerror(ret, "clCreateProgramWithSource");
            }

            std::ostringstream build_options;
            build_options << "-I. ";
            if (cl_data->platform_info.is_nvidia)
                    build_options << " -cl-nv-verbose";

            std::cout << "build_options = " << build_options.str();

            ret = clBuildProgram(cl_data->program, 1, &cl_data->device_info.device_id, build_options, NULL, NULL);
            stc::cerr << "clBuildProgram = " << ret << std::endl;

            cl_build_status build_status;
            CL_EXEC( clGetProgramBuildInfo (cl_data->program, cl_data->device_info.device_id, CL_PROGRAM_BUILD_STATUS,
                                            sizeof(cl_build_status), &build_status, NULL) );
            std::cout << "CL_PROGRAM_BUILD_STATUS = " << build_status;

            // get the program build log size
            size_t build_log_size;
            CL_EXEC( clGetProgramBuildInfo (cl_data->program, cl_data->device_info.device_id, CL_PROGRAM_BUILD_LOG,
                                            0, NULL, &build_log_size) );

            // and alloc the build log
            std::string build_log(build_log_size, '');
            CL_EXEC( clGetProgramBuildInfo (cl_data->program, cl_data->device_info.device_id, CL_PROGRAM_BUILD_LOG,
                                            build_log.length(), build_log.c_str(), &build_log_size) );
            if (build_log_size > 0)
            {
                stc::cout << "CL_PROGRAM_BUILD_LOG = " << build_log << std::endl;
            }

            if (ret != CL_SUCCESS)
            {
                __clerror(ret, "clBuildProgram");
            }

            if (cl_data->platform_info.is_nvidia && 0)
            {
                /* Query binary (PTX file) size */
                size_t binary_size;
                CL_EXEC( clGetProgramInfo (cl_data->program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL) );

                /* Read binary (PTX file) to memory buffer */
                std::string ptx_binary(binary_size, '');
                const char* c_ptx_binary = binary_size.c_str();
                CL_EXEC( clGetProgramInfo (cl_data->program, CL_PROGRAM_BINARIES, sizeof(unsigned char *), &c_ptx_binary, NULL) );
                /* Save PTX to add_vectors_ocl.ptx */
                FILE *ptx_binary_file = std::fopen("ptx_binary_ocl.ptx", "wb");
                std::fwrite(ptx_binary, sizeof(char), binary_size, ptx_binary_file);
                std::fclose(ptx_binary_file);
            }

            return CL_SUCCESS;
        }

    public:

        static constexpr double eps = EPS;
        static constexpr double small = SMALL;

        IntegratorBase(int neq, int numThreads,
                         const IVP& ivp,
                         const SolverOptions& options) :
            _numThreads(numThreads),
            _neq(neq),
            _log(),
            _ivp(ivp),
            _options(options)
        {

        }

        ~OpenCLIntegrator()
        {
            this->clean();
        }

        void log(const int NUM, const double t, double const * __restrict__ phi)
        {
            // allocate new memory
            _log.emplace_back(std::move(std::unique_ptr<double[]>(new double[1 + NUM * _neq])));
            double* __restrict__ set = _log.back().get();
            // and set
            set[0] = t;
            std::memcpy(&set[1], phi, NUM * _neq * sizeof(double));
        }

        /*! \brief Copy the current log of data to the given array */
        void getLog(const int NUM, double* __restrict__ times, double* __restrict__ phi) const
        {
            for (std::size_t index = 0; index < _log.size(); ++index)
            {
                const double* __restrict__ log_entry = _log[index].get();
                times[index] = log_entry[0];
                std::memcpy(&phi[index * NUM * _neq], &log_entry[1], NUM * _neq * sizeof(double));
            }
        }

        /*! \brief Return the number of integration steps completed by this Integrator */
        std::size_t numSteps() const
        {
            return _log.size();
        }

        void reinitialize(int numThreads)
        {
            throw "not implemented";
        }

        /*! checkError
                \brief Checks the return code of the given thread (IVP) for an error, and exits if found
                \param tid The thread (IVP) index
                \param code The return code of the thread
                @see ErrorCodes
        */
        void checkError(int tid, ErrorCode code) const
        {
            throw "not implemented";
        }

        //! return the absolute tolerance
        inline const double atol() const
        {
            return _options.atol();
        }

        //! return the relative tolerance
        inline const double rtol() const
        {
            return _options.rtol();
        }

        inline bool logging() const
        {
            return _options.logging();
        }

        //! return the number of equations to solve
        inline const int neq() const
        {
            return _neq;
        }

        //! return the number of equations to solve
        inline const int numThreads() const
        {
            return _numThreads;
        }

        //! return the initial step-size
        inline const double h_init() const
        {
            return _options.h_init();
        }


        /**
         * \brief Integration driver for the CPU integrators
         * \param[in]       NUM             The (non-padded) number of IVPs to integrate
         * \param[in]       t               The current system time
         * \param[in]       t_end           The IVP integration end time
         * \param[in]       param           The system constant variable (pressures / densities)
         * \param[in,out]   phi             The system state vectors at time t.
         * \returns system state vectors at time t_end
         *
         */
        void intDriver (const int NUM, const double t,
                                        const double t_end, const double* __restrict__ param,
                                        double* __restrict__ phi)
        {
            std::vector<double> t_vec (t, NUM);
            std::vector<double> t_end_vec (t_end, NUM);
            return this->intDriver(NUM, &t_vec[0], &t_end[0], param, phi)

        }

        /**
         * \brief Integration driver for the CPU integrators
         * \param[in]       NUM             The (non-padded) number of IVPs to integrate
         * \param[in]       t               The (array) of current system times
         * \param[in]       t_end           The (array) of IVP integration end times
         * \param[in]       param           The system constant variable (pressures / densities)
         * \param[in,out]   phi             The system state vectors at time t.
         * \returns system state vectors at time t_end
         *
         */
        virtual intDriver (const int NUM, const double* __restrict__ t,
                           const double* __restrict__ t_end,
                           const double* __restrict__ param, double* __restrict__ phi) = 0;



    protected:
        //! return reference to the beginning of the working memory
        //! for this thread `tid`
        double* phi(int tid);

        //! the number of OpenMP threads to use
        int _numThreads;
        //! the number of equations to solver per-IVP
        const int _neq;
        //! working memory for this integrator
        std::unique_ptr<char[]> working_buffer;
        //! log of state vectors / times
        std::vector<std::unique_ptr<double[]>> _log;
        //! \brief IVP information
        const IVP& _ivp;
        //! \brief Solver options for OpenCL execution
        const SolverOptions& _options;

        /*
         * \brief Return the required memory size in bytes (per-IVP)
         */
        virtual std::size_t requiredSolverMemorySize() = 0;

        //! \brief return the base kernel name
        virtual std::string solverName() const = 0;

        //! \brief return the list of files for this solver
        virtual const std::vector<std::string>& solverFiles() const = 0;


    private:
        void clean()
        {
            _log.clear();
        }

    };


    // skeleton of the opencl-solver
    template <struct solver_struct, struct counter_struct>
    class Integrator
    {

    public:
        Integrator(int neq, int numThreads,
                         const IVP& ivp,
                         const SolverOptions& options) :
            IntegratorBase(neq, numThreads, ivp, options)
        {

        }


        /**
         * \brief Integration driver for the CPU integrators
         * \param[in]       NUM             The (non-padded) number of IVPs to integrate
         * \param[in]       t               The (array) of current system times
         * \param[in]       t_end           The (array) of IVP integration end times
         * \param[in]       param           The system constant variable (pressures / densities)
         * \param[in,out]   phi             The system state vectors at time t.
         * \returns system state vectors at time t_end
         *
         */
        void intDriver (const int NUM, const double* __restrict__ t,
                        const double* __restrict__ t_end,
                        const double* __restrict__ param, double* __restrict__ phi)
        {
            cl_init(ck, _options);
            cl_int ret;

            auto t_start = std::chrono::high_resolution_clock::now();

            /* get the kernel name */
            std::ostringstream kernel_name;
            kernel_name << this->solverName();
            // all named driver
            kernel_name << "_driver";
            // and queue
            if (_options.use_queue())
            {
                kernel_name << "_queue"
            }
            std::string k_name = kernel_name.str();

            /* Extract the kernel from the program */
            cl_kernel kernel = clCreateKernel(cl_data->program, k_name.c_str(), &ret);
            if (ret != CL_SUCCESS)
            {

                __clerror(ret, "clCreateKernel");
                exit(-1);
            }

            // Query the kernel's info
            kernelInfo_t kernel_info;
            getKernelInfo (&kernel_info, kernel, cl_data->device_info.device_id);


            std::size_t lenrwk = (ivp.requiredMemorySize() + requiredSolverMemorySize())*cl_data->vectorSize;
            std::cout << "lenrwk = "<< lenrwk << std::endl;;

            size_t numThreads = cl_data->blockSize * cl_data->numBlocks;
            std::cout << "NP = " << NUM << ", blockSize = " << cl_data->blockSize <<
                         ", vectorSize = " << cl_data->vectorSize <<
                         ", numBlocks = " << cl_data->numBlocks <<
                         ", numThreads = " << numThreads << std::endl;

            auto t_data = std::chrono::high_resolution_clock::now();

            cl_mem buffer_param = CreateBuffer (&cl_data->context, CL_MEM_READ_ONLY, sizeof(double)*NUM, NULL);
            cl_mem t0 = CreateBuffer (&cl_data->context, CL_MEM_READ_ONLY, sizeof(double)*NUM, NULL);
            cl_mem tf = CreateBuffer (&cl_data->context, CL_MEM_READ_ONLY, sizeof(double)*NUM, NULL);
            cl_mem buffer_phi = CreateBuffer (&cl_data->context, CL_MEM_READ_WRITE, sizeof(double)*_neq*NUM, NULL);
            cl_mem buffer_solver = CreateBuffer (&cl_data->context, CL_MEM_READ_ONLY, sizeof(solver_struct), NULL);
            cl_mem buffer_rwk = CreateBuffer (&cl_data->context, CL_MEM_READ_WRITE, sizeof(double)*lenrwk*numThreads, NULL);
            cl_mem buffer_counters = CreateBuffer (&cl_data->context, CL_MEM_READ_WRITE, sizeof(counter_struct)*numProblems, NULL);

            /* transfer data to device */
            CL_EXEC( clEnqueueWriteBuffer(cl_data->command_queue, buffer_param, CL_TRUE, 0, sizeof(double)*NUM, p, 0, NULL, NULL) );
            CL_EXEC( clEnqueueWriteBuffer(cl_data->command_queue, t0, CL_TRUE, 0, sizeof(double)*NUM, t, 0, NULL, NULL) );
            CL_EXEC( clEnqueueWriteBuffer(cl_data->command_queue, tf, CL_TRUE, 0, sizeof(double)*NUM, t_end, 0, NULL, NULL) );
            CL_EXEC( clEnqueueWriteBuffer(cl_data->command_queue, buffer_phi, CL_TRUE, 0, sizeof(double)*NUM*_neq, phi, 0, NULL, NULL) );
            CL_EXEC( clEnqueueWriteBuffer(cl_data->command_queue, buffer_solver, CL_TRUE, 0, sizeof(solver_struct),
                                                                        &this->getSolverStruct(), 0, NULL, NULL) );
            cl_mem buffer_queue = NULL;
            if (_options.use_queue())
            {
                buffer_queue = CreateBuffer (&cl_data->context, CL_MEM_READ_WRITE, sizeof(int), NULL);
                int queue_val = 0;
                CL_EXEC( clEnqueueWriteBuffer(cl_data->command_queue, buffer_queue, CL_TRUE, 0, sizeof(int),
                                                                            &queue_val, 0, NULL, NULL) )
                std::cout << "Queue enabled" << std::endl;
            }

            std::cout << "Host->Dev + alloc = " <<
                std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::high_resolution_clock::now() - t_data).count() <<
                " (ms)" << std::endl;;

            /* Set kernel argument */
            int argc = 0;
            CL_EXEC( clSetKernelArg(kernel, argc++, sizeof(cl_mem), &buffer_param) );
            CL_EXEC( clSetKernelArg(kernel, argc++, sizeof(cl_mem), &t0) );
            CL_EXEC( clSetKernelArg(kernel, argc++, sizeof(cl_mem), &tf) );
            CL_EXEC( clSetKernelArg(kernel, argc++, sizeof(cl_mem), &buffer_phi) );
            CL_EXEC( clSetKernelArg(kernel, argc++, sizeof(cl_mem), &buffer_solver) );
            CL_EXEC( clSetKernelArg(kernel, argc++, sizeof(cl_mem), &buffer_rwk) );
            CL_EXEC( clSetKernelArg(kernel, argc++, sizeof(cl_mem), &buffer_counters) );
            CL_EXEC( clSetKernelArg(kernel, argc++, sizeof(int), &NUM) );
            if (_options.use_queue())
            {
                CL_EXEC( clSetKernelArg(kernel, argc++, sizeof(cl_mem), &buffer_queue) );
            }


            /* Execute kernel */
            auto tk = std::chrono::high_resolution_clock::now();
            cl_event ev;
            CL_EXEC( clEnqueueNDRangeKernel (cl_data->command_queue, kernel,
                                             1 /* work-group dims */,
                                             NULL /* offset */,
                                             &numThreads /* global work size */,
                                             &cl_data->blockSize /* local work-group size */,
                                             0, NULL, /* wait list */
                                             &ev /* this kernel's event */) );
            /* Wait for the kernel to finish */
            clWaitForEvents(1, &ev);
            auto tk_end = std::chrono::high_resolution_clock::now();
            std::cout << "Kernel execution = " <<
                std::chrono::duration_cast<std::chrono::milliseconds>(tk, tk_end).count() <<
                " (ms)" << std::endl;


            t_data = std::chrono::high_resolution_clock::now();
            /* copy out */
            CL_EXEC( clEnqueueReadBuffer(cl_data->command_queue, buffer_phi, CL_TRUE, 0, sizeof(double)*neq*numProblems, phi, 0, NULL, NULL) );

            counter_struct* counters = (counter_struct*) malloc(sizeof(counter_struct)*numProblems);
            if (counters == NULL)
            {
                fprintf(stderr,"Allocation error %s %d\n", __FILE__, __LINE__);
                exit(-1);
            }
            CL_EXEC( clEnqueueReadBuffer(cl_data->command_queue, buffer_counters, CL_TRUE, 0, sizeof(counter_struct)*numProblems, counters, 0, NULL, NULL) );

            int nst_ = 0, nit_ = 0;
            for (int i = 0; i < numProblems; ++i)
            {
                nst_ += counters[i].nsteps;
                nit_ += counters[i].niters;
            }
            std::cout << "nst = " << nst_ << ", nit = " << nit_ << std::endl;

            clReleaseKernel(kernel);
            clReleaseMemObject (buffer_param);
            clReleaseMemObject (t0);
            clReleaseMemObject (tf);
            clReleaseMemObject (buffer_phi);
            clReleaseMemObject (buffer_solver);
            clReleaseMemObject (buffer_rwk);
            clReleaseMemObject (buffer_counters);

            free(counters);
            free(u_out);

            return 0;
        }

    protected:
        //! \brief return a reference to the initialized solver_struct
        virtual const solver_struct& getSolverStruct() const = 0;

    };

}

 #endif
