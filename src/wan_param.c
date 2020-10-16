/*
 * Copyright 2019 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <errno.h>
#include <string.h>
#include <msgpack.h>
#include <stdarg.h>
#include "webcfg_log.h"
#include "comp_helpers.h"
#include "wan_param.h"
/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */
/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
enum {
    OK                       = HELPERS_OK,
    OUT_OF_MEMORY            = HELPERS_OUT_OF_MEMORY,
    INVALID_FIRST_ELEMENT    = HELPERS_INVALID_FIRST_ELEMENT,
    MISSING_ENTRY            = HELPERS_MISSING_WRAPPER,
    PARTIAL_APPLY            = HELPERS_PARTIAL_APPLY,
    INVALID_OBJECT,
    INVALID_VERSION,

};
/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */
/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
int process_wanparams( wanparam_t *e, msgpack_object_map *map );
int process_wandoc( wandoc_t *wd, int num, ...); 
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See wandoc.h for details. */
wandoc_t* wandoc_convert( const void *buf, size_t len )
{
	return comp_helper_convert( buf, len, sizeof(wandoc_t), "wan", 
                            MSGPACK_OBJECT_MAP, true,
                           (process_fn_t) process_wandoc,
                           (destroy_fn_t) wandoc_destroy );
}
/* See wandoc.h for details. */
void wandoc_destroy( wandoc_t *wd )
{
	if( NULL != wd )
	{
		if( NULL != wd->param )
		{
			if( NULL != wd->param->internal_ip )
			{
				free( wd->param->internal_ip );
			}
			
			free(wd->param);
			
		}
		if( NULL != wd->subdoc_name )
		{
			free( wd->subdoc_name );
		}
		free( wd );
	}
}
/* See wandoc.h for details. */
const char* wandoc_strerror( int errnum )
{
    struct error_map {
        int v;
        const char *txt;
    } map[] = {
        { .v = OK,                               .txt = "No errors." },
        { .v = OUT_OF_MEMORY,                    .txt = "Out of memory." },
        { .v = INVALID_FIRST_ELEMENT,            .txt = "Invalid first element." },
        { .v = INVALID_VERSION,                  .txt = "Invalid 'version' value." },
        { .v = INVALID_OBJECT,                   .txt = "Invalid 'value' array." },
	{ .v = PARTIAL_APPLY,                      .txt = "Contains additional param in array" },
        { .v = 0, .txt = NULL }
    };
    int i = 0;
    while( (map[i].v != errnum) && (NULL != map[i].txt) ) { i++; }
    if( NULL == map[i].txt )
    {
	WebcfgDebug("----wandoc_strerror----\n");
        return "Unknown error.";
    }
    return map[i].txt;
}
/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/**
 *  Convert the msgpack map into the doc_t structure.
 *
 *  @param e    the entry pointer
 *  @param map  the msgpack map pointer
 *
 *  @return 0 on success, error otherwise
 */
int process_wanparams( wanparam_t *e, msgpack_object_map *map )
{
    int left = map->size;
   printf("The map->size is %d\n", map->size);
    uint8_t objects_left = 0x02;
    msgpack_object_kv *p;
    p = map->ptr;
    while( (0 < objects_left) && (0 < left--) )
    {
        if( MSGPACK_OBJECT_STR == p->key.type )
        {
              if( MSGPACK_OBJECT_BOOLEAN == p->val.type )
              {
                 if( 0 == match(p, "Enable") )
                 {
                     e->enable = p->val.via.boolean;
                     objects_left &= ~(1 << 0);
                 }
              }
              else if(MSGPACK_OBJECT_STR == p->val.type)
              {
                 if( 0 == match(p, "InternalIP") )
                 {
                     e->internal_ip = strndup( p->val.via.str.ptr, p->val.via.str.size );
                     objects_left &= ~(1 << 1);
                 }
              }
        }
           p++;
	
    }
        
    printf("The left is %d\n",left);
    if( 0 == objects_left )
    {
       if(0 < left)
       {
           errno = PARTIAL_APPLY;
       }
       else
       {
           errno = OK;
       }
    }
    else
    {
        return -1;
    }

    return errno;   
   // return (0 == objects_left) ? 0 : -1;
}
int process_wandoc( wandoc_t *wd,int num, ... )
{
//To access the variable arguments use va_list 
	va_list valist;
	va_start(valist, num);//start of variable argument loop

	msgpack_object *obj = va_arg(valist, msgpack_object *);//each usage of va_arg fn argument iterates by one time
	msgpack_object_map *mapobj = &obj->via.map;

	msgpack_object *obj1 = va_arg(valist, msgpack_object *);
	wd->subdoc_name = strndup( obj1->via.str.ptr, obj1->via.str.size );

	msgpack_object *obj2 = va_arg(valist, msgpack_object *);
	wd->version = (uint32_t) obj2->via.u64;

	msgpack_object *obj3 = va_arg(valist, msgpack_object *);
	wd->transaction_id = (uint16_t) obj3->via.u64;
	va_end(valist);//End of variable argument loop


	wd->param = (wanparam_t *) malloc( sizeof(wanparam_t) );
        if( NULL == wd->param )
        {
	    WebcfgDebug("entries count malloc failed\n");
            return -1;
        }
        memset( wd->param, 0, sizeof(wanparam_t));


	if( 0 != process_wanparams(wd->param, mapobj) )
	{
		
		if(errno == PARTIAL_APPLY)
		{
			printf(" %s\n", wandoc_strerror(errno));
		}
		else
		{
			WebcfgDebug("process_wanparams failed\n");
		}
		return -1;
	}

    return 0;
}

