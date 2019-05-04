#include <widgets/qitem.h>
#include <widgets/palette_filter.h>

QHash<QString, QRegularExpression> capturing_regexp_cache;

QRegularExpression capturingRegexp(const QString& keyword) {
    QStringList regexp_before_join;

    if (capturing_regexp_cache.contains(keyword)) {
        return capturing_regexp_cache[keyword];
    }

    regexp_before_join << ("^");

    for (auto& x : keyword)
        if (!x.isSpace())
            regexp_before_join << (QString("(.*?)(") + x + ")");

    regexp_before_join.push_back("(.*)$");

    QRegularExpression result(regexp_before_join.join(""), QRegularExpression::CaseInsensitiveOption);

    capturing_regexp_cache[keyword] = result;
    return result;
}

const QString highlight(const QString & needle, const QString & haystack) {
    static QString em_("<em>"), emEnd_("</em>");
    QStringList highlights;

    if (needle.size()) {
        auto regexp = capturingRegexp(needle);
        auto match = regexp.match(haystack).capturedTexts();

        int i = -1;
        for (auto&& word : match) {
            if (++i == 0) continue;
            if (i % 2 == 0) highlights << em_ << word << emEnd_;
            else highlights << word;
        }
    }
    else {
        highlights << haystack;
    }

    return QString(highlights.join(""));
}

void QItem::paint(QPainter * painter,
    const QStyleOptionViewItem & option, const QModelIndex & index) const {

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    Action action = index.data().value<Action>();
    QString keyword = index.data(Qt::UserRole).value<QString>();

    auto document = const_cast<QItem *>(this)->renderAction(false, keyword, action);

    painter->save();

    const QWidget* widget = option.widget;
    QStyle* style = widget ? widget->style() : QApplication::style();

    opt.text = "";
    opt.state &= ~QStyle::State_HasFocus;
    opt.state |= QStyle::State_Active;
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

    painter->restore();

    painter->save();

    auto textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &option, widget);
    painter->translate(textRect.left(), textRect.top());
    
    textRect = textRect.intersected(widget->contentsRect());

    document->drawContents(painter, QRectF(0, 0, textRect.width(), textRect.height()));
    painter->restore();
}

QSize QItem::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const {
    QStyleOptionViewItemV4 options = option;
    initStyleOption(&options, index);

    Action action = index.data().value<Action>();

    auto document = const_cast<QItem *>(this)->renderAction(true, QString(), action);
    document->setTextWidth(option.rect.width());

    return QSize(option.rect.width(), (int)document->size().height());
}

void QItem::updateCSS(const QString& style_sheet) {
    document_->setDefaultStyleSheet(style_sheet);
    document_->setDocumentMargin(0);
}

QTextDocument* QItem::renderAction(bool size_hint, const QString& keyword, Action& action) {
    QString html = "<table width=100% cellpadding=0 cellspacing=0><tr><td class=\"name\">" + (!size_hint ? highlight(keyword, action.name) : "keyword") + "</td>";

    if (action.shortcut.size())
        html += "<td width=50px class=\"shortcut\" align=\"right\">" + action.shortcut + "</td>";

    html += "</tr>";

    if (action.description.size())
        html += "<tr><td class=\"description\" colspan=2>" + action.description + "</td></tr>";

    html += "</table>";
    document_->setHtml(html);
    return document_;
}

