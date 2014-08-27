/******************************************************************************
The MIT License (MIT)

Copyright (c) 2014 Andre Leiradella

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <png.h>

typedef struct
{
  int width;
  int height;
  uint32_t pixels[ 0 ];
}
image_t;

static uint32_t get_pixel( const image_t* image, int x, int y )
{
  if ( x >= 0 && x < image->width && y >= 0 && y < image->height )
  {
    return image->pixels[ y * image->width + x ];
  }

  /* return transparent pixel */
  return 0;
}

static image_t* load_png( const char* path )
{
  png_byte header[ 8 ]; // 8 is the maximum size that can be checked

  /* open file and test for it being a png */
  FILE *fp = fopen( path, "rb" );
  
  if ( fp == NULL )
  {
    fprintf( stderr, "File \"%s\" could not be opened for reading\n", path );
    return NULL;
  }
  
  fread( header, 1, 8, fp );
  
  if ( png_sig_cmp( header, 0, 8 ) )
  {
    fclose( fp );
    fprintf( stderr, "File \"%s\" is not recognized as a PNG file\n", path );
    return NULL;
  }

  /* initialize stuff */
  png_structp png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );

  if ( png_ptr == NULL )
  {
    fclose( fp );
    fprintf( stderr, "png_create_read_struct failed\n" );
    return NULL;
  }

  png_infop info_ptr = png_create_info_struct( png_ptr );
  
  if ( info_ptr == NULL )
  {
    png_destroy_read_struct( &png_ptr, &info_ptr, NULL );
    fclose( fp );
    fprintf( stderr, "png_create_info_struct failed\n" );
    return NULL;
  }

  if ( setjmp( png_jmpbuf( png_ptr ) ) )
  {
    png_destroy_read_struct( &png_ptr, &info_ptr, NULL );
    fclose( fp );
    fprintf( stderr, "Error during init_io\n" );
    return NULL;
  }

  png_init_io( png_ptr, fp );
  png_set_sig_bytes( png_ptr, 8 );
  
  png_read_info( png_ptr, info_ptr );
  png_set_expand( png_ptr );
  png_set_scale_16( png_ptr );

  int width = png_get_image_width( png_ptr, info_ptr );
  int height = png_get_image_height( png_ptr, info_ptr );
  int color_type = png_get_color_type( png_ptr, info_ptr );
  int bit_depth = png_get_bit_depth( png_ptr, info_ptr );
  int number_of_passes = png_set_interlace_handling( png_ptr );
  
  png_read_update_info( png_ptr, info_ptr );

  /* read file */
  if ( setjmp( png_jmpbuf( png_ptr ) ) )
  {
    png_destroy_read_struct( &png_ptr, &info_ptr, NULL );
    fclose( fp );
    fprintf( stderr, "Error during read_image\n" );
    return NULL;
  }
  
  image_t* image = (image_t*)malloc( sizeof( image_t ) + width * height * sizeof( uint32_t ) );
  
  if ( image == NULL )
  {
    png_destroy_read_struct( &png_ptr, &info_ptr, NULL );
    fclose( fp );
    fprintf( stderr, "Out of memory\n" );
    return NULL;
  }
  
  png_byte** row_pointers =  (png_byte**)malloc( height * sizeof( png_byte* ) );
  
  if ( row_pointers == NULL )
  {
    free( image );
    png_destroy_read_struct( &png_ptr, &info_ptr, NULL );
    fclose( fp );
    fprintf( stderr, "Out of memory\n" );
    return NULL;
  }
  
  for ( int y = 0; y < height; y++ )
  {
    row_pointers[ y ] = (png_byte*)( image->pixels + y * width );
  }

  png_read_image( png_ptr, row_pointers );

  png_destroy_read_struct( &png_ptr, &info_ptr, NULL );
  fclose( fp );
  
  image->width = width;
  image->height = height;
  return image;
}

static int generate_code( const char* path, const char* name, FILE* out )
{
  image_t* image = load_png( path );
  
  if ( image == NULL )
  {
    return 0;
  }
  
  int height_aligned = 8 * ( ( image->height + 7 ) / 8 );
  uint8_t* layer1 = (uint8_t*)malloc( image->width * height_aligned );
  uint8_t* layer2 = (uint8_t*)malloc( image->width * height_aligned );
  
  if ( layer1 == NULL || layer2 == NULL )
  {
    free( layer1 );
    free( layer2 );
    fprintf( stderr, "Out of memory\n" );
    return 0;
  }
  
  for ( int x = 0; x < image->width; ++x )
  {
    for ( int y = 0; y < height_aligned; ++y )
    {
      uint32_t color = get_pixel( image, x, y );
      uint8_t a = color >> 24;
      uint8_t b = ( color >> 16 ) & 255;
      uint8_t g = ( color >> 8 ) & 255;
      uint8_t r = color & 255;
      uint8_t grey = ( r * 76 + g * 150 + b * 30 ) / 256;
      
      printf( "%08x ", color );
      
      if ( a < 128 )
      {
        // transparent
        layer1[ x * height_aligned + y ] = 0;
        layer2[ x * height_aligned + y ] = 0;
      }
      else if ( grey > 192 )
      {
        // white
        layer1[ x * height_aligned + y ] = 0;
        layer2[ x * height_aligned + y ] = 128;
      }
      else if ( grey < 64 )
      {
        // black
        layer1[ x * height_aligned + y ] = 128;
        layer2[ x * height_aligned + y ] = 0;
      }
      else
      {
        // grey
        layer1[ x * height_aligned + y ] = 128;
        layer2[ x * height_aligned + y ] = 128;
      }
    }
    
    printf( "\n" );
  }
  
  fprintf( out, "static const uint8_t %s[] PROGMEM =\n", name );
  fprintf( out, "{\n" );
  fprintf( out, "  /* width in pixels  */ %4d,\n", image->width );
  fprintf( out, "  /* height in pixels */ %4d,\n", image->height );

  for ( int x = 0; x < image->width; ++x )
  {
    fprintf( out, "  /* column %3d l1/l2 */ ", x );
    
    for ( int y = 0; y < height_aligned - 1; y += 8 )
    {
      uint8_t byte = 0;
      
      for ( int k = 0; k < 8; ++k )
      {
        byte = byte >> 1 | layer1[ x * height_aligned + y + k ];
      }
      
      fprintf( out, "0x%02x, ", byte );
      
      byte = 0;
      
      for ( int k = 0; k < 8; ++k )
      {
        byte = byte >> 1 | layer2[ x * height_aligned + y + k ];
      }
      
      fprintf( out, "0x%02x, ", byte );
    }
    
    fprintf( out, "\n" );
  }

  fprintf( out, "};\n\n" );
  return 1;
}

static void show_usage( FILE* out )
{
  fprintf( out, "USAGE: bbltenc [ -n array_name ] [ -o output_path ] input_path\n\n" );
  fprintf( out, "-n array_name     set the generated array name (default = sprite)\n" );
  fprintf( out, "-o output_path    set the name of the generated file (default = stdout)\n" );
  fprintf( out, "input_path        set the name of the PNG image\n\n" );
}

int main( int argc, const char* argv[] )
{
  if ( argc < 2 )
  {
    show_usage( stderr );
    return 1;
  }
  
  const char* default_array_name = "sprite";
  
  const char* input_path = NULL;
  const char* output_path = NULL;
  const char* array_name = default_array_name;
  
  for ( int i = 1; i < argc; ++i )
  {
    if ( !strcmp( argv[ i ], "-o" ) )
    {
      if ( output_path != NULL )
      {
        fprintf( stderr, "Duplicated output path\n" );
        return 1;
      }
      
      if ( ++i == argc )
      {
        fprintf( stderr, "Missing argument to -o\n" );
        return 1;
      }
      
      output_path = argv[ i ];
    }
    else if ( !strcmp( argv[ i ], "-n" ) )
    {
      if ( array_name != default_array_name )
      {
        fprintf( stderr, "Duplicated array name\n" );
        return 1;
      }
      
      if ( ++i == argc )
      {
        fprintf( stderr, "Missing argument to -n\n" );
        return 1;
      }
      
      array_name = argv[ i ];
    }
    else if ( !strcmp( argv[ i ], "-h" ) )
    {
      show_usage( stdout );
      return 0;
    }
    else
    {
      if ( input_path != NULL )
      {
        fprintf( stderr, "Duplicated input file\n" );
        return 1;
      }
      
      input_path = argv[ i ];
    }
  }
  
  FILE* out = stdout;
  
  if ( output_path != NULL )
  {
    out = fopen( output_path, "w" );
    
    if ( out == NULL )
    {
      fprintf( stderr, "Error opening \"%s\"\n", output_path );
      return 1;
    }
  }
  
  int retcode = generate_code( input_path, array_name, out );
  
  if ( out != stdout )
  {
    fclose( out );
  }
  
  return retcode;
}
