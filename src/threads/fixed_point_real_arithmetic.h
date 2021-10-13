#ifndef FIXED_POINT
#define FIXED_POINT
#define F (1<<14)

int convert_fp(int n);
int x_multi_y(int x,int y);
int x_divide_y(int x,int y);
int round_nearest(int x);


int convert_fp(int n)
{
    return n*F;
}

int x_multi_y(int x,int y)
{
    return (((int64_t)x)*y)/F;
}

int x_divide_y(int x,int y)
{
    return ((((int64_t)x)*F)/y);
}

int round_nearest(int x)
{
    if(x>=0)
    {
        return (x+F/2)/F;
    }
    else
    {
        return (x-F/2)/F;
    }
}

#endif