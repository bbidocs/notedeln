// SheetScene.cpp

#include "SheetScene.H"
#include "BaseScene.H"
#include "Style.H"
#include "TextData.H"
#include "TitleItem.H"

#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QDebug>
#include <QPainter>
#include <QTextDocument>
#include <QGraphicsView>

SheetScene::SheetScene(class Style const &s, BaseScene *parent):
  QGraphicsScene(parent), base(parent), style_(s) {
  pgnoItem = 0;
  nOfNItem = 0;
  titleItem = 0;
  plainTitleItem = 0;
  fancyTitleItem_ = 0;
  contItem = 0;
  contdItem = 0;
  dateItem = 0;
  leftMarginItem = 0;
  topMarginItem = 0;
  //  bgItem = 0;
  contInMargin = false;
  
  setItemIndexMethod(NoIndex);

  makeBackground();
}

SheetScene::~SheetScene() {
}

void SheetScene::makeBackground() {
  setSceneRect(0, 0,
	       style_.real("page-width"), style_.real("page-height"));
  //  setBackgroundBrush(QBrush(style_.color("background-color")));

  //bgItem = addRect(0, 0,
  //		   style_.real("page-width"), style_.real("page-height"),
  //		   QPen(Qt::NoPen),
  //		   QBrush(style_.color("background-color"))); 
  //bgItem->setZValue(-100);
   
  leftMarginItem
    = addLine(style_.real("margin-left")-1, 0,
	      style_.real("margin-left")-1, style_.real("page-height"),
	      QPen(QBrush(style_.color("margin-left-line-color")),
		   style_.real("margin-left-line-width")));

    topMarginItem
      = addLine(0, style_.real("margin-top"),
		width(), style_.real("margin-top"),
		QPen(QBrush(style_.color("margin-top-line-color")),
		     style_.real("margin-top-line-width")));
}
  
void SheetScene::setDate(QDate const &date) {
  if (!dateItem) {
    dateItem = addText("-", style_.font("date-font"));
    dateItem->setDefaultTextColor(style_.color("date-color"));
  }
  if (date.isNull()) {
    dateItem->hide();
  } else {
    dateItem->setPlainText(date.toString(style_.string("date-format")));
    dateItem->show();
    repositionDate();
  }
}

void SheetScene::repositionDate() {
  QPointF br = dateItem->boundingRect().bottomRight();
  if (nOfNItem && nOfNItem->isVisible())
    dateItem->setPos(nOfNItem->sceneBoundingRect().topRight()
		     - br + QPointF(0, 8));
  else 
    dateItem->setPos(style().real("page-width") 
                     - style().real("margin-right-over") 
                     - br.x(),
                     style().real("margin-top") 
                     - style().real("title-sep") 
                     - br.y());
  repositionTitle();
}

void SheetScene::setNOfN(int n, int N, bool always) {
  if (!nOfNItem) {
     nOfNItem = addText("n/N", style_.font("pgno-font"));
     nOfNItem->setDefaultTextColor(style_.color("pgno-color"));
  }
  if (N>1 || always) {
    nOfNItem->setPlainText(QString("(%1/%2)").arg(n+1).arg(N));
    QPointF br = nOfNItem->boundingRect().bottomRight();
    nOfNItem->setPos(style_.real("page-width") 
		     - style_.real("margin-right-over") 
		     - br.x(),
		     style_.real("margin-top") 
		     - style_.real("title-sep") 
		     - br.y() + 8);
    nOfNItem->show();
  } else {
    nOfNItem->hide();
  }

  repositionDate();

  if (n>=1) {
    // this is a continuation page
    if (!contdItem) {
      contdItem = addText(">", style_.font("contd-font"));
      contdItem->setDefaultTextColor(style_.color("contd-color"));
      QPointF tr = contdItem->boundingRect().topRight();
      double x = contInMargin ? 4
	: style_.real("margin-left") - tr.x();
      contdItem->setPos(x, style_.real("margin-top") - tr.y());
    }
    contdItem->show();
  } else {
    if (contdItem) 
      contdItem->hide();
  }
  if (n<N-1) {
    // this is not the last page
    if (!contItem) {
      contItem = addText(">",style_.font("contd-font"));
      contItem->setDefaultTextColor(style_.color("contd-color"));
      contItem->setPos(style_.real("page-width")
		       - style_.real("margin-right-over"),
		       style_.real("page-height")
		       - style_.real("margin-bottom")
		       + style_.real("pgno-sep"));
    }
    contItem->show();
  } else {
    if (contItem)
      contItem->hide();
  }
}

void SheetScene::setTitle(QString const &title) {
  if (fancyTitleItem_)
    fancyTitleItem_->deleteLater();
  fancyTitleItem_ = 0;
  if (!plainTitleItem) {
    plainTitleItem = addText("", style_.font("title-font"));
    plainTitleItem->setDefaultTextColor(style_.color("title-color"));
  }
  plainTitleItem->setPlainText(title);
  titleItem = plainTitleItem;
  repositionTitle();
}

void SheetScene::repositionTitle() {
  double l = style_.real("margin-left") + style_.real("title-indent");
  double r = style_.real("page-width") - style_.real("margin-right");
  if (dateItem) {
    double dateX = dateItem->mapToScene(dateItem->boundingRect().topLeft()).x();
    dateX -= 5; // should be in style, I guess
    if (r>dateX)
      r = dateX;
  }
  double w = r - l;
  if (fancyTitleItem_)
    fancyTitleItem_->setTextWidth(w);
  if (plainTitleItem)
    plainTitleItem->setTextWidth(w);    
  QPointF bl = (fancyTitleItem_
		? fancyTitleItem_->netBounds() 
		: titleItem->boundingRect())
    .bottomLeft();
  titleItem->setPos(l - bl.x(),
                    style_.real("margin-top") - style_.real("title-sep") - bl.y());
}

TitleItem *SheetScene::fancyTitleItem() {
  return fancyTitleItem_;
}

void SheetScene::setFancyTitle(TitleData *data, int sheet,
			       QTextDocument *doc) {
  if (plainTitleItem)
    plainTitleItem->deleteLater();
  plainTitleItem = 0;
  if (fancyTitleItem_)
    fancyTitleItem_->deleteLater();
  fancyTitleItem_ = new TitleItem(data, sheet, doc);
  connect(fancyTitleItem_,
	  SIGNAL(futileMovementKey(int, Qt::KeyboardModifiers)),
	  SLOT(futileTitleMovement(int, Qt::KeyboardModifiers)));
  addItem(fancyTitleItem_);
  titleItem = fancyTitleItem_;
  repositionTitle();
}

QTextDocument *SheetScene::fancyTitleDocument() {
  if (fancyTitleItem_)
    return fancyTitleItem_->document();
  else
    return 0;
}      

void SheetScene::setPageNumber(QString n) {
  if (!pgnoItem) {
   pgnoItem = addText("", style_.font("pgno-font"));
   pgnoItem->setDefaultTextColor(style_.color("pgno-color"));
   pgnoItem->setZValue(10); // page number on top
  }

  pgnoItem->setPlainText(n);
  QPointF tr = pgnoItem->boundingRect().topRight();
  pgnoItem->setPos(style_.real("page-width") 
		   - style_.real("margin-right-over") 
		   - tr.x(),
		   style_.real("page-height")
		   - style_.real("margin-bottom") 
		   + style_.real("pgno-sep") 
		   - tr.y());
}

void SheetScene::setContInMargin(bool x) {
  contInMargin = x;
}

void SheetScene::setOverlay(QGraphicsObject *gi) {
  if (overlay)
    delete overlay;
  overlay = gi;
  if (overlay)
    addItem(overlay);
}

void SheetScene::mousePressEvent(QGraphicsSceneMouseEvent *e) {
  if (base && base->mousePressEvent(e, this))
    return;
  QGraphicsScene::mousePressEvent(e);
}

void SheetScene::keyPressEvent(QKeyEvent *e) {
  if (base && base->keyPressEvent(e, this))
    return;
  QGraphicsScene::keyPressEvent(e);      
}

void SheetScene::drawBackground(QPainter *p, const QRectF &r) {
  QBrush border(style().color("border-color"));
  QBrush background(style().color("background-color"));
  p->setPen(Qt::NoPen);
  p->setBrush(border);
  p->drawRect(r);
  p->setBrush(background);
  p->drawRect(sceneRect());
}


void SheetScene::setEventView(QGraphicsView *pv) {
  eventView_ = pv;
}

QGraphicsView *SheetScene::eventView() const {
  QGraphicsView *ev = eventView_;
  if (ev)
    return ev;

  QList<QGraphicsView *> vv = views();
  return vv.isEmpty() ? 0 : vv[0];
}

void SheetScene::futileTitleMovement(int key, Qt::KeyboardModifiers) {
  switch (key) {
  case Qt::Key_Enter:
  case Qt::Key_Return:
  case Qt::Key_Down:
    emit leaveTitle();
    break;
  default:
    break;
  }
}
