#ifndef HEXDUMP_H_INCLUDED
#define HEXDUMP_H_INCLUDED

/*
  HexDump - An Arduino library for hexadecimal/ASCII dumping of data.
  Created by Ivo Pullens, Emmission, 2016 -- www.emmission.nl

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/**
 * Print a single hexadecimal value to the provided stream.
 * The value will be prefixed by zeroes, when required, to force the
 * number of output digits to the 2 times the size of the valueT.
 * @tparam valueT  Type of value to be printed, e.g. size_t or uint8_t.
 * @param  stream  Output stream (e.g. Serial) to print to.
 * @param  val     Value to print as hexadecimal.
 */
template <typename valueT>
void PrintHex( Stream& stream, const valueT val )
{
  const uint8_t digits = sizeof(valueT) << 1;
  uint8_t i = 0;
  while (i < digits)
  {
    valueT v = (val >> ((digits-i-1) << 2)) & valueT(0x0F);     // Each nibble stores 1 digit
    stream.print(v, HEX);
    ++i;
  }
}

/**
 * Dump a block of data as hexadecimal and ASCII to the provided stream.
 * @tparam addrT        Address type. This type determines the amount of digits displayed for
 *                      the address, e.g. size_t to leave it up to the architecture or uint8_t
 *                      to use only a single byte.
 * @tparam bytesPerRow  Number of bytes to print per row, e.g. 16 or 8.
 * @param  stream       Output stream (e.g. Serial) to print to.
 * @param  buff         Ptr to block of data to print.
 * @param  len          Length of block to print, in [bytes].
 * @param  base         Optional address offset to use in priting.
 *
 * A typical example to dump a block of data to the serial console:
 * @code
 * uint8_t data[100];
 * HexDump(Serial, data, sizeof(data));
 * @endcode
*/
template <typename addrT = size_t, uint8_t bytesPerRow = 16>
void HexDump( Stream& stream, void* buff, size_t len, addrT base = 0 )
{
  uint8_t* p = reinterpret_cast<uint8_t*>(buff);
  const size_t rows = (len + bytesPerRow-1) / bytesPerRow;

  for (size_t r = 0; r < rows; ++r)
  {
    PrintHex<addrT>( stream, base + p - reinterpret_cast<uint8_t*>(buff) );
    stream.print(F(": "));

    char* pc = reinterpret_cast<char*>(p);
    const size_t cols = len < bytesPerRow ? len : bytesPerRow;
    for (size_t c = 0; c < bytesPerRow; ++c)
    {
      if (c < cols)
      {
        PrintHex<uint8_t>(stream, *p++);
      }
      else
      {
        stream.print(F("  "));
      }
      stream.print(F(" "));
    }
    stream.print(F(" "));
    yield();

    for (size_t i = 0; i < cols; ++i)
    {
      char c = *pc++;
      if (c >= ' ') stream.print(c);
      else          stream.print('.');
    }
    stream.println(F(""));
    len -= bytesPerRow;
    yield();
  }
}

#endif // #ifndef HEXDUMP_H_INCLUDED
