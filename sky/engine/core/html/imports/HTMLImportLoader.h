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

#ifndef SKY_ENGINE_CORE_HTML_IMPORTS_HTMLIMPORTLOADER_H_
#define SKY_ENGINE_CORE_HTML_IMPORTS_HTMLIMPORTLOADER_H_

#include "sky/engine/platform/fetcher/MojoFetcher.h"
#include "sky/engine/platform/heap/Handle.h"
#include "sky/engine/wtf/OwnPtr.h"
#include "sky/engine/wtf/PassOwnPtr.h"
#include "sky/engine/wtf/Vector.h"

namespace blink {

class Document;
class HTMLImportChild;
class HTMLImportsController;
class Module;

class HTMLImportLoader final : public MojoFetcher::Client {
public:
    enum State {
        StateLoading,
        StateWritten,
        StateParsed,
        StateLoaded,
        StateError
    };

    static PassOwnPtr<HTMLImportLoader> create(HTMLImportsController* controller)
    {
        return adoptPtr(new HTMLImportLoader(controller));
    }

    virtual ~HTMLImportLoader();

    Document* document() const { return m_document.get(); }
    Module* module() const { return m_module.get(); }

    void addImport(HTMLImportChild*);
#if !ENABLE(OILPAN)
    void removeImport(HTMLImportChild*);
#endif
    void moveToFirst(HTMLImportChild*);
    HTMLImportChild* firstImport() const { return m_imports[0]; }
    bool isFirstImport(const HTMLImportChild* child) const { return m_imports.size() ? firstImport() == child : false; }

    bool isDone() const { return m_state == StateLoaded || m_state == StateError; }
    bool hasError() const { return m_state == StateError; }
    bool shouldBlockScriptExecution() const;

#if !ENABLE(OILPAN)
    void importDestroyed();
#endif
    void startLoading(const KURL&);

    // Tells the loader that the parser is done with this import.
    // Called by Document::finishedParsing, after DOMContentLoaded was dispatched.
    void didFinishParsing();

private:
    HTMLImportLoader(HTMLImportsController*);

    // MojoFetcher::Client
    void OnReceivedResponse(mojo::URLResponsePtr) override;

    State startWritingAndParsing(mojo::URLResponsePtr);
    State finishWriting();
    State finishParsing();
    State finishLoading();

    void setState(State);
    void didFinishLoading();
#if !ENABLE(OILPAN)
    void clear();
#endif

    RawPtr<HTMLImportsController> m_controller;
    Vector<RawPtr<HTMLImportChild> > m_imports;
    State m_state;
    RefPtr<Module> m_module;
    RefPtr<Document> m_document;

    OwnPtr<MojoFetcher> m_fetcher;
};

} // namespace blink

#endif  // SKY_ENGINE_CORE_HTML_IMPORTS_HTMLIMPORTLOADER_H_
