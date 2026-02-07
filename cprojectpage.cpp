#include "cprojectpage.h"
#include "ui_cprojectpage.h"
//#include <QDir>
#include "quazipfile.h"
#include "idevice.h"
#include "qdprpixmap.h"

CProjectPage::CProjectPage(QString path, bool quickStart, QString bgImagePath, QString extension, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CProjectPage)
{
    Q_INIT_RESOURCE(projectpage);
    ui->setupUi(this);
    ui->listWidget->setIconSize(QSize(400,300));
    m_Icon = QIcon(createSquare(QPixmap(":/layout.png")));
    m_Ext = extension;
    m_QuickStart = quickStart;
    m_Path = path;
    //if (!bgImagePath.isEmpty()) setStyleSheet("QListWidget{background:url("+bgImagePath+");}");
    QDPRPixmap::setWidgetBackground(this,bgImagePath,QPalette::Base);
    ui->listWidget->setMouseTracking(true);
    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listWidget,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(itemDoubleClicked(QListWidgetItem*)));
    connect(ui->listWidget,SIGNAL(itemChanged(QListWidgetItem*)),this,SLOT(finishEditItem(QListWidgetItem*)));
    connect(ui->listWidget,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showPopup(QPoint)));
    connect(ui->listWidget,SIGNAL(itemClicked(QListWidgetItem*)),this,SLOT(itemClicked(QListWidgetItem*)));
    m_Menu = new QMenu(this);
    m_Menu->addAction("Open",this,&CProjectPage::open);
    m_Menu->addAction("Delete",this,&CProjectPage::remove);
    m_Menu->addAction("Rename",this,&CProjectPage::rename);
    m_Menu->addAction("Duplicate",this,&CProjectPage::duplicate);
    fillView();
}

CProjectPage::~CProjectPage()
{
    delete ui;
}

void CProjectPage::itemClicked(QListWidgetItem* i)
{
    //QString s(i->data(34).toString());
    if (i->text()=="New Project")
    {
        emit newProjectTriggered();
    }
    else if (i->text()=="Quick Start")
    {
        emit quickStartTriggered();
    }
}

void CProjectPage::itemDoubleClicked(QListWidgetItem* i)
{
    QString s(i->data(34).toString());
    if (i->text()=="New Project")
    {
        emit newProjectTriggered();
    }
    else if (i->text()=="Quick Start")
    {
        emit quickStartTriggered();
    }
    else
    {
        emit openProjectTriggered(s);
    }
}

void CProjectPage::finishEditItem(QListWidgetItem* i)
{
    QString s(i->data(34).toString());
    if (!s.isEmpty())
    {
        QFileInfo f(s);
        if (f.exists())
        {
            QString n = f.path() + "/" + i->text() + "." + f.suffix();
            QFile(s).rename(n);
            i->setData(34,n);
        }
    }
}

void CProjectPage::showPopup(QPoint pos)
{
    m_Menu->popup(mapToGlobal(pos));
}

void CProjectPage::open()
{
    if (!ui->listWidget->selectedItems().empty())
    {
        QString s(ui->listWidget->selectedItems().first()->data(34).toString());
        if (!s.isEmpty())
        {
            emit openProjectTriggered(s);
        }
    }
}

void CProjectPage::rename()
{
    if (!ui->listWidget->selectedItems().empty())
    {
        ui->listWidget->editItem(ui->listWidget->selectedItems().first());
    }
}

void CProjectPage::remove()
{
    if (!ui->listWidget->selectedItems().empty())
    {
        QString s(ui->listWidget->selectedItems().first()->data(34).toString());
        if (!s.isEmpty())
        {
            int r = nativeAlert(this,"Remove Document","Are You sure?",{"Ok","Cancel"});
            if (r == 1001) return;

            QFile(s).remove();
            fillView();
        }
    }
}

void CProjectPage::duplicate()
{
    if (!ui->listWidget->selectedItems().empty())
    {
        QString s(ui->listWidget->selectedItems().first()->data(34).toString());
        if (!s.isEmpty())
        {
            QFileInfo f(s);
            QString n = f.path() + "/" + f.baseName() + " copy." + f.suffix();
            QFile(s).copy(n);
            fillView();
        }
    }
}

QPixmap CProjectPage::createSquare(QPixmap p)
{
    QRect r = p.rect();
    if (r.width() > r.height())
    {
        r.setLeft((r.width()-r.height())/2);
        r.setWidth(r.height());
    }
    else if (r.height() > r.width())
    {
        r.setTop((r.height()-r.width())/2);
        r.setHeight(r.width());
    }
    return p.copy(r);
}

void CProjectPage::fillView()
{
    ui->listWidget->clear();
    ui->listWidget->setIconSize(QSize(180,180));
    auto i = new QListWidgetItem("New Project");
    i->setFlags(Qt::ItemIsEnabled);
    i->setIcon(QIcon(":/unnamed.png"));
    i->setSizeHint(QSize(200,200));
    ui->listWidget->addItem(i);
    if (m_QuickStart)
    {
        auto i = new QListWidgetItem("Quick Start");
        i->setFlags(Qt::ItemIsEnabled);
        i->setIcon(QIcon(":/quickstart.png"));
        i->setSizeHint(QSize(200,200));
        ui->listWidget->addItem(i);
    }
    for (const QString& s : std::as_const(m_CustomProjects)) {
        QFileInfo f(s);
        if (f.exists()) {
            auto i = new QListWidgetItem(f.baseName());
            i->setFlags(Qt::ItemIsEnabled);
            QPixmap m(f.filePath());
            if (f.suffix()=="zip")
            {
                QuaZip zip(f.filePath());
                if (zip.open(QuaZip::mdUnzip))
                {
                    QuaZipFile file(&zip);
                    //QStringList xmlfiles;
                    for (bool f=zip.goToFirstFile(); f; f=zip.goToNextFile())
                    {
                        if (!file.isOpen()) file.open(QIODevice::ReadOnly);
                        m.loadFromData(file.readAll());
                        file.close();
                        if (!m.isNull()) break;
                    }
                    zip.close();
                }
            }
            if (!m.isNull())
            {
                i->setIcon(QIcon(createSquare(m)));
            }
            else
            {
                i->setIcon(QIcon(":/quickstart.png"));
            }
            i->setSizeHint(QSize(200,200));
            i->setData(34,f.filePath());
            ui->listWidget->addItem(i);
        }
    }
    QDir d(m_Path);
    QStringList images = d.entryList(QStringList() << "*." + m_Ext << "*.zip", QDir::Files | QDir::Dirs, QDir::Time);
    for (const QString& s : std::as_const(images))
    {
        QFileInfo f(d.path() + "/" + s);
        if (f.exists())
        {
            auto i = new QListWidgetItem(f.baseName());
            i->setData(34,f.filePath());
            QPixmap m(f.filePath());
            if (m.isNull())
            {
                if (f.suffix()=="zip")
                {
                    QuaZip zip(f.filePath());
                    if (zip.open(QuaZip::mdUnzip))
                    {
                        QuaZipFile file(&zip);
                        //QStringList xmlfiles;
                        for (bool f=zip.goToFirstFile(); f; f=zip.goToNextFile())
                        {
                            if (!file.isOpen()) file.open(QIODevice::ReadOnly);
                            m.loadFromData(file.readAll());
                            file.close();
                            if (!m.isNull()) break;
                        }
                        zip.close();
                    }
                }
            }
            if (!m.isNull())
            {
                i->setIcon(QIcon(createSquare(m)));
            }
            else
            {
                if (!m_Icon.isNull()) i->setIcon(m_Icon);
            }
            i->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            i->setSizeHint(QSize(200,200));
            ui->listWidget->addItem(i);
        }
    }
}

QDomLiteDocument CProjectPage::openFile(const QString& path, const QString& suffix) {
    QFileInfo f(path);
    QDomLiteDocument d;
    if (f.suffix()=="zip")
    {
        QuaZip zip(f.filePath());
        if (zip.open(QuaZip::mdUnzip))
        {
            QuaZipFile file(&zip);
            //QStringList xmlfiles;
            for (bool f=zip.goToFirstFile(); f; f=zip.goToNextFile())
            {
                const QString fn = file.getActualFileName();
                if ((fn.endsWith(suffix)) || (fn.endsWith(".xml")))
                {
                    if (!file.isOpen()) file.open(QIODevice::ReadOnly);
                    //QDomLiteDocument d;
                    QByteArray b(file.readAll());
                    d.fromByteArray(b);
                    //unserializeCustom(d.documentElement);
                    file.close();
                    break;
                }
            }
            zip.close();
        }
    }
    else
    {
        //isImport = m_Document->Load(Path);
        //const QDomLiteDocument Doc(path);
        d.load(path);
        //qDebug() << Doc.toString();
        //unserializeCustom(Doc.documentElement);
    }
    return d;
}

void CProjectPage::saveFile(const QString& path, QDomLiteDocument* Doc, QPixmap pix) {
    QFileInfo f(path);
    QuaZip zip(f.path() + "/" + f.baseName() + ".zip");
    if(!zip.open(QuaZip::mdCreate)) return;

    QByteArray b(Doc->toByteArray());
    if (!b.isEmpty())
    {
        QuaZipFile zipFile(&zip);
        if(zipFile.open(QIODevice::WriteOnly,QuaZipNewInfo("data.xml")))
        {
            zipFile.write(b);
            zipFile.close();
        }
    }
    //QPixmap p(this->grab());
    if (!pix.isNull())
    {
        QuaZipFile zipFile(&zip);
        if(zipFile.open(QIODevice::WriteOnly,QuaZipNewInfo("image.jpg")))
        {
            pix.save(&zipFile,"JPG");
            zipFile.close();
        }
    }
    zip.close();
    //return zip.getZipName();
}

void CProjectPage::setPath(const QString &path)
{
    m_Path = path;
    //fillView();
}

void CProjectPage::setDefultIcon(const QIcon &icon)
{
    m_Icon = icon;
    //fillView();
}

void CProjectPage::setExtension(const QString &ext)
{
    m_Ext = ext;
    //fillView();
}

void CProjectPage::setQuickStart(const bool qs)
{
    m_QuickStart = qs;
    //fillView();
}

void CProjectPage::setCustomProjects(const QStringList& l)
{
    m_CustomProjects = l;
}
