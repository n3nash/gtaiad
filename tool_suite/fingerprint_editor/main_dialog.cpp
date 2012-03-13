#include <assert.h>

#include <QDebug>
#include <QMessageBox>
#include <QPixmap>
#include <QPointF>
#include <QSqlQuery>
#include <QBitmap>
#include <QGraphicsPixmapItem>

#include "main_dialog.h"
#include "map_scene.h"

MainDialog::MainDialog(QWidget* parent, const QSqlDatabase& db) : QDialog(parent),
    m_db(db), m_floor_image_filenames(QStringList())
{
  assert(db.isOpen());

  setupUi(this);

  m_floor_image_filenames << "../level_one.jpg";
  m_floor_image_filenames << "../level_two.jpg";
  m_floor_image_filenames << "../level_three.jpg";

  init_floor_scenes();

  change_floor(0);

  update_floor_scale(zoom_slider->value());

#ifdef Q_WS_MAC
  map_view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
#endif

  // Remove dialog example data
  loc_id_line_edit->clear();
  x_pos_label->clear();
  y_pos_label->clear();

}

// Slot called when the zoom slider position is changed
void MainDialog::update_floor_scale(int scaling_factor)
{
  map_view->resetTransform();
  map_view->scale(100.0/scaling_factor, 100.0/scaling_factor);
}

bool MainDialog::validate_loc_id(void)
{
  QString loc_id = loc_id_line_edit->displayText();

  if (loc_id.isEmpty())
  {
    QMessageBox::warning(this, "Invalid Loc ID",
        "Location ID can not be blank.");

    return false;
  }
  else
  {
    // Convert entered text to uppercase
    loc_id_line_edit->setText(loc_id.toUpper().trimmed());
  }

  return true;
}

void MainDialog::run_airodump_clicked(void)
{
  if (x_pos_label->text().isEmpty() || y_pos_label->text().isEmpty())
  {
    QMessageBox::warning(this, "Invalid Capture Location",
        "No capture location has been identified on the map.");
    return;
  }

  if (!validate_loc_id())
  {
    return;
  }

  // TODO: perform AP capture magic
  // TODO: perform Database magic

  //emit new_capture_added(new_capture_location);
}

void MainDialog::insert_location_id_clicked(void)
{
  if (!validate_loc_id())
  {
    return;
  }

  QSqlQuery q(m_db);

  q.prepare(" \
      INSERT INTO capture_locations ( \
        capture_location_name, capture_location_floor, \
        capture_location_x_pos, capture_location_y_pos) \
      VALUES (:capture_location_name, :capture_location_floor, \
              :capture_location_x_pos, :capture_location_y_pos)");

  QPoint new_capture_location(x_pos_label->text().toInt(),
                              y_pos_label->text().toInt());

  q.bindValue(":capture_location_name", loc_id_line_edit->displayText());
  q.bindValue(":capture_location_floor", floor_combobox->currentIndex() + 1);
  q.bindValue(":capture_location_x_pos", x_pos_label->text().toInt());
  q.bindValue(":capture_location_y_pos", y_pos_label->text().toInt());

  bool res = q.exec();

  if (!res)
  {
    QMessageBox::warning(this, "Database Error",
        "Failed during insert of new capture location.");
    return;
  }

  emit new_capture_added(new_capture_location);

  exit_capture_mode();
  loc_id_line_edit->clear();
}

// slot: called when user clicks on the floor map for red dot placement
void MainDialog::capture_location_changed(const QPointF& pos)
{
  // Ignore the update request when the new_capture_groupbox
  // is not enabled.
  // TODO: Update enables of buttons when loc ID is entered
  if (!new_capture_groupbox->isEnabled())
  {
    MapScene* map_scene;

    map_scene = static_cast<MapScene*>(map_view->scene());
    map_scene->clear_temp_marker();
    return;
  }

  QPoint int_pos = pos.toPoint();  // does proper rounding

  x_pos_label->setText(QString::number(int_pos.x()));
  y_pos_label->setText(QString::number(int_pos.y()));
}

// Slot called when the user hits the 'New Capture' button.
void MainDialog::enter_capture_mode(void)
{
  new_capture_push_button->setEnabled(false);
  new_capture_groupbox->setEnabled(true);
  new_capture_push_button->setEnabled(false);
  floor_combobox->setEnabled(false);
  loc_id_line_edit->setFocus();
}

// Do necessary clearing of widgets and enable/disables
void MainDialog::exit_capture_mode(void)
{
  x_pos_label->clear();
  y_pos_label->clear();
  new_capture_groupbox->setEnabled(false);
  new_capture_push_button->setEnabled(true);
  floor_combobox->setEnabled(true);

  emit new_capture_canceled();
}

// Generate the MapScene object for each floor map.  Store the scenes in
// the QList<T> generic container.
void MainDialog::init_floor_scenes(void)
{
  int floor_number = 0;

  // assumed order is 1, 2, 3
  foreach (const QString& filename, m_floor_image_filenames)
  {
    MapScene* map_scene = new MapScene(filename);
    QList< QPair<QString, QPointF> > measurement_locations;
    floor_number++;

    if (floor_number == 1) map_scene->set_marker_color(Qt::blue);
    else if (floor_number == 2) map_scene->set_marker_color(Qt::cyan);
    else map_scene->set_marker_color(Qt::darkGreen);

    m_map_scenes.append(map_scene);

    measurement_locations = get_measurement_locations(floor_number);
    QListIterator< QPair<QString, QPointF> > itr(measurement_locations);
    while (itr.hasNext())
    {
      QPair<QString, QPointF> measurement_location = itr.next();

      map_scene->add_marker(measurement_location.first, measurement_location.second);
    }

    // Connect signals of MapScene
    QObject::connect(map_scene, SIGNAL(temp_marker_set(const QPointF&)),
        this, SLOT(capture_location_changed(const QPointF&)));

    QObject::connect(this, SIGNAL(new_capture_canceled()),
        map_scene, SLOT(clear_temp_marker()));

    QObject::connect(this, SIGNAL(new_capture_added(const QPoint&)),
        map_scene, SLOT(clear_temp_marker()));

    QObject::connect(map_scene, SIGNAL(location_selected(const QString&)),
        this, SLOT(fingerprint_location_selected(const QString&)));
  }
}

// slot: called when floor dropdown is changed
void MainDialog::change_floor(int image_index)
{
  MapScene* map_scene = m_map_scenes.at(image_index);
  map_view->setScene(map_scene);

  // Change focus to any place other than the QComboBox
  map_view->setFocus();
}

// read from the database all of a floor's loc_ids and xpos,ypos information
QList< QPair<QString, QPointF> > MainDialog::get_measurement_locations(int floor_number)
{
  QSqlQuery q(m_db);
  QList< QPair<QString, QPointF> > rvalue;

  q.prepare(" \
      SELECT \
        capture_location_name, capture_location_x_pos, capture_location_y_pos \
      FROM capture_locations \
      WHERE capture_location_floor = :capture_location_floor");

  q.bindValue(":capture_location_floor", floor_number);

  bool res = q.exec();

  if (!res)
  {
    QMessageBox::warning(this, "Database Error",
        "Failed during SELECT from capture_locations table.");
    return rvalue;
  }

  while (q.next())
  {
    QPair<QString, QPointF> tuple;

    tuple.first = q.value(0).toString();
    tuple.second = QPointF(q.value(1).toFloat(), q.value(2).toFloat());

    rvalue.append(tuple);
  }

  return rvalue;
}

// slot to be called when user clicks on a fingerprint dot
void MainDialog::fingerprint_location_selected(const QString& loc_id)
{
  qDebug() << "Update table for" << loc_id;
}
