#include "myHeader.h"

int **y, *v, **z, *u, *x, w, t;

int main(){
    y = &v;
    z = &u;
    x = &t;
    isPointingTo(t,x);
    isPointingTo(y,v);
    if(*x > w)
    {
        x = &w;
    }
    else
    {
        x = *y;
    }
    while(w>0)
    {
        isLive(y);
        isLive(u);
        isLive(z);
        isLive(x);
        x = *y;
        z = &v;

    }
    return **z;
}
