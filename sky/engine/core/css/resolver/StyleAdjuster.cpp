/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2006, 2007 Nicholas Shanks (webkit@nickshanks.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 * Copyright (C) 2007, 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "sky/engine/config.h"
#include "sky/engine/core/css/resolver/StyleAdjuster.h"

#include "sky/engine/core/dom/ContainerNode.h"
#include "sky/engine/core/dom/Document.h"
#include "sky/engine/core/dom/Element.h"
#include "sky/engine/core/dom/NodeRenderStyle.h"
#include "sky/engine/core/rendering/style/RenderStyle.h"
#include "sky/engine/core/rendering/style/RenderStyleConstants.h"
#include "sky/engine/platform/Length.h"
#include "sky/engine/platform/transforms/TransformOperations.h"
#include "sky/engine/wtf/Assertions.h"

namespace blink {

static EDisplay equivalentInlineDisplay(EDisplay display)
{
    switch (display) {
    // TODO(ojan): Do we need an INLINE_PARAGRAPH display?
    case PARAGRAPH:
        return INLINE;

    case FLEX:
        return INLINE_FLEX;

    case INLINE:
    case INLINE_FLEX:
        return display;

    case NONE:
        ASSERT_NOT_REACHED();
        return NONE;
    }

    ASSERT_NOT_REACHED();
    return NONE;
}

static EDisplay equivalentBlockDisplay(EDisplay display)
{
    switch (display) {
    case PARAGRAPH:
    case FLEX:
        return display;

    case INLINE:
    case INLINE_FLEX:
        return FLEX;

    case NONE:
        ASSERT_NOT_REACHED();
        return NONE;
    }

    ASSERT_NOT_REACHED();
    return NONE;
}

// CSS requires text-decoration to be reset at each DOM element for tables,
// inline blocks, inline tables, shadow DOM crossings, floating elements,
// and absolute or relatively positioned elements.
static bool doesNotInheritTextDecoration(const RenderStyle* style, const Element& e)
{
    return isAtShadowBoundary(&e) || style->hasOutOfFlowPosition();
}

static bool parentStyleForcesZIndexToCreateStackingContext(const RenderStyle* parentStyle)
{
    return parentStyle->isDisplayFlexibleBox();
}

static bool hasWillChangeThatCreatesStackingContext(const RenderStyle* style)
{
    for (size_t i = 0; i < style->willChangeProperties().size(); ++i) {
        switch (style->willChangeProperties()[i]) {
        case CSSPropertyOpacity:
        case CSSPropertyTransform:
        case CSSPropertyWebkitTransform:
        case CSSPropertyTransformStyle:
        case CSSPropertyWebkitTransformStyle:
        case CSSPropertyPerspective:
        case CSSPropertyWebkitPerspective:
        case CSSPropertyWebkitClipPath:
        case CSSPropertyFilter:
        case CSSPropertyZIndex:
        case CSSPropertyPosition:
            return true;
        default:
            break;
        }
    }
    return false;
}

void StyleAdjuster::adjustRenderStyle(RenderStyle* style, RenderStyle* parentStyle, Element& element)
{
    ASSERT(parentStyle);

    if (style->display() != NONE) {
        if (style->hasOutOfFlowPosition() || parentStyle->requiresOnlyBlockChildren())
            style->setDisplay(equivalentBlockDisplay(style->display()));
        else
            style->setDisplay(equivalentInlineDisplay(style->display()));
    }

    // Make sure our z-index value is only applied if the object is positioned.
    if (style->position() == StaticPosition && !parentStyleForcesZIndexToCreateStackingContext(parentStyle))
        style->setHasAutoZIndex();

    if (style->hasAutoZIndex()
        && (style->hasOpacity()
            || style->hasTransformRelatedProperty()
            || style->clipPath()
            || style->hasFilter()
            || hasWillChangeThatCreatesStackingContext(style)))
        style->setZIndex(0);

    // will-change:transform should result in the same rendering behavior as having a transform,
    // including the creation of a containing block for fixed position descendants.
    if (!style->hasTransform() && (style->willChangeProperties().contains(CSSPropertyWebkitTransform) || style->willChangeProperties().contains(CSSPropertyTransform))) {
        bool makeIdentity = true;
        style->setTransform(TransformOperations(makeIdentity));
    }

    if (doesNotInheritTextDecoration(style, element))
        style->clearAppliedTextDecorations();

    style->applyTextDecorations();

    if (style->overflowX() != OVISIBLE || style->overflowY() != OVISIBLE)
        adjustOverflow(style);

    // Cull out any useless layers and also repeat patterns into additional layers.
    style->adjustBackgroundLayers();

    // If we have transitions, or animations, do not share this style.
    if (style->transitions() || style->animations())
        style->setUnique();

    // FIXME: when dropping the -webkit prefix on transform-style, we should also have opacity < 1 cause flattening.
    if (style->preserves3D() && (style->overflowX() != OVISIBLE
        || style->overflowY() != OVISIBLE
        || style->hasFilter()))
        style->setTransformStyle3D(TransformStyle3DFlat);

    adjustStyleForAlignment(*style, *parentStyle);
}

void StyleAdjuster::adjustStyleForAlignment(RenderStyle& style, const RenderStyle& parentStyle)
{
    if (style.justifyItems() == ItemPositionAuto) {
        style.setJustifyItems(parentStyle.justifyItems());
        style.setJustifyItemsPositionType(parentStyle.justifyItemsPositionType());
    }

    if (style.justifySelf() == ItemPositionAuto) {
        style.setJustifySelf(parentStyle.justifyItems());
        style.setJustifySelfOverflowAlignment(parentStyle.justifyItemsOverflowAlignment());
    }

    if (style.alignSelf() == ItemPositionAuto) {
        style.setAlignSelf(parentStyle.alignItems());
        style.setAlignSelfOverflowAlignment(parentStyle.alignItemsOverflowAlignment());
    }
}

void StyleAdjuster::adjustOverflow(RenderStyle* style)
{
    ASSERT(style->overflowX() != OVISIBLE || style->overflowY() != OVISIBLE);

    // If either overflow value is not visible, change to auto.
    if (style->overflowX() == OVISIBLE && style->overflowY() != OVISIBLE) {
        // FIXME: Once we implement pagination controls, overflow-x should default to hidden
        // if overflow-y is set to -webkit-paged-x or -webkit-page-y. For now, we'll let it
        // default to auto so we can at least scroll through the pages.
        style->setOverflowX(OAUTO);
    } else if (style->overflowY() == OVISIBLE && style->overflowX() != OVISIBLE) {
        style->setOverflowY(OAUTO);
    }
}

}
