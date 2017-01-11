#include "dwoctinformations.h"

#include <cmath>

#include <octdata/datastruct/series.h>
#include <octdata/datastruct/bscan.h>
#include <octdata/datastruct/oct.h>

#include <manager/octdatamanager.h>

#include <manager/octmarkermanager.h>

#include <QFormLayout>
#include <QLabel>



namespace
{
	void clearQLayout(QLayout* layout)
	{
		QLayoutItem* item;
		QLayout* sublayout;
		QWidget* widget;
		if(layout)
		{
			while(layout->count() > 0)
			{
				item = layout->takeAt(0);
				if((sublayout = item->layout()) != 0)
					clearQLayout(sublayout);
				else if((widget = item->widget()) != 0)
					{widget->hide(); delete widget;}
				else
					delete item;
			}
		}
	}
	
	void addInformation(QFormLayout* formlayout, const QString& labelText, const std::string& information, QWidget* parent)
	{
		if(!information.empty())
		{
			QLabel* label = new QLabel(QString::fromStdString(information), parent);
			label->setTextInteractionFlags(Qt::TextSelectableByMouse);
			formlayout->addRow(labelText, label);
		}
	}
	void addInformation(QFormLayout* formlayout, const QString& labelText, const QString& information, QWidget* parent)
	{
		if(!information.isEmpty())
		{
			QLabel* label = new QLabel(information, parent);
			label->setTextInteractionFlags(Qt::TextSelectableByMouse);
			formlayout->addRow(labelText, label);
		}
	}

	class LayoutFiller
	{
		int counter = 0;
		QWidget* parent;
	public:
		LayoutFiller(QWidget* parent) : parent(parent) {}

		void setInformation(QFormLayout* formlayout, const QString& labelText, const QString& information, DwOctInformations::OctInfoField& octInfo)
		{
			if(octInfo.labelDesc == nullptr)
				octInfo.labelDesc = new QLabel(labelText, parent);

			if(octInfo.labelInfo == nullptr)
			{
				octInfo.labelInfo = new QLabel(parent);
				octInfo.labelInfo->setTextInteractionFlags(Qt::TextSelectableByMouse);
			}

			if(!information.isEmpty())
			{
				octInfo.labelInfo->setText(information);
				if(!octInfo.showed)
				{
					formlayout->insertRow(counter, octInfo.labelDesc, octInfo.labelInfo);
					octInfo.labelDesc->show();
					octInfo.labelInfo->show();
					octInfo.showed = true;
				}
				++counter;
			}
			else
			{
				if(octInfo.showed)
				{
					octInfo.labelDesc->hide();
					octInfo.labelInfo->hide();
					formlayout->removeWidget(octInfo.labelDesc);
					formlayout->removeWidget(octInfo.labelInfo);
					octInfo.showed = false;
				}
			}
		}
	};
}


DwOctInformations::DwOctInformations(QWidget* parent)
:QDockWidget(parent)
{
	setupUi(this);
	
	OctDataManager& dataManager = OctDataManager::getInstance();
	
	patientInformations = new QFormLayout;
	studyInformations   = new QFormLayout;
	seriesInformations  = new QFormLayout;
	bscanInformations   = new QFormLayout;
	
	groupBoxPatient->setLayout(patientInformations);
	groupBoxStudy  ->setLayout(studyInformations  );
	groupBoxSeries ->setLayout(seriesInformations );
	groupBoxBScan  ->setLayout(bscanInformations );
		
	connect(&dataManager, &OctDataManager::patientChanged, this, &DwOctInformations::setPatient);
	connect(&dataManager, &OctDataManager::studyChanged  , this, &DwOctInformations::setStudy  );
	connect(&dataManager, &OctDataManager::seriesChanged , this, &DwOctInformations::setSeries );

	groupBoxBScan->setVisible(false);
}



void DwOctInformations::setBScanMarkerManager(OctMarkerManager* const manager)
{
	if(markerManager)
		disconnect(markerManager, &OctMarkerManager::newBScanShowed, this, &DwOctInformations::setBScan);
	if(manager)
		connect(manager, &OctMarkerManager::newBScanShowed, this, &DwOctInformations::setBScan);

	groupBoxBScan->setVisible(manager != nullptr);

	markerManager = manager;
}

void DwOctInformations::setPatient(const OctData::Patient* patient)
{
	clearQLayout(patientInformations);
	
	if(!patient)
		return;
	
	addInformation(patientInformations, tr("internal Id"), QString("%1").arg(patient->getInternalId()), this);
	
	addInformation(patientInformations, tr("Surname"    ), patient->getSurname (), this);
	addInformation(patientInformations, tr("Forename"   ), patient->getForename(), this);
	addInformation(patientInformations, tr("Title"      ), patient->getTitle()   , this);
	addInformation(patientInformations, tr("ID"         ), patient->getId()      , this);
	
	if(!patient->getBirthdate().isEmpty())
		addInformation(patientInformations, tr("Birthdate"), patient->getBirthdate().str(), this);
	
	QString sex;
	switch(patient->getSex())
	{
		case OctData::Patient::Sex::Female:
			sex = tr("Female");
			break;
		case OctData::Patient::Sex::Male:
			sex = tr("Male");
			break;
		case OctData::Patient::Sex::Unknown:
			break;
	}
	addInformation(patientInformations, tr("Sex"      ), sex, this);
	addInformation(patientInformations, tr("UID"      ), patient->getPatientUID(), this);
}


void DwOctInformations::setStudy(const OctData::Study* study)
{
	clearQLayout(studyInformations);
	
	if(!study)
		return;
	
	addInformation(studyInformations, tr("internal Id"), QString("%1").arg(study->getInternalId())   , this);
	addInformation(studyInformations, tr("Operator"   ), study->getStudyOperator(), this);
	addInformation(studyInformations, tr("UID"        ), study->getStudyUID()     , this);
	if(!study->getStudyDate().isEmpty())
		addInformation(studyInformations, tr("Study date"), study->getStudyDate().str(), this);
	
}


void DwOctInformations::setSeries(const OctData::Series* series)
{
	clearQLayout(seriesInformations);
	
	if(!series)
		return;
	
	addInformation(seriesInformations, tr("internal Id"), QString("%1").arg(series->getInternalId()), this);

	if(!series->getScanDate().isEmpty())
		addInformation(seriesInformations, tr("Scan date"), series->getScanDate().timeDateStr(), this);
	
	const std::string&    seriesUID = series->getSeriesUID()   ;
	const std::string& refSeriesUID = series->getRefSeriesUID();
	
	addInformation(seriesInformations, tr("UID"     ),    seriesUID, this);
	addInformation(seriesInformations, tr("Ref. UID"), refSeriesUID, this);
	
	if(!seriesUID.empty() && !refSeriesUID.empty())
	{
		addInformation(seriesInformations, tr("Baseline scan"), (seriesUID == refSeriesUID)?tr("true"):tr("false"), this);
	}
	
	
	QString scanPos;
	switch(series->getLaterality())
	{
		case OctData::Series::Laterality::OS:
			scanPos = tr("OS");
			break;
		case OctData::Series::Laterality::OD:
			scanPos = tr("OD");
			break;
		case OctData::Series::Laterality::undef:
			break;
	}
	addInformation(seriesInformations, tr("Laterality"     ), scanPos     , this);
	
	QString scanPattern;
	switch(series->getScanPattern())
	{
		case OctData::Series::ScanPattern::SingleLine:
			scanPattern = tr("Single line scan");
			break;
		case OctData::Series::ScanPattern::Circular:
			scanPattern = tr("Circular scan");
			break;
		case OctData::Series::ScanPattern::Volume:
			scanPattern = tr("Volume scan");
			break;
		case OctData::Series::ScanPattern::FastVolume:
			scanPattern = tr("Fast volume scan");
			break;
		case OctData::Series::ScanPattern::Radial:
			scanPattern = tr("Radial scan");
			break;
		case OctData::Series::ScanPattern::Unknown:
			break;
	}
	addInformation(seriesInformations, tr("Scan pattern"   ), scanPattern  , this);

	double scanFocus = series->getScanFocus();
	if(!std::isnan(scanFocus))
		addInformation(seriesInformations, tr("Scan focus"), QString("%1").arg(scanFocus), this);

}

void DwOctInformations::setBScan(const OctData::BScan* bscan)
{
	LayoutFiller filler(this);

	// clearQLayout(bscanInformations);

	if(!bscan)
		return;

	QString imageQuality;
	if(bscan->getImageQuality() > 0 && bscan->getImageQuality() < 1e10)
		imageQuality = QString("%1").arg(bscan->getImageQuality());

	QString imageAverage;
	if(bscan->getNumAverage() > 0)
		imageAverage = QString("%1").arg(bscan->getNumAverage());

	QString imageAcquisitionTime;
	if(!bscan->getAcquisitionTime().isEmpty())
		imageAcquisitionTime = QString::fromStdString(bscan->getAcquisitionTime().timeDateStr('.', ':', true));


	filler.setInformation(bscanInformations, tr("Acquisition time"), imageAcquisitionTime, bscanAcquisitionTime  );
	filler.setInformation(bscanInformations, tr("Image quality"   ), imageQuality        , bscanImageQuality     );
	filler.setInformation(bscanInformations, tr("Images average"  ), imageAverage        , bscanNumAverage       );

}

