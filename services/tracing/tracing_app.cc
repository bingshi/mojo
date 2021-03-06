// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/tracing/tracing_app.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"

namespace tracing {

TracingApp::TracingApp() : coordinator_binding_(this), tracing_active_(false) {
}

TracingApp::~TracingApp() {
}

bool TracingApp::ConfigureIncomingConnection(
    mojo::ApplicationConnection* connection) {
  connection->AddService<TraceCoordinator>(this);

  // If someone connects to us they may want to use the TraceCoordinator
  // interface and/or they may want to expose themselves to be traced. Attempt
  // to connect to the TraceController interface to see if the application
  // connecting to us wants to be traced. They can refuse the connection or
  // close the pipe if not.
  TraceControllerPtr controller_ptr;
  connection->ConnectToService(&controller_ptr);
  if (tracing_active_) {
    TraceDataCollectorPtr collector_ptr;
    collector_impls_.push_back(
        new CollectorImpl(GetProxy(&collector_ptr), sink_.get()));
    controller_ptr->StartTracing(tracing_categories_, collector_ptr.Pass());
  }
  controller_ptrs_.AddInterfacePtr(controller_ptr.Pass());
  return true;
}

// mojo::InterfaceFactory<TraceCoordinator> implementation.
void TracingApp::Create(mojo::ApplicationConnection* connection,
                        mojo::InterfaceRequest<TraceCoordinator> request) {
  coordinator_binding_.Bind(request.Pass());
}

// tracing::TraceCoordinator implementation.
void TracingApp::Start(mojo::ScopedDataPipeProducerHandle stream,
                       const mojo::String& categories) {
  tracing_categories_ = categories;
  sink_.reset(new TraceDataSink(stream.Pass()));
  controller_ptrs_.ForAllPtrs([categories, this](TraceController* controller) {
    TraceDataCollectorPtr ptr;
    collector_impls_.push_back(new CollectorImpl(GetProxy(&ptr), sink_.get()));
    controller->StartTracing(categories, ptr.Pass());
  });
  tracing_active_ = true;
}

void TracingApp::StopAndFlush() {
  tracing_active_ = false;
  controller_ptrs_.ForAllPtrs(
      [](TraceController* controller) { controller->StopTracing(); });

  // Sending the StopTracing message to registered controllers will request that
  // they send trace data back via the collector interface and, when they are
  // done, close the collector pipe. We don't know how long they will take. We
  // want to read all data that any collector might send until all collectors or
  // closed or an (arbitrary) deadline has passed. Since the bindings don't
  // support this directly we do our own MojoWaitMany over the handles and read
  // individual messages until all are closed or our absolute deadline has
  // elapsed.
  static const MojoDeadline kTimeToWaitMicros = 100 * 1000;
  MojoTimeTicks end = MojoGetTimeTicksNow() + kTimeToWaitMicros;

  while (!collector_impls_.empty()) {
    MojoTimeTicks now = MojoGetTimeTicksNow();
    if (now >= end)  // Timed out?
      break;

    MojoDeadline mojo_deadline = end - now;
    std::vector<mojo::Handle> handles;
    std::vector<MojoHandleSignals> signals;
    for (const auto& it : collector_impls_) {
      handles.push_back(it->TraceDataCollectorHandle());
      signals.push_back(MOJO_HANDLE_SIGNAL_READABLE |
                        MOJO_HANDLE_SIGNAL_PEER_CLOSED);
    }
    std::vector<MojoHandleSignalsState> signals_states(signals.size());
    const mojo::WaitManyResult wait_many_result =
        mojo::WaitMany(handles, signals, mojo_deadline, &signals_states);
    if (wait_many_result.result == MOJO_RESULT_DEADLINE_EXCEEDED) {
      // Timed out waiting, nothing more to read.
      break;
    }
    if (wait_many_result.IsIndexValid()) {
      // Iterate backwards so we can remove closed pipes from |collector_impls_|
      // without invalidating subsequent offsets.
      for (size_t i = signals_states.size(); i != 0; --i) {
        size_t index = i - 1;
        MojoHandleSignals satisfied = signals_states[index].satisfied_signals;
        if (satisfied & MOJO_HANDLE_SIGNAL_READABLE)
          collector_impls_[index]->TryRead();
        if (satisfied & MOJO_HANDLE_SIGNAL_PEER_CLOSED)
          collector_impls_.erase(collector_impls_.begin() + index);
      }
    }
  }
  AllDataCollected();
}

void TracingApp::AllDataCollected() {
  collector_impls_.clear();
  sink_.reset();
}

}  // namespace tracing
