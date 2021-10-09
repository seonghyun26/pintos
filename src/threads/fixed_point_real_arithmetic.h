#define F (1<<14)
#define x_multi_y(x,y) ((((int64_t)(x))*(y))/F)
#define x_divide_y(x,y) ((((int64_t)(x))*(F))/(y))
#define x_divide_n(x,y) (((x)/(y))/F)