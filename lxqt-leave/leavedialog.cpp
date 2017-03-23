/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * http://razor-qt.org, http://lxde.org/
 *
 * Copyright: 2010-2015 LXQt team
 * Authors:
 *   Paulo Lieuthier <paulolieuthier@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "leavedialog.h"
#include <QDebug>
#include <QItemDelegate>


class ItemDelegate : public QItemDelegate
{
public:
    static constexpr QMargins MARGINS{10, 10, 10, 10};
public:
    using QItemDelegate::QItemDelegate;
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt = option;
        opt.decorationPosition = QStyleOptionViewItem::Top;
        QSize size = QItemDelegate::sizeHint(opt, index);
        size += {MARGINS.left() + MARGINS.right(), MARGINS.top() + MARGINS.bottom()}; // add some margins
        return size;
    }

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt = option;
        opt.decorationPosition = QStyleOptionViewItem::Top;
        opt.displayAlignment = Qt::AlignHCenter | Qt::AlignTop;
        return QItemDelegate::paint(painter, opt, index);
    }

protected:
    // Note: We want to paint the focus rectangle around the whole (text+icon)
    //  (default in QItemDelegate is to draw the focus only in text rectangle)
    virtual void drawFocus(QPainter *painter
            , const QStyleOptionViewItem &option
            , const QRect &/*rect*/) const override
    {
        // don't override the rectangle to the text-only
        return QItemDelegate::drawFocus(painter, option, option.rect);
    }

    virtual void drawDisplay(QPainter *painter
            , const QStyleOptionViewItem &option
            , const QRect &rect
            , const QString &text) const override
    {
        // shrink (and move to bottom) the text rectangle
        QRect r = rect.adjusted(0, MARGINS.top(), 0, 0);
        return QItemDelegate::drawDisplay(painter, option, r, text);
    }

    virtual void drawDecoration(QPainter *painter
            , const QStyleOptionViewItem &option
            , const QRect &rect
            , const QPixmap &pixmap) const override
    {
        // move to bottom the pixmap rectangle
        QRect r = rect.translated(0, MARGINS.top());
        return QItemDelegate::drawDecoration(painter, option, r, pixmap);
    }
};
constexpr QMargins ItemDelegate::MARGINS;

LeaveDialog::LeaveDialog(QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint),
    ui(new Ui::LeaveDialog),
    mPower(new LXQt::Power(this)),
    mPowerManager(new LXQt::PowerManager(this)),
    mScreensaver(new LXQt::ScreenSaver(this))
{
    ui->setupUi(this);

    /* This is a special dialog. We want to make it hard to ignore.
       We make it:
           * Undraggable
           * Frameless
           * Stays on top of all other windows
           * Present in all desktops
    */
    setWindowFlags((Qt::CustomizeWindowHint | Qt::FramelessWindowHint |
                    Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint));

    ItemDelegate * delegate = new ItemDelegate(ui->tableWidget);
    {
        QScopedPointer<QAbstractItemDelegate> old_del{ui->tableWidget->itemDelegate()};
        ui->tableWidget->setItemDelegate(delegate);
    }
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    // populate the items
    ui->tableWidget->setColumnCount(3);
    ui->tableWidget->setRowCount(2);
    QTableWidgetItem * item = new QTableWidgetItem{QIcon::fromTheme(QStringLiteral("system-log-out")), tr("Logout")};
    item->setData(Qt::UserRole, LXQt::Power::PowerLogout);
    if (!mPower->canAction(LXQt::Power::PowerLogout))
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    ui->tableWidget->setItem(0, 0, item);
    item = new QTableWidgetItem{QIcon::fromTheme(QStringLiteral("system-shutdown")), tr("Shutdown")};
    item->setData(Qt::UserRole, LXQt::Power::PowerShutdown);
    if (!mPower->canAction(LXQt::Power::PowerShutdown))
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    ui->tableWidget->setItem(0, 1, item);
    item = new QTableWidgetItem{QIcon::fromTheme(QStringLiteral("system-suspend")), tr("Suspend")};
    item->setData(Qt::UserRole, LXQt::Power::PowerSuspend);
    if (!mPower->canAction(LXQt::Power::PowerSuspend))
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    ui->tableWidget->setItem(0, 2, item);
    item = new QTableWidgetItem{QIcon::fromTheme(QStringLiteral("system-lock-screen")), tr("Lock screen")};
    item->setData(Qt::UserRole, -1);
    ui->tableWidget->setItem(1, 0, item);
    item = new QTableWidgetItem{QIcon::fromTheme(QStringLiteral("system-reboot")), tr("Reboot")};
    item->setData(Qt::UserRole, LXQt::Power::PowerReboot);
    if (!mPower->canAction(LXQt::Power::PowerReboot))
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    ui->tableWidget->setItem(1, 1, item);
    item = new QTableWidgetItem{QIcon::fromTheme(QStringLiteral("system-suspend-hibernate")), tr("Hibernate")};
    item->setData(Qt::UserRole, LXQt::Power::PowerHibernate);
    if (!mPower->canAction(LXQt::Power::PowerHibernate))
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    ui->tableWidget->setItem(1, 2, item);

    // set unified size for all cells
    int height = 0;
    for (int row = 0; row < ui->tableWidget->rowCount(); ++row)
        height = qMax(height, ui->tableWidget->rowHeight(row));
    int width = 0;
    for (int col = 0; col < ui->tableWidget->columnCount(); ++col)
        width = qMax(width, ui->tableWidget->columnWidth(col));
    for (int row = 0; row < ui->tableWidget->rowCount(); ++row)
    {
        ui->tableWidget->verticalHeader()->setSectionResizeMode(row, QHeaderView::Fixed);
        ui->tableWidget->setRowHeight(row, height);
    }
    for (int col = 0; col < ui->tableWidget->columnCount(); ++col)
    {
        ui->tableWidget->horizontalHeader()->setSectionResizeMode(col, QHeaderView::Fixed);
        ui->tableWidget->setColumnWidth(col, width);
    }

    connect(ui->tableWidget, &QAbstractItemView::activated, this, [this] (const QModelIndex & index) {
        bool ok = false;
        const int action = index.data(Qt::UserRole).toInt(&ok);
        if (!ok)
        {
            qWarning("Invalid internal logic, no UserRole set!?");
            return;
        }
        close();
        switch (action)
        {
        case LXQt::Power::PowerLogout:
                mPowerManager->logout();
                break;
            case LXQt::Power::PowerShutdown:
                mPowerManager->shutdown();
                break;
            case LXQt::Power::PowerSuspend:
                mPowerManager->suspend();
                break;
            case -1:
                {
                    QEventLoop loop;
                    connect(mScreensaver, &LXQt::ScreenSaver::done, &loop, &QEventLoop::quit);
                    mScreensaver->lockScreen();
                    loop.exec();
                }
                break;
            case LXQt::Power::PowerReboot:
                mPowerManager->reboot();
                break;
            case LXQt::Power::PowerHibernate:
                mPowerManager->hibernate();
                break;
        }
    });
    connect(ui->cancelButton, &QAbstractButton::clicked, this, [this] { close(); });
}

LeaveDialog::~LeaveDialog()
{
    delete ui;
}

void LeaveDialog::resizeEvent(QResizeEvent* event)
{
    QRect screen = QApplication::desktop()->screenGeometry();
    move((screen.width()  - this->width()) / 2,
         (screen.height() - this->height()) / 2);

}
