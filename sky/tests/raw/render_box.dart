// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import '../resources/third_party/unittest/unittest.dart';
import '../resources/unit.dart';
import '../resources/display_list.dart';
import 'dart:sky' as sky;
import 'package:sky/framework/rendering/render_box.dart';

void main() {
  initUnit();

  test("should size to render view", () {
    RenderSizedBox root = new RenderSizedBox(
      child: new RenderDecoratedBox(
        decoration: new BoxDecoration(backgroundColor: 0xFF00FF00)
      )
    );
    TestView renderView = new TestView(child: root);
    renderView.layout(new ViewConstraints(width: sky.view.width, height: sky.view.height));
    expect(root.size.width, equals(sky.view.width));
    expect(root.size.height, equals(sky.view.height));
    renderView.paintFrame();
    print(renderView.lastPaint); // TODO(ianh): figure out how to make this fit the unit testing framework better
  });
}
