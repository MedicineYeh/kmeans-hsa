#define checkErr(fun, statement) err = fun;\
                                 if (err != CL_SUCCESS) {statement}
#define checkExit(value, message) if (value == 0) {printf(message); goto release;}

cl_int err = 0;
cl_uint num = 0;
cl_platform_id *platforms = NULL;
cl_context_properties prop[3] = {0};
cl_context context = 0;
cl_device_id *devices = NULL;
cl_command_queue queue = 0;
cl_program program = 0;
cl_mem cl_data = 0, cl_cent = 0, cl_table = 0, 
       cl_chpt = 0, cl_cent_c = 0, cl_min_dis = 0;
cl_kernel kernel = 0;
cl_event event;

int num_total_devices = 0;
char devname[16][256] = {{0}};

cl_program load_program(cl_context context, cl_device_id device, const char* filename)
{
    FILE *fp = fopen(filename, "rt");
    size_t length;
    char *data;
    char *build_log;
    size_t ret_val_size;
    cl_program program = 0;
    cl_int status = 0;

    if(!fp) return 0;

    // get file length
    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // read program source
    data = (char *)malloc(length + 1);
    fread(data, sizeof(char), length, fp);
    data[length] = '\0';

    // create and build program 
    program = clCreateProgramWithSource(context, 1, (const char **)&data, 0, 0);
    if (program == 0) return 0;

    status = clBuildProgram(program, 0, 0, 0, 0, 0);
    if (status != CL_SUCCESS) {
        printf("Error:  Building Program from file %s\n", filename);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &ret_val_size);
        build_log = (char *)malloc(ret_val_size + 1);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, ret_val_size, build_log, NULL);
        build_log[ret_val_size] = '\0';
        printf("Building Log:\n%s", build_log);
        return 0;
    }

    return program;
}

void initial_kernel()
{
    size_t cb;
    int i;

    checkErr(clGetPlatformIDs(0, 0, &num), 
            printf("Unable to get platforms\n");
            exit(1);
            );

    platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id) * num);
    checkErr(clGetPlatformIDs(num, platforms, NULL), 
            printf("Unable to get platform ID\n");
            exit(1);
            );

    checkErr(clGetPlatformIDs(0, 0, &num), 
            printf("Unable to get platforms\n");
            exit(1);
            );

    prop[0] = CL_CONTEXT_PLATFORM;
    prop[1] = (cl_context_properties)platforms[0];
    prop[2] = 0;
    context = clCreateContextFromType(prop, CL_DEVICE_TYPE_ALL, NULL, NULL, NULL);
    checkExit(context, "Can't create OpenCL context\n");

    clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &cb);
    devices = (cl_device_id *)malloc(cb);
    clGetContextInfo(context, CL_CONTEXT_DEVICES, cb, devices, 0);
    checkExit(cb, "Can't get devices\n");
    num_total_devices = cb / sizeof(cl_device_id);

    for (i = 0; i < num_total_devices; i++) {
        clGetDeviceInfo(devices[i], CL_DEVICE_NAME, 0, NULL, &cb);
        clGetDeviceInfo(devices[i], CL_DEVICE_NAME, cb, devname, 0);
        printf("Device(%d/%d): %s\n", i, num_total_devices, devname[i]);
    }

    queue = clCreateCommandQueue(context, devices[0], 0, 0);
    checkExit(queue, "Can't create command queue\n");

    program = load_program(context, devices[0], "shader.cl");
    checkExit(program, "Fail to build program\n");

    kernel = clCreateKernel(program, "cal_dis", &err);
    if (err == CL_INVALID_KERNEL_NAME) printf("CL_INVALID_KERNEL_NAME\n");
    checkExit(kernel, "Can't load kernel\n");
/*
    cl_data  = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR , sizeof(float)*DCNT*DIM  , &data_ker[0], NULL);
    cl_cent  = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR , sizeof(float)*K*DIM  , &cent_ker[0], NULL);
    cl_table = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR , sizeof(int)*DCNT, &table[0], NULL);
    cl_k = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_int), &k_ker, NULL);
    cl_dim = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_int), &dim_ker, NULL);
    cl_dcnt = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_int), &dcnt_ker, NULL);
    cl_chpt = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(int)*DCNT, &chpt[0], NULL);
    cl_cent_c = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(int)*DCNT, &cent_c_ker[0], NULL);
    cl_min_dis = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(float)*DCNT, &min_dis[0], NULL);  
*/
    cl_data    = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float)*DCNT*DIM  , NULL, NULL);
    cl_cent    = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float)*K*DIM  , NULL, NULL);
    cl_table   = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int)*DCNT, NULL, NULL);
    cl_chpt    = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int)*DCNT, NULL, NULL);
    cl_cent_c  = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int)*DCNT, NULL, NULL);
    cl_min_dis = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float)*DCNT, NULL, NULL);  

    if (    !cl_data || 
            !cl_cent || 
            !cl_table ||
            !cl_chpt ||
            !cl_cent_c ||
            !cl_min_dis) {
        printf("Error: Failed to allocate device memory!\n");
        goto release;
    }

    err = 0;
    err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &cl_data);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &cl_cent);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &cl_table);
    err |= clSetKernelArg(kernel, 6, sizeof(cl_mem), &cl_chpt);
    err |= clSetKernelArg(kernel, 7, sizeof(cl_mem), &cl_cent_c);
    err |= clSetKernelArg(kernel, 8, sizeof(cl_mem), &cl_min_dis);

    if (err != CL_SUCCESS) {
        printf("Error: Failed to set kernel arguments! %d\n", err);
        exit(1);
    }

    return ;

release:
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseMemObject(cl_data);
    clReleaseMemObject(cl_cent);
    clReleaseMemObject(cl_table);
    clReleaseMemObject(cl_chpt);
    clReleaseMemObject(cl_cent_c);
    clReleaseMemObject(cl_min_dis);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    return ;
}

// ------------------------------------
// 更新table, 傳回sse, 存入點之變動數
float update_table_cl(int* ch_pt)
{
    int i, j, k, min_k;
    float dis,  t_sse=0.0;
    float data_ker[DCNT*DIM];
    float cent_ker[K*DIM];
    int cent_c_ker[DCNT];
    //float min_dis;
    int chpt[DCNT];
    float min_dis[DCNT];
    unsigned int k_ker=K,dim_ker=DIM,dcnt_ker=DCNT;

    *ch_pt=0;                          // 變動點數設0
    memset(cent_c, 0, sizeof(cent_c)); // 各叢聚資料數清0
    memset(dis_k, 0, sizeof(dis_k));   // 各叢聚距離和清0
    /*OpenCL*/
    size_t work_size;

    for (i=0; i<DCNT;++i) {
        for(k=0; k<DIM;++k) {
            data_ker[i*DIM+k]=data[i][k];
        }
    }
    for (i=0; i<K;++i) {
        for(k=0; k<DIM;++k) {
            cent_ker[i*DIM+k]=cent[i][k];
        }
    }

    err = 0;
    err |= clEnqueueWriteBuffer(queue, cl_data, CL_TRUE, 0, 
                                sizeof(float) * DCNT * DIM, data_ker, 0, 0, 0);
    err |= clEnqueueWriteBuffer(queue, cl_cent, CL_TRUE, 0, 
                                sizeof(float) * K * DIM, cent_ker, 0, 0, 0);
    err |= clEnqueueWriteBuffer(queue, cl_table, CL_TRUE, 0, 
                                sizeof(int) * DCNT, table, 0, 0, 0);

    err |= clSetKernelArg(kernel, 3, sizeof(unsigned int), &k_ker);
    err |= clSetKernelArg(kernel, 4, sizeof(unsigned int), &dim_ker);
    err |= clSetKernelArg(kernel, 5, sizeof(unsigned int), &dcnt_ker);

    err |= clEnqueueWriteBuffer(queue, cl_chpt, CL_TRUE, 0, 
                                sizeof(int) * DCNT, chpt, 0, 0, 0);
    err |= clEnqueueWriteBuffer(queue, cl_cent_c, CL_TRUE, 0, 
                                sizeof(int) * DCNT, cent_c_ker, 0, 0, 0);
    err |= clEnqueueWriteBuffer(queue, cl_min_dis, CL_TRUE, 0, 
                                sizeof(float) * DCNT, min_dis, 0, 0, 0);
    if (err) {
        printf("ERROR! WRITE BUFFER\n");
        exit(1);
    }

    work_size = DCNT;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, 0, &work_size, 0, 0, 0, 0);

    if (err == CL_SUCCESS) {
        err |= clEnqueueReadBuffer(queue, cl_chpt, CL_TRUE, 0, sizeof(cl_int)*DCNT, &chpt[0], 0, 0, 0);
        err |= clEnqueueReadBuffer(queue, cl_table, CL_TRUE, 0, sizeof(cl_int)*DCNT, &table[0], 0, 0, 0);
        err |= clEnqueueReadBuffer(queue, cl_cent_c, CL_TRUE, 0, sizeof(cl_int)*DCNT, &cent_c_ker[0], 0, 0, 0);
        err |= clEnqueueReadBuffer(queue, cl_min_dis, CL_TRUE, 0, sizeof(cl_float)*DCNT, &min_dis[0], 0, 0, 0);
    }
    for(i=0;i<DCNT;i++) {
        *ch_pt+=chpt[i];
        ++cent_c[cent_c_ker[i]];
        t_sse+=min_dis[i];
        for(j=0;j<DIM;j++) {
            dis_k[table[i]][j]+=data[i][j];
        }
    }

    return t_sse;
}

// ------------------------------------
// 更新重心位置
void update_cent_cl()
{
    int k, j;
    for(k=0; k<K; ++k)
        for(j=0; j<DIM; ++j)
            cent[k][j]=dis_k[k][j]/cent_c[k];
}

// ------------------------------------
// 顯示重心位置
void print_cent_cl()
{
    int j, k;
    for(k=0; k<K; ++k) {
        for(j=0; j<DIM; ++j)
            printf("%6.2lf ", cent[k][j]);
        putchar('\n');
    }
}


