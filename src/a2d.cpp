#include "a2d.h"
#include "ui_a2d.h"
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QDebug>
#include <QIcon>
#include <QStyle>
#include <QScrollBar>
#include <QMessageBox>
#include <QFile>
#include <QIcon>
#include <QTextStream>
#include <QtConcurrent>
#include <QValidator>
#include <QtSvg/QSvgRenderer>
#include <QtSvg/QtSvg>
#include <QPainter>
#include <QFileDialog>
QString Name;
QString Nameand;
QString Path;
QString AppimageName;
QString Version;
int step=1;

A2D::A2D(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::A2D)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/images/Icon.png"));
    setWindowOpacity(1);
    setAttribute(Qt::WA_TranslucentBackground);
    ui->label->setPixmap(QIcon(":/images/builder.png").pixmap(135,135));
    ui->notify->setText("拖拽文件至左侧");
    ui->lineEdit_2->setPlaceholderText("输入软件版本");
    ui->progressBar->setValue(0);
    this->setAcceptDrops(true);
    QRegExp regx("[0-9]+[0-9a-zA-Z.-+~]{29}");  //使用正则表达式规范输入一定程度上防止用户非法输入
    QValidator *validator = new QRegExpValidator(regx, ui->lineEdit_2 );
    ui->lineEdit_2->setValidator( validator );
    ui->label->setScaledContents(true);
}

A2D::~A2D()
{
    delete ui;
}
void A2D::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}
void A2D::dropEvent(QDropEvent *event)
{
    QString name = event->mimeData()->urls().at(0).toLocalFile();
    ui->lineEdit->clear();
    ui->notify->setText("正在解析软件包");
    ui->lineEdit_2->setText("1.0");
    ui->progressBar->setValue(10);
    QFileInfo fileInfo;
    fileInfo=QFileInfo(name);
    Name=fileInfo.baseName();
    Nameand=fileInfo.fileName();
    Path=fileInfo.absolutePath();
    AppimageName=fileInfo.filePath();
    ui->lineEdit->setText(Name);
    unpackAppimage(AppimageName);
}
void A2D::unpackAppimage(QString filePath)
{
    {
        system("rm -r /tmp/squashfs-root");
        QDir::setCurrent("/tmp/");
        QProcess process;
        process.start("cd /tmp");
        process.waitForFinished();
        system("chmod +x "+filePath.toUtf8());
        process.start(filePath.toUtf8() + " --appimage-extract");
        process.waitForFinished();
                process.close();
        ui->progressBar->setValue(20);
        ui->notify->setText("开始解析图标");
        //开始读取图标
        QProcess lspng;
        lspng.start("ls /tmp/squashfs-root/");
        lspng.waitForFinished();
        QString output=lspng.readAllStandardOutput();
        QStringList pngname = output.split("\n");
        for (int i = 0;i<pngname.size();i++) {
            if(pngname[i].right(4)==".png" || pngname[i].right(4)==".svg"){
                pngname[0]=pngname[i];
            }
        }
        qDebug()<<pngname[0];
        if(pngname[0].right(3)=="png"){
            ui->label->setPixmap(QPixmap::fromImage(QImage("/tmp/squashfs-root/"+pngname[0])));
            QFileInfo Icon;
            Icon=QFileInfo("/tmp/squashfs-root/"+pngname[0]);
            Name=Icon.baseName();
            ui->lineEdit->setText(Name);
            ui->notify->setText("开始构建目录");
            Version=ui->lineEdit_2->text();
            ui->progressBar->setValue(40);
        }else {
            QString svgPath = "/tmp/squashfs-root/"+pngname[0].toUtf8();
            QSvgRenderer* svgRender = new QSvgRenderer();
            svgRender->load(svgPath);
            QPixmap* pixmap = new QPixmap(128,128);
            pixmap->fill(Qt::transparent);//设置背景透明
            QPainter p(pixmap);
            svgRender->render(&p);
            ui->label->setPixmap(*pixmap);
            ui->label->setAlignment(Qt::AlignHCenter);
            QFileInfo Icon;
            Icon=QFileInfo(svgPath);
            Name=Icon.baseName();
            ui->lineEdit->setText(Name);
            Version=ui->lineEdit_2->text();
            ui->progressBar->setValue(40);
            ui->notify->setText("解析完毕");
        }
}
}
void A2D::on_pushButton_clicked(bool checked)
{
    if (AppimageName=="")
    {
        QMessageBox::critical(this,"怎么骗人家啊嘤嘤嘤","您似乎没有选中文件呐~","好哒~");
    }
    else
    {
        ui->pushButton->setEnabled(false);
        mkdirAllDirs();
        ui->progressBar->setValue(60);
        MoveFiles(AppimageName);
        ui->progressBar->setValue(80);
        WriteControl();
        ui->progressBar->setValue(90);
        ui->notify->setText("打包中");
        //ui->progressBar->hide();
        QtConcurrent::run([=](){
            MakeDeb();
        });
        ui->pushButton->setEnabled(true);
    }
}
void A2D::mkdirAllDirs()
{
    QProcess *mkdir = new QProcess();
    mkdir->start("mkdir /tmp/"+Name);
    mkdir->waitForFinished();
    mkdir->start("mkdir /tmp/"+Name+"/opt");
    mkdir->waitForFinished();
    mkdir->start("mkdir /tmp/"+Name+"/opt/durapps");
    mkdir->waitForFinished();
    mkdir->start("mkdir /tmp/"+Name+"/opt/durapps/"+Name);
    mkdir->waitForFinished();
    mkdir->start("mkdir /tmp/"+Name+"/DEBIAN");
    mkdir->waitForFinished();
    if (isDirExist("/tmp/squashfs-root/usr/share/applications"))
    {
        return;
    }
    else
    {
        mkdir->start("mkdir /tmp/"+Name+"/usr");
        mkdir->waitForFinished();
        mkdir->start("mkdir /tmp/"+Name+"/usr/share");
        mkdir->waitForFinished();
        mkdir->start("mkdir /tmp/"+Name+"/usr/share/applications");
        mkdir->waitForFinished();
        mkdir->close();
    }
    ui->notify->setText("移入文件中");
}
bool A2D::isDirExist(QString fullPath)
{
    QDir dir(fullPath);
    if(dir.exists())
    {
        return true;
    }
    return false;
}
void A2D::MoveFiles(QString AppimageName)
{

    QProcess *moveAppImage = new QProcess;
    moveAppImage->start("cp -a "+AppimageName+" /tmp/"+Name+"/opt/durapps/"+Name);
    moveAppImage->waitForFinished();
    moveAppImage->start("mv /tmp/"+Name+"/opt/durapps/"+Name+"/"+Nameand+" /tmp/"+Name+"/opt/durapps/"+Name+"/"+Name+".AppImage");
    Nameand=Name+".AppImage";
    moveAppImage->waitForFinished();
    moveAppImage->start("sleep 2");
    moveAppImage->waitForFinished();
    moveAppImage->close();
    QProcess *moveDesktop= new QProcess();
    if(isDirExist("/tmp/squashfs-root/usr/share/applications"))
    {
        moveDesktop->start("cp -a /tmp/squashfs-root/usr /tmp/"+Name);
        moveDesktop->waitForFinished();
        moveDesktop->start("rm -rf /tmp/"+Name+"/usr/lib");
        moveDesktop->waitForFinished();
    }
    else
    {
        moveDesktop->start("cp -a /tmp/squashfs-root/"+Name+".desktop "+"/tmp/"+Name+"/usr/share/applications");
        moveDesktop->waitForFinished();
        moveDesktop->start("cp -a /tmp/squashfs-root/usr /tmp/"+Name);
        moveDesktop->waitForFinished();
        moveDesktop->start("rm -rf /tmp/"+Name+"/usr/lib");
        moveDesktop->waitForFinished();
    }
    moveDesktop->close();
    QProcess *rewriteDesktop = new QProcess();
    rewriteDesktop->start("sed -i \"/AppRun/d\" /tmp/"+Name+"/usr/share/applications/"+Name+".desktop");
    rewriteDesktop->waitForFinished();
    rewriteDesktop->close();
    QFile file("/tmp/"+Name+"/usr/share/applications/"+Name+".desktop");
    if(! file.open(QIODevice::Append|QIODevice::Text))  //append追加，不会覆盖之前的文件
    {
        QMessageBox::critical(this,"错误","拉取桌面配置文件失败！","确定");
        return;
    }
    QTextStream out(&file);//写入
    out <<"Exec=/opt/durapps/"+Name+"/"+Nameand<<endl;
    out.flush();
    file.close();
    ui->notify->setText("写入控制文件");
}
void A2D::WriteControl()
{
    QFile control("/tmp/"+Name+"/DEBIAN/control");
    if(! control.open(QIODevice::Append|QIODevice::Text))  //append追加，不会覆盖之前的文件
    {
        QMessageBox::critical(this,"错误","写入控制文件失败！","确定");
        return;
    }
    QTextStream write(&control);//写入
    write <<"Package:"+Name<<endl;
    write <<"Version:"+Version<<endl;
    write <<"Section:utils"<<endl;
    write <<"Priority:optional"<<endl;
    write <<"Architecture:amd64"<<endl;
    write <<"Essential:no"<<endl;
    write <<"Depends:"<<endl;
    write <<"Recommends:"<<endl;
    write <<"Suggests:"<<endl;
    write <<"Maintainer:DCS[http://http://www.shenmo.tech:420]"<<endl;
    write <<"Conflicts:"<<endl;
    write <<"Replaces:"<<endl;
    write <<"Description:An application made from a2d(Appimage to Deb)"<<endl;
    write <<endl;
    write.flush();
    control.close();
    ui->notify->setText("一切就绪，开始打包");
    QMessageBox::warning(this,"提示","构建deb包的过程可能会很慢，请您耐心等待","确定");
}
void A2D::MakeDeb()
{
    QProcess *dpkg = new QProcess();
    dpkg->start("fakeroot dpkg -b /tmp/"+Name+" "+QDir::homePath()+"/"+"桌面/"+Name+"-"+Version+"_amd64.deb");
    dpkg->waitForFinished();
    QProcess *rm = new QProcess();
    rm->start("rm -rf /tmp/squashfs-root");
    rm->waitForFinished();
    rm->start("rm -rf /tmp/"+Name);
    rm ->waitForFinished();
    rm->close();
    ui->notify->setText("打包完毕");
    ui->progressBar->setValue(100);
}
