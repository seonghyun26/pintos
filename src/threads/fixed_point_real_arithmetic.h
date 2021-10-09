#define F (1<<14)

int x_multi_y(int x,int y);
int x_divide_y(int x,int y);

int x_multi_y(int x,int y)
{
    return (((int64_t)x)*y)/F;
}

int x_divide_y(int x,int y)
{
    return ((((int64_t)x)*F)/y);
}

