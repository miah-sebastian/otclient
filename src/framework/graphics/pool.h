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

#pragma once

#include <utility>

#include "declarations.h"
#include "framebuffer.h"
#include "framework/core/timer.h"
#include "texture.h"
#include "drawmethod.h"

#include "../stdext/storage.h"

enum class PoolType : uint8_t
{
    MAP,
    CREATURE_INFORMATION,
    LIGHT,
    TEXT,
    FOREGROUND,
    UNKNOW
};

class DrawBuffer
{
public:
    static const DrawBufferPtr create() { return std::make_shared<DrawBuffer>(); }

    bool isValid() { return m_i > -1; }
    bool validate(const Point& p)
    {
        if (m_ref != p) { m_ref = p; invalidate(); }
        return isValid();
    }

private:
    void invalidate() { m_i = -1; m_hashs.clear(); }
    int m_i{ 0 };
    Point m_ref;

    std::vector<size_t> m_hashs;
    std::shared_ptr<CoordsBuffer> m_coords;

    friend class Pool;
    friend class DrawPool;
};

class Pool
{
public:
    void setEnable(const bool v) { m_enabled = v; }
    bool isEnabled() const { return m_enabled; }
    PoolType getType() const { return m_type; }

protected:

private:
    static Pool* create(const PoolType type);

    void add(const Color& color, const TexturePtr& texture, const DrawMethod* method, DrawMode drawMode = DrawMode::TRIANGLES, DrawBufferPtr drawBuffer = nullptr);
    void updateHash(const Painter::PainterState& state, const DrawMethod* method, size_t& stateHash, size_t& methodHash);
    void free();

    float getOpacity(const int pos = -1) { return pos == -1 ? m_state.opacity : m_objects[pos - 1].state.opacity; }
    Rect getClipRect(const int pos = -1) { return pos == -1 ? m_state.clipRect : m_objects[pos - 1].state.clipRect; }

    void setCompositionMode(CompositionMode mode, int pos = -1);
    void setBlendEquation(BlendEquation equation, int pos = -1);
    void setClipRect(const Rect& clipRect, int pos = -1);
    void setOpacity(float opacity, int pos = -1);
    void setShaderProgram(const PainterShaderProgramPtr& shaderProgram, int pos = -1, const std::function<void()>& action = nullptr);

    void resetState();
    void resetOpacity() { m_state.opacity = 1.f; }
    void resetClipRect() { m_state.clipRect = {}; }
    void resetShaderProgram() { m_state.shaderProgram = nullptr; }
    void resetCompositionMode() { m_state.compositionMode = CompositionMode::NORMAL; }
    void resetBlendEquation() { m_state.blendEquation = BlendEquation::ADD; }

    void flush() { m_drawObjectPointer.clear(); }

    virtual bool hasFrameBuffer() const { return false; };
    virtual PoolFramed* toPoolFramed() { return nullptr; }

    bool hasModification(bool autoUpdateStatus = false);
    void updateStatus() { m_status.first = m_status.second; m_refreshTime.restart(); }

    bool m_enabled{ true },
        m_forceGrouping{ false },
        m_autoUpdate{ false };

    _DrawState m_state;

    PoolType m_type{ PoolType::UNKNOW };

    Timer m_refreshTime;

    std::pair<size_t, size_t> m_status{ 0,0 };

    std::vector<DrawObject> m_objects;

    stdext::unordered_map<size_t, size_t> m_drawObjectPointer;

    friend class DrawPool;
};

class PoolFramed : public Pool
{
public:
    void onBeforeDraw(std::function<void()> f) { m_beforeDraw = std::move(f); }
    void onAfterDraw(std::function<void()> f) { m_afterDraw = std::move(f); }
    void setSmooth(bool enabled) { m_framebuffer->setSmooth(enabled); }
    void resize(const Size& size) { m_framebuffer->resize(size); }
    Size getSize() { return m_framebuffer->getSize(); }

protected:
    PoolFramed(const FrameBufferPtr& fb) : m_framebuffer(fb) {};

    friend class DrawPool;
    friend class Pool;

private:
    bool hasFrameBuffer() const override { return true; }
    PoolFramed* toPoolFramed() override { return this; }

    FrameBufferPtr m_framebuffer;
    Rect m_dest, m_src;

    std::function<void()> m_beforeDraw, m_afterDraw;
};

extern DrawPool g_drawPool;
