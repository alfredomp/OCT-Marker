#ifndef MARKERMANAGER_H
#define MARKERMANAGER_H

#include <QObject>
#include <vector>

#include <boost/property_tree/ptree_fwd.hpp>

namespace OctData { class Series; }

class BscanMarkerBase;

class BScanMarkerManager : public QObject
{
public:

	BScanMarkerManager();
	virtual ~BScanMarkerManager();

	int getActBScan() const                                         { return actBScan; }

	const OctData::Series* getSeries() const                        { return series;   }


	BscanMarkerBase* getActMarker()                                 { return actMarker; }
	int getActMarkerId() const                                      { return actMarkerId; }
	
	const std::vector<BscanMarkerBase*>& getMarker() const          { return markerObj; }


private:
	int                    actBScan = 0;
	const OctData::Series* series   = nullptr;
	
	std::vector<BscanMarkerBase*> markerObj;
	BscanMarkerBase* actMarker = nullptr;
	int actMarkerId = -1;

	boost::property_tree::ptree* markerTree = nullptr;
	
private slots:
	virtual void saveMarkerStateSlot(const OctData::Series* series);
	virtual void loadMarkerStateSlot(const OctData::Series* series);

	virtual void udateFromMarkerModul();

public slots:
	virtual void chooseBScan(int bscan);
	virtual void inkrementBScan(int inkrement)                      { chooseBScan(actBScan + inkrement); }

	virtual void nextBScan()                                        { inkrementBScan(+1); }
	virtual void previousBScan()                                    { inkrementBScan(-1); }

	virtual void showSeries(const OctData::Series* series);
	
	virtual void setMarker(int id);

signals:
	void bscanChanged(int bscan);
	void newSeriesShowed(const OctData::Series* series);

private:
	Q_OBJECT
};

#endif // MARKERMANAGER_H
