#define _POSIX_C_SOURCE 200809L
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include<dirent.h>
#define MKDIR(path) _mkdir(path)
#else
#define MKDIR(path)mkdir(path,0700)
#endif
#include <sys/types.h>
#include <unistd.h>

static void safe_copy(char *dst, size_t dstsize, const char *src) {
    if(dstsize == 0) return;
    if(!src) src = "";
    snprintf(dst,dstsize,"%s",src);
}

static void config_path(char *out, size_t outlen) {
    const char *xdg = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");
    if (xdg && *xdg) {
        snprintf(out, outlen, "%s/crescent-tui", xdg);
    } else {
        snprintf(out, outlen, "%s/.config/crescent-tui", home ? home : ".");
    }
}

void config_load(crescent_config *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    safe_copy(cfg->region, sizeof(cfg->region), "US");

    char dir[512];
    config_path(dir, sizeof(dir));
    char file[600];
    snprintf(file, sizeof(file), "%s/config", dir);

    FILE *f = fopen(file, "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            char *nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            char *eq = strchr(line, '=');
            if (!eq) continue;
            *eq = '\0';
            const char *key = line;
            const char *val = eq + 1;
            if (strcmp(key, "api_key") == 0)
                safe_copy(cfg->api_key, sizeof(cfg->api_key), val);
            else if (strcmp(key, "region") == 0 && *val)
                safe_copy(cfg->region, sizeof(cfg->region), val);
        }
        fclose(f);
    }

    const char *env_key = getenv("CRESCENT_API_KEY");
    if (env_key && *env_key)
        safe_copy(cfg->api_key, sizeof(cfg->api_key), env_key);
}

int config_save(const crescent_config *cfg) {
    char dir[512];
    config_path(dir, sizeof(dir));

    char tmp[512];
    safe_copy(tmp, sizeof(tmp), dir);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp);
            *p = '/';
        }
    }
    mkdir(tmp);

    char file[600];
    snprintf(file, sizeof(file), "%s/config", dir);

    FILE *f = fopen(file, "w");
    if (!f) return -1;
    fprintf(f, "api_key=%s\n", cfg->api_key);
    fprintf(f, "region=%s\n", cfg->region);
    fclose(f);
    chmod(file, S_IRUSR | S_IWUSR);
    return 0;
}