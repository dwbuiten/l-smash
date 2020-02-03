/*****************************************************************************
 * av1_obu.c
 *****************************************************************************
 * Copyright (C) 2020 L-SMASH Project
 *
 * Authors: Derek Buitenhuis <derek.buitenhuis@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *****************************************************************************/

/* This file is available under an ISC license. */


#include "common/internal.h" /* must be placed first */

#include "codecs/av1.h"

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#define OBU_SEQUENCE_HEADER 1
#define OBU_TEMPORAL_DELIMITER 2
#define OBU_FRAME_HEADER 3
#define OBU_TILE_GROUP 4
#define OBU_METADATA 5
#define OBU_FRAME 6
#define OBU_REDUNDANT_FRAME_HEADER 7
#define OBU_PADDING 15

static uint64_t obu_av1_leb128
(
    lsmash_bs_t *bs,
    uint32_t offset,
    uint32_t *consumed
)
{
    uint64_t value = 0;
    *consumed = 0;

    for( uint64_t i = 0; i < 8; i++ )
    {
        uint8_t b = lsmash_bs_show_byte( bs, offset + (*consumed) );
        value |= ((uint64_t)(b & 0x7F)) << (i * 7);
        (*consumed)++;
        if ((b & 0x80) != 0x80)
            break;
    }

    return value;
}

static int obu_parse_seq_header( uint8_t *obubuf, uint32_t obusize, lsmash_av1_specific_parameters_t *param )
{
    return 0;
}

lsmash_av1_specific_parameters_t *obu_av1_parse_seq_header
(
    lsmash_bs_t *bs,
    uint32_t length,
    uint32_t offset
)
{
    lsmash_av1_specific_parameters_t *param = lsmash_malloc_zero(sizeof(lsmash_av1_specific_parameters_t));
    if ( !param )
        return NULL;

    uint32_t off = 0;
    printf("\n len = %d\n", length);

    while( off < length )
    {
        uint8_t temp8 = lsmash_bs_show_byte( bs, off + offset );
        uint8_t obutype = (temp8 & 0x78) >> 3;
        int extension = (((temp8 & 0x04) >> 2) == 1);
        int hassize = (((temp8 & 0x02) >> 1) == 1);

        off++;
        if( extension )
            off++;
        if( !hassize )
            continue;

        uint32_t consumed = 0;
        uint64_t obusize = obu_av1_leb128( bs, off + offset, &consumed );
        off += consumed;
        assert ( obusize <= UINT32_MAX );

        switch( obutype )
        {
            case 1:
            {
                uint8_t *obubuf = lsmash_malloc( obusize );
                if( !obubuf ) {
                    av1_destruct_specific_data( param );
                    return NULL;
                }
                for( uint32_t i = 0; i < obusize; i++ )
                    obubuf[i] = lsmash_bs_show_byte( bs, off + offset + i );
                off += obusize;

                int ret = obu_parse_seq_header( obubuf, obusize, param );
                if( ret < 0 ) {
                    lsmash_free(obubuf);
                    av1_destruct_specific_data( param );
                    return NULL;
                }

                uint32_t oldpos = param->configOBUs.sz;
                param->configOBUs.sz += obusize;
                uint8_t *newdata = lsmash_realloc( param->configOBUs.data, param->configOBUs.sz );
                if( !newdata ) {
                    lsmash_free(obubuf);
                    av1_destruct_specific_data( param );
                    return NULL;
                }
                param->configOBUs.data = newdata;
                memcpy( param->configOBUs.data + oldpos, obubuf, obusize );
                lsmash_free( obubuf );
                
                break;
            }
            default:
                off += obusize;
                break;
        }
    }

    return param;
}
