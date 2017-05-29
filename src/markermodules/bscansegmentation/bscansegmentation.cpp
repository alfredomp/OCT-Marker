#include "bscansegmentation.h"

#include <QPainter>
#include <QMouseEvent>
#include <QWidget>

#include <manager/octmarkermanager.h>

#include <widgets/bscanmarkerwidget.h>

#include <octdata/datastruct/bscan.h>
#include <octdata/datastruct/series.h>

#include <opencv/cv.h>

#include "bscansegmentationptree.h"

#include "wgsegmentation.h"
#include "bscansegalgorithm.h"
#include "bscansegtoolbar.h"

#include "bscanseglocalop.h"
#include "bscanseglocalopnn.h"

#include <data_structure/simplecvmatcompress.h>
#include "importsegmentation.h"


BScanSegmentation::BScanSegmentation(OctMarkerManager* markerManager)
: BscanMarkerBase(markerManager)
, actMat(new cv::Mat)
{
	name = tr("Segmentation marker");
	id   = "SegmentationMarker";
	
	icon = QIcon(":/icons/segline_edit.png");

	localOpPaint              = new BScanSegLocalOpPaint    (*this);
	localOpThresholdDirection = new BScanSegLocalOpThresholdDirection(*this);
	localOpThreshold          = new BScanSegLocalOpThreshold(*this);
	localOpOperation          = new BScanSegLocalOpOperation(*this);
#ifdef ML_SUPPORT
	localOpNN                 = new BScanSegLocalOpNN       (*this);
#endif

	setLocalMethod(BScanSegmentationMarker::LocalMethod::Paint);

	widget = new WGSegmentation(this);
	widgetPtr2WGSegmentation = widget;

	
	// connect(markerManager, &BScanMarkerManager::newSeriesShowed, this, &BScanSegmentation::newSeriesLoaded);
}

BScanSegmentation::~BScanSegmentation()
{
	clearSegments();

	delete localOpPaint             ;
	delete localOpThresholdDirection;
	delete localOpThreshold         ;
	delete localOpOperation         ;
#ifdef ML_SUPPORT
	delete localOpNN                ;
#endif

	delete widget;
	delete actMat;
}

QToolBar* BScanSegmentation::createToolbar(QObject* parent)
{
	BScanSegToolBar* toolBar = new BScanSegToolBar(this, parent);
	connectToolBar(toolBar);

	return toolBar;
}



// ----------------------
// Draw segmentation line
// ----------------------

namespace
{
	struct PaintFactor1
	{
		inline static void paint(QPainter& painter, const uint8_t* p00, const uint8_t* p10, const uint8_t* p01, int w, int h, uint8_t mask, double /*factor*/)
		{
			if((*p00 & mask) != (*p10 & mask))
			{
				painter.drawPoint(w  , h);
				painter.drawPoint(w+1, h);
			}
			if((*p00 & mask) != (*p01 & mask))
			{
				painter.drawPoint(w, h  );
				painter.drawPoint(w, h+1);
			}
		}
	};

	struct PaintFactorN
	{
		inline static void paint(QPainter& painter, const uint8_t* p00, const uint8_t* p10, const uint8_t* p01, int w, int h, uint8_t mask, double factor)
		{
			if((*p00 & mask) != (*p10 & mask)) painter.drawLine(static_cast<int>((w+1)*factor + 0.5), static_cast<int>((h  )*factor + 0.5), static_cast<int>((w+1)*factor + 0.5), static_cast<int>((h+1)*factor + 0.5));
			if((*p00 & mask) != (*p01 & mask)) painter.drawLine(static_cast<int>((w  )*factor + 0.5), static_cast<int>((h+1)*factor + 0.5), static_cast<int>((w+1)*factor + 0.5), static_cast<int>((h+1)*factor + 0.5));
		}
	};

	template<typename T>
	void drawSegmentLineRec(QPainter& painter, const cv::Mat& actMat, uint8_t mask, double factor, int startH, int endH, int startW, int endW)
	{
		for(int h = startH; h < endH; ++h)
		{
			const uint8_t* p00 = actMat.ptr<uint8_t>(h);
			const uint8_t* p10 = p00+1;
			const uint8_t* p01 = actMat.ptr<uint8_t>(h+1);

			p00 += startW;
			p10 += startW;
			p01 += startW;

			for(int w = startW; w < endW; ++w)
			{
				T::paint(painter, p00, p10, p01, w, h, mask, factor); // for faster drawing, only int supported (and only int used at the moment)

				++p00;
				++p10;
				++p01;
			}
			T::paint(painter, p00, p00, p01, endW, h, mask, factor); // last col p10 replaced by p00, because p10 is outside
		}
	}
	// TODO draw last row

}


template<typename T>
void BScanSegmentation::drawSegmentLine(QPainter& painter, double factor, const QRect& rect) const
{
	if(actMatNr != getActBScanNr())
	{
		qDebug("BScanSegmentation::drawSegmentLine: actMatNr != getActBScanNr()");
		return;
	}
	if(!actMat || actMat->empty())
		return;

	if(factor <= 0)
		return;

	int drawX      = static_cast<int>((rect.x()     )/factor + 0.5)-2;
	int drawY      = static_cast<int>((rect.y()     )/factor + 0.5)-2;
	int drawWidth  = static_cast<int>((rect.width() )/factor + 0.5)+4;
	int drawHeight = static_cast<int>((rect.height())/factor + 0.5)+4;


	int mapHeight = actMat->rows-1; // -1 for p01
	int mapWidth  = actMat->cols-1; // -1 for p10

	int startH = std::max(drawY, 0);
	int endH   = std::min(drawY+drawHeight, mapHeight);
	int startW = std::max(drawX, 0);
	int endW   = std::min(drawX+drawWidth , mapWidth);

	painter.setPen(QPen(Qt::green));
	drawSegmentLineRec<T>(painter, *actMat, 1 << 1, factor, startH, endH, startW, endW);


	QPen pen(Qt::red);
	pen.setWidth(seglinePaintSize);
	painter.setPen(pen);
	drawSegmentLineRec<T>(painter, *actMat, 1 << 0, factor, startH, endH, startW, endW);

	/*
	for(int h = startH; h < endH; ++h)
	{
		uint8_t* p00 = actMat->ptr<uint8_t>(h);
		uint8_t* p10 = p00+1;
		uint8_t* p01 = actMat->ptr<uint8_t>(h+1);

		p00 += startW;
		p10 += startW;
		p01 += startW;

		for(int w = startW; w < endW; ++w)
		{
			T::paint(painter, p00, p10, p01, w, h, factor); // for faster drawing, only int supported (and only int used at the moment)

			++p00;
			++p10;
			++p01;
		}
		T::paint(painter, p00, p00, p01, endW, h, factor); // last col p10 replaced by p00, because p10 is outside
	}
	// TODO draw last row
	*/
}

void BScanSegmentation::transformCoordWidget2Mat(int xWidget, int yWidget, double factor, int& xMat, int& yMat)
{
	yMat = static_cast<int>(yWidget/factor + 0.5);
	xMat = static_cast<int>(xWidget/factor + 0.5);
}




void BScanSegmentation::drawMarker(QPainter& p, BScanMarkerWidget* widget, const QRect& rect) const
{
	double factor = widget->getImageScaleFactor();
	if(factor <= 0)
		return;
	
	if(factor == 1)
		drawSegmentLine<PaintFactor1>(p, factor, rect);
	else
		drawSegmentLine<PaintFactorN>(p, factor, rect);

	QPoint paintPoint = mousePoint;
	if(factor > 1)
	{
		int x = static_cast<int>(std::round(static_cast<double>(mousePoint.x())/factor)*factor + 0.5);
		int y = static_cast<int>(std::round(static_cast<double>(mousePoint.y())/factor)*factor + 0.5);
		paintPoint = QPoint(x, y);
	}

	if(inWidget && markerActive && actLocalOperator)
	{
		QPen pen(Qt::green);
		p.setPen(pen);
		actLocalOperator->drawMarkerPaint(p, paintPoint, factor);
	}
}


bool BScanSegmentation::setOnCoord(int x, int y, double factor)
{
	if(factor == 0)
		return false;

	int xD, yD;
	transformCoordWidget2Mat(x, y, factor, xD, yD);

	if(actLocalOperator)
		return actLocalOperator->drawOnCoord(xD, yD);

	return false;
}



bool BScanSegmentation::startOnCoord(int x, int y, double factor)
{
	if(factor == 0)
		return false;

	int xD, yD;
	transformCoordWidget2Mat(x, y, factor, xD, yD);

	if(actLocalOperator)
		actLocalOperator->startOnCoord(xD, yD);

	return true;
}




uint8_t BScanSegmentation::valueOnCoord(int x, int y)
{
	setActMat(getActBScanNr());
	if(!actMat || actMat->empty())
		return BScanSegmentationMarker::markermatInitialValue;

	return actMat->at<uint8_t>(cv::Point(x, y));
}


QRect BScanSegmentation::getWidgetPaintSize(const QPoint& p1, const QPoint& p2, double factor)
{
	int height = 2;
	int width  = 2;
	if(actLocalOperator)
	{
		height += actLocalOperator->getOperatorHeight();
		width  += actLocalOperator->getOperatorWidth ();
	}

	height = static_cast<int>(height*factor + 0.5);
	width  = static_cast<int>(width *factor + 0.5);

	QRect rect = QRect(p1, p2).normalized(); // old and new pos
	rect.adjust(-width, -height, width, height);

	return rect;
}


BscanMarkerBase::RedrawRequest BScanSegmentation::mouseMoveEvent(QMouseEvent* e, BScanMarkerWidget* widget)
{
	RedrawRequest result;

	inWidget = true;

	if(markerActive)
	{
		if(!(e->buttons() & Qt::LeftButton))
			paint = false;

		double factor = widget->getImageScaleFactor();

		result.redraw = false; // cursor need redraw
		result.rect   = getWidgetPaintSize(mousePoint, e->pos(), factor);

		int x = e->x();
		int y = e->y();

		if(actLocalOperator)
		{
			if(paint)
				result.redraw = setOnCoord(x, y, factor);

			result.redraw |= actLocalOperator->drawMarker();
		}
	}
	mousePoint = e->pos();

	return result;
}

BscanMarkerBase::RedrawRequest  BScanSegmentation::mousePressEvent(QMouseEvent* e, BScanMarkerWidget* widget)
{
	double factor = widget->getImageScaleFactor();

	RedrawRequest result;
	result.redraw = false;
	result.rect   = getWidgetPaintSize(mousePoint, e->pos(), factor);

	paint = (e->buttons() == Qt::LeftButton);
	if(paint)
	{
		startOnCoord(e->x(), e->y(), factor);
		result.redraw = setOnCoord(e->x(), e->y(), factor);
	}
	return result;
}

BscanMarkerBase::RedrawRequest  BScanSegmentation::mouseReleaseEvent(QMouseEvent* e, BScanMarkerWidget* widget)
{
	double factor = widget->getImageScaleFactor();

	RedrawRequest result;
	result.redraw = false;
	result.rect   = getWidgetPaintSize(mousePoint, e->pos(), factor);

	paint = false;
	if(factor == 0)
		return result;

	if(actLocalOperator)
	{
		int x = e->x();
		int y = e->y();

		int xD, yD;
		transformCoordWidget2Mat(x, y, factor, xD, yD);

		result.redraw = actLocalOperator->endOnCoord(xD, yD);
	}

	return result;
}




bool BScanSegmentation::leaveWidgetEvent(QEvent*, BScanMarkerWidget*)
{
	inWidget = false;
	return true;
}

bool BScanSegmentation::keyPressEvent(QKeyEvent* e, BScanMarkerWidget*)
{
	switch(e->key())
	{
		case Qt::Key_X:
			opencloseBScan();
			return true;
		case Qt::Key_V:
			medianBScan();
			return true;
		case Qt::Key_U:
			removeUnconectedAreas();
			return true;
		case Qt::Key_I:
			extendLeftRightSpace();
			return true;
		case Qt::Key_1:
			setLocalMethod(BScanSegmentationMarker::LocalMethod::ThresholdDirection);
			return true;
		case Qt::Key_2:
			setLocalMethod(BScanSegmentationMarker::LocalMethod::Paint);
			return true;
		case Qt::Key_3:
			setLocalMethod(BScanSegmentationMarker::LocalMethod::Operation);
			return true;
		case Qt::Key_4:
			setLocalMethod(BScanSegmentationMarker::LocalMethod::Threshold);
			return true;
		case Qt::Key_5:
			setLocalMethod(BScanSegmentationMarker::LocalMethod::NN);
			return true;
		case Qt::Key_Z:
			if(e->modifiers() == Qt::ControlModifier)
			{
				rejectMatChanges();
				return true;
			}
	}

	return false;
}





void BScanSegmentation::dilateBScan()
{
	setActMat(getActBScanNr());
	if(!actMat || actMat->empty())
		return;

	int iterations = 1;
	cv::dilate(*actMat, *actMat, cv::Mat(), cv::Point(-1, -1), iterations, cv::BORDER_REFLECT_101, 1);

	requestFullUpdate();
}

void BScanSegmentation::erodeBScan()
{
	setActMat(getActBScanNr());
	if(!actMat || actMat->empty())
		return;

	int iterations = 1;
	cv::erode(*actMat, *actMat, cv::Mat(), cv::Point(-1, -1), iterations, cv::BORDER_REFLECT_101, 1);

	requestFullUpdate();
}

void BScanSegmentation::opencloseBScan()
{
	setActMat(getActBScanNr());
	if(!actMat || actMat->empty())
		return;

	BScanSegAlgorithm::openClose(*actMat);

	requestFullUpdate();
}


void BScanSegmentation::medianBScan()
{
	setActMat(getActBScanNr());
	if(!actMat || actMat->empty())
		return;

	medianBlur(*actMat, *actMat, 3);

	requestFullUpdate();
}

void BScanSegmentation::removeUnconectedAreas()
{
	if(!actMat)
		return;

	if(BScanSegAlgorithm::removeUnconectedAreas(*actMat))
		requestFullUpdate();
}

void BScanSegmentation::extendLeftRightSpace()
{
	if(!actMat)
		return;

	if(BScanSegAlgorithm::extendLeftRightSpace(*actMat))
		requestFullUpdate();
}

void BScanSegmentation::seriesRemoveUnconectedAreas()
{
	const OctData::Series* series = getSeries();
	for(std::size_t i=0; i<series->bscanCount(); ++i)
	{
		if(setActMat(i))
		{
			if(actMat)
				BScanSegAlgorithm::removeUnconectedAreas(*actMat);
		}
	}
	setActMat(getActBScanNr());
	requestFullUpdate();
}

void BScanSegmentation::seriesExtendLeftRightSpace()
{
	const OctData::Series* series = getSeries();
	for(std::size_t i=0; i<series->bscanCount(); ++i)
	{
		if(setActMat(i))
		{
			if(actMat)
				BScanSegAlgorithm::extendLeftRightSpace(*actMat);
		}
	}
	setActMat(getActBScanNr());
	requestFullUpdate();
}




void BScanSegmentation::clearSegments()
{
	for(auto mat : segments)
		delete mat;

	segments.clear();
}

void BScanSegmentation::createSegments()
{
	const OctData::Series* series = getSeries();

	if(!series)
		return;

	clearSegments();
	createSegments(series);
}


void BScanSegmentation::createSegments(const OctData::Series* series)
{
	if(!series)
		return;

	clearSegments();

	for(std::size_t i=0; i<series->bscanCount(); ++i)
	{
		SimpleCvMatCompress* mat;
		const OctData::BScan* bscan = getBScan(i);
		if(bscan)
			mat = new SimpleCvMatCompress(bscan->getHeight(), bscan->getWidth(), BScanSegmentationMarker::markermatInitialValue);
		else
			mat = new SimpleCvMatCompress;
		segments.push_back(mat);
	}
}






void BScanSegmentation::newSeriesLoaded(const OctData::Series* series, boost::property_tree::ptree& markerTree)
{
	if(!series)
		return;
	createSegments(series);
	loadState(markerTree);
}

void BScanSegmentation::saveState(boost::property_tree::ptree& markerTree)
{
	saveActMatState();
	BScanSegmentationPtree::fillPTree(markerTree, this);
	stateChangedSinceLastSave = false;
}

void BScanSegmentation::loadState(boost::property_tree::ptree& markerTree)
{
	BScanSegmentationPtree::parsePTree(markerTree, this);
	setActMat(getActBScanNr(), false);
	stateChangedSinceLastSave = false;
}


void BScanSegmentation::updateCursor()
{
	if(inWidget)
		requestFullUpdate();
}


void BScanSegmentation::initBScanFromThreshold(const BScanSegmentationMarker::ThresholdDirectionData& data)
{
	setActMat(getActBScanNr());
	if(!actMat || actMat->empty())
		return;

	const OctData::Series* series = getSeries();
	if(!series)
		return;

	const OctData::BScan* bscan = series->getBScan(getActBScanNr());
	if(!bscan)
		return;

	const cv::Mat& image = bscan->getImage();
	if(image.empty())
		return;


	BScanSegAlgorithm::initFromThresholdDirection(image, *actMat, data);

	requestFullUpdate();
}


void BScanSegmentation::initSeriesFromThreshold(const BScanSegmentationMarker::ThresholdDirectionData& data)
{
	const OctData::Series* series = getSeries();

	std::size_t bscanCount = 0;
	for(const OctData::BScan* bscan : series->getBScans())
	{
		const cv::Mat& image = bscan->getImage();

		if(setActMat(bscanCount))
		{
			if(actMat && !actMat->empty())
				BScanSegAlgorithm::initFromThresholdDirection(image, *actMat, data);
		}
		++bscanCount;
	}
	setActMat(getActBScanNr());
	requestFullUpdate();
}

void BScanSegmentation::initBScanFromSegline()
{
	setActMat(getActBScanNr());
	if(!actMat || actMat->empty())
		return;

	const OctData::Series* series = getSeries();
	if(!series)
		return;

	const OctData::BScan* bscan = series->getBScan(getActBScanNr());
	if(!bscan)
		return;

	BScanSegAlgorithm::initFromSegline(*bscan, *actMat);

	requestFullUpdate();
}

void BScanSegmentation::initSeriesFromSegline()
{
	const OctData::Series* series = getSeries();


	std::size_t bscanCount = 0;
	for(const OctData::BScan* bscan : series->getBScans())
	{
		if(!bscan)
			continue;

		if(setActMat(bscanCount))
		{
			if(actMat && !actMat->empty())
				BScanSegAlgorithm::initFromSegline(*bscan, *actMat);

		}
		++bscanCount;
	}
	setActMat(getActBScanNr());
	requestFullUpdate();
}



void BScanSegmentation::setLocalMethod(BScanSegmentationMarker::LocalMethod method)
{
	if(localMethod != method)
	{
		localMethod = method;
		switch(localMethod)
		{
			case BScanSegmentationMarker::LocalMethod::None:
				actLocalOperator = nullptr;
				break;
			case BScanSegmentationMarker::LocalMethod::Paint:
				actLocalOperator = localOpPaint;
				break;
			case BScanSegmentationMarker::LocalMethod::Operation:
				actLocalOperator = localOpOperation;
				break;
			case BScanSegmentationMarker::LocalMethod::Threshold:
				actLocalOperator = localOpThreshold;
				break;
			case BScanSegmentationMarker::LocalMethod::NN:
				actLocalOperator = localOpNN;
				break;
			case BScanSegmentationMarker::LocalMethod::ThresholdDirection:
				actLocalOperator = localOpThresholdDirection;
				break;
		}
		localOperatorChanged(method);
	}
}

void BScanSegmentation::setSeglinePaintSize(int size)
{
	seglinePaintSize = size;
	requestFullUpdate();
}


bool BScanSegmentation::setActMat(std::size_t nr, bool saveOldState)
{
	if(nr == actMatNr && saveOldState)
		return true;

	if(actMat)
	{
		if(saveOldState)
			saveActMatState(); // save state from old bscan

		if(segments.size() > nr)
		{
			segments[nr]->writeToMat(*actMat); // load state from new bscan
			actMatNr = nr;

			if(actMat->empty())
			{
				const OctData::BScan* bscan = getBScan(nr);
				if(bscan)
				{
					*actMat = cv::Mat(bscan->getHeight(), bscan->getWidth(), cv::DataType<uint8_t>::type, cvScalar(BScanSegmentationMarker::markermatInitialValue));
				}
			}

			return true;
	}
	}
	return false;
}

void BScanSegmentation::rejectMatChanges()
{
	setActMat(actMatNr, false);
}

void BScanSegmentation::saveActMatState()
{
	if(actMat && segments.size() > actMatNr)
	{
		if((*segments[actMatNr]) != *actMat)
		{
			segments[actMatNr]->readFromMat(*actMat);
			stateChangedSinceLastSave = true;
		}
	}
}

bool BScanSegmentation::hasActMatChanged() const
{
	if(actMat && segments.size() > actMatNr)
		return (*segments[actMatNr]) != *actMat;
	return false;
}


void BScanSegmentation::importSegmentationFromOct(const std::string& filename)
{
	if(ImportSegmentation::importOct(this, filename))
		requestFullUpdate();
}


