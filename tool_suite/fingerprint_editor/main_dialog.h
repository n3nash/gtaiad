#ifndef MAIN_DIALOG_H
#define MAIN_DIALOG_H

#include <QDialog>
#include <QGraphicsView>
#include <QPointF>
#include <QStringList>
#include <QSqlDatabase>
#include <QPair>
#include <QList>
#include <QTableWidgetItem>

#include "ui_main_dialog.h"
#include "map_scene.h"

class MainDialog : public QDialog, public Ui::MainDialog
{
  Q_OBJECT

public:
  MainDialog(QWidget* parent = 0, const QSqlDatabase& db = QSqlDatabase());

public slots:
  void update_floor_scale(int scaling_factor);   // Set scale to 1/scaling_factor
  void enter_capture_mode(void);
  void exit_capture_mode(void);
  void run_airodump_clicked(void);
  void capture_location_changed(const QPointF& pos);
  void insert_location_id_clicked(void);
  void change_floor(int image_index);
  void fingerprint_location_dot_selected(const QString&);
  void fingerprint_location_row_changed(int current_row, int current_column, int previous_row, int previous_column);
  void reset_table_signal_filter_kludge(void);

signals:
  void new_capture_canceled(void);
  void new_capture_added(const QPoint&);

private:
  bool validate_loc_id(void);
  void init_floor_scenes(void);
  QList< QPair<QString, QPointF> > get_measurement_locations(int floor_number);
  void repopulate_fingerprints_table(const QString& loc_id);
  void add_new_capture_point(const QPoint& point);
  void highlight_selected_capture_point(int row_index);

  const QSqlDatabase& m_db;
  QStringList m_floor_image_filenames;
  QList<MapScene*> m_map_scenes;
  QList< QList< QPair<QString, QPointF> > > all_measurement_locations;
  int table_signal_filter_kludge;
};

#endif
