#include "myHeader.h"

int **y, *v, **z, *u, *x, w;

void func(int* b)
{
    x = &w;
    isLive(x);
    z = y;
}

int main(){
    y = &v;
    z = &u;
    isLive(y);
    func(u);
    *z = x;
    isPointingTo(z,u);
    isPointingTo(z,v);
    isPointingTo(v,w);
    return **z;
}
