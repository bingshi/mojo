// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SKY_ENGINE_CORE_SCRIPT_DOM_DART_STATE_H_
#define SKY_ENGINE_CORE_SCRIPT_DOM_DART_STATE_H_

#include "dart/runtime/include/dart_api.h"
#include "sky/engine/tonic/dart_state.h"
#include "sky/engine/core/dom/Document.h"
#include "sky/engine/wtf/RefPtr.h"

namespace blink {
class DartLoader;
class LocalFrame;
class LocalDOMWindow;

class DOMDartState : public DartState {
 public:
  explicit DOMDartState(Document* document);
  ~DOMDartState() override;

  virtual void DidSetIsolate();

  static DOMDartState* Current();

  static Document* CurrentDocument();
  static LocalFrame* CurrentFrame();
  static LocalDOMWindow* CurrentWindow();

  Document* document() const { return document_.get(); }
  DartLoader& loader() const { return *loader_; }

  // Cached handles to strings used in Dart/C++ conversions.
  Dart_Handle x_handle() { return x_handle_.value(); }
  Dart_Handle y_handle() { return y_handle_.value(); }
  Dart_Handle value_handle() { return value_handle_.value(); }

 private:
  RefPtr<Document> document_;
  OwnPtr<DartLoader> loader_;

  DartPersistentValue x_handle_;
  DartPersistentValue y_handle_;
  DartPersistentValue value_handle_;
};

}  // namespace blink

#endif // SKY_ENGINE_CORE_SCRIPT_DOM_DART_STATE_H_
