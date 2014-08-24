#include <Python.h>

#include <QMessageBox>
#include <QFileDialog>
#include <QThread>
#include <QActionGroup>

#include <cmath>

#include "app.h"

#include "ui_main_window.h"
#include "ui/main_window.h"
#include "ui/resolution_dialog.h"
#include "ui/exporting_dialog.h"

#include "ui/canvas.h"
#include "ui/colors.h"

#include "node/manager.h"
#include "cpp/shape.h"
#include "render/export_mesh.h"
#include "render/export_json.h"

App::App(int& argc, char** argv) :
    QApplication(argc, argv), window(new MainWindow)
{
    setShortcuts();
    setGlobalStyle();
    connectActions();
    window->show();
}

App::~App()
{
    NodeManager::manager()->clear();
    delete window;
}

App* App::instance()
{
    return dynamic_cast<App*>(QApplication::instance());
}

Canvas* App::getCanvas() const
{
    return window->canvas;
}

MainWindow* App::getWindow() const
{
    return window;
}

void App::onAbout()
{
    QMessageBox::about(NULL, "antimony",
            "<b>Antimony</b><br><br>"
            "CAD from a parallel universe.<br>"
            "<a href=\"https://github.com/mkeeter/antimony\">https://github.com/mkeeter/antimony</a><br><br>"
            "© 2013-2014 Matthew Keeter<br>"
            "<a href=\"mailto:matt.j.keeter@gmail.com\">matt.j.keeter@gmail.com</a><br>"
            "_________________________________________________<br><br>"
            "Includes code from <a href=\"https://github.com/mkeeter/kokopelli\">kokopelli</a>, which is <br>"
            "© 2012-2013 MIT<br>"
            "© 2013-2014 Matthew Keeter<br><br>"
            "Inspired by the <a href=\"http://kokompe.cba.mit.edu\">fab modules</a>"
            );
}

void App::onNew()
{
    NodeManager::manager()->clear();
}

void App::onSave()
{
    if (filename.isEmpty())
    {
        return onSaveAs();
    }

    QFile file(filename);
    file.open(QIODevice::WriteOnly);
    file.write(NodeManager::manager()->getSerializedScene());
}

void App::onSaveAs()
{
    QString f = QFileDialog::getSaveFileName(window, "Save as", "", "*.sb");
    if (!f.isEmpty())
    {
        filename = f;
        return onSave();
    }
}

void App::onOpen()
{
    QString f = QFileDialog::getOpenFileName(window, "Open", "", "*.sb");
    if (!f.isEmpty())
    {
        filename = f;
        NodeManager::manager()->clear();
        QFile file(f);
        file.open(QIODevice::ReadOnly);
        NodeManager::manager()->deserializeScene(file.readAll());
        NodeManager::manager()->makeControls(window->canvas);
        NodeManager::manager()->makeConnections(window->canvas);
    }
}

void App::onExportSTL()
{
    Shape s = NodeManager::manager()->getCombinedShape();
    if (!s.tree)
    {
        QMessageBox::critical(window, "Export error",
                "<b>Export error:</b><br>"
                "Cannot export without any shapes in the scene.");
        return;
    }
    if (isinf(s.bounds.xmin) || isinf(s.bounds.xmax) ||
        isinf(s.bounds.ymin) || isinf(s.bounds.ymax) ||
        isinf(s.bounds.zmin) || isinf(s.bounds.zmax))
    {
        QMessageBox::critical(window, "Export error",
                "<b>Export error:</b><br>"
                "Some shapes do not have 3D bounds;<br>"
                "cannot export mesh.");
        return;

    }

    ResolutionDialog* resolution_dialog = new ResolutionDialog(&s);
    if (!resolution_dialog->exec())
    {
        return;
    }

    QString file_name = QFileDialog::getSaveFileName(
            window, "Export STL", "", "*.stl");
    if (file_name.isEmpty())
    {
        return;
    }

    ExportingDialog* exporting_dialog = new ExportingDialog(window);

    QThread* thread = new QThread();
    ExportMeshWorker* worker = new ExportMeshWorker(
            s, resolution_dialog->getResolution(),
            file_name);
    worker->moveToThread(thread);

    connect(thread, SIGNAL(started()),
            worker, SLOT(render()));
    connect(worker, SIGNAL(finished()),
            thread, SLOT(quit()));
    connect(thread, SIGNAL(finished()),
            thread, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()),
            worker, SLOT(deleteLater()));
    connect(thread, SIGNAL(destroyed()),
            exporting_dialog, SLOT(accept()));

    thread->start();
    exporting_dialog->exec();
}

void App::onExportJSON()
{
    QMap<QString, Shape> s = NodeManager::manager()->getShapes();
    if (s.isEmpty())
    {
        QMessageBox::critical(window, "Export error",
                "<b>Export error:</b><br>"
                "Cannot export without any shapes in the scene.");
        return;
    }

    QString file_name = QFileDialog::getSaveFileName(
            window, "Export JSON", "", "*.json");
    if (file_name.isEmpty())
    {
        return;
    }

    ExportingDialog* exporting_dialog = new ExportingDialog(window);

    QThread* thread = new QThread();
    ExportJSONWorker* worker = new ExportJSONWorker(s, file_name);
    worker->moveToThread(thread);

    connect(thread, SIGNAL(started()),
            worker, SLOT(run()));
    connect(worker, SIGNAL(finished()),
            thread, SLOT(quit()));
    connect(thread, SIGNAL(finished()),
            thread, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()),
            worker, SLOT(deleteLater()));
    connect(thread, SIGNAL(destroyed()),
            exporting_dialog, SLOT(accept()));

    thread->start();
    exporting_dialog->exec();
}



void App::setShortcuts()
{
    window->ui->actionNew->setShortcuts(QKeySequence::New);
    window->ui->actionOpen->setShortcuts(QKeySequence::Open);
    window->ui->actionSave->setShortcuts(QKeySequence::Save);
    window->ui->actionSaveAs->setShortcuts(QKeySequence::SaveAs);
    window->ui->actionQuit->setShortcuts(QKeySequence::Quit);
}

void App::connectActions()
{
    connect(window->ui->actionQuit, SIGNAL(triggered()),
            this, SLOT(quit()));
    connect(window->ui->actionAbout, SIGNAL(triggered()),
            this, SLOT(onAbout()));
    connect(window->ui->actionSave, SIGNAL(triggered()),
            this, SLOT(onSave()));
    connect(window->ui->actionSaveAs, SIGNAL(triggered()),
            this, SLOT(onSaveAs()));
    connect(window->ui->actionNew, SIGNAL(triggered()),
            this, SLOT(onNew()));
    connect(window->ui->actionOpen, SIGNAL(triggered()),
            this, SLOT(onOpen()));
    connect(window->ui->actionExportMesh, SIGNAL(triggered()),
            this, SLOT(onExportSTL()));
    connect(window->ui->actionExportJSON, SIGNAL(triggered()),
            this, SLOT(onExportJSON()));

    auto painter_modes = new QActionGroup(this);
    painter_modes->addAction(window->ui->actionRaster);
    painter_modes->addAction(window->ui->actionOpenGL);
    painter_modes->setExclusive(true);

    window->ui->actionRaster->setChecked(!window->canvas->usingGL());
    window->ui->actionOpenGL->setChecked(window->canvas->usingGL());

    connect(window->ui->actionRaster, SIGNAL(triggered()),
            window->canvas, SLOT(useRaster()));
    connect(window->ui->actionOpenGL, SIGNAL(triggered()),
            window->canvas, SLOT(useGL()));
}

void App::setGlobalStyle()
{
    setStyleSheet(QString(
            "QToolTip {"
            "   color: %1;"
            "   background-color: %2;"
            "   border: 1px solid %1;"
            "   font-family: Courier"
            "}").arg(Colors::base03.name())
                .arg(Colors::base04.name()));
}
