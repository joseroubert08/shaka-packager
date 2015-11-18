// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "packager/media/base/bit_reader.h"

#include <algorithm>

namespace edash_packager {
namespace media {

BitReader::BitReader(const uint8_t* data, off_t size)
    : data_(data),
      initial_size_(size),
      bytes_left_(size),
      num_remaining_bits_in_curr_byte_(0) {
  DCHECK(data_ != NULL && bytes_left_ > 0);

  UpdateCurrByte();
}

BitReader::~BitReader() {}

bool BitReader::SkipBits(int num_bits) {
  DCHECK_GE(num_bits, 0);

  // Skip any bits in the current byte waiting to be processed, then
  // process full bytes until less than 8 bits remaining.
  if (num_bits > num_remaining_bits_in_curr_byte_) {
    num_bits -= num_remaining_bits_in_curr_byte_;
    num_remaining_bits_in_curr_byte_ = 0;

    int num_bytes = num_bits / 8;
    num_bits %= 8;
    if (bytes_left_ < num_bytes) {
      bytes_left_ = 0;
      return false;
    }
    bytes_left_ -= num_bytes;
    data_ += num_bytes;
    UpdateCurrByte();

    // If there is no more data remaining, only return true if we
    // skipped all that were requested.
    if (num_remaining_bits_in_curr_byte_ == 0)
      return (num_bits == 0);
  }

  // Less than 8 bits remaining to skip. Use ReadBitsInternal to verify
  // that the remaining bits we need exist, and adjust them as necessary
  // for subsequent operations.
  uint64_t not_needed;
  return ReadBitsInternal(num_bits, &not_needed);
}

bool BitReader::ReadBitsInternal(int num_bits, uint64_t* out) {
  DCHECK_LE(num_bits, 64);

  *out = 0;

  while (num_remaining_bits_in_curr_byte_ != 0 && num_bits != 0) {
    int bits_to_take = std::min(num_remaining_bits_in_curr_byte_, num_bits);

    *out <<= bits_to_take;
    *out += curr_byte_ >> (num_remaining_bits_in_curr_byte_ - bits_to_take);
    num_bits -= bits_to_take;
    num_remaining_bits_in_curr_byte_ -= bits_to_take;
    curr_byte_ &= (1 << num_remaining_bits_in_curr_byte_) - 1;

    if (num_remaining_bits_in_curr_byte_ == 0)
      UpdateCurrByte();
  }

  return num_bits == 0;
}

void BitReader::UpdateCurrByte() {
  DCHECK_EQ(num_remaining_bits_in_curr_byte_, 0);

  if (bytes_left_ == 0)
    return;

  // Load a new byte and advance pointers.
  curr_byte_ = *data_;
  ++data_;
  --bytes_left_;
  num_remaining_bits_in_curr_byte_ = 8;
}

}  // namespace media
}  // namespace edash_packager
