/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SKY_ENGINE_CORE_ANIMATION_ANIMATIONPLAYER_H_
#define SKY_ENGINE_CORE_ANIMATION_ANIMATIONPLAYER_H_

#include "sky/engine/core/animation/AnimationNode.h"
#include "sky/engine/core/dom/ActiveDOMObject.h"
#include "sky/engine/core/events/EventTarget.h"
#include "sky/engine/wtf/RefPtr.h"

namespace blink {

class AnimationTimeline;
class ExceptionState;

class AnimationPlayer final : public RefCounted<AnimationPlayer>
    , public ActiveDOMObject
    , public EventTargetWithInlineData {
    DEFINE_WRAPPERTYPEINFO();
    REFCOUNTED_EVENT_TARGET(AnimationPlayer);
public:
    enum AnimationPlayState {
        Idle,
        Pending,
        Running,
        Paused,
        Finished
    };

    ~AnimationPlayer();
    static PassRefPtr<AnimationPlayer> create(ExecutionContext*, AnimationTimeline&, AnimationNode*);

    // Returns whether the player is finished.
    bool update(TimingUpdateReason);

    // timeToEffectChange returns:
    //  infinity  - if this player is no longer in effect
    //  0         - if this player requires an update on the next frame
    //  n         - if this player requires an update after 'n' units of time
    double timeToEffectChange();

    void cancel();

    double currentTime();
    void setCurrentTime(double newCurrentTime);

    double calculateCurrentTime() const;
    double currentTimeInternal();
    void setCurrentTimeInternal(double newCurrentTime, TimingUpdateReason = TimingUpdateOnDemand);

    bool paused() const { return m_paused && !m_isPausedForTesting; }
    String playState();
    AnimationPlayState playStateInternal();

    void pause();
    void play();
    void reverse();
    void finish(ExceptionState&);
    bool finished() { return limited(currentTimeInternal()); }
    bool playing() { return !(finished() || m_paused || m_isPausedForTesting); }
    // FIXME: Resolve whether finished() should just return the flag, and
    // remove this method.
    bool finishedInternal() const { return m_finished; }

    virtual const AtomicString& interfaceName() const override;
    virtual ExecutionContext* executionContext() const override;
    virtual bool hasPendingActivity() const override;
    virtual void stop() override;
    virtual bool dispatchEvent(PassRefPtr<Event>) override;

    double playbackRate() const { return m_playbackRate; }
    void setPlaybackRate(double);
    const AnimationTimeline* timeline() const { return m_timeline; }
    AnimationTimeline* timeline() { return m_timeline; }

    void timelineDestroyed() { m_timeline = nullptr; }

    double calculateStartTime(double currentTime) const;
    bool hasStartTime() const { return !isNull(m_startTime); }
    double startTime() const;
    double startTimeInternal() const { return m_startTime; }
    void setStartTime(double);
    void setStartTimeInternal(double);

    const AnimationNode* source() const { return m_content.get(); }
    AnimationNode* source() { return m_content.get(); }
    void setSource(AnimationNode*);

    // Pausing via this method is not reflected in the value returned by
    // paused() and must never overlap with pausing via pause().
    void pauseForTesting(double pauseTime);
    // This should only be used for CSS
    void unpause();

    void setOutdated();
    bool outdated() { return m_outdated; }

    void setPending();
    void notifyCompositorStartTime(double timelineTime);


    void preCommit();
    void postCommit(double timelineTime);

    unsigned sequenceNumber() const { return m_sequenceNumber; }

    static bool hasLowerPriority(AnimationPlayer* player1, AnimationPlayer* player2)
    {
        return player1->sequenceNumber() < player2->sequenceNumber();
    }

    // Checks if the AnimationStack is the last reference holder to the Player.
    bool canFree() const;

    virtual bool addEventListener(const AtomicString& eventType, PassRefPtr<EventListener>, bool useCapture = false) override;

private:
    AnimationPlayer(ExecutionContext*, AnimationTimeline&, AnimationNode*);
    double sourceEnd() const;
    bool limited(double currentTime) const;
    void updateCurrentTimingState(TimingUpdateReason);
    void unpauseInternal();

    double m_playbackRate;

    double m_startTime;
    double m_holdTime;

    unsigned m_sequenceNumber;

    RefPtr<AnimationNode> m_content;
    RawPtr<AnimationTimeline> m_timeline;
    // Reflects all pausing, including via pauseForTesting().
    bool m_paused;
    bool m_held;
    bool m_isPausedForTesting;

    // This indicates timing information relevant to the player's effect
    // has changed by means other than the ordinary progression of time
    bool m_outdated;

    bool m_finished;
    // Holds a 'finished' event queued for asynchronous dispatch via the
    // ScriptedAnimationController. This object remains active until the
    // event is actually dispatched.
    RefPtr<Event> m_pendingFinishedEvent;

    bool m_pending;
    bool m_currentTimePending;
};

} // namespace blink

#endif  // SKY_ENGINE_CORE_ANIMATION_ANIMATIONPLAYER_H_
