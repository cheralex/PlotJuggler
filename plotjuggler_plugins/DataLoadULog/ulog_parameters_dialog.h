#pragma once

#include <QDialog>
#include <QString>
#include <QSlider>
#include <QStack>
#include <QTableWidget>

#include "ulog_parser.h"

namespace Ui
{
class ULogParametersDialog;
}


class ULogParametersDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ULogParametersDialog(const ULogParser& parser, QWidget* parent = nullptr, double gps_time_offset_sec = 0.0);

  void restoreSettings();

  ~ULogParametersDialog();

private:
  Ui::ULogParametersDialog* ui;
  QSlider* timeSlider;
  std::map<int, ULogParser::AlertDefinition> alertsDefs;
  std::set<std::pair<double, int>> info_rows_by_ts;
  std::set<std::pair<double, int>> alerts_rows_by_ts;
  QTableWidget* activeWidget;
  double gps_time_offset_sec;

  void logsCellPressed(int row, int colum);
  void alertCellPressed(int row, int colum);
  bool loadAlertDefinitions();

  QString getAlertLevel(int level);
  int replaceParamPlaceholder(QString& pattern);

signals:
  void setTime(double);
  void alertRowSelected(int);
  void logRowSelected(int);

private slots:
  void timeSliderChanged(double);
};
