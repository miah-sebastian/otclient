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

#include "houses.h"

#include "map.h"

#include <framework/core/resourcemanager.h>

HouseManager g_houses;

House::House(uint32_t hId, const std::string_view name, const Position& pos)
{
    setId(hId);
    setName(name);
    if (pos.isValid())
        setEntry(pos);
}

void House::setTile(const TilePtr& tile)
{
    tile->setFlag(TILESTATE_HOUSE);
    tile->setHouseId(getId());
    m_tiles.emplace(tile->getPosition(), tile);
}

TilePtr House::getTile(const Position& position)
{
    const TileMap::const_iterator iter = m_tiles.find(position);
    if (iter != m_tiles.end())
        return iter->second;
    return nullptr;
}

void House::addDoor(const ItemPtr& door)
{
    if (!door) return;
    door->setDoorId(m_lastDoorId);
    m_doors[++m_lastDoorId] = door;
}

void House::removeDoorById(uint32_t doorId)
{
    if (doorId >= m_lastDoorId)
        throw Exception("Failed to remove door of id %d (would overflow), max id: %d",
                                               doorId, m_lastDoorId);
    m_doors[doorId] = nullptr;
}

void House::load(const TiXmlElement* elem)
{
    std::string name = elem->Attribute("name");
    if (name.empty())
        name = stdext::format("Unnamed house #%lu", getId());

    setName(name);
    setRent(elem->readType<uint32_t >("rent"));
    setSize(elem->readType<uint32_t >("size"));
    setTownId(elem->readType<uint32_t >("townid"));
    m_isGuildHall = elem->readType<bool>("guildhall");

    Position entryPos;
    entryPos.x = elem->readType<int>("entryx");
    entryPos.y = elem->readType<int>("entryy");
    entryPos.z = elem->readType<int>("entryz");
    setEntry(entryPos);
}

void House::save(TiXmlElement* elem)
{
    elem->SetAttribute("name", getName());
    elem->SetAttribute("houseid", getId());

    const Position entry = getEntry();
    elem->SetAttribute("entryx", entry.x);
    elem->SetAttribute("entryy", entry.y);
    elem->SetAttribute("entryz", entry.z);

    elem->SetAttribute("rent", getRent());
    elem->SetAttribute("townid", getTownId());
    elem->SetAttribute("size", getSize());
    elem->SetAttribute("guildhall", m_isGuildHall);
}

HouseManager::HouseManager()
= default;

void HouseManager::addHouse(const HousePtr& house)
{
    if (findHouse(house->getId()) == m_houses.end())
        m_houses.push_back(house);
}

void HouseManager::removeHouse(uint32_t houseId)
{
    const auto it = findHouse(houseId);
    if (it != m_houses.end())
        m_houses.erase(it);
}

HousePtr HouseManager::getHouse(uint32_t houseId)
{
    const auto it = findHouse(houseId);
    return it != m_houses.end() ? *it : nullptr;
}

HousePtr HouseManager::getHouseByName(const std::string_view name)
{
    const auto it = std::find_if(m_houses.begin(), m_houses.end(),
                                 [=](const HousePtr& house) -> bool { return house->getName() == name; });
    return it != m_houses.end() ? *it : nullptr;
}

void HouseManager::load(const std::string& fileName)
{
    try {
        TiXmlDocument doc;
        doc.Parse(g_resources.readFileContents(fileName).c_str());
        if (doc.Error())
            throw Exception("failed to load '%s': %s (House XML)", fileName, doc.ErrorDesc());

        TiXmlElement* root = doc.FirstChildElement();
        if (!root || root->ValueTStr() != "houses")
            throw Exception("invalid root tag name");

        for (TiXmlElement* elem = root->FirstChildElement(); elem; elem = elem->NextSiblingElement()) {
            if (elem->ValueTStr() != "house")
                throw Exception("invalid house tag.");

            const auto houseId = elem->readType<uint32_t >("houseid");
            HousePtr house = getHouse(houseId);
            if (!house)
                house = HousePtr(new House(houseId)), addHouse(house);

            house->load(elem);
        }
    } catch (std::exception& e) {
        g_logger.error(stdext::format("Failed to load '%s': %s", fileName, e.what()));
    }
    sort();
}

void HouseManager::save(const std::string& fileName)
{
    try {
        TiXmlDocument doc;
        doc.SetTabSize(2);

        auto* const decl = new TiXmlDeclaration("1.0", "UTF-8", "");
        doc.LinkEndChild(decl);

        auto* const root = new TiXmlElement("houses");
        doc.LinkEndChild(root);

        for (const auto& house : m_houses) {
            auto* const elem = new TiXmlElement("house");
            house->save(elem);
            root->LinkEndChild(elem);
        }

        if (!doc.SaveFile("data" + fileName))
            throw Exception("failed to save houses XML %s: %s", fileName, doc.ErrorDesc());
    } catch (std::exception& e) {
        g_logger.error(stdext::format("Failed to save '%s': %s", fileName, e.what()));
    }
}

HouseList HouseManager::filterHouses(uint32_t townId)
{
    HouseList ret;
    for (const HousePtr& house : m_houses)
        if (house->getTownId() == townId)
            ret.push_back(house);
    return ret;
}

HouseList::iterator HouseManager::findHouse(uint32_t houseId)
{
    return std::find_if(m_houses.begin(), m_houses.end(),
                        [=](const HousePtr& house) -> bool { return house->getId() == houseId; });
}

void HouseManager::sort()
{
    m_houses.sort([](const HousePtr& lhs, const HousePtr& rhs) { return lhs->getName() < rhs->getName(); });
}

/* vim: set ts=4 sw=4 et: */
