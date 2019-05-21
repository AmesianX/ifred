#ifndef QITEM_H
#define QITEM_H

#include <QtGui>
#include <QtWidgets>

#include <action.h>
#include <observers.h>

class QItem : public QStyledItemDelegate {
    QSize cached_size_;
    QTextDocument* document_;
    int recents_;

public:
    explicit QItem(QWidget* parent)
        : QStyledItemDelegate(parent), document_(new QTextDocument(this)) {
        updateCSS(loadFile("theme/window.css"));
    }

    void updateCSS(const QString& style_sheet);

    void paint(QPainter* painter,
        const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    QTextDocument* renderAction(bool just_return, const QString &className, const QString& keyword, Action& action);

    void setRecents(int index) { recents_ = index; }
};

#endif // QITEM_H
