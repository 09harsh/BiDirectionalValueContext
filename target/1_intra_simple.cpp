#include "myHeader.h"

int **y, *v, **z, *u, *x, w;

int main(){
    isLive(u);
    y = &v;
    z = &u;
    x = &w;
    isLive(u);
    isLive(z);
    isPointingTo(z,u);
    *z = x;
    return 0;
}
