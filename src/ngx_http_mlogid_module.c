/**
 * @author jianghua <Mr.CoreDumped@gmail.com>
 * @copyright (c) FightingMan
 */

#ifndef DDEBUG
#define DDEBUG 0
#endif

#ifndef IN_BAIDU
#define IN_BAIDU 0
#endif

#include "ddebug.h"
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_md5.h>

#define MD5_BHASH_LEN 16
#define MD5_HASH_LEN 16
#define HOSTNAME_LEN 30
#define HOSTNAME_HASH_LEN 48

typedef struct {
	ngx_flag_t    enable;
} ngx_http_mlogid_conf_t;

static ngx_int_t ngx_http_mlogid_add_variables(ngx_conf_t *cf);
static void *ngx_http_mlogid_create_conf(ngx_conf_t *cf);
static char *ngx_http_mlogid_merge_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_str_t  ngx_http_mlogid = ngx_string("mlogid");
static u_char  hostname[HOSTNAME_LEN];
static const u_char hex[] = "0123456789abcdef";

/* {{{ static ngx_command_t  ngx_http_mlogid_commands[] = */
static ngx_command_t  ngx_http_mlogid_commands[] = {
	 { ngx_string("mlogid"),
	  NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
	  ngx_conf_set_flag_slot,
	  NGX_HTTP_LOC_CONF_OFFSET,
	  offsetof(ngx_http_mlogid_conf_t, enable),
	  NULL },

      ngx_null_command
};
/* }}} */

/* {{{ static ngx_http_module_t  ngx_http_mlogid_module_ctx = */
static ngx_http_module_t  ngx_http_mlogid_module_ctx = {
	ngx_http_mlogid_add_variables,      /* preconfiguration */
    NULL,                  				/* postconfiguration */

    NULL,                               /* create main configuration */
    NULL,                               /* init main configuration */

    NULL,                               /* create server configuration */
    NULL,                               /* merge server configuration */

    ngx_http_mlogid_create_conf,        /* create location configration */
    ngx_http_mlogid_merge_conf          /* merge location configration */
};
/* }}} */

/* {{{ ngx_module_t  ngx_http_mlogid_module = */
ngx_module_t  ngx_http_mlogid_module = {
    NGX_MODULE_V1,
    &ngx_http_mlogid_module_ctx,        /* module context */
    ngx_http_mlogid_commands,           /* module directives */
    NGX_HTTP_MODULE,                    /* module type */
    NULL,                               /* init master */
    NULL,                               /* init module */
    NULL,           					/* init process */
    NULL,                               /* init thread */
    NULL,                               /* exit thread */
    NULL,                               /* exit process */
    NULL,                               /* exit master */
    NGX_MODULE_V1_PADDING
};
/* }}} */

static ngx_int_t
ngx_http_mlogid_set_variable(ngx_http_request_t *r,
     ngx_http_variable_value_t *v, uintptr_t data) 
/* {{{  */
{
    ngx_md5_t                      md5;
    ngx_http_mlogid_conf_t         *conf;
    u_char       				   *end, *p;
	u_char 						   val[NGX_INT64_LEN * 3 + 2];
    u_char 						   hashb[MD5_BHASH_LEN];
    u_char 						   hasht[MD5_HASH_LEN];
    u_char 						   hashr[HOSTNAME_HASH_LEN];

    conf = ngx_http_get_module_loc_conf(r, ngx_http_mlogid_module);

    if (conf->enable == 0) {
    	v->not_found = 1;
    	return NGX_OK;
    }

	end = ngx_sprintf(val, "%i,%ui,%i", ngx_pid, r->connection->number, ngx_random());
	dd("base value %s\n", val);
	*end = 0;

    ngx_md5_init(&md5);
    ngx_md5_update(&md5, val, end - val);
    ngx_md5_final(hashb, &md5);

	v->valid = 1;
	v->no_cacheable = 0;
	v->not_found = 0;
    v->len = HOSTNAME_HASH_LEN;

    v->data = ngx_pnalloc(r->pool, v->len);
    if (v->data == NULL) {
        return NGX_ERROR;
    }

    ngx_uint_t i;
	p = hasht;
	for (i = 0; i < MD5_HASH_LEN; i++) {
		*p++ = hex[hashb[i] >> 4];
		*p++ = hex[hashb[i] & 0xf];
	}
	dd("hash value %s\n", hasht);

    ngx_snprintf(hashr, HOSTNAME_HASH_LEN - 2, "%s%s", hostname, hasht);
	hashr[HOSTNAME_HASH_LEN - 1] = '\0';
	dd("last value %s\n", hashr);
    ngx_memcpy(v->data, hashr, v->len);

	return NGX_OK;
}
/* }}} */

static void *
ngx_http_mlogid_create_conf(ngx_conf_t *cf)
/* {{{  */
{
    ngx_http_mlogid_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_mlogid_conf_t));
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }

    conf->enable = NGX_CONF_UNSET;
    return conf;
}
/* }}} */

static char *
ngx_http_mlogid_merge_conf(ngx_conf_t *cf, void *parent, void *child)
/* {{{  */
{
    ngx_http_mlogid_conf_t *prev = parent;
    ngx_http_mlogid_conf_t *conf = child;

    ngx_conf_merge_value(conf->enable, prev->enable, 0);

    return NGX_CONF_OK;
}
/* }}} */

static ngx_int_t
ngx_http_mlogid_add_variables(ngx_conf_t *cf)
/* {{{  */
{
    ngx_http_variable_t  *var;

    var = ngx_http_add_variable(cf, &ngx_http_mlogid, NGX_HTTP_VAR_CHANGEABLE);
    if (var == NULL) {
        return NGX_ERROR;
    }

#ifdef IN_BAIDU
	if ((cf->cycle->hostname.len - 14) > HOSTNAME_LEN) {
		ngx_snprintf(hostname, HOSTNAME_LEN, "%s", cf->cycle->hostname.data);
	} else {
		ngx_snprintf(hostname, cf->cycle->hostname.len - 14, "%s", cf->cycle->hostname.data);
	}
#else
	ngx_snprintf(hostname, HOSTNAME_LEN, "%s", cf->cycle->hostname.data);
#endif

	hostname[HOSTNAME_LEN - 1] =  '\0';
    var->get_handler = ngx_http_mlogid_set_variable;

    return NGX_OK;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
