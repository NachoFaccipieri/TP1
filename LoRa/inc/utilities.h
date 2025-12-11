/*!
 * \file      utilities.h
 *
 * \brief     Helper functions
 */
#ifndef __UTILITIES_H__
#define __UTILITIES_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifndef MIN
#define MIN( a, b ) ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )
#endif

#ifndef MAX
#define MAX( a, b ) ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )
#endif

#define randr( min, max ) ( ( ( uint32_t )rand( ) % ( ( max ) - ( min ) + 1 ) ) + ( min ) )

#define MEMCPY_8( dst, src, size )                            \
    do                                                        \
    {                                                         \
        uint8_t* dst8 = ( uint8_t* )( dst );                  \
        const uint8_t* src8 = ( const uint8_t* )( src );      \
        for( uint16_t i = 0; i < ( size ); i++ )             \
        {                                                     \
            *dst8++ = *src8++;                                \
        }                                                     \
    } while( 0 );

#ifdef __cplusplus
}
#endif

#endif // __UTILITIES_H__
