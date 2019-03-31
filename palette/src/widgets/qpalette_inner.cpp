#include <widgets/qpalette_inner.h>
#include <widgets/qpalettecontainer.h>

QPaletteInner::QPaletteInner(QWidget* parent, const QString& name, const QVector<Action>& items)
    : QFrame(parent),
    css_observer_(new CSSObserver(this, "theme/window.css")),
    styles_observer_(new StylesObserver(this)),
    layout_(new QVBoxLayout(this)),
    entries_(new QItems(this, items)),
    name_(name),
    searchbox_(new QSearch(this, entries_)) {

    // Add widgets
    layout_->addWidget(searchbox_);
    layout_->addWidget(entries_);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(0);

    setLayout(layout_);

    connect(searchbox_, &QSearch::returnPressed, this, &QPaletteInner::onEnterPressed);
    connect(entries_, &QListView::clicked, this, &QPaletteInner::onItemClicked);

    searchbox_->installEventFilter(this);
    entries_->installEventFilter(this);
}

void QPaletteInner::processEnterResult(EnterResult res) {
    if (res.hide()) {
        closeWindow();
    }
    else {
        // hide=true if nextPalette != NULL
        Q_ASSERT(res.nextPalette().empty());
    }
}

void QPaletteInner::closeWindow() {
    container()->close();
}

QPaletteContainer* QPaletteInner::container() {
    return dynamic_cast<QPaletteContainer*>(window());
}

void QPaletteInner::onItemClicked(const QModelIndex &index) {
    auto action = index.data().value<Action>();

    processEnterResult(enter_callback(action));
}

void QPaletteInner::keyPressEvent(QKeyEvent *e) {
    if (e->key() != Qt::Key_Escape)
        QFrame::keyPressEvent(e);
    else {
        closeWindow();
    }
}

bool QPaletteInner::eventFilter(QObject *obj, QEvent *event) {
    switch (event->type()) {
        case QEvent::KeyPress: {
            auto* ke = dynamic_cast<QKeyEvent*>(event);
            switch (ke->key()) {
                case Qt::Key_Down:
                case Qt::Key_Up:
                    return onArrowPressed(ke->key());
                case Qt::Key_Enter:
                case Qt::Key_Return:
                    return onEnterPressed();
                case Qt::Key_Escape:
                    closeWindow();
                    return true;
                case Qt::Key_PageDown:
                case Qt::Key_PageUp:
                    event->ignore();
                    entries_->keyPressEvent(ke);
                    return true;
                default:
                    return obj->eventFilter(obj, event);
            }
        }
        case QEvent::ShortcutOverride: {
            event->accept();
            return true;
        }
        default:
            return QFrame::eventFilter(obj, event);
    }
}

bool QPaletteInner::onArrowPressed(int key) {
    int delta = key == Qt::Key_Down ? 1: -1;
    auto new_row = entries_->currentIndex().row() + delta;

    if (new_row == -1)
        new_row = 0;
    else if (new_row == entries_->model()->rowCount())
        new_row--;

    entries_->setCurrentIndex(entries_->model()->index(new_row, 0));
    return true;
}
