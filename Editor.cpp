//
// Created by pikachu on 2021/3/19.
//

#include "Editor.h"
#include <QApplication>
#include <QDate>
#include <QFile>
#include <QDebug>
#include <QTextFrame>
#include <QTextList>
#include <QTextBlock>
#include <QTextTable>
#include "QtMarkdownParser"
#include <QPainter>
#include <QScrollArea>
#include <QMessageBox>
#include <QDesktopServices>
#include <QDebug>
#include <QFontDatabase>
#include <QStyle>
#include <QTimer>
#include <QtConcurrent>

namespace Element {
    struct Link {
        QString text;
        QString url;
        QList<QRect> rects;
    };
}


class EditorWidget: public QWidget {
Q_OBJECT
public:
    explicit EditorWidget(Editor* parent = nullptr);

protected:
    void paintEvent(QPaintEvent *e) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

    void resizeEvent(QResizeEvent *event) override;

public:
    bool eventFilter(QObject *watched, QEvent *event) override;
private:
    void drawInBackground();
    void drawAsync();
private:
    QImage m_buffer;
    bool m_needDraw;
    int m_rightMargin;
    QList<Element::Link*> m_links;
    Editor* m_editor;
    bool m_isDrawing;
    int m_maxWidth;
    QSize m_fixedSize;
};

struct DefaultEditorVisitor: MultipleVisitor<Header,
        Text, ItalicText, BoldText, ItalicBoldText,
        Image, Link, CodeBlock, InlineCode, Paragraph,
        UnorderedList, OrderedList,
        Hr, QuoteBlock, Table> {
    explicit DefaultEditorVisitor(QPainter& painter, int w, int rightMargin):
            m_painter(painter), m_maxWidth(w - rightMargin) {
//        qDebug() << "width: " << m_maxWidth;
        m_curX = 0;
        m_curY = 0;
        m_lastMaxHeight = 0;
        m_lastMaxWidth = w;
        QFont font;
        font.setFamily("Microsoft YaHei UI");
        font.setPixelSize(16);
        m_painter.setFont(font);
    }
    QRect textRect(const QString& text) {
        QFontMetrics metrics = m_painter.fontMetrics();
        QRect textBoundingRect = metrics.boundingRect(QRect(m_curX, m_curY, m_maxWidth, 0), Qt::TextWordWrap, text);
        return textBoundingRect;
    }
    int textWidth(const QString& text) {
        QFontMetrics metrics = m_painter.fontMetrics();
        int w = metrics.horizontalAdvance(text);
        return w;
    }
    int charWidth(const QChar& ch) {
        QFontMetrics metrics = m_painter.fontMetrics();
        int w = metrics.horizontalAdvance(ch);
        return w;
    }
    bool currentLineCanDrawText(const QString& text) {
        auto needWidth = textWidth(text);
        // qDebug() << "need" << needWidth << text;
        if (m_curX + needWidth < m_maxWidth) {
            return true;
        } else {
            return false;
        }
    }
    QList<QRect> drawText(const QString& text) {
        if (text == "\r") return {};
//         qDebug() << "draw" << text;
        if (text.isEmpty()) return {};
        if (currentLineCanDrawText(text)) {
            auto rect = drawTextInCurrentLine(text);
            return {rect};
        } else {
            auto ch_w = charWidth(text.at(0));
            if (m_curX + ch_w <= m_maxWidth) {
                // 计算这一行可以画多少个字符
                int left_w = m_maxWidth - m_curX;
                int may_ch_count = left_w / ch_w - 1;
                if (currentLineCanDrawText(text.left(may_ch_count + 1))) {
                    while (currentLineCanDrawText(text.left(may_ch_count + 1))) {
                        may_ch_count++;
                    }
                } else {
                    while (!currentLineCanDrawText(text.left(may_ch_count))) {
                        may_ch_count--;
                    }
                }
                auto rect1 = drawTextInCurrentLine(text.left(may_ch_count));
                m_curY += m_lastMaxHeight;
                auto rect2 = drawTextInNewLine(text.right(text.size() - may_ch_count));
                return {rect1,rect2};
            } else {
                // 如果一个字符都画不了，直接画矩形
                m_curX = 0;
                m_curY += m_lastMaxHeight;
                auto rect = drawTextInNewLine(text);
                return {rect};
            }
        }
    }
    QRect drawTextInCurrentLine(const QString& text) {
//        qDebug() << "cur";
        auto rect = textRect(text);
//        qDebug() << rect << text;
        m_painter.drawText(rect, text);
        m_curX += rect.width();
        m_lastMaxHeight = qMax(m_lastMaxHeight, rect.height());
        return rect;
    }
    QRect drawTextInNewLine(const QString& text) {
//        qDebug() << "new";
        m_curX = 0;
        auto rect = textRect(text);
//        qDebug() << rect << text;
        m_painter.drawText(rect, text);
        m_curY += rect.height();
        m_lastMaxHeight = 0;
        return rect;
    }
    void moveToNewLine() {
        m_curY += m_lastMaxHeight;
        m_curX = 0;
        m_lastMaxHeight = 0;
    }
    void visit(Header *node) override {
        std::array<int, 6> fontSize = {
            36, 28,24, 20, 16, 14
        };
        m_painter.save();
        auto font = QFont();
        font.setPixelSize(fontSize[node->level() - 1]);
        m_painter.setFont(font);
        moveToNewLine();
//        QString hn = "h" + String::number(node->level());
//        drawText(hn);
        m_curX += 10;
        for(auto it: node->children()) {
            it->accept(this);
        }
        m_painter.restore();
    }
    void visit(Text *node) override {
        drawText(node->str());
    }
    void visit(ItalicText *node) override {
        m_painter.save();
        QFont font = m_painter.font();
        font.setItalic(true);
        m_painter.setFont(font);
        drawText(node->str());
        m_painter.restore();
    }
    void visit(BoldText *node) override {
        m_painter.save();
        QFont font = m_painter.font();
        font.setBold(true);
        m_painter.setFont(font);
        drawText(node->str());
        m_painter.restore();
    }
    void visit(ItalicBoldText *node) override {
        m_painter.save();
        QFont font = m_painter.font();
        font.setItalic(true);
        font.setBold(true);
        m_painter.setFont(font);
        drawText(node->str());
        m_painter.restore();
    }
    void visit(Image *node) override {
        moveToNewLine();
        QString imgPath = node->src()->str();
        QFile file(imgPath);
        if (file.exists()) {
            QImage image(imgPath);
            int imageMaxWidth = qMin(1080, m_maxWidth);
            if (image.width() > imageMaxWidth) {
                image = image.scaledToWidth(imageMaxWidth);
            }
            QRect rect(QPoint(m_curX, m_curY), image.size());
//            qDebug() << "image rect" << rect;
            m_painter.drawImage(rect, image);
            m_lastMaxHeight = rect.height();
        } else {
            qWarning() << "image not exist." << imgPath;
        }
    }
    void visit(Link *node) override {
        m_painter.save();
        m_painter.setPen(Qt::blue);
        auto font = m_painter.font();
        font.setUnderline(true);
        m_painter.setFont(font);
        auto rects = drawText(node->content()->str());
        auto link = new Element::Link();
        link->text = node->content()->str();
        link->url = node->href()->str();
        link->rects = rects;
        m_links.append(link);
        m_painter.restore();
    }
    void visit(CodeBlock *node) override {
        moveToNewLine();
        m_curY += 10;
        m_painter.save();
        // #f9f9f9
//        m_painter.setBackground(QBrush(QColor(249, 249, 249)));
        QFont font;
        font.setPixelSize(16);
        font.setFamily("Cascadia Code");
        m_painter.setFont(font);
        auto rect = textRect(node->code()->str());
        m_painter.fillRect(QRect(m_curX, m_curY, m_maxWidth, rect.height()), QBrush(QColor(249, 249, 249)));
        m_painter.drawText(rect, node->code()->str());
        m_painter.restore();
        m_curY += rect.height();
        m_curY += 10;
    }
    void visit(InlineCode *node) override {
        m_painter.save();
        // #f9f9f9
//        m_painter.setBackground(QBrush(QColor(249, 249, 249)));
        QFont font;
        font.setPixelSize(16);
        font.setFamily("Cascadia Code");
        m_painter.setFont(font);
        auto rect = textRect(node->code()->str());
        m_painter.fillRect(rect, QBrush(QColor(249, 249, 249)));
        m_painter.drawText(rect, node->code()->str());
        m_curX += rect.width();
        m_painter.restore();
    }
    void visit(Paragraph *node) override {
        m_painter.save();
        QFont font;
        font.setPixelSize(14);
        m_painter.setFont(font);
        moveToNewLine();
        for(auto it: node->children()) {
            it->accept(this);
        }
        m_painter.restore();
    }
    void visit(UnorderedList *node) override {
        for (const auto &item : node->children()) {
            moveToNewLine();
            drawText("● ");
            item->accept(this);
        }

    }
    void visit(OrderedList *node) override {
        int i = 0;
        for (const auto &item : node->children()) {
            i++;
            moveToNewLine();
            QString numStr = QString("%1. ").arg(i);
            drawText(numStr);
            item->accept(this);
        }
    }
    void visit(Hr *node) override {
    }
    void visit(QuoteBlock *node) override {
        moveToNewLine();
        int startY = m_curY;
        for(auto child: node->children()) {
            m_curX += 10;
            child->accept(this);
            moveToNewLine();
        }
        int endY = m_curY;
        const QRect rect = QRect(2, startY, 5, endY - startY);
        // qDebug() << rect;
        // #eee
        m_painter.fillRect(rect, QBrush(QColor(238, 238, 238)));
    }
    void visit(Table *node) override {
    }
    int realHeight() const {
        return m_curY + m_lastMaxHeight;
    }
    int realWidth() const {
        return m_lastMaxWidth;
    }
    const QList<Element::Link*>& links() {
        return m_links;
    }
private:
    QPainter& m_painter;
    int m_curX;
    int m_curY;
    int m_lastMaxHeight;
    int m_lastMaxWidth;
    int m_maxWidth;
    QList<Element::Link*> m_links;
};

template<typename T>
void checkFuture(QFuture<T> future, std::function<void(T)> callback) {
    if (!future.isFinished()) {
        QTimer::singleShot(100, [future, callback](){
            checkFuture(future, callback);
        });
    } else {
        callback(future.result());
    }
}
EditorWidget::EditorWidget(Editor *parent)
    : QWidget(parent)
    , m_needDraw(true)
    , m_editor(parent)
    {
    m_rightMargin = 0;
    setMouseTracking(true);
    m_buffer = QImage(this->size(), QImage::Format_RGB32);
}

void EditorWidget::paintEvent(QPaintEvent *e) {
    Q_UNUSED(e);
//    qDebug() << e;
    if (m_needDraw) {
        m_needDraw = false;
        this->drawInBackground();
    }
    QPainter painter(this);
    painter.drawImage(0, 0, m_buffer);
}

void EditorWidget::mouseMoveEvent(QMouseEvent *event) {
    auto pos = event->pos();
    for(auto link: m_links) {
        for(auto rect: link->rects) {
            if (rect.contains(pos)) {
                setCursor(QCursor(Qt::PointingHandCursor));
                return;
            }
        }
    }

    setCursor(QCursor(Qt::ArrowCursor));
}

void EditorWidget::mousePressEvent(QMouseEvent *event) {
    auto pos = event->pos();
    for(auto link: m_links) {
        for(auto rect: link->rects) {
            if (rect.contains(pos)) {
                auto ret = QMessageBox::question(this, tr("Open URL"),
                                         QString("%1").arg(link->url)
                                         );
                if (ret == QMessageBox::Yes) {
                    QDesktopServices::openUrl(QUrl(link->url));
                }
            }
        }
    }
}

void EditorWidget::resizeEvent(QResizeEvent *event) {
    qDebug() << __FUNCTION__ ;
//    m_needDraw = true;
    this->update();
}

bool EditorWidget::eventFilter(QObject *watched, QEvent *event) {
//    qDebug() << event->type() << watched << m_editor;
    if (watched == m_editor) {
        if (event->type() == QEvent::Resize) {
            QResizeEvent* e = dynamic_cast<QResizeEvent *>(event);
            qDebug() << e->size();
            auto scrollBarWidth = this->style()->pixelMetric(QStyle::PM_ScrollBarSliderMin);
            this->m_maxWidth = qMax(600, e->size().width()) - scrollBarWidth - 1;
            if (!m_isDrawing) {
                m_needDraw = true;
                qDebug() << __FUNCTION__ ;
                this->update();
            }
        }
    }
    return QObject::eventFilter(watched, event);
}

void EditorWidget::drawInBackground() {
//    m_maxWidth = qMax(600, parentWidget()->width());
    auto ret = QtConcurrent::run([this](int){
        this->m_isDrawing = true;
        this->drawAsync();
        return 0;
    }, 0);
    checkFuture<int>(ret, [this](int) {
        setFixedSize(m_fixedSize);
        this->m_isDrawing = false;
        this->update();
    });
}

void EditorWidget::drawAsync() {
    QImage tmp(this->size(), QImage::Format_RGB32);
    QPainter painter(&tmp);
    painter.setRenderHint(QPainter::Antialiasing);
    QFile mdFile("../test.md");
    if (!mdFile.exists()) {
        qDebug() << "file not exist:" << mdFile.fileName();
        return;
    }
    mdFile.open(QIODevice::ReadOnly);
    auto mdText = mdFile.readAll();
    mdFile.close();
//    qDebug().noquote().nospace() << mdText;
    Document doc(mdText);
    DefaultEditorVisitor visitor(painter, m_maxWidth, m_rightMargin);
    doc.accept(&visitor);
    m_links = visitor.links();
    int h = visitor.realHeight();
    if (h < 0) {
        h = 600;
    }
    auto w = qMax(m_maxWidth, visitor.realWidth());
    // qDebug() << "set size:" << w << h;
    m_fixedSize = QSize(w, h);
    auto buffer = QImage(w, h, QImage::Format_RGB32);
    buffer.fill(Qt::white);
    QPainter p(&buffer);
    p.setRenderHint(QPainter::Antialiasing);
    DefaultEditorVisitor _visitor(p, w, m_rightMargin);
    doc.accept(&_visitor);
    m_buffer = buffer;
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    Editor w;
    w.resize(800, 600);
    w.show();
    return QApplication::exec();
}

Editor::Editor(QWidget *parent) : QScrollArea(parent) {
    setWidgetResizable(true);
    auto w = new EditorWidget(this);
    setWidget(w);
    installEventFilter(w);
}
#include "Editor.moc"