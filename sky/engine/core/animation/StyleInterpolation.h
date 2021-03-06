// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SKY_ENGINE_CORE_ANIMATION_STYLEINTERPOLATION_H_
#define SKY_ENGINE_CORE_ANIMATION_STYLEINTERPOLATION_H_

#include "gen/sky/core/CSSPropertyNames.h"
#include "sky/engine/core/animation/Interpolation.h"

namespace blink {

class StyleResolverState;

class StyleInterpolation : public Interpolation {
public:
    // 1) convert m_cachedValue into an X
    // 2) shove X into StyleResolverState
    // X can be:
    // (1) a CSSValue (and applied via StyleBuilder::applyProperty)
    // (2) an AnimatableValue (and applied via // AnimatedStyleBuilder::applyProperty)
    // (3) a custom value that is inserted directly into the StyleResolverState.
    virtual void apply(StyleResolverState&) const = 0;

    virtual bool isStyleInterpolation() const override final { return true; }

    CSSPropertyID id() const { return m_id; }

protected:
    CSSPropertyID m_id;

    StyleInterpolation(PassOwnPtr<InterpolableValue> start, PassOwnPtr<InterpolableValue> end, CSSPropertyID id)
        : Interpolation(start, end)
        , m_id(id)
    {
    }
};

DEFINE_TYPE_CASTS(StyleInterpolation, Interpolation, value, value->isStyleInterpolation(), value.isStyleInterpolation());

}

#endif  // SKY_ENGINE_CORE_ANIMATION_STYLEINTERPOLATION_H_
