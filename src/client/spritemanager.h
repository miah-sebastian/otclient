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

#include <framework/core/declarations.h>
#include <framework/graphics/declarations.h>

 //@bindsingleton g_sprites
class SpriteManager
{
public:
    void init();
    void terminate();

    bool loadSpr(std::string file);
    void unload();

    void saveSpr(const std::string& fileName);

    uint32_t getSignature() { return m_signature; }
    int getSpritesCount() { return m_spritesCount; }

    ImagePtr getSpriteImage(int id);
    bool isLoaded() { return m_loaded; }

    const TexturePtr& getLightTexture() const { return m_lightTexture; }
    const TexturePtr& getShadeTexture() const { return m_shadeTexture; }

private:
    enum
    {
        SPRITE_DATA_SIZE = SPRITE_SIZE * SPRITE_SIZE * 4
    };

    void generateLightTexture(),
        generateShadeTexture();

    TexturePtr m_lightTexture, m_shadeTexture;

    bool m_loaded{ false };
    uint32_t m_signature{ 0 };
    int m_spritesCount{ 0 },
        m_spritesOffset{ 0 };
    FileStreamPtr m_spritesFile;
};

extern SpriteManager g_sprites;
