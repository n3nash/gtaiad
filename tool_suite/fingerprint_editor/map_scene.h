#ifndef MAP_SCENE_H
#define MAP_SCENE_H

#include <QWidget>
#include <QGraphicsSceneMouseEvent>
#include <QPointF>
#include <QGraphicsScene>
#include <QColor>
#include <QList>
#include <QGraphicsEllipseItem>

// structure holding information to represent the measurement locations
class PermanentMarker : public QGraphicsEllipseItem
{
public:
  PermanentMarker(const QRectF& rect, const QString& loc_id, QGraphicsEllipseItem* parent=0);

  QString m_loc_id;
};

class MapScene : public QGraphicsScene
{
  Q_OBJECT

public:
  MapScene(const QString& map_filename, QObject* parent=0);
  void add_marker(const QString& loc_id, const QPointF& pos);
  void set_marker_color(const QColor& color = Qt::black);

signals:
  void temp_marker_set(const QPointF& pos);
  void location_selected(const QString& loc_id);

public slots:
  void set_temp_marker(const QPointF& pos, const QColor& color = Qt::red);
  void clear_temp_marker(void);
  void highlight_location(const QPointF& pos);
  void unhighlight_location();

private:
  void mousePressEvent(QGraphicsSceneMouseEvent* e);

  QGraphicsEllipseItem* m_pending_capture_loc_ellipseitem_ptr;
  QGraphicsEllipseItem* m_highlighted_capture_loc_ellipseitem_ptr;
  QColor m_permanent_marker_color;
  QList<PermanentMarker*> m_markers_list;
};

#endif
