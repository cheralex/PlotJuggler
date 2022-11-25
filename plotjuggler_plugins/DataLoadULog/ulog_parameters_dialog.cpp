#include "ulog_parameters_dialog.h"
#include "ui_ulog_parameters_dialog.h"

#include <QTableWidget>
#include <QSettings>
#include <QHeaderView>

#include <QDebug>

#include <math.h>

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalBlocker>

#include <fmt/core.h>

ULogParametersDialog::ULogParametersDialog(const ULogParser& parser, QWidget* parent)
  : QDialog(parent), ui(new Ui::ULogParametersDialog)
{
  ui->setupUi(this);
  QTableWidget* table_info = ui->tableWidgetInfo;
  QTableWidget* table_params = ui->tableWidgetParams;
  QTableWidget* table_logs = ui->tableWidgetLogs;
  QTableWidget* table_alerts = ui->tableWidgetAlerts;


  timeSlider = parent->findChild<QSlider*>("timeSlider");
  if (timeSlider) {
      connect(this, SIGNAL(setTime(double)), timeSlider, SLOT(setCurrentValue(double)));
      connect(timeSlider, SIGNAL(realValueChanged(double)), this, SLOT(timeSliderChanged(double)));
  }

  table_info->setRowCount(parser.getInfo().size());
  int row = 0;
  for (const auto& it : parser.getInfo())
  {
    table_info->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(it.first)));
    table_info->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(it.second)));
    row++;
  }
  table_info->sortItems(0);

  table_params->setRowCount(parser.getParameters().size());
  row = 0;
  for (const auto& param : parser.getParameters())
  {
    table_params->setItem(row, 0,
                          new QTableWidgetItem(QString::fromStdString(param.name)));
    if (param.val_type == ULogParser::FLOAT)
    {
      table_params->setItem(row, 1,
                            new QTableWidgetItem(QString::number(param.value.val_real)));
    }
    else
    {
      table_params->setItem(row, 1,
                            new QTableWidgetItem(QString::number(param.value.val_int)));
    }
    row++;
  }
  table_params->sortItems(0);

  table_logs->setRowCount(parser.getLogs().size());
  row = 0;

  for (const auto& log_msg : parser.getLogs())
  {
    QString time = QString::number(0.001 * double(log_msg.timestamp / 1000), 'f', 2);
    table_logs->setItem(row, 0, new QTableWidgetItem(time));

    switch (log_msg.level)
    {
      case '0':
        table_logs->setItem(row, 1, new QTableWidgetItem("EMERGENCY"));
        break;
      case '1':
        table_logs->setItem(row, 1, new QTableWidgetItem("ALERT"));
        break;
      case '2':
        table_logs->setItem(row, 1, new QTableWidgetItem("CRITICAL"));
        break;
      case '3':
        table_logs->setItem(row, 1, new QTableWidgetItem("ERROR"));
        break;
      case '4':
        table_logs->setItem(row, 1, new QTableWidgetItem("WARNING"));
        break;
      case '5':
        table_logs->setItem(row, 1, new QTableWidgetItem("NOTICE"));
        break;
      case '6':
        table_logs->setItem(row, 1, new QTableWidgetItem("INFO"));
        break;
      case '7':
        table_logs->setItem(row, 1, new QTableWidgetItem("DEBUG"));
        break;
      default:
        table_logs->setItem(row, 1, new QTableWidgetItem(QString::number(log_msg.level)));
    }
    table_logs->setItem(row, 2,
                        new QTableWidgetItem(QString::fromStdString(log_msg.msg)));

    info_rows_by_ts.emplace(0.001 * double(log_msg.timestamp / 1000), row);

    row++;
  }
  connect(table_logs, &QTableWidget::cellPressed, this, &ULogParametersDialog::logsCellPressed, Qt::DirectConnection);
  connect(this, &ULogParametersDialog::logRowSelected, table_logs, &QTableWidget::selectRow, Qt::DirectConnection);

  loadAlertDefinitions();

  if (!parser.getAlerts().empty()) {
      table_alerts->setRowCount(parser.getAlerts().size());

      row = 0;
      for (const auto& alert : parser.getAlerts()) {
          QString time = QString::number(0.001 * double(alert.ts / 1000), 'f', 2);

          table_alerts->setItem(row, 0, new QTableWidgetItem(time));

          if (auto def = alertsDefs.find(alert.code); def != alertsDefs.end()) {
              QString message = def->second.message;
              if (replaceParamPlaceholder(message) > 0) {
                    message = message.arg(alert.param1, alert.param2, alert.param3);
              }
              table_alerts->setItem(row, 2, new QTableWidgetItem(message));
          } else {
              table_alerts->setItem(row, 2, new QTableWidgetItem(alert.code));
          }

         table_alerts->setItem(row, 1, new QTableWidgetItem(getAlertLevel(alert.level)));

         alerts_rows_by_ts.emplace(0.001 * double(alert.ts / 1000), row);

         row++;
      }
      connect(table_alerts, &QTableWidget::cellPressed, this, &ULogParametersDialog::alertCellPressed, Qt::DirectConnection);
      connect(this, &ULogParametersDialog::alertRowSelected, table_alerts, &QTableWidget::selectRow, Qt::DirectConnection);
  } else {
      table_alerts->setVisible(false);
  }
}

void ULogParametersDialog::restoreSettings()
{
  QTableWidget* table_info = ui->tableWidgetInfo;
  QTableWidget* table_params = ui->tableWidgetParams;

  QSettings settings;
  restoreGeometry(settings.value("ULogParametersDialog/geometry").toByteArray());
  table_info->horizontalHeader()->restoreState(
      settings.value("ULogParametersDialog/info/state").toByteArray());
  table_params->horizontalHeader()->restoreState(
      settings.value("ULogParametersDialog/params/state").toByteArray());

  table_info->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
  table_info->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);

  table_params->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
  table_params->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
}

ULogParametersDialog::~ULogParametersDialog()
{
  QTableWidget* table_info = ui->tableWidgetInfo;
  QTableWidget* table_params = ui->tableWidgetParams;

  QSettings settings;
  settings.setValue("ULogParametersDialog/geometry", this->saveGeometry());
  settings.setValue("ULogParametersDialog/info/state",
                    table_info->horizontalHeader()->saveState());
  settings.setValue("ULogParametersDialog/params/state",
                    table_params->horizontalHeader()->saveState());

  delete ui;
}

void ULogParametersDialog::logsCellPressed(int row, int colum)
{
    QTableWidget* info_table_logs = ui->tableWidgetLogs;
    QTableWidget* alert_table_logs = ui->tableWidgetAlerts;

    QTableWidgetItem* timeWidget = info_table_logs->item(row, 0);
    QString timeStr = timeWidget->text();

    activeWidget = info_table_logs;
    emit setTime(timeStr.toDouble());
}


void ULogParametersDialog::alertCellPressed(int row, int colum)
{
    QTableWidget* alert_table_logs = ui->tableWidgetAlerts;
    QTableWidget* info_table_logs = ui->tableWidgetLogs;

    QTableWidgetItem* timeWidget = alert_table_logs->item(row, 0);
    QString timeStr = timeWidget->text();

    activeWidget = alert_table_logs;
    emit setTime(timeStr.toDouble());
}


bool ULogParametersDialog::loadAlertDefinitions() {
    QFile file_obj(QString::fromStdString("alert.json"));
    if (!file_obj.open(QIODevice::ReadOnly)) {
       qDebug() << "Failed to open " << "alert.json";
       return false;
    }

    QTextStream file_text(&file_obj);
    QString json_string;
    json_string = file_text.readAll();
    file_obj.close();
    QByteArray json_bytes = json_string.toLocal8Bit();

    QJsonDocument json_doc = QJsonDocument::fromJson(json_bytes);

    if (json_doc.isObject()) {
        QJsonObject obj = json_doc.object();
        QJsonArray alerts =  obj["alerts"].toArray();
        for (QJsonValue alert : alerts) {
            int message_id = alert["code"].toInt();
            QString mess = alert["message"].toString();

            alertsDefs[message_id] = ULogParser::AlertDefinition{alert["message"].toString(), alert["description"].toString()};
        }
    }
    return true;
}

QString ULogParametersDialog::getAlertLevel(int level) {
    switch (level) {
        case 0: return QString("NONE");
        case 1: return QString("INFO");
        case 2: return QString("PREFLIGHT");
        case 3: return QString("WARNING");
        case 4: return QString("ERROR");
        case 5: return QString("CRITICAL");
        case 6: return QString("EMERGENCY");
    default :
        return QString("**Unknown %1 **").arg(level);
    }
}

int ULogParametersDialog::replaceParamPlaceholder(QString& pattern) {
    int start_index = 0;
    int end_index = 0;
    int counter = 0;
    for (start_index = pattern.indexOf("{"); start_index > 0; start_index = pattern.indexOf("{")) {
        end_index = pattern.indexOf("}", start_index);
        if (end_index > 0) {
            pattern = pattern.replace(start_index, end_index - start_index + 1, QString("%%1").arg(counter + 1) );
            counter++;
        }
    }
    return counter;
}

void ULogParametersDialog::timeSliderChanged(double value) {
    QTableWidget* table_logs = ui->tableWidgetLogs;
    QTableWidget* table_alerts = ui->tableWidgetAlerts;

    std::pair<double, int> tmp(-1, 0);
    for (auto const &pair : info_rows_by_ts) {
        if (pair.first > value)
            break;
        tmp = pair;
    }

    if (tmp.first > 0 && activeWidget != table_logs) {
        emit logRowSelected(tmp.second);
    }

    tmp.first = -1;
    for (auto const &pair : alerts_rows_by_ts) {
        if (pair.first > value)
            break;
        tmp = pair;
    }

    if (tmp.first > 0 && activeWidget != table_alerts) {
        emit alertRowSelected(tmp.second);
    }

    activeWidget = nullptr;
}
