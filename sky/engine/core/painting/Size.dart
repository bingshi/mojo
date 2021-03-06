// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

part of dart.sky;

/// Holds a 2D floating-point size.
class Size {
  final double width;
  final double height;

  const Size(this.width, this.height);

  const Size.infinite() : width = double.INFINITY, height = double.INFINITY;

  bool operator ==(other) {
    if (!(other is Size)) return false;
    return width == other.width && height == other.height;
  }
  int get hashCode {
    int result = 373;
    result = 37 * result + width.hashCode;
    result = 37 * result + height.hashCode;
    return result;
  }
  String toString() => "Size($width, $height)";
}
