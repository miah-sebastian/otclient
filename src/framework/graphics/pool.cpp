/*
 * Copyright (c) 2010-2022 OTClient <https://github.com/edubart/otclient>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "pool.h"
#include <framework/graphics/framebuffermanager.h>

Pool* Pool::create(const PoolType type)
{
    Pool* pool;
    if (type == PoolType::MAP || type == PoolType::LIGHT || type == PoolType::FOREGROUND) {
        const auto& frameBuffer = g_framebuffers.createFrameBuffer(true);

        pool = new PoolFramed{ frameBuffer };

        if (type == PoolType::MAP) frameBuffer->disableBlend();
        else if (type == PoolType::LIGHT) {
            pool->m_forceGrouping = true;
            frameBuffer->setCompositionMode(CompositionMode::LIGHT);
        }
    } else {
        pool = new Pool;
        pool->m_forceGrouping = true; // CREATURE_INFORMATION & TEXT
    }

    pool->m_type = type;
    return pool;
}

void Pool::add(const Color& color, const TexturePtr& texture, const DrawMethod* method, const DrawMode drawMode, DrawBufferPtr drawBuffer)
{
    const auto& state = Painter::PainterState{
       g_painter->getTransformMatrixRef(), color, m_state.opacity,
       m_state.compositionMode, m_state.blendEquation,
       m_state.clipRect, texture, m_state.shaderProgram
    };

    if (state.shaderProgram)
        m_autoUpdate = true;

    size_t stateHash = 0, methodHash = 0;
    updateHash(state, method, stateHash, methodHash);

    auto& list = m_objects;

    if (m_forceGrouping || drawBuffer) {
        auto& pointer = m_drawObjectPointer;
        if (auto it = pointer.find(stateHash); it != pointer.end()) {
            auto& buffer = list[it->second].buffer;
            if (buffer->isValid()) {
                auto& hashList = buffer->m_hashs;
                if (++buffer->m_i == hashList.size()) {
                    hashList.push_back(methodHash);
                    method->add(*buffer->m_coords, DrawMode::TRIANGLES);
                } else if (hashList[buffer->m_i] != methodHash) {
                    buffer->invalidate();
                }
            }
        } else {
            pointer[stateHash] = list.size();

            if (!drawBuffer) {
                drawBuffer = std::make_shared<DrawBuffer>();
            }

            if (drawBuffer->m_hashs.empty()) {
                if (drawBuffer->m_coords)
                    drawBuffer->m_coords->clear();
                else
                    drawBuffer->m_coords = std::make_shared<CoordsBuffer>();

                drawBuffer->m_hashs.push_back(methodHash);
                method->add(*drawBuffer->m_coords, DrawMode::TRIANGLES);
            }
            drawBuffer->m_i = 0;

            list.push_back({ state, DrawMode::TRIANGLES, {}, drawBuffer });
        }

        delete method;
        return;
    }

    if (!list.empty()) {
        auto& prevObj = list.back();

        const bool sameState = prevObj.state == state;

        if (method->hasRefPoint()) {
            const auto* methodT = static_cast<const DrawTextureRect*>(method);
            // Look for identical or opaque textures that are greater than or
            // equal to the size of the previous texture, if so, remove it from the list so they don't get drawn.
            for (auto itm = prevObj.drawMethods.begin(); itm != prevObj.drawMethods.end(); ++itm) {
                const auto* prevMtd = static_cast<const DrawTextureRect*>(*itm);
                if (prevMtd->m_destP == methodT->m_destP &&
               ((sameState && prevMtd->m_src == methodT->m_src) || (state.texture->isOpaque() && prevObj.state.texture->canSuperimposed()))) {
                    prevObj.drawMethods.erase(itm);
                    delete prevMtd;
                    break;
                }
            }
        }

        if (sameState) {
            prevObj.drawMode = DrawMode::TRIANGLES;
            prevObj.drawMethods.push_back(method);
            return;
        }
    }

    list.push_back({ state, drawMode, {method} });
}

void Pool::updateHash(const Painter::PainterState& state, const DrawMethod* method,
                            size_t& stateHash, size_t& methodhash)
{
    { // State Hash
        if (state.blendEquation != BlendEquation::ADD)
            stdext::hash_combine(stateHash, state.blendEquation);

        if (state.clipRect.isValid()) stdext::hash_union(stateHash, state.clipRect.hash());
        if (state.color != Color::white)
            stdext::hash_combine(stateHash, state.color.rgba());

        if (state.compositionMode != CompositionMode::NORMAL)
            stdext::hash_combine(stateHash, state.compositionMode);

        if (state.opacity < 1.f)
            stdext::hash_combine(stateHash, state.opacity);

        if (state.shaderProgram) {
            m_autoUpdate = true;
            stdext::hash_combine(stateHash, state.shaderProgram->getProgramId());
        }

        if (state.texture) {
            // TODO: use uniqueID id when applying multithreading, not forgetting that in the APNG texture, the id changes every frame.
            stdext::hash_combine(stateHash, !state.texture->isEmpty() ? state.texture->getId() : state.texture->getUniqueId());
        }

        if (state.transformMatrix != DEFAULT_MATRIX_3)
            stdext::hash_union(stateHash, state.transformMatrix.hash());

        stdext::hash_union(m_status.second, stateHash);
    }

    { // Method Hash
        method->updateHash(methodhash);
        stdext::hash_union(m_status.second, methodhash);
    }
}

void Pool::free()
{
    for (const auto& o : m_objects)
        for (const auto* m : o.drawMethods)
            delete m;
}

void Pool::setCompositionMode(const CompositionMode mode, const int pos)
{
    if (pos == -1) {
        m_state.compositionMode = mode;
        return;
    }

    m_objects[pos - 1].state.compositionMode = mode;
    stdext::hash_combine(m_status.second, mode);
}

void Pool::setBlendEquation(BlendEquation equation, const int pos)
{
    if (pos == -1) {
        m_state.blendEquation = equation;
        return;
    }

    m_objects[pos - 1].state.blendEquation = equation;
    stdext::hash_combine(m_status.second, equation);
}

void Pool::setClipRect(const Rect& clipRect, const int pos)
{
    if (pos == -1) {
        m_state.clipRect = clipRect;
        return;
    }

    m_objects[pos - 1].state.clipRect = clipRect;
    stdext::hash_combine(m_status.second, clipRect.hash());
}

void Pool::setOpacity(const float opacity, const int pos)
{
    if (pos == -1) {
        m_state.opacity = opacity;
        return;
    }

    m_objects[pos - 1].state.opacity = opacity;
    stdext::hash_combine(m_status.second, opacity);
}

void Pool::setShaderProgram(const PainterShaderProgramPtr& shaderProgram, const int pos, const std::function<void()>& action)
{
    const auto& shader = shaderProgram ? shaderProgram.get() : nullptr;

    if (pos == -1) {
        m_state.shaderProgram = shader;
        m_state.action = action;
        return;
    }

    if (shader) {
        m_autoUpdate = true;
    }

    auto& o = m_objects[pos - 1];
    o.state.shaderProgram = shader;
    o.state.action = action;
}

void Pool::resetState()
{
    resetOpacity();
    resetClipRect();
    resetShaderProgram();
    resetBlendEquation();
    resetCompositionMode();

    m_autoUpdate = false;
    m_status.second = 0;
    m_drawObjectPointer.clear();
}

bool Pool::hasModification(const bool autoUpdateStatus)
{
    const bool hasModification = m_status.first != m_status.second || (m_autoUpdate && m_refreshTime.ticksElapsed() > 50);

    if (hasModification && autoUpdateStatus)
        updateStatus();

    return hasModification;
}
