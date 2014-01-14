// App/ToolItem.cpp - This file is part of eln

/* eln is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   eln is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with eln.  If not, see <http://www.gnu.org/licenses/>.
*/

// ToolItem.C

#include "ToolItem.H"
#include "Toolbar.H"
#include "Style.H"
#include <QSvgRenderer>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QTextDocument>
#include <QGraphicsScene>

#define TOOLSIZE 31.5
#define TOOLRAD 3.0
#define HOVERDX 1.5
#define HOVERDX1 0.5
#define SHRINK 1
#define POPUPDELAY 1500

ToolItem::ToolItem(): QGraphicsObject() {
  popupDelay = 0;
  balloon = 0;
  svg = 0;
  sel = false;
  hov = false;
  setAcceptHoverEvents(true);
}

ToolItem::~ToolItem() {
}

void ToolItem::setSvg(QString filename) {
  if (svg)
    delete svg;
  svg = new QSvgRenderer(filename, this);
  update();
}

bool ToolItem::isSelected() const {
  return sel;
}

void ToolItem::setSelected(bool s) {
  sel = s;
  update();
}

QRectF ToolItem::boundingRect() const {
  return QRectF(0, 0, TOOLSIZE, TOOLSIZE);
}

void ToolItem::paintContents(QPainter *p) {
  if (svg) 
    svg->render(p, boundingRect());
}

void ToolItem::setBalloonHelpText(QString txt) {
  helpText = txt;
}

void ToolItem::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *) {
  QColor blk("black");
  blk.setAlphaF(0.25);
  QColor wht("white");
  wht.setAlphaF(0.5);

  if (sel) {  
    p->setPen(Qt::NoPen);
    p->setBrush(blk);
    p->drawRoundedRect(QRectF(0, 0, TOOLSIZE, TOOLSIZE), TOOLRAD, TOOLRAD);

    QColor grn("green");
    grn.setAlphaF(0.75);
    p->setBrush(QBrush(Qt::NoBrush));
    p->setPen(QPen(grn, 4));
    p->drawRoundedRect(QRectF(2, 2, TOOLSIZE-4, TOOLSIZE-4), TOOLRAD, TOOLRAD);

    p->setPen(Qt::NoPen);
    p->setBrush(wht);
    p->drawRoundedRect(QRectF(0, 0, TOOLSIZE-HOVERDX, TOOLSIZE-HOVERDX),
		       TOOLRAD, TOOLRAD);
  } else {
    p->setPen(Qt::NoPen);
    p->setBrush(blk);
    p->drawRoundedRect(QRectF(SHRINK, SHRINK,
			      TOOLSIZE-2*SHRINK, TOOLSIZE-2*SHRINK),
		       TOOLRAD, TOOLRAD);
    
    p->setBrush(wht);
    if (hov) 
      p->drawRoundedRect(QRectF(SHRINK+HOVERDX, SHRINK+HOVERDX,
				TOOLSIZE-2*SHRINK-HOVERDX,
				TOOLSIZE-2*SHRINK-HOVERDX),
			 TOOLRAD, TOOLRAD);
    else 
      p->drawRoundedRect(QRectF(SHRINK, SHRINK,
				TOOLSIZE-2*SHRINK-HOVERDX,
				TOOLSIZE-2*SHRINK-HOVERDX),
			 TOOLRAD, TOOLRAD);

    if (hov)
      p->translate(HOVERDX1, HOVERDX1);
  }    

  paintContents(p);
}

void ToolItem::hoverEnterEvent(QGraphicsSceneHoverEvent *e) {
  hov = true;
  update();
  if (!helpText.isEmpty()) {
    if (popupDelay==0) {
      popupDelay = new QTimer(this);
      popupDelay->setSingleShot(true);
      connect(popupDelay, SIGNAL(timeout()), SLOT(popup()));
    }
    popupDelay->start(POPUPDELAY);
    popupPos = e->pos();
  }
}

void ToolItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *) {
  hov = false;
  update();
  if (popupDelay)
    popupDelay->stop();
  if (balloon)
    delete balloon;
  balloon = 0;
}

void ToolItem::hoverMoveEvent(QGraphicsSceneHoverEvent *e) {
  if (!helpText.isEmpty()) {
    if (popupDelay) 
      popupDelay->start(POPUPDELAY);
    popupPos = e->pos();
  }
}   

void ToolItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
  emit release();
  e->accept();
}

void ToolItem::mousePressEvent(QGraphicsSceneMouseEvent *e) {
  switch (e->button()) {
  case Qt::LeftButton:
    emit leftClick();
    e->accept();
    break;
  case Qt::RightButton:
    emit rightClick();
    e->accept();
    break;
  default:
    e->ignore();
    break;
  }
}

 void ToolItem::popup() {
   if (balloon)
     return;
   if (helpText.isEmpty())
     return;
   if (!scene())
     return;

   Style const &style = Style::defaultStyle();
   
   QGraphicsTextItem *ti = new QGraphicsTextItem(this);
   ti->document()->setDefaultFont(style.font("popup-font"));
   ti->setZValue(1);
   ti->setPlainText(helpText);
   if (ti->boundingRect().width()>500)
     ti->setTextWidth(500);
   double margin = style.real("popup-margin");
   QGraphicsRectItem *rect
     = scene()->addRect(ti->boundingRect().adjusted(-margin, -margin,
						    margin, margin),
			QPen(), style.color("popup-background-color"));

   ti->setParentItem(rect);
   rect->setPos(mapToScene(popupPos + QPointF(10, 10)));
   // How to pick font etc?
   balloon = rect;
 }
