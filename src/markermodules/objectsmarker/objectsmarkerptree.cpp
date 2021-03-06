/*
 * Copyright (c) 2018 Kay Gawlik <kaydev@amarunet.de> <kay.gawlik@beuth-hochschule.de> <kay.gawlik@charite.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "objectsmarkerptree.h"


#include <helper/ptreehelper.h>
#include <boost/property_tree/ptree.hpp>
namespace bpt = boost::property_tree;


#include"objectsmarker.h"
#include"objectsmarkerfactory.h"

#include <markerobjects/rectitem.h>

namespace
{
	template<typename T>
	inline T readOptinalNode(const bpt::ptree& basis, const char* nodeName, T defaultValue)
	{
		boost::optional<const bpt::ptree&> optNode  = basis.get_child_optional(nodeName);
		if(optNode)
			return optNode->get_value<T>(defaultValue);
		return defaultValue;
	}

	void saveItemState(bpt::ptree& ptree, const RectItem& item)
	{
		QRectF  rect = item.rect();
		QPointF pos  = item.mapToScene(rect.topLeft());

		ptree.put("ItemType"  , "Rect"       );
		ptree.put("PosX"      , pos.x      ());
		ptree.put("PosY"      , pos.y      ());
		ptree.put("Height"    , rect.height());
		ptree.put("Width"     , rect.width ());
	}

	void loadItemState(const bpt::ptree& ptree, RectItem& item)
	{
		double posX   = readOptinalNode<double>(ptree, "PosX"  , 100);
		double posY   = readOptinalNode<double>(ptree, "PosY"  , 100);
		double height = readOptinalNode<double>(ptree, "Height", 50);
		double width  = readOptinalNode<double>(ptree, "Width" , 50);

		QPointF pos  = item.mapFromScene(QPointF(posX, posY));
		QRectF  rect(pos.x(), pos.y(), width, height);

		item.setRect(rect);
	}
}


void ObjectsMarkerPTree::fillPTree(boost::property_tree::ptree& ptree, const Objectsmarker* markerManager)
{
	ptree.clear();
	const std::vector<std::vector<RectItem*>>& itemsList = markerManager->itemsList;

	std::size_t numBscans = itemsList.size();
	for(std::size_t bscan = 0; bscan < numBscans; ++bscan)
	{
		const std::vector<RectItem*>& items = itemsList[bscan];
		if(items.size() == 0)
			continue;

		static const std::string nodeName = "BScan";
		bpt::ptree& bscanNode = PTreeHelper::getNodeWithId(ptree, nodeName, static_cast<int>(bscan));
		bpt::ptree& objectsNode = bscanNode.add("Objects", "");

		for(const RectItem* item : items)
		{
			if(!item)
				continue;
			bpt::ptree itemNode;
			saveItemState(itemNode, *item);
			objectsNode.push_back(std::make_pair("", itemNode));
		}
	}
}

bool ObjectsMarkerPTree::parsePTree(const boost::property_tree::ptree& ptree, Objectsmarker* markerManager, const ObjectsmarkerFactory& factory)
{
	markerManager->removeAllItems();

	std::vector<std::vector<RectItem*>>& itemsList = markerManager->itemsList;
	std::size_t numBscans = itemsList.size();


	for(const std::pair<const std::string, const bpt::ptree>& bscanPair : ptree)
	{
		if(bscanPair.first != "BScan")
			continue;

		try
		{
			const bpt::ptree& bscanNode = bscanPair.second;
			std::size_t bscanId         = bscanNode.get_child("ID").get_value<std::size_t>();

			if(numBscans <= bscanId)
				continue; // TODO: Error message

			std::vector<RectItem*>& items = itemsList[bscanId];


			boost::optional<const bpt::ptree&> ptreeObjects  = bscanNode.get_child_optional("Objects");

			for(const std::pair<const std::string, bpt::ptree>& child : *ptreeObjects)
			{
// 				const std::string& itemName = child.first;
				      std::string  itemType = readOptinalNode(child.second, "ItemType", std::string());

				if(itemType == "Rect")
				{
					RectItem* item = factory.createObject();
					loadItemState(child.second, *item);
					items.push_back(item);
				}
			}
		}
		catch(...)
		{
		}
	}



	return true;
}
