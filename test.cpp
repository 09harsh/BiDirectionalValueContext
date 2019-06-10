#include "myHeader.h"

int **y, *v, **z, *u, *x, w, b;

void func()
{
    if(w>0)
        func();
    x = u;
}

int main(){
    y = &v;
    z = &u;
    v = &b;
    isLive(x);
    isLive(v);
    isPointingTo(v,b);
    while(w>0)
    {
        func();
        u = *y;
        isPointingTo(u,b);
    }

    return *x;
}
