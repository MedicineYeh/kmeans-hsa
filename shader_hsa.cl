__kernel void cal_diskernel(
        __global float * data_ker,
        __global float * cent_ker,
        __global int * table,
        const unsigned int K,
        const unsigned int DIM,
        const unsigned int DCNT,
        __global int * chpt,
        __global int * cent_c_ker,
        __global float * min_dis)
{
	int core = get_global_id(0);
	int worksize = get_global_size(0);
	int i,j,k,l,m,n,o,min_k;
	float t ,min_sum=0.0; 

	min_k=0;
	for(i=0;i<DIM;i++)
	{
		t=data_ker[core*DIM+i]-cent_ker[i],min_sum+=t*t;
	}
	
	for(j=1;j<K;j++)
	{
		float sum=0.0;
		for(k=0;k<DIM;k++)
		{
			t=data_ker[core*DIM+k]-cent_ker[j*DIM+k],sum+=t*t;

		}
		if(sum<min_sum)
		{
			min_sum=sum; min_k=j;
		}
	}

	chpt[core]=(table[core]!=min_k);
	table[core]=min_k;
	cent_c_ker[core]=min_k;
	min_dis[core]=min_sum;
	
}
