#ifndef EDITPEN_H
#define EDITPEN_H

#include"editbase.h"

#include<vector>


class EditPen : public EditBase
{
	struct SegPoint
	{
		SegPoint() : x(0), y(0) {}
		SegPoint(std::size_t x, double y) : x(x), y(y) {}

		std::size_t x;
		double      y;
	};

	SegPoint lastPoint;
	std::size_t actPaintMinX;
	std::size_t actPaintMaxX;

	bool paintSegLine = false;
	bool lineChanged  = false;

	OctData::Segmentationlines::Segmentline* segLine = nullptr;

	void setLinePoint2Point(const SegPoint& p1, const SegPoint& p2, OctData::Segmentationlines::Segmentline& segLine);
	QRect getWidgetPaintSize(const SegPoint& p1, const SegPoint& p2, const ScaleFactor& scaleFactor);
	QRect getWidgetPaintSize(std::size_t x1, std::size_t x2, const ScaleFactor& scaleFactor);

	SegPoint calcPoint(int x, int y, const ScaleFactor& scaleFactor, int bscanWidth);

	void updateMinMaxX(const SegPoint& p);
	BscanMarkerBase::RedrawRequest smoothMinMaxIntervall(const ScaleFactor& scaleFactor);

public:
	EditPen(BScanLayerSegmentation* base) : EditBase(base) {}
	virtual ~EditPen() {}


	virtual void drawMarker(QPainter& painter, BScanMarkerWidget* widget, const QRect& /*drawrect*/, const ScaleFactor& scaleFactor) const override;

	virtual BscanMarkerBase::RedrawRequest mouseMoveEvent   (QMouseEvent*, BScanMarkerWidget*) override;
	virtual BscanMarkerBase::RedrawRequest mousePressEvent  (QMouseEvent*, BScanMarkerWidget*) override;
	virtual BscanMarkerBase::RedrawRequest mouseReleaseEvent(QMouseEvent*, BScanMarkerWidget*) override;


	virtual void segLineChanged(OctData::Segmentationlines::Segmentline* segLine) override;


};

#endif // EDITPEN_H

