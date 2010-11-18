/*
 * This file is part of OpenModelica.
 *
 * Copyright (c) 1998-CurrentYear, Linkoping University,
 * Department of Computer and Information Science,
 * SE-58183 Linkoping, Sweden.
 *
 * All rights reserved.
 *
 * THIS PROGRAM IS PROVIDED UNDER THE TERMS OF GPL VERSION 3 
 * AND THIS OSMC PUBLIC LICENSE (OSMC-PL). 
 * ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS PROGRAM CONSTITUTES RECIPIENT'S  
 * ACCEPTANCE OF THE OSMC PUBLIC LICENSE.
 *
 * The OpenModelica software and the Open Source Modelica
 * Consortium (OSMC) Public License (OSMC-PL) are obtained
 * from Linkoping University, either from the above address,
 * from the URLs: http://www.ida.liu.se/projects/OpenModelica or  
 * http://www.openmodelica.org, and in the OpenModelica distribution. 
 * GNU version 3 is obtained from: http://www.gnu.org/copyleft/gpl.html.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without
 * even the implied warranty of  MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE, EXCEPT AS EXPRESSLY SET FORTH
 * IN THE BY RECIPIENT SELECTED SUBSIDIARY LICENSE CONDITIONS
 * OF OSMC-PL.
 *
 * See the full OSMC Public License conditions for more details.
 *
 * Main Authors 2010: Syed Adeel Asghar, Sonia Tariq
 *
 */

/*
 * HopsanGUI
 * Fluid and Mechatronic Systems, Department of Management and Engineering, Linkoping University
 * Main Authors 2009-2010:  Robert Braun, Bjorn Eriksson, Peter Nordin
 * Contributors 2009-2010:  Mikael Axin, Alessandro Dell'Amico, Karl Pettersson, Ingo Staack
 */

#ifndef PROJECTTABWIDGET_H
#define PROJECTTABWIDGET_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QTabWidget>
#include <map>

#include "Component.h"
#include "ConnectorWidget.h"
#include "StringHandler.h"
#include "ModelicaEditor.h"

class ProjectTab;
class Component;
class ComponentAnnotation;
class Connector;

class GraphicsScene : public QGraphicsScene
{
    Q_OBJECT
public:
    GraphicsScene(int iconType, ProjectTab *parent = 0);
    ProjectTab *mpParentProjectTab;
    int mIconType;
};

class GraphicsView : public QGraphicsView
{
    Q_OBJECT
private:
    Connector *mpConnector;
    int mIconType;
    void createActions();
    void createMenus();
public:
    GraphicsView(int iconType, ProjectTab *parent = 0);
    void addComponentObject(Component *icon);
    void deleteComponentObject(Component *icon);
    Component* getComponentObject(QString componentName);
    QString getUniqueComponentName(QString iconName, int number = 1);
    bool checkComponentName(QString iconName);

    QList<Component*> mComponentsList;
    bool mIsCreatingConnector;
    QVector<Connector*> mConnectorsVector;
    ProjectTab *mpParentProjectTab;
    QAction *mpCancelConnectionAction;
    QAction *mpRotateIconAction;
    QAction *mpRotateAntiIconAction;
    QAction *mpResetRotation;
    QAction *mpDeleteIconAction;
signals:
    void keyPressDelete();
    void keyPressUp();
    void keyPressDown();
    void keyPressLeft();
    void keyPressRight();
    void keyPressRotateClockwise();
    void keyPressRotateAntiClockwise();
public slots:
    void addConnector(Component *pComponent);
    void removeConnector();
    void removeConnector(Connector* pConnector);
    void resetZoom();
    void zoomIn();
    void zoomOut();
    void showGridLines(bool showLines);
    void selectAll();
    void saveModelAnnotation();
protected:
    virtual void dragMoveEvent(QDragMoveEvent *event);
    virtual void dropEvent(QDropEvent *event);
    virtual void drawBackground(QPainter *painter, const QRectF &rect);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);
    virtual void contextMenuEvent(QContextMenuEvent *event);
};

class GraphicsViewScroll : public QScrollArea
{
public:
    GraphicsViewScroll(GraphicsView *graphicsView, QWidget *parent = 0);

    GraphicsView *mpGraphicsView;
protected:
    virtual void scrollContentsBy(int dx, int dy);
};

class ProjectTabWidget; //Forward declaration
class ModelicaEditor;
class ProjectTab : public QWidget
{
    Q_OBJECT
private:
    ModelicaEditor *mpModelicaEditor;
    QStatusBar *mpProjectStatusBar;
    QButtonGroup *mpViewsButtonGroup;
    QToolButton *mpIconToolButton;
    QToolButton *mpDiagramToolButton;
    QToolButton *mpModelicaTextToolButton;
    QLabel *mpReadOnlyLabel;
    QLabel *mpModelicaTypeLabel;
    QLabel *mpViewTypeLabel;
    QLabel *mpModelFilePathLabel;
    bool mReadOnly;
public:
    ProjectTab(int modelicaType, int iconType, bool readOnly, ProjectTabWidget *parent = 0);
    ~ProjectTab();
    void updateTabName(QString name, QString nameStructure);
    void updateModel(QString name);
    bool loadModelFromText(QString name);
    bool loadRootModel(QString model);
    bool loadSubModel(QString model);
    void getModelComponents();
    void getModelConnections();
    void setReadOnly(bool readOnly);
    bool isReadOnly();
    void setModelFilePathLabel(QString filePath);

    ProjectTabWidget *mpParentProjectTabWidget;
    GraphicsView *mpGraphicsView;
    GraphicsScene *mpGraphicsScene;
    GraphicsView *mpDiagramGraphicsView;
    GraphicsScene *mpDiagramGraphicsScene;
    QString mModelFileName;
    QString mModelName;
    QString mModelNameStructure;
    int mModelicaType;
    int mIconType;
    bool mIsSaved;
    int mTabPosition;
signals:
    void disableMainWindow(bool disable);
    void updateAnnotations();
public slots:
    void hasChanged();
    void showDiagramView(bool checked);
    void showIconView(bool checked);
    void showModelicaTextView(bool checked);
    bool ModelicaEditorTextChanged();
    void updateModelAnnotations();
};

class MainWindow;
class LibraryTreeNode;
class ProjectTabWidget : public QTabWidget
{
    Q_OBJECT
public:
    ProjectTabWidget(MainWindow *parent = 0);
    ~ProjectTabWidget();
    ProjectTab* getCurrentTab();
    ProjectTab* getTabByName(QString name);
    ProjectTab* getRemovedTabByName(QString name);
    int addTab(ProjectTab *tab, QString tabName);
    void removeTab(int index);
    void disableTabs(bool disable);
    void setSourceFile(QString modelName, QString modelFileName);

    MainWindow *mpParentMainWindow;
    bool mShowLines;
    bool mToolBarEnabled;
    QList<ProjectTab*> mRemovedTabsList;
signals:
    void tabAdded();
    void tabRemoved();
public slots:
    void addProjectTab(ProjectTab *projectTab, QString modelName, QString modelStructure);
    void addNewProjectTab(QString modelName, QString modelStructure, int modelicaType);
    void addDiagramViewTab(QTreeWidgetItem *item, int column);
    void saveProjectTab();
    void saveProjectTabAs();
    void saveProjectTab(int index, bool saveAs);
    bool saveModel(bool saveAs);
    bool closeProjectTab(int index);
    bool closeAllProjectTabs();
    void openModel();
    void resetZoom();
    void zoomIn();
    void zoomOut();
    void updateTabIndexes();
    void enableProjectToolbar();
    void disableProjectToolbar();
};

#endif // PROJECTTABWIDGET_H
