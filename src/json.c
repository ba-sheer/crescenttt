//remember to mention usage of AI for debuging and parsing implementation(for myself)-shad
#define _POSIX_C_SOURCE 200809L /* strdup */
#include "json.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

typedef struct {
    const char *p;
    const char *start;
} parser;

static void skip_ws(parser *ps) {
    while (*ps->p == ' ' || *ps->p == '\t' || *ps->p == '\n' || *ps->p == '\r')
        ps->p++;
}

static json_value *new_value(json_type t) {
    json_value *v = calloc(1, sizeof(json_value));
    v->type = t;
    return v;
}

static int fail(char **err, parser *ps, const char *msg) {
    if (err) {
        long off = (long)(ps->p - ps->start);
        char buf[160];
        snprintf(buf, sizeof(buf), "JSON parse error at offset %ld: %s", off, msg);
        *err = strdup(buf);
    }
    return 0;
}

static json_value *parse_value(parser *ps, char **err);

/* Append a UTF-8 encoding of a Unicode code point to a growable buffer. */
static void append_utf8(char **buf, size_t *len, size_t *cap, unsigned int cp) {
    char tmp[4];
    int n = 0;
    if (cp <= 0x7F) {
        tmp[0] = (char)cp; n = 1;
    } else if (cp <= 0x7FF) {
        tmp[0] = (char)(0xC0 | (cp >> 6));
        tmp[1] = (char)(0x80 | (cp & 0x3F));
        n = 2;
    } else if (cp <= 0xFFFF) {
        tmp[0] = (char)(0xE0 | (cp >> 12));
        tmp[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        tmp[2] = (char)(0x80 | (cp & 0x3F));
        n = 3;
    } else {
        tmp[0] = (char)(0xF0 | (cp >> 18));
        tmp[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        tmp[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        tmp[3] = (char)(0x80 | (cp & 0x3F));
        n = 4;
    }
    if (*len + (size_t)n + 1 > *cap) {
        *cap = (*cap + n + 1) * 2;
        *buf = realloc(*buf, *cap);
    }
    memcpy(*buf + *len, tmp, (size_t)n);
    *len += (size_t)n;
}

static unsigned int hex4(const char *p) {
    unsigned int v = 0;
    for (int i = 0; i < 4; i++) {
        char c = p[i];
        v <<= 4;
        if (c >= '0' && c <= '9') v |= (unsigned int)(c - '0');
        else if (c >= 'a' && c <= 'f') v |= (unsigned int)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') v |= (unsigned int)(c - 'A' + 10);
    }
    return v;
}

static char *parse_string_raw(parser *ps, char **err) {
    if (*ps->p != '"') { fail(err, ps, "expected string"); return NULL; }
    ps->p++;
    size_t cap = 32, len = 0;
    char *buf = malloc(cap);
    while (*ps->p && *ps->p != '"') {
        unsigned char c = (unsigned char)*ps->p;
        if (c == '\\') {
            ps->p++;
            switch (*ps->p) {
                case '"': append_utf8(&buf, &len, &cap, '"'); ps->p++; break;
                case '\\': append_utf8(&buf, &len, &cap, '\\'); ps->p++; break;
                case '/': append_utf8(&buf, &len, &cap, '/'); ps->p++; break;
                case 'b': append_utf8(&buf, &len, &cap, '\b'); ps->p++; break;
                case 'f': append_utf8(&buf, &len, &cap, '\f'); ps->p++; break;
                case 'n': append_utf8(&buf, &len, &cap, '\n'); ps->p++; break;
                case 'r': append_utf8(&buf, &len, &cap, '\r'); ps->p++; break;
                case 't': append_utf8(&buf, &len, &cap, '\t'); ps->p++; break;
                case 'u': {
                    ps->p++;
                    unsigned int cp = hex4(ps->p);
                    ps->p += 4;
                    if (cp >= 0xD800 && cp <= 0xDBFF && ps->p[0] == '\\' && ps->p[1] == 'u') {
                        unsigned int lo = hex4(ps->p + 2);
                        if (lo >= 0xDC00 && lo <= 0xDFFF) {
                            cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                            ps->p += 6;
                        }
                    }
                    append_utf8(&buf, &len, &cap, cp);
                    break;
                }
                default:
                    free(buf);
                    fail(err, ps, "bad escape");
                    return NULL;
            }
        } else {
            append_utf8(&buf, &len, &cap, c);
            ps->p++;
        }
    }
    if (*ps->p != '"') { free(buf); fail(err, ps, "unterminated string"); return NULL; }
    ps->p++;
    buf[len] = '\0';
    return buf;
}

static json_value *parse_object(parser *ps, char **err) {
    json_value *v = new_value(JSON_OBJECT);
    ps->p++; /* { */
    skip_ws(ps);
    if (*ps->p == '}') { ps->p++; return v; }
    size_t cap = 4;
    v->u.object.keys = malloc(cap * sizeof(char *));
    v->u.object.values = malloc(cap * sizeof(json_value *));
    for (;;) {
        skip_ws(ps);
        char *key = parse_string_raw(ps, err);
        if (!key) { json_free(v); return NULL; }
        skip_ws(ps);
        if (*ps->p != ':') { free(key); json_free(v); fail(err, ps, "expected ':'"); return NULL; }
        ps->p++;
        skip_ws(ps);
        json_value *val = parse_value(ps, err);
        if (!val) { free(key); json_free(v); return NULL; }
        if (v->u.object.count == cap) {
            cap *= 2;
            v->u.object.keys = realloc(v->u.object.keys, cap * sizeof(char *));
            v->u.object.values = realloc(v->u.object.values, cap * sizeof(json_value *));
        }
        v->u.object.keys[v->u.object.count] = key;
        v->u.object.values[v->u.object.count] = val;
        v->u.object.count++;
        skip_ws(ps);
        if (*ps->p == ',') { ps->p++; continue; }
        if (*ps->p == '}') { ps->p++; break; }
        json_free(v);
        fail(err, ps, "expected ',' or '}'");
        return NULL;
    }
    return v;
}

static json_value *parse_array(parser *ps, char **err) {
    json_value *v = new_value(JSON_ARRAY);
    ps->p++; /* [ */
    skip_ws(ps);
    if (*ps->p == ']') { ps->p++; return v; }
    size_t cap = 4;
    v->u.array.items = malloc(cap * sizeof(json_value *));
    for (;;) {
        skip_ws(ps);
        json_value *val = parse_value(ps, err);
        if (!val) { json_free(v); return NULL; }
        if (v->u.array.count == cap) {
            cap *= 2;
            v->u.array.items = realloc(v->u.array.items, cap * sizeof(json_value *));
        }
        v->u.array.items[v->u.array.count++] = val;
        skip_ws(ps);
        if (*ps->p == ',') { ps->p++; continue; }
        if (*ps->p == ']') { ps->p++; break; }
        json_free(v);
        fail(err, ps, "expected ',' or ']'");
        return NULL;
    }
    return v;
}

static json_value *parse_value(parser *ps, char **err) {
    skip_ws(ps);
    char c = *ps->p;
    if (c == '{') return parse_object(ps, err);
    if (c == '[') return parse_array(ps, err);
    if (c == '"') {
        char *s = parse_string_raw(ps, err);
        if (!s) return NULL;
        json_value *v = new_value(JSON_STRING);
        v->u.string = s;
        return v;
    }
    if (c == 't' && strncmp(ps->p, "true", 4) == 0) {
        ps->p += 4;
        json_value *v = new_value(JSON_BOOL);
        v->u.boolean = 1;
        return v;
    }
    if (c == 'f' && strncmp(ps->p, "false", 5) == 0) {
        ps->p += 5;
        json_value *v = new_value(JSON_BOOL);
        v->u.boolean = 0;
        return v;
    }
    if (c == 'n' && strncmp(ps->p, "null", 4) == 0) {
        ps->p += 4;
        return new_value(JSON_NULL);
    }
    if (c == '-' || isdigit((unsigned char)c)) {
        char *end;
        double d = strtod(ps->p, &end);
        if (end == ps->p) { fail(err, ps, "bad number"); return NULL; }
        ps->p = end;
        json_value *v = new_value(JSON_NUMBER);
        v->u.number = d;
        return v;
    }
    fail(err, ps, "unexpected character");
    return NULL;
}

json_value *json_parse(const char *text, char **err) {
    if (err) *err = NULL;
    parser ps;
    ps.p = text;
    ps.start = text;
    json_value *v = parse_value(&ps, err);
    if (!v) return NULL;
    skip_ws(&ps);
    /* Trailing garbage is tolerated; we only need the first value. */
    return v;
}

void json_free(json_value *v) {
    if (!v) return;
    switch (v->type) {
        case JSON_STRING:
            free(v->u.string);
            break;
        case JSON_ARRAY:
            for (size_t i = 0; i < v->u.array.count; i++)
                json_free(v->u.array.items[i]);
            free(v->u.array.items);
            break;
        case JSON_OBJECT:
            for (size_t i = 0; i < v->u.object.count; i++) {
                free(v->u.object.keys[i]);
                json_free(v->u.object.values[i]);
            }
            free(v->u.object.keys);
            free(v->u.object.values);
            break;
        default:
            break;
    }
    free(v);
}

json_value *json_get(const json_value *obj, const char *key) {
    if (!obj || obj->type != JSON_OBJECT) return NULL;
    for (size_t i = 0; i < obj->u.object.count; i++) {
        if (strcmp(obj->u.object.keys[i], key) == 0) {
            json_value *v = obj->u.object.values[i];
            if (v->type == JSON_NULL) return NULL;
            return v;
        }
    }
    return NULL;
}

int json_is_null(const json_value *obj, const char *key) {
    if (!obj || obj->type != JSON_OBJECT) return 1;
    for (size_t i = 0; i < obj->u.object.count; i++) {
        if (strcmp(obj->u.object.keys[i], key) == 0)
            return obj->u.object.values[i]->type == JSON_NULL;
    }
    return 1;
}

const char *json_get_string(const json_value *obj, const char *key, const char *def) {
    json_value *v = json_get(obj, key);
    if (!v || v->type != JSON_STRING) return def;
    return v->u.string;
}

long json_get_int(const json_value *obj, const char *key, long def) {
    json_value *v = json_get(obj, key);
    if (!v || v->type != JSON_NUMBER) return def;
    return (long)v->u.number;
}

double json_get_double(const json_value *obj, const char *key, double def) {
    json_value *v = json_get(obj, key);
    if (!v || v->type != JSON_NUMBER) return def;
    return v->u.number;
}

int json_get_bool(const json_value *obj, const char *key, int def) {
    json_value *v = json_get(obj, key);
    if (!v || v->type != JSON_BOOL) return def;
    return v->u.boolean;
}

size_t json_array_count(const json_value *arr) {
    if (!arr || arr->type != JSON_ARRAY) return 0;
    return arr->u.array.count;
}

json_value *json_array_at(const json_value *arr, size_t idx) {
    if (!arr || arr->type != JSON_ARRAY || idx >= arr->u.array.count) return NULL;
    return arr->u.array.items[idx];
}