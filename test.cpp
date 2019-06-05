#include "myHeader.h"

int **y, *v, **z, *u, *x, w;

int main(){
    isLive(u);
    y = &v;
    z = &u;
    x = &w;
    isLive(z);
    if(w)
    {
        isPointingTo(y,v);
        isPointingTo(z,u);
        **z = *x;
    }
    else
    {
        z = y;
    }
    isPointingTo(z,u);
    isPointingTo(u,w);
    isLive(u);
    return *u;
}
