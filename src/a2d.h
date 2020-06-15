#ifndef A2D_H
#define A2D_H

#include <QMainWindow>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMouseEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class A2D; }
QT_END_NAMESPACE

class A2D : public QMainWindow
{
    Q_OBJECT

public:
    A2D(QWidget *parent = nullptr);
    ~A2D();
    void dropEvent(QDropEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void unpackAppimage(QString filePath);
    void mkdirAllDirs();
    bool isDirExist(QString fullPath);
    void MoveFiles(QString AppimageName);
    void WriteControl();
    void MakeDeb();

private slots:
    void on_pushButton_clicked(bool checked);
    QStringList getFileNames(const QString &path);

private:
    Ui::A2D *ui;
};
#endif // A2D_H
