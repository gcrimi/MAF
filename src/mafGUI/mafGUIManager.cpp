/*
 *  mafGUIManager.cpp
 *  mafGUI
 *
 *  Created by Paolo Quadrani on 26/10/10.
 *  Copyright 2010 B3C. All rights reserved.
 *
 *  See Licence at: http://tiny.cc/QXJ4D
 *
 */

#include "mafGUIManager.h"
#include "mafUILoaderQt.h"
#include "mafTreeWidget.h"
#include "mafTreeModel.h"
#include "mafLoggerWidget.h"
#include "mafTextEditWidget.h"
#include "mafTextHighlighter.h"
#include "mafGUIApplicationSettingsDialog.h"
#include "mafTreeItemDelegate.h"
#include "mafTreeItemSceneNodeDelegate.h"

#include <mafOperationWidget.h>

using namespace mafCore;
using namespace mafEventBus;
using namespace mafGUI;

mafGUIManager::mafGUIManager(QMainWindow *main_win, const QString code_location) : mafObjectBase(code_location)
    , m_NewAct(NULL), m_CollaborateAct(NULL)
    , m_OpenAct(NULL), m_SaveAct(NULL), m_SaveAsAct(NULL), m_RecentFilesSeparatorAct(NULL), m_ExitAct(NULL)
    , m_CutAct(NULL), m_CopyAct(NULL), m_PasteAct(NULL), m_AboutAct(NULL)
    , m_MaxRecentFiles(5), m_ActionsCreated(false), m_MainWindow(main_win)
    , m_Model(NULL), m_TreeWidget(NULL) {

    m_SettingsDialog = new mafGUIApplicationSettingsDialog();
    m_OperationWidget = new mafOperationWidget();
    connect(m_OperationWidget, SIGNAL(operationDismissed()), this, SLOT(removeOperationGUI()));

    m_Logger = new mafLoggerWidget(mafCodeLocation);

    mafCore::mafMessageHandler::instance()->setActiveLogger(m_Logger);

    mafRegisterLocalCallback("maf.local.resources.plugin.registerLibrary", this, "fillMenuWithPluggedObjects(mafCore::mafPluggedObjectsHash)")
    mafRegisterLocalCallback("maf.local.resources.vme.select", this, "updateMenuForSelectedVme(mafCore::mafObjectBase *)")

    m_UILoader = mafNEW(mafGUI::mafUILoaderQt);
    connect(m_UILoader, SIGNAL(uiLoadedSignal(mafCore::mafContainerInterface*)), this, SLOT(uiLoaded(mafCore::mafContainerInterface*)));
}

mafGUIManager::~mafGUIManager() {
    mafDEL(m_Logger);
    mafDEL(m_UILoader);
}

void mafGUIManager::parseMenuAttributes(QDomNode current) {
    if (current.nodeType() == QDomNode::ElementNode) {
        QDomElement ce = current.toElement();
        qDebug() << ce.tagName();
        QDomNamedNodeMap attributes = ce.attributes();
        QDomNode att;
        for (int i = 0; i < attributes.size(); ++i) {
            att = attributes.item(i);
            qDebug() << att.nodeName() << " = " << att.nodeValue();
        }
    }
}

QDomNode mafGUIManager::parseMenuTree(QDomNode current) {
    // Get the menu items...
    QDomNodeList dnl = current.childNodes();
    for (int n=0; n < dnl.count(); ++n) {
        QDomNode node = dnl.item(n);
        parseMenuAttributes(node);
        // Get the inner actions.
        parseMenuTree(node);
    }
    
    return current;
}

void mafGUIManager::createActions() {
    m_NewAct = new QAction(QIcon(":/images/new.png"), mafTr("&New"), this);
    m_NewAct->setIconText(mafTr("New"));
    m_NewAct->setObjectName("New");
    m_NewAct->setShortcuts(QKeySequence::New);
    m_NewAct->setStatusTip(mafTr("Create a new file"));
    m_MenuItemList.append(m_NewAct);

    m_CollaborateAct = new QAction(QIcon(":/images/collaborate.png"), mafTr("Collaborate"), this);
    m_CollaborateAct->setObjectName("Collaborate");
    m_CollaborateAct->setCheckable(true);
    m_CollaborateAct->setChecked(false);
    m_CollaborateAct->setIconText(mafTr("Collaborate"));
    m_CollaborateAct->setStatusTip(mafTr("Collaborate with your conmtacts in gtalk."));
    m_MenuItemList.append(m_CollaborateAct);

    m_OpenAct = new QAction(QIcon(":/images/open.png"), mafTr("&Open..."), this);
    m_OpenAct->setObjectName("Open");
    m_OpenAct->setIconText(mafTr("Open"));
    m_OpenAct->setShortcuts(QKeySequence::Open);
    m_OpenAct->setStatusTip(mafTr("Open an existing file"));
    m_MenuItemList.append(m_OpenAct);

    m_SaveAct = new QAction(QIcon(":/images/save.png"), mafTr("&Save"), this);
    m_SaveAct->setObjectName("Save");
    m_SaveAct->setIconText(mafTr("Save"));
    m_SaveAct->setShortcuts(QKeySequence::Save);
    m_SaveAct->setStatusTip(mafTr("Save the document to disk"));
    m_MenuItemList.append(m_SaveAct);

    m_SaveAsAct = new QAction(mafTr("Save &As..."), this);
    m_SaveAsAct->setObjectName("SaveAs");
    m_SaveAsAct->setIconText(mafTr("Save As"));
    m_SaveAsAct->setShortcuts(QKeySequence::SaveAs);
    m_SaveAsAct->setStatusTip(mafTr("Save the document under a new name"));
    m_MenuItemList.append(m_SaveAsAct);

    m_RecentFileActs.clear();
    for (int i = 0; i < m_MaxRecentFiles; ++i) {
        QAction *recentFileAction = new QAction(this);
        m_MenuItemList.append(recentFileAction);
        recentFileAction->setVisible(false);
        connect(recentFileAction, SIGNAL(triggered()), this, SLOT(openRecentFile()));
        m_RecentFileActs.append(recentFileAction);
    }

    m_ExitAct = new QAction(mafTr("E&xit"), this);
    m_ExitAct->setObjectName("Exit");
    m_ExitAct->setIconText(mafTr("Exit"));
    m_ExitAct->setShortcuts(QKeySequence::Quit);
    m_ExitAct->setStatusTip(mafTr("Exit the application"));
    m_MenuItemList.append(m_ExitAct);
    connect(m_ExitAct, SIGNAL(triggered()), m_MainWindow, SLOT(close()));

    m_CutAct = new QAction(QIcon(":/images/cut.png"), mafTr("Cu&t"), this);
    m_CutAct->setObjectName("Cut");
    m_CutAct->setIconText(mafTr("Cut"));
    m_CutAct->setShortcuts(QKeySequence::Cut);
    m_CutAct->setStatusTip(mafTr("Cut the current selection's contents to the "
                            "clipboard"));
    m_MenuItemList.append(m_CutAct);

    m_CopyAct = new QAction(QIcon(":/images/copy.png"), mafTr("&Copy"), this);
    m_CopyAct->setObjectName("Copy");
    m_CopyAct->setIconText(mafTr("Copy"));
    m_CopyAct->setShortcuts(QKeySequence::Copy);
    m_CopyAct->setStatusTip(mafTr("Copy the current selection's contents to the "
                             "clipboard"));
    m_MenuItemList.append(m_CopyAct);

    m_PasteAct = new QAction(QIcon(":/images/paste.png"), tr("&Paste"), this);
    m_PasteAct->setObjectName("Paste");
    m_PasteAct->setIconText(mafTr("Paste"));
    m_PasteAct->setShortcuts(QKeySequence::Paste);
    m_PasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                              "selection"));
    m_MenuItemList.append(m_PasteAct);

    m_AboutAct = new QAction(tr("&About"), this);
    m_AboutAct->setObjectName("About");
    m_AboutAct->setIconText(mafTr("About"));
    m_AboutAct->setStatusTip(tr("Show the application's About box"));
    m_MenuItemList.append(m_AboutAct);

    m_SettingsAction = new QAction(mafTr("Settings"), this);
    m_SettingsAction->setObjectName("Settings");
    m_SettingsAction->setIconText(mafTr("Settings"));
    m_SettingsAction->setStatusTip(tr("Show the application's Settings dialog"));
    m_MenuItemList.append(m_SettingsAction);
    connect(m_SettingsAction, SIGNAL(triggered()), SLOT(showSettingsDialog()));

    m_SideBarAct = new QAction(tr("Sidebar"), this);
    m_SideBarAct->setObjectName("SideBar");
    m_SideBarAct->setCheckable(true);
    m_SideBarAct->setChecked(true);
    m_MenuItemList.append(m_SideBarAct);

    m_LogBarAct = new QAction(tr("LogBar"), this);
    m_LogBarAct->setObjectName("LogBar");
    m_LogBarAct->setCheckable(true);
    m_LogBarAct->setChecked(true);
    m_MenuItemList.append(m_LogBarAct);

    m_ActionsCreated = true;

    registerEvents();
}

void mafGUIManager::showSettingsDialog() {
    m_SettingsDialog->show();
}

void mafGUIManager::fillMenuWithPluggedObjects(mafCore::mafPluggedObjectsHash pluginHash) {
    mafCore::mafObjectBase *sel_vme;
    QGenericReturnArgument ret_val = mafEventReturnArgument(mafCore::mafObjectBase *, sel_vme);
    mafEventBusManager::instance()->notifyEvent("maf.local.resources.vme.selected", mafEventTypeLocal, NULL, &ret_val);

    if(!m_ActionsCreated) {
        // Actions has not been created, so neither the menu.
        // Ask to create it which will crete also the actions.
        createMenus();
    }
    QString base_class("");
    mafPluggedObjectInformation objInfo;
    mafPluggedObjectsHash::iterator iter = pluginHash.begin();
    while(iter != pluginHash.end()) {
        objInfo = iter.value();
        base_class = iter.key();
        if(base_class == "mafResources::mafOperation" || base_class == "mafResources::mafImporter" || 
           base_class == "mafResources::mafExporter" || base_class == "mafResources::mafView") {
            QAction *action = new QAction(mafTr(objInfo.m_Label.toAscii()), NULL);
            QVariant data_type(objInfo.m_ClassType);
            action->setData(data_type);
            QMenu *menu;
            char *bc = base_class.toAscii().data();
            if(base_class == "mafResources::mafOperation") {
                // Add a new item to the operation's menu.
                menu = (QMenu *)this->menuItemByName("Operations");
                menu->addAction(action);
                connect(action, SIGNAL(triggered()), this, SLOT(startOperation()));
            } else if(base_class == "mafResources::mafImporter") {
                // Add a new item to the importer's menu.
                menu = (QMenu *)this->menuItemByName("Import");
                menu->addAction(action);
                connect(action, SIGNAL(triggered()), this, SLOT(startOperation()));
            } else if(base_class == "mafResources::mafExporter") {
                // Add a new item to the exporter's menu.
                menu = (QMenu *)this->menuItemByName("Export");
                menu->addAction(action);
                connect(action, SIGNAL(triggered()), this, SLOT(startOperation()));
            } else if(base_class == "mafResources::mafView") {
                // Add a new item to the view's menu.
                menu = (QMenu *)this->menuItemByName("Views");
                menu->addAction(action);
                connect(action, SIGNAL(triggered()), this, SLOT(createView()));
            }
        }
        ++iter;
    }

    this->updateMenuForSelectedVme(sel_vme);
}

QObject *mafGUIManager::menuItemByName(QString name) {
    if(!m_ActionsCreated || name.length() == 0) {
        return NULL;
    }
    for (int i = 0; i < m_MenuItemList.size(); ++i) {
        QObject *action = m_MenuItemList.at(i);
        QString an = action->objectName();
        if (an == name) {
            return action;
        }
    }
    return NULL;
}

void mafGUIManager::updateMenuForSelectedVme(mafCore::mafObjectBase *vme) {
    QStringList accepted_list;
    accepted_list = mafCoreRegistration::acceptObject(vme);

    QMenu *opMenu = (QMenu *)this->menuItemByName("Operations");
    QList<QAction *> opActions = opMenu->actions();
    QString op;
    foreach(QAction *action, opActions) {
        op = action->data().toString();
        action->setEnabled(accepted_list.contains(op));
    }
}

void mafGUIManager::registerEvents() {
    mafIdProvider *provider = mafIdProvider::instance();
    provider->createNewId("maf.local.gui.action.new");
    provider->createNewId("maf.local.gui.action.open");
    provider->createNewId("maf.local.gui.action.save");
    provider->createNewId("maf.local.gui.action.saveas");
    provider->createNewId("maf.local.gui.action.cut");
    provider->createNewId("maf.local.gui.action.copy");
    provider->createNewId("maf.local.gui.action.paste");
    provider->createNewId("maf.local.gui.action.about");
    provider->createNewId("maf.local.gui.pathSelected");

    // Register API signals.
    mafRegisterLocalSignal("maf.local.gui.action.new", m_NewAct, "triggered()");
    mafRegisterLocalSignal("maf.local.gui.action.open", m_OpenAct, "triggered()");
    mafRegisterLocalSignal("maf.local.gui.action.save", m_SaveAct, "triggered()");
    mafRegisterLocalSignal("maf.local.gui.action.saveas", m_SaveAsAct, "triggered()");
    mafRegisterLocalSignal("maf.local.gui.action.cut", m_CutAct, "triggered()");
    mafRegisterLocalSignal("maf.local.gui.action.copy", m_CopyAct, "triggered()");
    mafRegisterLocalSignal("maf.local.gui.action.paste", m_PasteAct, "triggered()");
    mafRegisterLocalSignal("maf.local.gui.action.about", m_AboutAct, "triggered()");
    mafRegisterLocalSignal("maf.local.gui.pathSelected", this, "pathSelected(const QString)");

    // OperationManager's callback
    mafRegisterLocalCallback("maf.local.resources.operation.started", this, "operationDidStart(mafCore::mafObjectBase *)");

    // ViewManager's callback.
    mafRegisterLocalCallback("maf.local.resources.view.selected", this, "viewSelected(mafCore::mafObjectBase *)");
    mafRegisterLocalCallback("maf.local.resources.view.noneViews", this, "viewDestroyed()");
}

void mafGUIManager::createDefaultMenus() {
    if(!m_ActionsCreated) {
        createActions();
    }

    QMenuBar *menuBar = m_MainWindow->menuBar();
    
    QMenu *importMenu = new QMenu(tr("Import"));
    importMenu->setObjectName("Import");
    QMenu *exportMenu = new QMenu(tr("Export"));
    exportMenu->setObjectName("Export");
    
    QMenu *fileMenu = menuBar->addMenu(tr("File"));
    fileMenu->setObjectName("File");
    fileMenu->addAction(m_NewAct);
    fileMenu->addSeparator();
    fileMenu->addAction(m_OpenAct);
    fileMenu->addAction(m_CollaborateAct);
    fileMenu->addSeparator();
    fileMenu->addMenu(importMenu);
    fileMenu->addMenu(exportMenu);
    fileMenu->addSeparator();
    fileMenu->addAction(m_SaveAct);
    fileMenu->addAction(m_SaveAsAct);
    
    m_RecentFilesSeparatorAct = fileMenu->addSeparator();
    for (int i = 0; i < m_MaxRecentFiles; ++i) {
        fileMenu->addAction(m_RecentFileActs.at(i));
    }
    
    fileMenu->addSeparator();
    fileMenu->addAction(m_ExitAct);
    
    m_MenuItemList.append(fileMenu);
    
    menuBar->addSeparator();
    
    QMenu *editMenu = menuBar->addMenu(tr("Edit"));
    editMenu->setObjectName("Edit");
    editMenu->addAction(m_CutAct);
    editMenu->addAction(m_CopyAct);
    editMenu->addAction(m_PasteAct);
    m_MenuItemList.append(editMenu);
    
    menuBar->addSeparator();
    
    QMenu *viewMenu = menuBar->addMenu(tr("Views"));
    viewMenu->setObjectName("Views");
    m_MenuItemList.append(viewMenu);
    
    menuBar->addSeparator();
    
    QMenu *opMenu = menuBar->addMenu(tr("Operations"));
    opMenu->setObjectName("Operations");
    m_MenuItemList.append(opMenu);
    
    menuBar->addSeparator();
    
    QMenu *windowMenu = menuBar->addMenu(tr("Window"));
    windowMenu->setObjectName("Window");
    windowMenu->addAction(m_SideBarAct);
    windowMenu->addAction(m_LogBarAct);
    m_MenuItemList.append(windowMenu);
    
    menuBar->addSeparator();
    
    QMenu *helpMenu = menuBar->addMenu(tr("Help"));
    helpMenu->setObjectName("Help");
    helpMenu->addAction(m_AboutAct);
    m_MenuItemList.append(helpMenu);
    
    QMenu *optionsMenu = menuBar->addMenu(tr("Options"));
    optionsMenu->setObjectName("Options");
    optionsMenu->addAction(m_SettingsAction);
    m_MenuItemList.append(optionsMenu);
}

void mafGUIManager::createMenus() {
    int errorLine, errorColumn;
    QString errorMsg;
    QFile modelFile("Menu.mnu");
    if (!modelFile.exists()) {
        qWarning() << "Menu.mnu " << tr("doesn't exists. The default menu will be created");
        createDefaultMenus();
        return;
    }

    QDomDocument document;
    if (!document.setContent(&modelFile, &errorMsg, &errorLine, &errorColumn)) {
        QString error(tr("Syntax error line %1, column %2:\n%3"));
        error = error
        .arg(errorLine)
        .arg(errorColumn)
        .arg(errorMsg);
        qCritical() << error;
        return;
    }
    
    QDomNode m_CurrentNode = document.firstChild();
    parseMenuTree(m_CurrentNode);
}

void mafGUIManager::createToolBars() {
    if(!m_ActionsCreated) {
        createActions();
    }
    m_FileToolBar = m_MainWindow->addToolBar(tr("File"));
    m_FileToolBar->addAction(m_NewAct);
    m_FileToolBar->addAction(m_CollaborateAct);
    m_FileToolBar->addAction(m_OpenAct);
    m_FileToolBar->addAction(m_SaveAct);
    m_FileToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    m_EditToolBar = m_MainWindow->addToolBar(tr("Edit"));
    m_EditToolBar->addAction(m_CutAct);
    m_EditToolBar->addAction(m_CopyAct);
    m_EditToolBar->addAction(m_PasteAct);
    m_EditToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
}

void mafGUIManager::startOperation() {
    QAction *opAction = (QAction *)QObject::sender();
    m_OperationWidget->setOperationName(opAction->text());

    QString op(opAction->data().toString());
    mafEventArgumentsList argList;
    argList.append(mafEventArgument(QString, op));
    mafEventBusManager::instance()->notifyEvent("maf.local.resources.operation.start", mafEventTypeLocal, &argList);
}

void mafGUIManager::operationDidStart(mafCore::mafObjectBase *operation) {
    QMenu *opMenu = (QMenu *)this->menuItemByName("Operations");
    opMenu->setEnabled(false);

    // Get the started operation
    QString guiFilename = operation->uiFilename();
    m_OperationWidget->setOperation(operation);
    operation->setObjectName(m_OperationWidget->operationName());

    if(guiFilename.isEmpty()) {
        return;
    }
    m_GUILoadedType = mafGUILoadedTypeOperation;

    // Ask the UI Loader to load the operation's GUI.
    m_UILoader->uiLoad(guiFilename);
}

void mafGUIManager::removeOperationGUI() {
    QMenu *opMenu = (QMenu *)this->menuItemByName("Operations");
    opMenu->setEnabled(true);
    m_GUILoadedType = mafGUILoadedTypeOperation;
    emit guiTypeToRemove(m_GUILoadedType);
}

mafTreeWidget *mafGUIManager::createTreeWidget(mafTreeModel *model, QWidget *parent) {
//    QSettings settings;
    m_Model = model;
    m_TreeWidget = new mafTreeWidget();
    mafTreeItemDelegate *itemDelegate = new mafTreeItemDelegate(this);
    m_TreeWidget->setAnimated(true);
    m_TreeWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_TreeWidget->setMinimumSize(200, 0);
    m_TreeWidget->setMaximumSize(16777215, 16777215);
    connect(m_TreeWidget, SIGNAL(clicked(QModelIndex)), this, SLOT(selectVME(QModelIndex)));
    connect(m_Model, SIGNAL(itemAdded(QModelIndex)), m_TreeWidget, SLOT(expand(QModelIndex)));

    if(parent) {
        if(parent->layout()) {
            parent->layout()->addWidget(m_TreeWidget);
        } else {
            m_TreeWidget->setParent(parent);
        }
    }
    m_TreeWidget->setModel( m_Model );
    m_TreeWidget->setItemDelegate(itemDelegate);
    return m_TreeWidget;
}

mafTextEditWidget *mafGUIManager::createLogWidget(QWidget *parent) {

    mafTextEditWidget *w = m_Logger->textWidgetLog();
    w->setParent(parent);
    w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    w->enableEditing(false);

    if(parent) {
        if(parent->layout()) {
            parent->layout()->addWidget(w);
        } else {
            w->setParent(parent);
        }
    }

    return w;
}


void mafGUIManager::uiLoaded(mafCore::mafContainerInterface *guiWidget) {
    // Get the widget from the container
    mafContainer<QWidget> *w = mafContainerPointerTypeCast(QWidget, guiWidget);
    QWidget *widget = *w;

    switch(m_GUILoadedType) {
        case mafGUILoadedTypeOperation:
            m_OperationWidget->setOperationGUI(widget);
        break;
        case mafGUILoadedTypeView:
        break;
        case mafGUILoadedTypeVisualPipe:
        break;
        case mafGUILoadedTypeVme:
        break;
        default:
            qWarning() << mafTr("type %1 not recognized...").arg(m_GUILoadedType);
            return;
        break;
    }
    emit guiLoaded(m_GUILoadedType, m_OperationWidget);
}

void mafGUIManager::createView() {
    QAction *viewAction = (QAction *)QObject::sender();
    QString view(viewAction->data().toString());
    mafEventArgumentsList argList;
    argList.append(mafEventArgument(QString, view));
    mafEventBusManager::instance()->notifyEvent("maf.local.resources.view.create", mafEventTypeLocal, &argList);
}

void mafGUIManager::viewSelected(mafCore::mafObjectBase *view) {
    REQUIRE(view != NULL);
    // Set current hierarchy
    mafHierarchyPointer sceneGraph;
    sceneGraph = view->property("hierarchy").value<mafCore::mafHierarchyPointer>();
    if (m_Model) {
        // Set hierarchy of selected view and set the current index
        m_Model->clear();
        m_Model->setHierarchy(sceneGraph);
        QModelIndex index = m_Model->index(0, 0);
        // TODO: select previous index
        m_TreeWidget->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select);
        mafTreeItemSceneNodeDelegate *itemSceneNodeDelegate = new mafTreeItemSceneNodeDelegate(this);
        m_TreeWidget->setItemDelegate(itemSceneNodeDelegate);

    }
    // Get the selected view's UI file
    QString guiFilename = view->uiFilename();
    if(guiFilename.isEmpty()) {
        return;
    }
    // Set the current panel to the parent panel of view properties.
    m_GUILoadedType = mafGUILoadedTypeView;
    // Ask the UI Loader to load the view's GUI.
    m_UILoader->uiLoad(guiFilename);
}

void mafGUIManager::viewDestroyed() {
    // Get hierarchy from mafVMEManager
    mafCore::mafHierarchyPointer hierarchy;
    QGenericReturnArgument ret_val = mafEventReturnArgument(mafCore::mafHierarchyPointer, hierarchy);
    mafEventBusManager::instance()->notifyEvent("maf.local.resources.hierarchy.request", mafEventTypeLocal, NULL, &ret_val);
    if (m_Model) {
        // Set hierarchy of selected view and set the current index
        m_Model->clear();
        m_Model->setHierarchy(hierarchy);
        QModelIndex index = m_Model->index(0, 0);
        // TODO: select previous index
        m_TreeWidget->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select);
        mafTreeItemDelegate *itemDelegate = new mafTreeItemDelegate(this);
        m_TreeWidget->setItemDelegate(itemDelegate);
    }
}

void mafGUIManager::selectVME(QModelIndex index) {
    QTreeView *tree = (QTreeView *)QObject::sender();
    //m_Model = (mafTreeModel *)tree->model();
    mafTreeItem *item = (mafTreeItem *)m_Model->itemFromIndex(index);
    QObject *obj = item->data();
    QVariant sel(true);
    obj->setProperty("selected", sel);

    // Notify the item selection.
    mafEventArgumentsList argList;
    argList.append(mafEventArgument(mafCore::mafObjectBase*, qobject_cast<mafCore::mafObjectBase *>(obj)));
    mafEventBusManager::instance()->notifyEvent("maf.local.resources.vme.select", mafEventTypeLocal, &argList);
}

void mafGUIManager::chooseFileDialog(const QString title, const QString start_dir, const QString wildcard) {
    QString fileName = QFileDialog::getOpenFileName(m_MainWindow, title, start_dir, wildcard);
}

void mafGUIManager::openRecentFile() {
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        QString file_to_open(action->data().toString());
        mafEventArgumentsList argList;
        argList.append(mafEventArgument(QString, file_to_open));
        mafEventBusManager::instance()->notifyEvent("maf.local.logic.openFile", mafEventTypeLocal, &argList);
    }
}

void mafGUIManager::updateRecentFileActions() {
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();

    int numRecentFiles = qMin(files.size(), (int)m_MaxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));
        m_RecentFileActs.at(i)->setText(text);
        m_RecentFileActs.at(i)->setData(files[i]);
        m_RecentFileActs.at(i)->setVisible(true);
    }
    for (int j = numRecentFiles;  j < m_MaxRecentFiles; ++j)
        m_RecentFileActs.at(j)->setVisible(false);

    m_RecentFilesSeparatorAct->setVisible(numRecentFiles > 0);
}

QString mafGUIManager::strippedName(const QString &fullFileName) {
    return QFileInfo(fullFileName).fileName();
}