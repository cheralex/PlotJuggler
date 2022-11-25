#pragma once

#include <QDialog>
#include <QString>
#include <QSlider>

#include "ulog_parser.h"

namespace Ui
{
class ULogParametersDialog;
}


class ULogParametersDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ULogParametersDialog(const ULogParser& parser, QWidget* parent = nullptr);

  void restoreSettings();

  ~ULogParametersDialog();

private:
  Ui::ULogParametersDialog* ui;
  QSlider* timeSlider;
  std::map<int, ULogParser::AlertDefinition> alertsDefs;
  void paramCellPressed(int row, int colum);
  void alertCellPressed(int row, int colum);
  bool loadAlertDefinitions();

  QString getAlertLevel(int level);
  int replaceParamPlaceholder(QString& pattern);

signals:
  void setTime(double);

};
