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

//! @file   OMCProxy.cpp
//! @author Sonia Tariq <sonta273@student.liu.se>
//! @date   2010-06-25

//! @brief Contains functions used for communication with Open Modelica Compiler.

#include <stdexcept>
#include <stdlib.h>

#include "OMCProxy.h"
#include "OMCThread.h"
#include "StringHandler.h"
#include <omniORB4/CORBA.h>

//! @class OMCProxy
//! @brief The OMCProxy is a singleton class. It contains the reference of the CORBA object used to communicate with the Open Modelica Compiler.

//! Constructor
//! @param mOMC is the CORBA object.
//! @param mHasInitialized is the boolean variable used for checking server intialization.
//! @param mIsStandardLibraryLoaded is the boolean variable used for checking OM Standard Library intialization.
//! @param mResult contains the result obtained from OMC.
OMCProxy::OMCProxy(MainWindow *pParent, bool displayErrors)
    : mOMC(0), mHasInitialized(false), mIsStandardLibraryLoaded(false),
      mName(Helper::omcServerName) ,mResult(""), mDisplayErrors(displayErrors)
{
    this->mpParentMainWindow = pParent;
    this->mAnnotationVersion = OMCProxy::ANNOTATION_VERSION3X;
    this->mpOMCLogger = new QDialog();
    this->mpOMCLogger->setWindowFlags(Qt::WindowTitleHint);
    this->mpOMCLogger->setMinimumSize(640, 480);
    this->mpOMCLogger->setWindowIcon(QIcon(":/Resources/icons/console.png"));
    this->mpOMCLogger->setWindowTitle(QString(Helper::applicationName).append(" - OMC Messages Log"));
    // Set the QTextEdit Box
    this->mpTextEdit = new QTextEdit();
    this->mpTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    this->mpTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    this->mpTextEdit->setReadOnly(true);
    // Set the Layout
    QHBoxLayout *horizontallayout = new QHBoxLayout;
    horizontallayout->setContentsMargins(0, 0, 0, 0);
    mpExpressionTextBox = new QLineEdit;
    mpExpressionTextBox->installEventFilter(this);
    mpSendButton = new QPushButton("Send");
    connect(mpSendButton, SIGNAL(pressed()), SLOT(sendCustomExpression()));
    horizontallayout->addWidget(mpExpressionTextBox);
    horizontallayout->addWidget(mpSendButton);
    QVBoxLayout *verticalallayout = new QVBoxLayout;
    verticalallayout->addWidget(this->mpTextEdit);
    verticalallayout->addLayout(horizontallayout);
    mpOMCLogger->setLayout(verticalallayout);
    //start the server
    if(!startServer())      // if we are unable to start OMC. Exit the application.
    {
        mpParentMainWindow->mExitApplication = true;
        return;
    }
}

//! Destructor
OMCProxy::~OMCProxy()
{
    delete mpOMCLogger;
}

bool OMCProxy::eventFilter(QObject *object, QEvent *event)
{
    if (mpExpressionTextBox->hasFocus())
    {
        if (event->type() == QEvent::KeyPress)
        {
            if (dynamic_cast<QKeyEvent*>(event)->key() == Qt::Key_Up)
                getPreviousCommand();
            else if (dynamic_cast<QKeyEvent*>(event)->key() == Qt::Key_Down)
                getNextCommand();
            else
                return QObject::eventFilter(object, event);
            return true;
        }
    }
    else
    {
         // standard event processing
         return QObject::eventFilter(object, event);
     }

}

void OMCProxy::getPreviousCommand()
{
    mpExpressionTextBox->setText(mCommandsList.at(mCommandsList.count() - 1));
    QString tempCommand = mCommandsList.at(mCommandsList.count() - 1);
    mCommandsList.insert(0, tempCommand);
    mCommandsList.removeLast();
}

void OMCProxy::getNextCommand()
{
    mpExpressionTextBox->setText(mCommandsList.at(0));
    QString tempCommand = mCommandsList.at(0);
    mCommandsList.append(tempCommand);
    mCommandsList.removeFirst();
}

//! Starts the Open Modelica Compiler.
bool OMCProxy::startServer()
{
    try
    {
        QString msg;
        QString omHome (Helper::OpenModelicaHome);
        if (omHome.isEmpty())
            throw std::runtime_error(GUIMessages::getMessage(GUIMessages::OPEN_MODELICA_HOME_NOT_FOUND).toStdString());

        QDir dir;

        #ifdef WIN32
            if( dir.exists( omHome + "\\bin\\omc.exe" ) )
                    omHome += "\\bin\\";
            else if( dir.exists( omHome + "\\omc.exe" ) )
                    omHome += "";
            else
            {
                msg = "Unable to find " + Helper::applicationName + " server, searched in:\n" +
                        omHome + "\\bin\\\n" +
                        omHome + "\n" +
                        dir.absolutePath();

                throw std::runtime_error(msg.toStdString().c_str());
            }
        #else /* unix */
            if( dir.exists( omHome + "/bin/omc" ) )
                    omHome += "/bin/";
            else if( dir.exists( omHome + "/omc" ) )
                    omHome += "";
            else
            {
                msg = "Unable to find " + Helper::applicationName + " server, searched in:\n" +
                  omHome + "/bin/\n" +
                  omHome + "\n" +
                  dir.absolutePath();

                throw std::runtime_error(msg.toStdString().c_str());
            }
        #endif

        QString omcPath;

        #ifdef WIN32
                omcPath = omHome + "omc.exe";
        #else
                omcPath = omHome + "omc";
        #endif

        // Check the IOR file created by omc.exe
        QFile objectRefFile;
        QString fileIdentifier;
        if (mDisplayErrors)
            fileIdentifier = qApp->sessionId().append(QTime::currentTime().toString().remove(":"));
        else
            fileIdentifier = QString("temp-").append(qApp->sessionId().append(QTime::currentTime().toString().remove(":")));

        #ifdef WIN32 // Win32
            objectRefFile.setFileName(QString(QDir::tempPath()).append(QDir::separator()).append("openmodelica.objid.").append(this->mName).append(fileIdentifier));
        #else // UNIX environment
            char *user = getenv("USER");
            if (!user) { user = "nobody"; }
            objectRefFile.setFileName(QString(QDir::tempPath()).append(QDir::separator()).append("openmodelica.").append(*(new QString(user))).append(".objid.").append(this->mName).append(fileIdentifier));
        #endif

        if (objectRefFile.exists())
            objectRefFile.remove();

        mObjectRefFile = objectRefFile.fileName();
        // Start the omc.exe
        QStringList parameters;
        parameters << QString("+c=").append(this->mName).append(fileIdentifier) << QString("+d=interactiveCorba");

        QProcess *omcProcess = new QProcess();
        omcProcess->start( omcPath, parameters );

        // wait for the server to start.
        int ticks = 0;
        while (!objectRefFile.exists())
        {
            OMCThread::sleep(1);
            ticks++;
            if (ticks > 20)
            {
                msg = "Unable to find " + Helper::applicationName + " server, No OMC object reference file created.";
                throw std::runtime_error(msg.toStdString().c_str());
            }
        }
        // ORB initialization.
        int argc = 2;
        static const char *argv[] = { "-ORBgiopMaxMsgSize", "10485760" };
        CORBA::ORB_var orb = CORBA::ORB_init(argc, (char **)argv);

        objectRefFile.open(QIODevice::ReadOnly);

        char buf[1024];
        objectRefFile.readLine( buf, sizeof(buf) );
        QString uri( (const char*)buf );

        CORBA::Object_var obj = orb->string_to_object(uri.trimmed().toLatin1());

        mOMC = OmcCommunication::_narrow(obj);
        mHasInitialized = true;
    }
    catch(std::exception &e)
    {
        if (mDisplayErrors)
        {
            QString msg = e.what();
            QMessageBox::critical(mpParentMainWindow, Helper::applicationName + " - Error",
                                  msg.append("\n\n").append(Helper::applicationName).append(" will close."), "OK");
        }
        mHasInitialized = false;
        return false;
    }
    catch (CORBA::Exception&)
    {
        if (mDisplayErrors)
        {
            QMessageBox::critical(mpParentMainWindow, Helper::applicationName + " - Error",
                                  QString("Unable to communicate with ").append(Helper::applicationName).append(" server.")
                                  .append("\n\n").append(Helper::applicationName).append(" will close."), "OK");
        }
        mHasInitialized = false;
        return false;
    }
    QDir dir;
    if (!dir.exists(Helper::tmpPath)) {
     if (!dir.mkdir(Helper::tmpPath)) {
       QMessageBox::critical(mpParentMainWindow, Helper::applicationName + " - Error",
                              QString("Failed to create temp dir ").append(Helper::tmpPath), "OK");
       return false;
     }
    }
    changeDirectory(Helper::tmpPath);
    return true;
}

//! Stops the Open Modelica Compiler. Kill the process omc and also deletes the CORBA reference file.
//! @see startServer
void OMCProxy::stopServer()
{
    sendCommand("quit()");
}

//! Sends the user commands to OMC in a thread.
//! @param expression is used to send command as a string.
//! @see evalCommand(QString expression)
//! @see OMCThread
void OMCProxy::sendCommand(QString expression)
{
    if (!mHasInitialized)
        if(!startServer())      // if we are unable to start OMC. Exit the application.
        {
            mpParentMainWindow->mExitApplication = true;
            return;
        }

    // Send command to server
    try
    {
        mResult = mOMC->sendExpression(expression.toLatin1());
        logOMCMessages(expression);
    }
    catch(CORBA::Exception&)
    {
        // if the command is quit() and we get exception just simply quit
        if (expression == "quit()")
            return;

        removeObjectRefFile();
        if (mDisplayErrors)
        {
            QMessageBox::critical(mpParentMainWindow, Helper::applicationName + " - Error",
                                  QString("Communication with ").append(Helper::applicationName).append(" server has lost.")
                                  .append("\n\n").append(Helper::applicationName).append(" will restart."), "OK");
            restartApplication();
        }
    }
}

void OMCProxy::setResult(QString value)
{
    mResult = value;
}

//! Returns the result obtained from OMC.
QString OMCProxy::getResult()
{
    return mResult.trimmed();
}

void OMCProxy::logOMCMessages(QString expression)
{
    mCommandsList.append(expression);
    mpTextEdit->setCurrentFont(QFont("Times New Roman", 10, QFont::Bold, false));
    mpTextEdit->append(">>  " + expression);

    mpTextEdit->setCurrentFont(QFont("Times New Roman", 10, QFont::Normal, false));
    mpTextEdit->insertPlainText("\n>>  " + getResult());
    mpTextEdit->insertPlainText("\n");
}

QStringList OMCProxy::createPackagesList()
{
    QStringList packagesList;
    QStringList classesList = getPackages(tr(""));

    foreach (QString package, classesList)
    {
        // if package is Modelica skip it.
        if (package != tr("Modelica"))
            addPackage(&packagesList, package, tr(""));
    }
    return packagesList;
}

void OMCProxy::addPackage(QStringList *list, QString package, QString parentPackage)
{
    if (!parentPackage.isEmpty())
        package = parentPackage + tr(".") + package;

    list->append(package);
    QStringList classesList = getPackages(package);

    foreach (QString packageStr, classesList)
        addPackage(list, packageStr, package);
}

//! Opens the OMC Logger dialog.
void OMCProxy::openOMCLogger()
{
    this->mpOMCLogger->raise();
    this->mpOMCLogger->show();
}

void OMCProxy::catchException()
{
    removeObjectRefFile();
    if (mDisplayErrors)
    {
        QMessageBox::critical(mpParentMainWindow, Helper::applicationName + " - Error",
                              QString("Communication with ").append(Helper::applicationName).append(" server has lost.")
                              .append("\n\n").append(Helper::applicationName).append(" will restart."), "OK");

        restartApplication();
    }
}

void OMCProxy::sendCustomExpression()
{
    if (mpExpressionTextBox->text().isEmpty())
        return;

    sendCommand(mpExpressionTextBox->text());
    mpExpressionTextBox->setText(QString());
}

void OMCProxy::removeObjectRefFile()
{
    QFile::remove(mObjectRefFile);
}

void OMCProxy::restartApplication()
{
    //QProcess *applicationProcess = new QProcess();
    //applicationProcess->start(qApp->applicationFilePath());
    qApp->exit();
}

QString OMCProxy::getErrorString()
{
    sendCommand("getErrorString()");
    if (getResult().size() > 3)
        return getResult();
    else
    {
        setResult(tr(""));
        return getResult();
    }
}

QString OMCProxy::getVersion()
{
    sendCommand("getVersion()");
    return getResult();
}

bool OMCProxy::setAnnotationVersion(int version)
{
    if (version == OMCProxy::ANNOTATION_VERSION2X)
    {
        mAnnotationVersion = OMCProxy::ANNOTATION_VERSION2X;
        setEnvironmentVar("OPENMODELICALIBRARY", QString(Helper::OpenModelicaHome).append("/lib/omc/omlibrary/msl221"));
        sendCommand("setAnnotationVersion(\"2.x\")");
    }
    else if (version == OMCProxy::ANNOTATION_VERSION3X)
    {
        mAnnotationVersion = OMCProxy::ANNOTATION_VERSION3X;
        setEnvironmentVar("OPENMODELICALIBRARY", QString(Helper::OpenModelicaHome).append("/lib/omc/omlibrary/msl31"));
        sendCommand("setAnnotationVersion(\"3.x\")");
    }

    if (getResult().toLower().contains("true"))
        return true;
    else
    {
        setEnvironmentVar("OPENMODELICALIBRARY", QString(Helper::OpenModelicaHome).append("/lib/omc/omlibrary/msl221"));
        mAnnotationVersion = OMCProxy::ANNOTATION_VERSION2X;
        return false;
    }
}

QString OMCProxy::getAnnotationVersion()
{
    sendCommand("getAnnotationVersion()");
    return getResult();
}

bool OMCProxy::setEnvironmentVar(QString name, QString value)
{
    sendCommand("setEnvironmentVar(\"" + name + "\", \"" + value + "\")");
    if (getResult().toLower().contains("ok"))
        return true;
    else
        return false;
}

QString OMCProxy::getEnvironmentVar(QString name)
{
    sendCommand("getEnvironmentVar(\"" + name + "\")");
    return getResult();
}

//! Loads the Open Modelica Standard Library.
void OMCProxy::loadStandardLibrary()
{
    sendCommand("loadModel(Modelica)");
    sendCommand("loadModel(ModelicaServices)");

    //! @todo Remove it once we have removed Media and Fluid from MSL.
    // just added to remove Fluid and Media Library...

    deleteClass("Modelica.Media");
    deleteClass("Modelica.Fluid");

    if (getResult().contains("true"))
    {
        mIsStandardLibraryLoaded = true;
    }
}

//! Checks whether the Open Modelica Standard Library is loaded or not.
bool OMCProxy::isStandardLibraryLoaded()
{
    return mIsStandardLibraryLoaded;
}

//! Gets the list of classes from OMC.
//! @param className is the name of the class whose sub classes are retrieved.
QStringList OMCProxy::getClassNames(QString className)
{
    sendCommand("getClassNames(" + className + ")");
    QString result = StringHandler::removeFirstLastCurlBrackets(getResult());
    QStringList list = result.split(",", QString::SkipEmptyParts);
    return list;
}

//! Gets the list of packages from OMC.
//! @param packageName is the name of the package whose sub packages are retrieved.
QStringList OMCProxy::getPackages(QString packageName)
{
    sendCommand("getPackages(" + packageName + ")");
    QString result = StringHandler::removeFirstLastCurlBrackets(getResult());
    QStringList list = result.split(",", QString::SkipEmptyParts);
    return list;
}

//! Checks whether the class is a package or not.
//! @param className is the name of the class which is checked.
bool OMCProxy::isPackage(QString className)
{
    sendCommand("isPackage(" + className + ")");
    if (getResult().contains("true"))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool OMCProxy::isWhat(int type, QString className)
{
    switch (type)
    {
    case StringHandler::MODEL:
        sendCommand("isModel(" + className + ")");
        break;
    case StringHandler::CLASS:
        sendCommand("isClass(" + className + ")");
        break;
    case StringHandler::CONNECTOR:
        sendCommand("isConnector(" + className + ")");
        break;
    case StringHandler::RECORD:
        sendCommand("isRecord(" + className + ")");
        break;
    case StringHandler::BLOCK:
        sendCommand("isBlock(" + className + ")");
        break;
    case StringHandler::FUNCTION:
        sendCommand("isFunction(" + className + ")");
        break;
    case StringHandler::PACKAGE:
        sendCommand("isPackage(" + className + ")");
        break;
    default:
        return false;
    }

    if (getResult().contains("true"))
        return true;
    else
        return false;
}

int OMCProxy::getClassRestriction(QString modelName)
{
    sendCommand("getClassRestriction(" + modelName + ")");

    if (getResult().toLower().contains("model"))
        return StringHandler::MODEL;
    else if (getResult().toLower().contains("class"))
        return StringHandler::CLASS;
    else if (getResult().toLower().contains("connector"))
        return StringHandler::CONNECTOR;
    else if (getResult().toLower().contains("record"))
        return StringHandler::RECORD;
    else if (getResult().toLower().contains("block"))
        return StringHandler::BLOCK;
    else if (getResult().toLower().contains("function"))
        return StringHandler::FUNCTION;
    else if (getResult().toLower().contains("package"))
        return StringHandler::PACKAGE;
    else
        return 0;
}

QList<IconParameters*> OMCProxy::getParameters(QString className)
{
    QList<IconParameters*> iconParametersList;
    QStringList list = getParameterNames(className);

    for (int i = 0 ; i < list.size() ; i++)
    {
        QString value = getParameterValue(className, list.at(i));
        IconParameters *iconParameter = new IconParameters(list.at(i), value);
        iconParametersList.append(iconParameter);
    }

    return iconParametersList;
}

QStringList OMCProxy::getParameterNames(QString className)
{
    sendCommand("getParameterNames(" + className + ")");
    QString result = StringHandler::removeFirstLastCurlBrackets(getResult());
    if (result.isEmpty())
        return QStringList();
    else
    {
        QStringList list = result.split(",", QString::SkipEmptyParts);
        return list;
    }
}

QString OMCProxy::getParameterValue(QString className, QString parameter)
{
    sendCommand("getParameterValue(" + className + "," + parameter + ")");
    return getResult();
}

bool OMCProxy::setParameterValue(QString className, QString parameter, QString value)
{
    sendCommand("setParameterValue(" + className + "," + parameter + "," + value + ")");
    if (getResult().contains("Ok"))
        return true;
    else
        return false;
}

//! Gets the Icon Annotation of a specified class from OMC.
//! @param className is the name of the class.
QString OMCProxy::getIconAnnotation(QString className)
{
    sendCommand("getIconAnnotation(" + className + ")");
    return getResult();
}

QString OMCProxy::getDiagramAnnotation(QString className)
{
    sendCommand("getDiagramAnnotation(" + className + ")");
    return getResult();
}

int OMCProxy::getConnectionCount(QString className)
{
    sendCommand("getConnectionCount(" + className + ")");
    if (!getResult().isEmpty())
    {
        bool ok;
        int result = getResult().toInt(&ok);
        if (ok)
            return result;
        else
            return 0;
    }
    else
        return 0;
}

QString OMCProxy::getNthConnection(QString className, int num)
{
    QString number;
    sendCommand("getNthConnection(" + className + ", " + number.setNum(num) + ")");
    return getResult();
}

QString OMCProxy::getNthConnectionAnnotation(QString className, int num)
{
    QString number;
    sendCommand("getNthConnectionAnnotation(" + className + ", " + number.setNum(num) + ")");
    return getResult();
}

int OMCProxy::getInheritanceCount(QString className)
{
    sendCommand("getInheritanceCount(" + className + ")");
    if (!getResult().isEmpty())
    {
        bool ok;
        int result = getResult().toInt(&ok);
        if (ok)
            return result;
        else
            return 0;
    }
    else
        return 0;
}

QString OMCProxy::getNthInheritedClass(QString className, int num)
{
    QString number;
    sendCommand("getNthInheritedClass(" + className + ", " + number.setNum(num) + ")");
    return getResult();
}

QList<ComponentsProperties*> OMCProxy::getComponents(QString className)
{
    sendCommand("getComponents(" + className + ")");

    QList<ComponentsProperties*> components;
    QStringList list = StringHandler::getStrings(StringHandler::removeFirstLastCurlBrackets(getResult()));

    for (int i = 0 ; i < list.size() ; i++)
    {
        components.append(new ComponentsProperties(StringHandler::removeFirstLastCurlBrackets(list.at(i))));
    }

    return components;
}

QStringList OMCProxy::getComponentAnnotations(QString className)
{
    sendCommand("getComponentAnnotations(" + className + ")");
    return StringHandler::getStrings(StringHandler::removeFirstLastCurlBrackets(getResult()));
}

QString OMCProxy::getDocumentationAnnotation(QString className)
{
    sendCommand("getDocumentationAnnotation(" + className + ")");
    return getResult();
}

QString OMCProxy::changeDirectory(QString directory)
{
    directory = directory.replace("\\", "/");
    sendCommand("cd(\"" + directory + "\")");
    return getResult();
}

bool OMCProxy::loadFile(QString fileName)
{
    sendCommand("loadFile(\"" + fileName + "\")");
    if (getResult().toLower().contains("true"))
        return true;
    else
        return false;
}

bool OMCProxy::createClass(QString type, QString className)
{
    sendCommand(type + " " + className + " end " + className + ";");
    if (getResult().toLower().contains("error"))
        return false;
    else
        return true;
}

bool OMCProxy::createSubClass(QString type, QString className, QString parentClassName)
{
    sendCommand("within " + parentClassName + "; " + type + " " + className + " end " + className + ";");
    if (getResult().toLower().contains("error"))
        return false;
    else
        return true;
}

bool OMCProxy::updateSubClass(QString parentClassName, QString modelText)
{
    sendCommand("within " + parentClassName + "; " + modelText);
    if (getResult().toLower().contains("error"))
        return false;
    else
        return true;
}

bool OMCProxy::createModel(QString modelName)
{
    sendCommand("createModel(" + modelName + ")");
    if (getResult().toLower().contains("true"))
        return true;
    else
        return false;
}

bool OMCProxy::newModel(QString modelName, QString parentModelName)
{
    sendCommand("newModel(" + modelName + ", " + parentModelName + ")");
    if (getResult().toLower().contains("true"))
        return true;
    else
        return false;
}

bool OMCProxy::existClass(QString className)
{
    sendCommand("existClass(" + className + ")");
    if (getResult().toLower().contains("true"))
        return true;
    else
        return false;
}

bool OMCProxy::renameClass(QString oldName, QString newName)
{
    sendCommand("renameClass(" + oldName + ", " + newName + ")");
    if (getResult().toLower().contains("error"))
        return false;
    else
        return true;
}

bool OMCProxy::deleteClass(QString className)
{
    sendCommand("deleteClass(" + className + ")");
    if (getResult().contains("true"))
        return true;
    else
        return false;
}

QString OMCProxy::getSourceFile(QString modelName)
{
    sendCommand("getSourceFile(" + modelName + ")");
    return getResult();
}

bool OMCProxy::setSourceFile(QString modelName, QString path)
{
    sendCommand("setSourceFile(" + modelName + ", \"" + path + "\")");
    if (getResult().toLower().contains("ok"))
        return true;
    else
        return false;
}

bool OMCProxy::save(QString modelName)
{
    sendCommand("save(" + modelName + ")");
    if (getResult().toLower().contains("true"))
        return true;
    else
        return false;
}

bool OMCProxy::saveModifiedModel(QString modelText)
{
    sendCommand(modelText);
    if (getResult().toLower().contains("error"))
        return false;
    else
        return true;
}

QString OMCProxy::list(QString className)
{
    sendCommand("list(" + className + ")");
    return StringHandler::removeFirstLastQuotes(getResult()).trimmed();
}

bool OMCProxy::addClassAnnotation(QString className, QString annotation)
{
    sendCommand("addClassAnnotation(" + className + ", " + annotation + ")");
    if (getResult().toLower().contains("true"))
        return true;
    else
        return false;
}

bool OMCProxy::addComponent(QString name, QString className, QString modelName)
{
    sendCommand("addComponent(" + name + ", " + className + "," + modelName + ")");
    if (getResult().toLower().contains("true"))
        return true;
    else
        return false;
}

bool OMCProxy::deleteComponent(QString name, QString modelName)
{
    sendCommand("deleteComponent(" + name + "," + modelName + ")");
    if (getResult().toLower().contains("true"))
        return true;
    else
        return false;
}

bool OMCProxy::renameComponent(QString modelName, QString oldName, QString newName)
{
    sendCommand("renameComponent(" + modelName + "," + oldName + "," + newName + ")");
    if (getResult().toLower().contains("error"))
        return false;
    else
        return true;
}

bool OMCProxy::updateComponent(QString name, QString className, QString modelName, QString annotation)
{
    sendCommand("updateComponent(" + name + "," + className + "," + modelName + "," + annotation + ")");
    if (getResult().toLower().contains("true"))
        return true;
    else
        return false;
}

bool OMCProxy::renameComponentInClass(QString modelName, QString oldName, QString newName)
{
    sendCommand("renameComponentInClass(" + modelName + "," + oldName + "," + newName + ")");
    if (getResult().toLower().contains("error"))
        return false;
    else
        return true;
}

bool OMCProxy::updateConnection(QString from, QString to, QString modelName, QString annotation)
{
    sendCommand("updateConnection(" + from + "," + to + "," + modelName + "," + annotation + ")");
    if (getResult().contains("Ok"))
        return true;
    else
        return false;
}

bool OMCProxy::setComponentProperties(QString modelName, QString componentName, QString isFinal, QString isFlow,
                                      QString isProtected, QString isReplaceAble, QString variability, QString isInner,
                                      QString isOuter, QString causality)
{
    sendCommand("setComponentProperties(" + modelName + "," + componentName + ",{" + isFinal + "," + isFlow +
                "," + isProtected + "," + isReplaceAble + "}, {\"" + variability + "\"}, {" + isInner +
                "," + isOuter + "}, {\"" + causality + "\"})");

    if (getResult().toLower().contains("error"))
        return false;
    else
        return true;
}

bool OMCProxy::setComponentComment(QString modelName, QString componentName, QString comment)
{
    sendCommand("setComponentComment(" + modelName + "," + componentName + ",\"" + comment + "\")");
    if (getResult().toLower().contains("error"))
        return false;
    else
        return true;
}

bool OMCProxy::addConnection(QString from, QString to, QString className)
{
    sendCommand("addConnection(" + from + "," + to + "," + className + ")");
    if (getResult().contains("Ok"))
        return true;
    else
        return false;
}

bool OMCProxy::deleteConnection(QString from, QString to, QString className)
{
    sendCommand("deleteConnection(" + from + "," + to + "," + className + ")");
    if (getResult().contains("Ok"))
        return true;
    else
        return false;
}

bool OMCProxy::instantiateModel(QString modelName)
{
    sendCommand("instantiateModel(" + modelName + ")");
    if (getResult().size() < 3)
        return false;
    else
        return true;
}

bool OMCProxy::simulate(QString modelName, QString simualtionParameters)
{
    sendCommand("simulate(" + modelName + "," + simualtionParameters + ")");
    if (getResult().contains("res.plt"))
        return true;
    else
        return false;
}

bool OMCProxy::plot(QString modelName, QString plotVariables)
{
    sendCommand("plot(" + modelName + ",{" + plotVariables + "})");
    if (getResult().contains("true"))
        return true;
    else
        return false;
}

bool OMCProxy::plotParametric(QString modelName, QString plotVariables)
{
    sendCommand("plotParametric(" + modelName + "," + plotVariables + ")");
    if (getResult().contains("true"))
        return true;
    else
        return false;
}

bool OMCProxy::visualize(QString modelName)
{
    sendCommand("visualize(" + modelName + ")");
    if (getResult().contains("true"))
        return true;
    else
        return false;
}

QString OMCProxy::checkModel(QString modelName)
{
    sendCommand("checkModel(" + modelName + ")");
    return getResult();
}
