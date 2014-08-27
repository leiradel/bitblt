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

#include <bitblt.h>
#include <Gamebuino.h>

// Define to log access outside the display buffer.
#undef BITBLT_CHECK_DISPLAY_BOUNDS

// l = x << log2( y ); r = x >> ( 8 - log2( y ) );
#define SHLR( x, y, l, r )   \
  asm(                       \
    "/* SHLR */\n\t"         \
    "mul %2, %3\n\t"         \
    "mov %0, r0\n\t"         \
    "mov %1, r1\n\t"         \
    "clr r1\n\t"             \
    : "=r" ( l ), "=r" ( r ) \
    : "r" ( x ), "r" ( y )   \
    : /* none */             \
  )

extern uint8_t _displayBuffer[ LCDWIDTH * LCDHEIGHT / 8 ];

void bitblt( const uint8_t* sprite, int8_t x, int8_t y, bool hide_grey )
{
  // Read the width and height of the sprite (one byte each.)
  uint8_t width = pgm_read_byte( sprite++ );
  uint8_t height = pgm_read_byte( sprite++ );
  
  // Return if the sprite doesn't intersect the screen.
  if ( y <= -height || y >= LCDHEIGHT_NOROT || x <= -width || x >= LCDWIDTH_NOROT )
  {
    return;
  }
  
  // Screen points to the beginning of the display buffer.
  uint8_t* screen = _displayBuffer;
  
  // Evaluate the shift ammount for the inner loop.
  uint8_t y_and_7 = y & 7;
  uint8_t shift = 1 << y_and_7;
  
  // Evaluate the height in bytes of the sprite.
  uint8_t height_bytes = height + 7;
  height_bytes /= 8; // Leave this here; if the division is put into the above expression the right shift becomes a loop shifting a word :/
  
  // Evaluate how many bytes in each column must be blit to the screen.
  uint8_t col_span_bytes = y_and_7 + height + 7;
  col_span_bytes /= 8; // Leave this here.
  
  // Frame mask is 0 when hide_grey is true and 255 otherwise.
  uint8_t frame_mask = (uint8_t)hide_grey - 1;
  
  // How many bytes to skip at the top and the bottom of the sprite because of cropping.
  uint8_t top_gap, bottom_gap;
  
  // If x is negative we must crop columns at the left.
  if ( x < 0 )
  {
    // Evaluate number of columns to crop.
    uint16_t left_crop = -x * height_bytes;
    // Update sprite pointer.
    sprite += left_crop * 2;
    // Decrease the number of rows to blit.
    width += x;
  }
  else
  {
    // If x is positive we just update the screen pointer by it.
    screen += x;
  }
  
  // Check if we must crop columns at the right.
  if ( ( x + width ) > LCDWIDTH_NOROT )
  {
    // Yes, just decrease the number of columns to blit.
    width -= x + width - LCDWIDTH_NOROT;
  }
  
  // If y is negative we must crop rows at the top.
  if ( y < 0 )
  {
    // Evaluate the number of bytes to skip at the top.
    top_gap = -y + 7;
    top_gap /= 8; // Leave this here.
    // Decrease the number of rows to blit.
    height_bytes -= top_gap;
    // Multiply by two to update the sprite pointer inside the outter loop.
    top_gap <<= 1;
  }
  else
  {
    // No cropping rows at the top.
    top_gap = 0;
    // Update the screen pointer to the correct location.
    screen += ( y / 8 ) * LCDWIDTH_NOROT;
  }
  
  // This is true if we must blit an additional row at the end, i.e. when the height of the sprite in bytes is less than the number of bytes to blit in each column.
  bool additional_row;
  
  // Check if we must crop rows at the bottom.
  if ( ( y + height ) > LCDHEIGHT_NOROT )
  {
    // Yes, evaluate the gap at the bottom.
    uint8_t start_row = y / 8;
    bottom_gap = start_row + height_bytes - LCDHEIGHT_NOROT / 8;
    // Decrease the number of rows to blit.
    height_bytes -= bottom_gap;
    // No additional rows at the bottom when we're cropping.
    additional_row = false;
    // Multiply by two to update the sprite pointer inside the outter loop.
    bottom_gap <<= 1;
  }
  else
  {
    // No cropping at the bottom.
    bottom_gap = 0;
    // Draw one additional row with the leftovers of the previous row.
    additional_row = col_span_bytes > height_bytes;
  }
  
#ifdef BITBLT_CHECK_DISPLAY_BOUNDS
  bool oos1 = false;
  bool oos2 = false;
#endif
  
  // Count columns first because the way the display buffer is layed out.
  for ( uint8_t i = width; i != 0; --i )
  {
    // Save the current screen pointer to make it easier to get the pointer to the next column.
    uint8_t* col = screen;
    uint8_t last_and_mask;
    uint8_t last_or_mask;
    
    // If there's a gap at the top...
    if ( top_gap )
    {
      // Update the sprite pointer minus two, since we need to evaluate the starting values for last_and_mask and last_or_mask.
      sprite += top_gap - 2;
      
      // Read one byte from each sprite layer.
      uint8_t l1 = pgm_read_byte( sprite++ );
      uint8_t l2 = pgm_read_byte( sprite++ );
      
      // l1 | l2 sets bits that are white/black/gray. In other words, transparent pixels have the corresponding bits reset.
      uint8_t and_mask = l1 | l2;
      // l1 & ~l2 sets bits that are black. l1 & l2 & frame_mask sets bits that are grey depending on frame_mask.
      uint8_t or_mask = l1 & ( ~l2 | ( l2 & frame_mask ) );
      
      // Shift the masks depending on the starting y coordinate.
      SHLR( and_mask, shift, and_mask, last_and_mask );
      SHLR( or_mask, shift, or_mask, last_or_mask );
    }
    else
    {
      // No previous row so both masks are 0.
      last_and_mask = last_or_mask = 0;
    }
    
    // Count rows of entire bytes.
    for ( uint8_t j = height_bytes; j != 0; --j )
    {
      // Read one byte from each sprite layer.
      uint8_t l1 = pgm_read_byte( sprite++ );
      uint8_t l2 = pgm_read_byte( sprite++ );
      
      // l1 | l2 sets bits that are white/black/gray. In other words, transparent pixels have the corresponding bits reset.
      uint8_t and_mask = l1 | l2;
      // l1 & ~l2 sets bits that are black. l1 & l2 & frame_mask sets bits that are grey depending on frame_mask.
      uint8_t or_mask = l1 & ( ~l2 | ( l2 & frame_mask ) );
      
      // Update the current masks and compute the masks for the next iteration.
      uint8_t next_and_mask, next_or_mask;
      SHLR( and_mask, shift, and_mask, next_and_mask );
      SHLR( or_mask, shift, or_mask, next_or_mask );

      // Read the byte with the eight pixels to update.
      uint8_t pixel = *screen;
      // Retain transparent pixels. White/black/grey pixels are reset.
      pixel &= ~( and_mask | last_and_mask );
      // Set black and grey pixels.
      pixel |= or_mask | last_or_mask;
      // Write the updated pixels back to the screen.
      *screen = pixel;
      
#ifdef BITBLT_CHECK_DISPLAY_BOUNDS
      if ( screen < _displayBuffer || screen >= ( _displayBuffer + LCDWIDTH * LCDHEIGHT / 8 ) )
      {
        oos1 = true;
      }
#endif

      // Set the masks for the next iteration.
      last_and_mask = next_and_mask;
      last_or_mask = next_or_mask;
      
      // Update the screen pointer to the next byte in the same column.
      screen += LCDWIDTH_NOROT;
    }
    
    // If there's an additional row...
    if ( additional_row )
    {
      // Update the pixels with the leftover masks from the inner loop.
      uint8_t pixel = *screen;
      pixel &= ~last_and_mask;
      pixel |= last_or_mask;
      *screen = pixel;
      
#ifdef BITBLT_CHECK_DISPLAY_BOUNDS
      if ( screen < _displayBuffer || screen >= ( _displayBuffer + LCDWIDTH * LCDHEIGHT / 8 ) )
      {
        oos2 = true;
      }
#endif
    }
    
    // Update the screen pointer to the beginning of the next column.
    screen = col + 1;
    // Skip bytes at the end of the sprite column.
    sprite += bottom_gap;
  }
  
#ifdef BITBLT_CHECK_DISPLAY_BOUNDS
  if ( oos1 )
  {
    gb.display.println( "OOB in the inner loop" );
  }
  
  if ( oos2 )
  {
    gb.display.println( "OOB in the add. row" );
  }
#endif
}
