/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * RomDataView.hpp: RomData viewer.                                        *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "RomDataView.hpp"
#include "RpQt.hpp"
#include "RpQImageBackend.hpp"

#include "libromdata/RomData.hpp"
#include "libromdata/RomFields.hpp"
#include "libromdata/img/rp_image.hpp"
using LibRomData::RomData;
using LibRomData::RomFields;
using LibRomData::rp_image;
using LibRomData::rp_string;

// C includes. (C++ namespace)
#include <cassert>
#include <cstdio>

// C++ includes.
#include <vector>
using std::vector;

#include <QtCore/QDateTime>
#include <QtCore/QTimer>

#include <QLabel>
#include <QCheckBox>

#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QTreeWidget>

class RomDataViewPrivate
{
	public:
		RomDataViewPrivate(RomDataView *q, RomData *romData);
		~RomDataViewPrivate();

	private:
		RomDataView *const q_ptr;
		Q_DECLARE_PUBLIC(RomDataView)
	private:
		Q_DISABLE_COPY(RomDataViewPrivate)

	public:
		struct Ui {
			void setupUi(QWidget *RomDataView);

			QFormLayout *formLayout;
			// TODO: Store the field widgets?

			// Header row.
			QHBoxLayout *hboxHeaderRow;
			QLabel *lblSysInfo;
			QLabel *lblBanner;
			QLabel *lblIcon;

			QTimer *tmrIconAnim;

			Ui()	: formLayout(nullptr)
				, hboxHeaderRow(nullptr)
				, lblSysInfo(nullptr)
				, lblBanner(nullptr)
				, lblIcon(nullptr)
				, tmrIconAnim(nullptr)
				{ }
		};
		Ui ui;
		RomData *romData;

		// Animated icon data.
		// TODO: Animated icon helper class like
		// in GCN MemCard Recover? (and maybe share
		// it with the Windows version.)
		const RomData::IconAnimData *iconAnimData;
		QPixmap iconFrames[RomData::ICONANIMDATA_MAX_FRAMES];
		int anim_cur_frame;		// Current frame.
		int anim_last_valid_frame;	// Last frame that had a valid image.
		bool anim_running;		// Animation is running.
		// Current animation direction:
		// - false == forwards
		// - true == backwards
		bool anim_cur_direction;

		/**
		 * Create the header row.
		 * @return QLayout containing the header row.
		 */
		QLayout *createHeaderRow(void);

		/**
		 * Update the display widgets.
		 * FIXME: Allow running this multiple times?
		 */
		void updateDisplay(void);

		bool displayInit;

		/**
		 * Start the animation timer.
		 */
		void startAnimTimer(void);

		/**
		 * Stop the animation timer.
		 */
		void stopAnimTimer(void);
};

/** RomDataViewPrivate **/

RomDataViewPrivate::RomDataViewPrivate(RomDataView *q, RomData *romData)
	: q_ptr(q)
	, romData(romData)
	, iconAnimData(nullptr)
	, anim_cur_frame(0)
	, anim_last_valid_frame(0)
	, anim_running(false)
	, anim_cur_direction(false)
	, displayInit(false)
{
	// Register RpQImageBackend.
	// TODO: Static initializer somewhere?
	rp_image::setBackendCreatorFn(RpQImageBackend::creator_fn);
}

RomDataViewPrivate::~RomDataViewPrivate()
{
	stopAnimTimer();
	delete romData;
}

void RomDataViewPrivate::Ui::setupUi(QWidget *RomDataView)
{
	// Only the formLayout is initialized here.
	// Everything else is initialized in updateDisplay.
	formLayout = new QFormLayout(RomDataView);
}

QLayout *RomDataViewPrivate::createHeaderRow(void)
{
	Q_Q(RomDataView);
	assert(romData != nullptr);
	if (!romData) {
		// No ROM data.
		return nullptr;
	}

	// TODO: Delete the old widgets if they're already present.
	ui.hboxHeaderRow = new QHBoxLayout(q);

	// System name.
	// TODO: System logo and/or game title?
	const rp_char *systemName = romData->systemName(
		RomData::SYSNAME_TYPE_LONG | RomData::SYSNAME_REGION_ROM_LOCAL);

	// File type.
	const rp_char *fileType = nullptr;
	switch (romData->fileType()) {
		case RomData::FTYPE_ROM_IMAGE:
			fileType = _RP("ROM Image");
			break;
		case RomData::FTYPE_DISC_IMAGE:
			fileType = _RP("Disc Image");
			break;
		case RomData::FTYPE_SAVE_FILE:
			fileType = _RP("Save File");
			break;
		case RomData::FTYPE_UNKNOWN:
		default:
			fileType = nullptr;
			break;
	}

	QString sysInfo;
	if (systemName) {
		sysInfo = RP2Q(systemName);
	}
	if (fileType) {
		if (!sysInfo.isEmpty()) {
			sysInfo += QChar(L'\n');
		}
		sysInfo += RP2Q(fileType);
	}

	if (!sysInfo.isEmpty()) {
		ui.lblSysInfo = new QLabel(q);
		ui.lblSysInfo->setAlignment(Qt::AlignCenter);
		ui.lblSysInfo->setTextFormat(Qt::PlainText);
		ui.lblSysInfo->setText(sysInfo);

		// Use a bold font.
		QFont font = ui.lblSysInfo->font();
		font.setBold(true);
		ui.lblSysInfo->setFont(font);

		ui.hboxHeaderRow->addWidget(ui.lblSysInfo);
	}

	// Supported image types.
	const uint32_t imgbf = romData->supportedImageTypes();

	// Banner.
	if (imgbf & RomData::IMGBF_INT_BANNER) {
		// Get the banner.
		const rp_image *banner = romData->image(RomData::IMG_INT_BANNER);
		if (banner) {
			QImage img = rpToQImage(banner);
			if (!img.isNull()) {
				ui.lblBanner = new QLabel(q);
				ui.lblBanner->setPixmap(QPixmap::fromImage(img));
				ui.hboxHeaderRow->addWidget(ui.lblBanner);
			}
		}
	}

	// Icon.
	// TODO: Animated icon.
	if (imgbf & RomData::IMGBF_INT_ICON) {
		// Get the banner.
		const rp_image *icon = romData->image(RomData::IMG_INT_ICON);
		if (icon) {
			QImage img = rpToQImage(icon);
			if (!img.isNull()) {
				ui.lblIcon = new QLabel(q);
				ui.lblIcon->setPixmap(QPixmap::fromImage(img));
				ui.hboxHeaderRow->addWidget(ui.lblIcon);
			}

			// Get the animated icon data.
			iconAnimData = romData->iconAnimData();
			if (iconAnimData) {
				// Convert the icons to QPixmaps.
				iconFrames[0] = QPixmap::fromImage(img);
				for (int i = 1; i < iconAnimData->count; i++) {
					if (iconAnimData->frames[i]) {
						iconFrames[i] = QPixmap::fromImage(
							rpToQImage(iconAnimData->frames[i]));
					}
				}

				// Create the animation timer.
				if (!ui.tmrIconAnim) {
					ui.tmrIconAnim = new QTimer(q);
					ui.tmrIconAnim->setSingleShot(true);
					QObject::connect(ui.tmrIconAnim, SIGNAL(timeout()),
							 q, SLOT(tmrIconAnim_timeout()));
				}
			}
		}
	}

	// Add spacers.
	ui.hboxHeaderRow->insertStretch(0, 1);
	ui.hboxHeaderRow->insertStretch(-1, 1);
	return ui.hboxHeaderRow;
}

/**
 * Update the display widgets.
 * FIXME: Allow running this multiple times?
 */
void RomDataViewPrivate::updateDisplay(void)
{
	if (!romData || displayInit)
		return;
	displayInit = true;

	// Get the fields.
	const RomFields *fields = romData->fields();
	if (!fields) {
		// No fields.
		// TODO: Show an error?
		return;
	}
	const int count = fields->count();

	// Header row:
	// - System name and file type.
	//   - TODO: System logo.
	// - Banner (if present)
	// - Icon (if present)
	QLayout *headerRow = createHeaderRow();
	if (headerRow) {
		ui.formLayout->addRow(headerRow);
	}

	// Make sure the underlying file handle is closed,
	// since we don't need it anymore.
	romData->close();

	// Create the data widgets.
	Q_Q(RomDataView);
	for (int i = 0; i < count; i++) {
		const RomFields::Desc *desc = fields->desc(i);
		const RomFields::Data *data = fields->data(i);
		if (!desc || !data)
			continue;
		if (desc->type != data->type)
			continue;
		if (!desc->name || desc->name[0] == '\0')
			continue;

		QLabel *lblDesc = new QLabel(q);
		lblDesc->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		lblDesc->setTextFormat(Qt::PlainText);
		lblDesc->setText(RomDataView::tr("%1:").arg(RP2Q(desc->name)));

		switch (desc->type) {
			case RomFields::RFT_INVALID:
				// No data here.
				delete lblDesc;
				break;

			case RomFields::RFT_STRING: {
				// String type.
				QLabel *lblString = new QLabel(q);
				lblString->setAlignment(Qt::AlignLeft | Qt::AlignTop);
				lblString->setTextFormat(Qt::PlainText);
				lblString->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByMouse);
				if (data->str) {
					lblString->setText(RP2Q(data->str));
				}

				// Check for any formatting options.
				if (desc->str_desc) {
					// Monospace font?
					if (desc->str_desc->formatting & RomFields::StringDesc::STRF_MONOSPACE) {
						QFont font(QLatin1String("Monospace"));
						font.setStyleHint(QFont::TypeWriter);
						lblString->setFont(font);
					}
				}
\
				ui.formLayout->addRow(lblDesc, lblString);
				break;
			}

			case RomFields::RFT_BITFIELD: {
				// Bitfield type. Create a grid of checkboxes.
				const RomFields::BitfieldDesc *bitfieldDesc = desc->bitfield;
				QGridLayout *gridLayout = new QGridLayout();
				int row = 0, col = 0;
				for (int i = 0; i < bitfieldDesc->elements; i++) {
					const rp_char *name = bitfieldDesc->names[i];
					if (!name)
						continue;
					// TODO: Prevent toggling; disable automatic alt key.
					QCheckBox *checkBox = new QCheckBox(q);
					checkBox->setText(RP2Q(name));
					if (data->bitfield & (1 << i)) {
						checkBox->setChecked(true);
					}
					gridLayout->addWidget(checkBox, row, col, 1, 1);
					col++;
					if (col == bitfieldDesc->elemsPerRow) {
						row++;
						col = 0;
					}
				}
				ui.formLayout->addRow(lblDesc, gridLayout);
				break;
			}

			case RomFields::RFT_LISTDATA: {
				// ListData type. Create a QTreeWidget.
				const RomFields::ListDataDesc *listDataDesc = desc->list_data;
				QTreeWidget *treeWidget = new QTreeWidget(q);
				treeWidget->setRootIsDecorated(false);
				treeWidget->setUniformRowHeights(true);

				// Set up the column names.
				const int count = listDataDesc->count;
				treeWidget->setColumnCount(count);
				QStringList columnNames;
				columnNames.reserve(count);
				for (int i = 0; i < count; i++) {
					if (listDataDesc->names[i]) {
						columnNames.append(RP2Q(listDataDesc->names[i]));
					} else {
						// Don't show this column.
						columnNames.append(QString());
						treeWidget->setColumnHidden(i, true);
					}
				}
				treeWidget->setHeaderLabels(columnNames);

				// Add the row data.
				const RomFields::ListData *listData = data->list_data;
				for (int i = 0; i < (int)listData->data.size(); i++) {
					const vector<rp_string> &data_row = listData->data.at(i);
					QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeWidget);
					int field = 0;
					for (vector<rp_string>::const_iterator iter = data_row.begin();
					     iter != data_row.end(); ++iter, ++field)
					{
						treeWidgetItem->setData(field, Qt::DisplayRole, RP2Q(*iter));
					}
				}

				// Resize the columns to fit the contents.
				for (int i = 0; i < count; i++) {
					treeWidget->resizeColumnToContents(i);
				}
				treeWidget->resizeColumnToContents(count);

				ui.formLayout->addRow(lblDesc, treeWidget);
				break;
			}

			case RomFields::RFT_DATETIME: {
				// Date/Time.
				const RomFields::DateTimeDesc *const dateTimeDesc = desc->date_time;

				QLabel *lblDateTime = new QLabel(q);
				lblDateTime->setTextFormat(Qt::PlainText);
				lblDateTime->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByMouse);

				QDateTime dateTime;
				dateTime.setTimeSpec(
					dateTimeDesc->flags & RomFields::RFT_DATETIME_IS_UTC
						? Qt::UTC : Qt::LocalTime);
				dateTime.setMSecsSinceEpoch(data->date_time * 1000);

				QString str;
				switch (dateTimeDesc->flags &
					(RomFields::RFT_DATETIME_HAS_DATE | RomFields::RFT_DATETIME_HAS_TIME))
				{
					case RomFields::RFT_DATETIME_HAS_DATE:
						// Date only.
						str = dateTime.date().toString(Qt::DefaultLocaleShortDate);
						break;

					case RomFields::RFT_DATETIME_HAS_TIME:
						// Time only.
						str = dateTime.time().toString(Qt::DefaultLocaleShortDate);
						break;

					case RomFields::RFT_DATETIME_HAS_DATE |
					     RomFields::RFT_DATETIME_HAS_TIME:
						// Date and time.
						str = dateTime.toString(Qt::DefaultLocaleShortDate);
						break;

					default:
						// Invalid combination.
						assert(!"Invalid Date/Time formatting.");
						delete lblDateTime;
						delete lblDesc;
						break;
				}
				lblDateTime->setText(str);

				ui.formLayout->addRow(lblDesc, lblDateTime);
				break;
			}

			default:
				// Unsupported right now.
				assert(!"Unsupported RomFields::RomFieldsType.");
				delete lblDesc;
				break;
		}
	}
}

/**
 * Start the animation timer.
 */
void RomDataViewPrivate::startAnimTimer(void)
{
	if (!iconAnimData || !ui.tmrIconAnim || !ui.lblIcon) {
		// Not an animated icon.
		return;
	}

	if (anim_cur_frame >= iconAnimData->count) {
		// Out of bounds...
		// Reset the animation.
		anim_cur_frame = 0;
		anim_last_valid_frame = 0;
		anim_cur_direction = false;
		ui.lblIcon->setPixmap(iconFrames[0]);
	}

	// Set a single-shot timer for the current frame.
	anim_running = true;
	ui.tmrIconAnim->start(iconAnimData->delays[anim_cur_frame]);
}

/**
 * Stop the animation timer.
 */
void RomDataViewPrivate::stopAnimTimer(void)
{
	if (ui.tmrIconAnim) {
		anim_running = false;
		ui.tmrIconAnim->stop();
	}
}

/** RomDataView **/

RomDataView::RomDataView(RomData *rom, QWidget *parent)
	: super(parent)
	, d_ptr(new RomDataViewPrivate(this, rom))
{
	Q_D(RomDataView);
	d->ui.setupUi(this);

	// Update the display widgets.
	d->updateDisplay();
}

RomDataView::~RomDataView()
{
	delete d_ptr;
}

/** QWidget overridden functions. **/

/**
 * Window has been hidden.
 * This means that this tab has been selected.
 * @param event QShowEvent.
 */
void RomDataView::showEvent(QShowEvent *event)
{
	// Start the icon animation.
	Q_D(RomDataView);
	d->startAnimTimer();

	// Pass the event to the superclass.
	super::showEvent(event);
}

/**
 * Window has been hidden.
 * This means that a different tab has been selected.
 * @param event QHideEvent.
 */
void RomDataView::hideEvent(QHideEvent *event)
{
	// Stop the icon animation.
	Q_D(RomDataView);
	d->stopAnimTimer();

	// Pass the event to the superclass.
	super::hideEvent(event);
}

/** Widget slots. **/

/**
 * Animated icon timer.
 */
void RomDataView::tmrIconAnim_timeout(void)
{
	Q_D(RomDataView);

	// Go to the next frame.
	if (!d->anim_cur_direction) {
		// Animation is moving forwards.
		// Check if we're at the last frame.
		if (d->anim_cur_frame == (d->iconAnimData->count - 1)) {
			// Last frame.
			if (d->iconAnimData->bounce) {
				// "Bounce" animation. Start playing backwards.
				d->anim_cur_direction = true;
				d->anim_cur_frame--;	// Go to the previous frame.
			} else {
				// "Looping" animation.
				// Reset to frame 0.
				d->anim_cur_frame = 0;
			}
		} else {
			// Not the last frame.
			// Go to the next frame.
			d->anim_cur_frame++;
		}
	} else {
		// Animation is moving backwards. ("Bounce" animation only.)
		// Check if we're at the first frame.
		if (d->anim_cur_frame == 0) {
			// First frame. Start playing forwards.
			d->anim_cur_direction = false;
			d->anim_cur_frame++;	// Go to the next frame.
		} else {
			// Not the first frame.
			// Go to the previous frame.
			d->anim_cur_frame--;
		}
	}

	// Check if this frame has an icon.
	if (!d->iconFrames[d->anim_cur_frame].isNull() &&
	    d->anim_last_valid_frame != d->anim_cur_frame)
	{
		// This frame has an icon.
		d->anim_last_valid_frame = d->anim_cur_frame;

		// Update the icon.
		d->ui.lblIcon->setPixmap(d->iconFrames[d->anim_cur_frame]);
	}

	// Set the single-shot timer.
	if (d->anim_running) {
		d->ui.tmrIconAnim->start(d->iconAnimData->delays[d->anim_cur_frame]);
	}
}
