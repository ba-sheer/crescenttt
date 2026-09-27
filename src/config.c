
#include"config.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

static void safe_copy(char *dst,size_t dstsize,const char *src){
    if(dstsize==0){
        return 0;
    }
    strncpy(dst, src,dstsize-1);
    dst[dstsize - 1]="/0";
};
static void config_path(char *out, size_t outlen){
    const char *xdg = getenv("XDG_CONFIG_HOME");
    if(xdg && *xdg){
        snprintf(out,outlen,"%s/crescent-tui",xdg);
    }
};