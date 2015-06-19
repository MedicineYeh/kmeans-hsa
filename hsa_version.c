// ------------------------------------
// variable
float data[N_DCNT][N_DIM]; /* 原始資料   */
float cent[N_K][N_DIM]; /* 重心       */
float dis_k[N_K][N_DIM];   /* 叢聚距離   */
int table[N_DCNT];        /* 資料所屬叢聚*/
int cent_c[N_K];          /* 該叢聚資料數*/
int cent_c_ker[N_DCNT];
float min_dis[N_DCNT];
int chpt[N_DCNT];

#define LOCAL_SIZE 512
//#define SLEEP_TIME 50000 //這個數字是個magic number，總之要等到agent的資料同步回來才行，找找看有沒有API取代usleep，以及可以多把其他部份也放到GPU上面做，因為都在GPU的話已經測試過是可以不用等待直接enqueue就可以了
#define SLEEP_TIME 6000 //這個數字是個magic number 2號，想要知道最快可以多快，結果雖然會錯但誤差不會太多。

hsa_queue_t* queue; 
hsa_signal_t signal;
uint64_t kernel_object;
uint32_t kernarg_segment_size;
uint32_t group_segment_size;
uint32_t private_segment_size;
void* kernarg_address = NULL;

#define check(msg, status)\
if (status != HSA_STATUS_SUCCESS) { \
    printf("%s failed.\n", #msg); \
    exit(1); \
}
/*
} else { \
    printf("%s succeed.\n", #msg); \
}
*/

/*
 * Loads a BRIG module from a specified file. This
 * function does not validate the module.
 */
int load_module_from_file(const char* file_name, hsa_ext_module_t* module) {
    int rc = -1;

    FILE *fp = fopen(file_name, "rb");
    rc = fseek(fp, 0, SEEK_END);

    size_t file_size = (size_t) (ftell(fp) * sizeof(char));

    rc = fseek(fp, 0, SEEK_SET);

    char* buf = (char*) malloc(file_size);

    memset(buf,0,file_size);

    size_t read_size = fread(buf,sizeof(char),file_size,fp);

    if(read_size != file_size) {
        free(buf);
    } else {
        rc = 0;
        *module = (hsa_ext_module_t) buf;
    }

    fclose(fp);

    return rc;
}

/*
 * Determines if the given agent is of type HSA_DEVICE_TYPE_GPU
 * and sets the value of data to the agent handle if it is.
 */
static hsa_status_t get_gpu_agent(hsa_agent_t agent, void *data) {
    hsa_status_t status;
    hsa_device_type_t device_type;
    status = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &device_type);
    if (HSA_STATUS_SUCCESS == status && HSA_DEVICE_TYPE_GPU == device_type) {
        hsa_agent_t* ret = (hsa_agent_t*)data;
        *ret = agent;
        return HSA_STATUS_INFO_BREAK;
    }
    return HSA_STATUS_SUCCESS;
}

/*
 * Determines if a memory region can be used for kernarg
 * allocations.
 */
static hsa_status_t get_kernarg_memory_region(hsa_region_t region, void* data) {
    hsa_region_segment_t segment;
    hsa_region_get_info(region, HSA_REGION_INFO_SEGMENT, &segment);
    if (HSA_REGION_SEGMENT_GLOBAL != segment) {
        return HSA_STATUS_SUCCESS;
    }

    hsa_region_global_flag_t flags;
    hsa_region_get_info(region, HSA_REGION_INFO_GLOBAL_FLAGS, &flags);
    if (flags & HSA_REGION_GLOBAL_FLAG_KERNARG) {
        hsa_region_t* ret = (hsa_region_t*) data;
        *ret = region;
        return HSA_STATUS_INFO_BREAK;
    }

    return HSA_STATUS_SUCCESS;
}

void initial_kernel()
{
    hsa_status_t err;
    err = hsa_init();
    check(Initializing the hsa runtime, err);

    /* 
     * Iterate over the agents and pick the gpu agent using 
     * the get_gpu_agent callback.
     */
    hsa_agent_t agent;
    err = hsa_iterate_agents(get_gpu_agent, &agent);
    if(err == HSA_STATUS_INFO_BREAK) { err = HSA_STATUS_SUCCESS; }
    check(Getting a gpu agent, err);

    /*
     * Query the name of the agent.
     */
    char name[64] = { 0 };
    err = hsa_agent_get_info(agent, HSA_AGENT_INFO_NAME, name);
    check(Querying the agent name, err);
    fprintf(stderr, "The agent name is %s.\n", name);

    /*
     * Query the maximum size of the queue.
     */
    uint32_t queue_size = 0;
    err = hsa_agent_get_info(agent, HSA_AGENT_INFO_QUEUE_MAX_SIZE, &queue_size);
    check(Querying the agent maximum queue size, err);
    fprintf(stderr, "The maximum queue size is %u.\n", (unsigned int) queue_size);

    /*
     * Create a queue using the maximum size.
     */
    err = hsa_queue_create(agent, queue_size, HSA_QUEUE_TYPE_SINGLE, NULL, NULL, UINT32_MAX, UINT32_MAX, &queue);
    check(Creating the queue, err);

    /*
     * Load the BRIG binary.
     */
    hsa_ext_module_t module;
    load_module_from_file("shader_hsa.brig",&module);

    /*
     * Create hsa program.
     */
    hsa_ext_program_t program;
    memset(&program,0,sizeof(hsa_ext_program_t));
    err = hsa_ext_program_create(HSA_MACHINE_MODEL_LARGE, HSA_PROFILE_FULL, HSA_DEFAULT_FLOAT_ROUNDING_MODE_DEFAULT, NULL, &program);
    check(Create the program, err);

    /*
     * Add the BRIG module to hsa program.
     */
    err = hsa_ext_program_add_module(program, module);
    check(Adding the brig module to the program, err);

    /*
     * Determine the agents ISA.
     */
    hsa_isa_t isa;
    err = hsa_agent_get_info(agent, HSA_AGENT_INFO_ISA, &isa);
    check(Query the agents isa, err);

    /*
     * Finalize the program and extract the code object.
     */
    hsa_ext_control_directives_t control_directives;
    memset(&control_directives, 0, sizeof(hsa_ext_control_directives_t));
    hsa_code_object_t code_object;
    err = hsa_ext_program_finalize(program, isa, 0, control_directives, "", HSA_CODE_OBJECT_TYPE_PROGRAM, &code_object);
    check(Finalizing the program, err);

    /*
     * Destroy the program, it is no longer needed.
     */
    err=hsa_ext_program_destroy(program);
    check(Destroying the program, err);

    /*
     * Create the empty executable.
     */
    hsa_executable_t executable;
    err = hsa_executable_create(HSA_PROFILE_FULL, HSA_EXECUTABLE_STATE_UNFROZEN, "", &executable);
    check(Create the executable, err);

    /*
     * Load the code object.
     */
    err = hsa_executable_load_code_object(executable, agent, code_object, "");
    check(Loading the code object, err);

    /*
     * Freeze the executable; it can now be queried for symbols.
     */
    err = hsa_executable_freeze(executable, "");
    check(Freeze the executable, err);

   /*
    * Extract the symbol from the executable.
    */
    hsa_executable_symbol_t symbol;
    err = hsa_executable_get_symbol(executable, "", "&__OpenCL_cal_diskernel_kernel", agent, 0, &symbol);
    check(Extract the symbol from the executable, err);

    /*
     * Extract dispatch information from the symbol
     */
    err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT, &kernel_object);
    check(Extracting the symbol from the executable, err);
    err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_SIZE, &kernarg_segment_size);
    check(Extracting the kernarg segment size from the executable, err);
    err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_GROUP_SEGMENT_SIZE, &group_segment_size);
    check(Extracting the group segment size from the executable, err);
    err = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_PRIVATE_SEGMENT_SIZE, &private_segment_size);
    check(Extracting the private segment from the executable, err);

    /*
     * Create a signal to wait for the dispatch to finish.
     */ 
    err=hsa_signal_create(1, 0, NULL, &signal);
    check(Creating a HSA signal, err);

    /*
     * Allocate and initialize the kernel arguments and data.
     */
    err |= hsa_memory_register(data, sizeof(float)*N_DCNT*N_DIM);
    err |= hsa_memory_register(cent, sizeof(float)*N_K*N_DIM);
    err |= hsa_memory_register(table, sizeof(int)*N_DCNT);

    err |= hsa_memory_register(chpt, sizeof(int)*N_DCNT);
    err |= hsa_memory_register(cent_c, sizeof(int)*N_DCNT);
    err |= hsa_memory_register(min_dis, sizeof(float)*N_DCNT);

    check(Registering argument memory for output parameter, err);

    struct __attribute__ ((aligned(16))) args_t {
        uint64_t global_offset_0;
    	uint64_t global_offset_1;
    	uint64_t global_offset_2;
    	uint64_t printf_buffer;
    	uint64_t vqueue_pointer;
    	uint64_t aqlwrap_pointer;

        void * data_ker;
        void * cent_ker;
        void * table;
        unsigned int K;
        unsigned int DIM;
        unsigned int DCNT;
        void * chpt;
        void * cent_c_ker;
        void * min_dis;
    } args;
    memset(&args, 0, sizeof(args));
    args.data_ker = data;
    args.cent_ker = cent;
    args.table = table;
    args.K = N_K;
    args.DIM = N_DIM;
    args.DCNT = N_DCNT;
    args.chpt = chpt;
    args.cent_c_ker = cent_c_ker;
    args.min_dis = min_dis;

    /*
     * Find a memory region that supports kernel arguments.
     */
    hsa_region_t kernarg_region;
    kernarg_region.handle=(uint64_t)-1;
    hsa_agent_iterate_regions(agent, get_kernarg_memory_region, &kernarg_region);
    err = (kernarg_region.handle == (uint64_t)-1) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;
    check(Finding a kernarg memory region, err);

    /*
     * Allocate the kernel argument buffer from the correct region.
     */   
    err = hsa_memory_allocate(kernarg_region, kernarg_segment_size, &kernarg_address);
    check(Allocating kernel argument memory buffer, err);
    memcpy(kernarg_address, &args, sizeof(args));
}

void hsa_enqueue()
{
    hsa_status_t err = 0;
    /*
     * Obtain the current queue write index.
     */
    uint64_t index = hsa_queue_load_write_index_relaxed(queue);

    /*
     * Write the aql packet at the calculated queue index address.
     */
    const uint32_t queueMask = queue->size - 1;
    hsa_kernel_dispatch_packet_t* dispatch_packet = &(((hsa_kernel_dispatch_packet_t*)(queue->base_address))[index&queueMask]);

    dispatch_packet->header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE;
    dispatch_packet->header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE;
    dispatch_packet->setup  |= 1 << HSA_KERNEL_DISPATCH_PACKET_SETUP_DIMENSIONS;
    dispatch_packet->workgroup_size_x = (uint16_t)LOCAL_SIZE;
    dispatch_packet->workgroup_size_y = (uint16_t)1;
    dispatch_packet->workgroup_size_z = (uint16_t)1;
    dispatch_packet->grid_size_x = (uint32_t) (N_DCNT);
    dispatch_packet->grid_size_y = 1;
    dispatch_packet->grid_size_z = 1;
    dispatch_packet->completion_signal = signal;
    dispatch_packet->kernel_object = kernel_object;
    dispatch_packet->kernarg_address = (void*) kernarg_address;
    dispatch_packet->private_segment_size = private_segment_size;
    dispatch_packet->group_segment_size = group_segment_size;
    __atomic_store_n((uint8_t*)(&dispatch_packet->header), (uint8_t)HSA_PACKET_TYPE_KERNEL_DISPATCH, __ATOMIC_RELEASE);

    /*
     * Increment the write index and ring the doorbell to dispatch the kernel.
     */
    hsa_queue_store_write_index_relaxed(queue, index+1);
    hsa_signal_store_relaxed(queue->doorbell_signal, index);
    check(Dispatching the kernel, err);

    /*
     * Wait on the dispatch completion signal until the kernel is finished.
     */
    hsa_signal_wait_acquire(signal, HSA_SIGNAL_CONDITION_LT, 1, UINT64_MAX, HSA_WAIT_STATE_BLOCKED);


}

void kmeans_main()
{
    int    ch_pt = 0;      /* 紀錄變動之點 */
    int    iter = 0;       /* 迭代計數器   */
    float  sse1 = 0.0;     /* 上一迭代之sse */
    float  sse2 = 0.0;     /* 此次迭代之sse */
    float  t_sse = 0.0;    /* 此次迭代之sse */
    int i, j;
    struct timespec timer_1, timer_2;

    tic(&timer_1);
    initial_kernel();
    toc("HSA initial Time", &timer_1, &timer_2);

    tic(&timer_1);
    ch_pt=0;
    memset(cent_c, 0, sizeof(cent_c)); // 各叢聚資料數清0
    memset(dis_k, 0, sizeof(dis_k));   // 各叢聚距離和清0

    // step 3 - 更新重心
    hsa_enqueue();
    usleep(SLEEP_TIME);
    // step 4 - 更新對應表
    for(i=0;i<N_DCNT;i++) {
        ch_pt+=chpt[i];
        ++cent_c[cent_c_ker[i]];
        t_sse+=min_dis[i];
        for(j=0;j<N_DIM;j++) {
            dis_k[table[i]][j]+=data[i][j];
        }
    }
    sse2 = t_sse;
    do {
        sse1 = sse2, ++iter;
        update_cent();             /* step 3 - 更新重心            */

        t_sse=0.0;
        ch_pt=0;
        memset(cent_c, 0, sizeof(cent_c)); // 各叢聚資料數清0
        memset(dis_k, 0, sizeof(dis_k));   // 各叢聚距離和清0
        // step 3 - 更新重心
        hsa_enqueue();
        usleep(SLEEP_TIME);
        // step 4 - 更新對應表
        for(i=0;i<N_DCNT;i++) {
            ch_pt+=chpt[i];
            ++cent_c[cent_c_ker[i]];
            t_sse+=min_dis[i];
            for(j=0;j<N_DIM;j++) {
                dis_k[table[i]][j]+=data[i][j];
            }
        }
        sse2=t_sse;
    }while(iter<MAX_ITER && sse1!=sse2 && ch_pt>MIN_PT); // 收斂條件
    toc("HSA Execution Time", &timer_1, &timer_2);
    
    print_cent(); // 顯示最後重心位置
    printf("sse   = %.2lf\n", sse2);
    printf("ch_pt = %d\n", ch_pt);
    printf("iter  = %d\n", iter);

}

