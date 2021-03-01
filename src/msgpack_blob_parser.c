/*
 * Copyright 2020 Comcast Cable Communications Management, LLC
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

#include "msgpack_blob_parser.h"
#include "webcfg_log.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */	
/*----------------------------------------------------------------------------*/
msgpack_object* __finder1( const char *name, 
                          msgpack_object_type expect_type,
                          msgpack_object_map *map );
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
// To extract the msgpack first level objects
void* msgpack_process( const void *buf, size_t len,
                      size_t struct_size, const char *wrapper,
                      msgpack_object_type expect_type, bool optional,
                      process2_fn_t process,
                      destroy2_fn_t destroy, int level )
{
    void *p = malloc( struct_size );
    if( NULL == p ) {
        errno = MSGPACK_OUT_OF_MEMORY;
    } else {
        memset( p, 0, struct_size );
	printf("Inside function\n");
        if( NULL != buf && 0 < len ) {
            size_t offset = 0;
            msgpack_unpacked msg;
            msgpack_unpack_return mp_rv;

            msgpack_unpacked_init( &msg );

            /* The outermost wrapper MUST be a map. */
            mp_rv = msgpack_unpack_next( &msg, (const char*) buf, len, &offset );
	    //msgpack_object obj = msg.data;
	    //msgpack_object_print(stdout, obj);
	    //WebcfgDebug("\nMSGPACK_OBJECT_MAP is %d  msg.data.type %d\n", MSGPACK_OBJECT_MAP, msg.data.type);

            if( (MSGPACK_UNPACK_SUCCESS == mp_rv) && (0 != offset) &&
                (MSGPACK_OBJECT_MAP == msg.data.type) )
            {
		msgpack_object *inner;
                if(level == 1)
		{
		        if( NULL != wrapper )
			{
				printf("The wrapper is %s\n", wrapper);
		           inner = __finder1( wrapper, expect_type, &msg.data.via.map );
                    
		            if( ((NULL != inner) && (0 == (process)(p, 1, inner))) || 
		                      ((true == optional) && (NULL == inner)) )
		            {
		                 msgpack_unpacked_destroy( &msg );
		                 errno = MSGPACK_OK;
		                 return p;
		            }
		            else 
		            {
		                 errno = MSGPACK_INVALID_ELEMENT;
		            }
			}
		}
		else
		{
		        msgpack_object *subdoc_name;
		        msgpack_object *version;
		        msgpack_object *transaction_id;

			if( NULL != wrapper) 
		        {
			 printf("The level 2 wrapper is %s\n", wrapper);
		            inner = __finder1( wrapper, expect_type, &msg.data.via.map );
		            subdoc_name =  __finder1( "subdoc_name", expect_type, &msg.data.via.map );
		            version =  __finder1( "version", expect_type, &msg.data.via.map );
		            transaction_id =  __finder1( "transaction_id", expect_type, &msg.data.via.map );
		            printf("Before process\n");
		            if( ((NULL != inner && NULL != subdoc_name && NULL != version && NULL != transaction_id) && (0 == (process)(p,4, inner, subdoc_name, version, transaction_id))) ||((true == optional) && (NULL == inner)) )
		            {
		                 msgpack_unpacked_destroy( &msg );
		                 errno = MSGPACK_OK;
		                 return p;
		            }
		            else 
		            {     
		                 WebcfgDebug("Invalid element\n");
		                 errno = MSGPACK_INVALID_ELEMENT;
		            }
		        } 
			else
			{
				errno = MSGPACK_MISSING_WRAPPER;
			}
		}
            } else {
                errno = MSGPACK_UNPACK_FAILED;
            }

            msgpack_unpacked_destroy( &msg );

            (destroy)( p );
            p = NULL;
        }
    }

    return p;
}

int match1(msgpack_object_kv *p, const char * s)
{
     return strncmp((p)->key.via.str.ptr, s, (p)->key.via.str.size);
}

msgpack_object* __finder1( const char *name, 
                          msgpack_object_type expect_type,
                          msgpack_object_map *map )
{
    uint32_t i;
    
    for( i = 0; i < map->size; i++ ) 
    {

        if( MSGPACK_OBJECT_STR == map->ptr[i].key.type ) 
        {
            if( expect_type == map->ptr[i].val.type )
            {
                if( 0 == match1(&(map->ptr[i]), name) )
                {
                    return &map->ptr[i].val;
                }
            }
            else if(MSGPACK_OBJECT_STR == map->ptr[i].val.type)
            {   
                if(0 == strncmp(map->ptr[i].key.via.str.ptr, name, strlen(name)))
                {   
                    return &map->ptr[i].val;
                }
                
             }
             else 
            {   
                if(0 == strncmp(map->ptr[i].key.via.str.ptr, name, strlen(name)))
                {   
                    return &map->ptr[i].val;
                }
                
             }
            }
        }
	WebcfgDebug("The wrapper %s is missing\n", name);
     errno = MSGPACK_MISSING_WRAPPER;
    return NULL;
}

void msgpack_print(const void *data, size_t len)
{
      if( NULL != data && 0 < len )
      {
          size_t offset = 0;
          FILE *fd = fopen("/tmp/output.bin", "w+");
          msgpack_unpacked msg;
          msgpack_unpack_return msgpk_rv;
          msgpack_unpacked_init( &msg );

          msgpk_rv = msgpack_unpack_next( &msg, (const char*) data, len, &offset );
          WebcfgDebug("msgpk_rv value is %d\n",msgpk_rv);
          msgpack_object obj = msg.data;
          msgpack_object_print(fd, obj);
          msgpack_unpacked_destroy( &msg );
          fclose(fd);
      }
}

int msgpack_array_count(msgpack_object* obj)
{
	msgpack_object_array *array = &obj->via.array;
	return array->size;
}
